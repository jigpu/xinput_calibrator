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

// Must be before Xlib stuff
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/drawingarea.h>
#include <cairomm/context.h>

#include "main_common.hpp"
#include "gui_gtkmm.cpp"

int main(int argc, char** argv)
{
    struct Calib* calibrator = main_common(argc, argv);

    // GTK setup
    gtk_init(&argc, &argv);

    GdkScreen *screen = gdk_screen_get_default();
    //int num_monitors = screen->get_n_monitors(); TODO, multiple monitors?
    GdkRectangle rect;
    gdk_screen_get_monitor_geometry(screen, 0, &rect);

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    // when no window manager: explicitely take size of full screen
    gtk_window_move(GTK_WINDOW(win), rect.x, rect.y);
    gtk_window_set_default_size(GTK_WINDOW(win), rect.width, rect.height);
    // in case of window manager: set as full screen to hide window decorations
    gtk_window_fullscreen(GTK_WINDOW(win));

    struct CalibArea *calib_area = CalibrationArea_(calibrator);

    gtk_container_add(GTK_CONTAINER(win), calib_area->drawing_area);
    gtk_widget_show_all(win);

    gtk_main();

    free(calibrator);
    return 0;
}
