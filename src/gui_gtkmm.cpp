/*
 * Copyright (c) 2009 Tias Guns
 * Copyright (c) 2009 Soren Hauberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>

#include "calibrator.hh"


// Timeout parameters
const int time_step = 100;  // in milliseconds
const int max_time = 15000; // 5000 = 5 sec

// Clock appereance
const int cross_lines = 25;
const int cross_circle = 4;
const int clock_radius = 50;
const int clock_line_width = 10;

// Text printed on screen
const int font_size = 16;
const int help_lines = 4;
const char *help_text[help_lines] = {
    "Touchscreen Calibration",
    "Press the point, use a stylus to increase precision.",
    "",
    "(To abort, press any key or wait)"
};


struct CalibArea* CalibrationArea_(struct Calib* c);
void set_display_size(CalibArea *calib_area, int width, int height);
bool on_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data);
void redraw(CalibArea *calib_area);
bool on_timer_signal(CalibArea *calib_area);
bool on_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
void draw_message(CalibArea *calib_area, const char* msg);
bool on_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data);


struct CalibArea
{
    struct Calib* calibrator;
    double X[4], Y[4];
    int display_width, display_height;
    int time_elapsed;

    const char* message;

    GtkWidget *drawing_area;
};

struct CalibArea* CalibrationArea_(struct Calib* c)
{
    struct CalibArea *calib_area = (struct CalibArea*)calloc(1, sizeof(struct CalibArea));
    calib_area->calibrator = c;
    calib_area->drawing_area = gtk_drawing_area_new();

    // Listen for mouse events
    gtk_widget_add_events(calib_area->drawing_area, GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK);
    gtk_widget_set_can_focus(calib_area->drawing_area, TRUE);

    // Connect callbacks
    g_signal_connect(calib_area->drawing_area, "expose-event", G_CALLBACK(on_expose_event), calib_area);
    g_signal_connect(calib_area->drawing_area, "button-press-event", G_CALLBACK(on_button_press_event), calib_area);
    g_signal_connect(calib_area->drawing_area, "key-press-event", G_CALLBACK(on_key_press_event), calib_area);

    // parse geometry string
    const char* geo = get_geometry(calib_area->calibrator);
    if (geo != NULL) {
        int gw,gh;
        int res = sscanf(geo,"%dx%d",&gw,&gh);
        if (res != 2) {
            fprintf(stderr,"Warning: error parsing geometry string - using defaults.\n");
            geo = NULL;
        } else {
            set_display_size(calib_area, gw, gh );
        }
    }
    if (geo == NULL)
    {
        int width  = calib_area->drawing_area->allocation.width;
        int height = calib_area->drawing_area->allocation.height;
        set_display_size(calib_area, width, height);
    }

    // Setup timer for animation
    g_timeout_add(time_step, (GSourceFunc)on_timer_signal, calib_area);

    return calib_area;
}

void set_display_size(struct CalibArea *calib_area, int width, int height) {
    calib_area->display_width = width;
    calib_area->display_height = height;

    // Compute absolute circle centers
    const int delta_x = calib_area->display_width/num_blocks;
    const int delta_y = calib_area->display_height/num_blocks;
    calib_area->X[UL] = delta_x;                                 calib_area->Y[UL] = delta_y;
    calib_area->X[UR] = calib_area->display_width - delta_x - 1; calib_area->Y[UR] = delta_y;
    calib_area->X[LL] = delta_x;                                 calib_area->Y[LL] = calib_area->display_height - delta_y - 1;
    calib_area->X[LR] = calib_area->display_width - delta_x - 1; calib_area->Y[LR] = calib_area->display_height - delta_y - 1;

    // reset calibration if already started
    reset(calib_area->calibrator);
}

bool on_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    struct CalibArea *calib_area = (struct CalibArea*)data;

    // check that screensize did not change (if no manually specified geometry)
    int width  = calib_area->drawing_area->allocation.width;
    int height = calib_area->drawing_area->allocation.height;
    if (get_geometry(calib_area->calibrator) == NULL &&
         (calib_area->display_width != width ||
         calib_area->display_height != height )) {
        set_display_size(calib_area, width, height);
    }

    GdkWindow *window = gtk_widget_get_window(calib_area->drawing_area);
    if (window) {
        cairo_t *cr = gdk_cairo_create(window);
        cairo_save(cr);

        cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
        cairo_clip(cr);

        // Print the text
        cairo_set_font_size(cr, font_size);
        double text_height = -1;
        double text_width = -1;
        cairo_text_extents_t extent;
        for (int i = 0; i != help_lines; i++) {
            cairo_text_extents(cr, help_text[i], &extent);
            text_width = std::max(text_width, extent.width);
            text_height = std::max(text_height, extent.height);
        }
        text_height += 2;

        double x = (calib_area->display_width - text_width) / 2;
        double y = (calib_area->display_height - text_height) / 2 - 60;
        cairo_set_line_width(cr, 2);
        cairo_rectangle(cr, x - 10, y - (help_lines*text_height) - 10,
                text_width + 20, (help_lines*text_height) + 20);

        // Print help lines
        y -= 3;
        for (int i = help_lines-1; i != -1; i--) {
            cairo_text_extents(cr, help_text[i], &extent);
            cairo_move_to(cr, x + (text_width-extent.width)/2, y);
            cairo_show_text(cr, help_text[i]);
            y -= text_height;
        }
        cairo_stroke(cr);

        // Draw the points
        for (int i = 0; i <= get_numclicks(calib_area->calibrator); i++) {
            // set color: already clicked or not
            if (i < get_numclicks(calib_area->calibrator))
                cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            else
                cairo_set_source_rgb(cr, 0.8, 0.0, 0.0);

            cairo_set_line_width(cr, 1);
            cairo_move_to(cr, calib_area->X[i] - cross_lines, calib_area->Y[i]);
            cairo_rel_line_to(cr, cross_lines*2, 0);
            cairo_move_to(cr, calib_area->X[i], calib_area->Y[i] - cross_lines);
            cairo_rel_line_to(cr, 0, cross_lines*2);
            cairo_stroke(cr);

            cairo_arc(cr, calib_area->X[i], calib_area->Y[i], cross_circle, 0.0, 2.0 * M_PI);
            cairo_stroke(cr);
        }

        // Draw the clock background
        cairo_arc(cr, calib_area->display_width/2, calib_area->display_height/2, clock_radius/2, 0.0, 2.0 * M_PI);
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_fill_preserve(cr);
        cairo_stroke(cr);

        cairo_set_line_width(cr, clock_line_width);
        cairo_arc(cr, calib_area->display_width/2, calib_area->display_height/2, (clock_radius - clock_line_width)/2,
             3/2.0*M_PI, (3/2.0*M_PI) + ((double)calib_area->time_elapsed/(double)max_time) * 2*M_PI);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_stroke(cr);


        // Draw the message (if any)
        if (calib_area->message != NULL) {
            // Frame the message
            cairo_set_font_size(cr, font_size);
            cairo_text_extents_t extent;
            cairo_text_extents(cr, calib_area->message, &extent);
            text_width = extent.width;
            text_height = extent.height;

            x = (calib_area->display_width - text_width) / 2;
            y = (calib_area->display_height - text_height + clock_radius) / 2 + 60;
            cairo_set_line_width(cr, 2);
            cairo_rectangle(cr, x - 10, y - text_height - 10,
                    text_width + 20, text_height + 25);

            // Print the message
            cairo_move_to(cr, x, y);
            cairo_show_text(cr, calib_area->message);
            cairo_stroke(cr);
        }

        cairo_restore(cr);
    }

    return true;
}

void redraw(struct CalibArea *calib_area)
{
    GdkWindow *win = gtk_widget_get_window(calib_area->drawing_area);
    if (win) {
        const GdkRectangle rect = {0, 0, calib_area->display_width, calib_area->display_height};
        gdk_window_invalidate_rect(win, &rect, false);
    }
}

bool on_timer_signal(struct CalibArea *calib_area)
{
    calib_area->time_elapsed += time_step;
    if (calib_area->time_elapsed > max_time) {
        exit(0);
    }

    // Update clock
    GdkWindow *win = gtk_widget_get_window(calib_area->drawing_area);
    if (win) {
        const GdkRectangle rect = {calib_area->display_width/2 - clock_radius - clock_line_width,
                                 calib_area->display_height/2 - clock_radius - clock_line_width,
                                 2 * clock_radius + 1 + 2 * clock_line_width,
                                 2 * clock_radius + 1 + 2 * clock_line_width};
        gdk_window_invalidate_rect(win, &rect, false);
    }

    return true;
}

bool on_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    struct CalibArea *calib_area = (struct CalibArea*)data;

    // Handle click
    calib_area->time_elapsed = 0;
    bool success = add_click(calib_area->calibrator, (int)event->x_root, (int)event->y_root);

    if (!success && get_numclicks(calib_area->calibrator) == 0) {
        draw_message(calib_area, "Mis-click detected, restarting...");
    } else {
        draw_message(calib_area, NULL);
    }

    // Are we done yet?
    if (get_numclicks(calib_area->calibrator) >= 4) {
        // Recalibrate
        success = finish(calib_area->calibrator, calib_area->display_width, calib_area->display_height);

        if (success) {
            exit(0);
        } else {
            // TODO, in GUI ?
            fprintf(stderr, "Error: unable to apply or save configuration values");
            exit(1);
        }
    }

    // Force a redraw
    redraw(calib_area);

    return true;
}

void draw_message(struct CalibArea *calib_area, const char* msg)
{
    calib_area->message = msg;
}

bool on_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    struct CalibArea *calib_area = (struct CalibArea*)data;

    (void) event;
    exit(0);
}
