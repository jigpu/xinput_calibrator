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
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include "calibrator.hpp"

#define SWAP(x,y)  do { int t; t=(x); x=(y); y=t; } while (0)

void reset(struct Calib* c)
{
	c->num_clicks = 0;
}

void set_threshold_doubleclick(struct Calib* c, int t)
{
    c->threshold_doubleclick = t;
}

void set_threshold_misclick(struct Calib* c, int t)
{
    c->threshold_misclick = t;
}

int get_numclicks(struct Calib* c)
{
    return c->num_clicks;
}

const char* get_geometry(struct Calib* c)
{
    return c->geometry;
}

bool add_click(struct Calib* c, int x, int y)
{
    /* Double-click detection */
    if (c->threshold_doubleclick > 0 && c->num_clicks > 0) {
        int i = c->num_clicks-1;
        while (i >= 0) {
            if (abs(x - c->clicked_x[i]) <= c->threshold_doubleclick
                && abs(y - c->clicked_y[i]) <= c->threshold_doubleclick) {
                if (c->verbose) {
                    printf("DEBUG: Not adding click %i (X=%i, Y=%i): within %i pixels of previous click\n",
                        c->num_clicks, x, y, c->threshold_doubleclick);
                }
                return false;
            }
            i--;
        }
    }

    /* Mis-click detection */
    if (c->threshold_misclick > 0 && c->num_clicks > 0) {
        bool misclick = true;

        if (c->num_clicks == 1) {
            /* check that along one axis of first point */
            if (along_axis(c, x,c->clicked_x[0],c->clicked_y[0]) ||
                along_axis(c, y,c->clicked_x[0],c->clicked_y[0]))
                misclick = false;
        } else if (c->num_clicks == 2) {
            /* check that along other axis of first point than second point */
            if ((along_axis(c, y,c->clicked_x[0],c->clicked_y[0]) &&
                 along_axis(c, c->clicked_x[1],c->clicked_x[0],c->clicked_y[0])) ||
                (along_axis(c, x,c->clicked_x[0],c->clicked_y[0]) &&
                 along_axis(c, c->clicked_y[1],c->clicked_x[0],c->clicked_y[0])))
                misclick = false;
        } else if (c->num_clicks == 3) {
            /* check that along both axis of second and third point */
            if ((along_axis(c, x,c->clicked_x[1],c->clicked_y[1]) &&
                 along_axis(c, y,c->clicked_x[2],c->clicked_y[2])) ||
                (along_axis(c, y,c->clicked_x[1],c->clicked_y[1]) &&
                 along_axis(c, x,c->clicked_x[2],c->clicked_y[2])))
                misclick = false;
        }

        if (misclick) {
            if (c->verbose) {
                if (c->num_clicks == 1)
                    printf("DEBUG: Mis-click detected, click %i (X=%i, Y=%i) not aligned with click 0 (X=%i, Y=%i) (threshold=%i)\n", c->num_clicks, x, y, c->clicked_x[0], c->clicked_y[0], c->threshold_misclick);
                else if (c->num_clicks == 2)
                    printf("DEBUG: Mis-click detected, click %i (X=%i, Y=%i) not aligned with click 0 (X=%i, Y=%i) or click 1 (X=%i, Y=%i) (threshold=%i)\n", c->num_clicks, x, y, c->clicked_x[0], c->clicked_y[0], c->clicked_x[1], c->clicked_y[1], c->threshold_misclick);
                else if (c->num_clicks == 3)
                    printf("DEBUG: Mis-click detected, click %i (X=%i, Y=%i) not aligned with click 1 (X=%i, Y=%i) or click 2 (X=%i, Y=%i) (threshold=%i)\n", c->num_clicks, x, y, c->clicked_x[1], c->clicked_y[1], c->clicked_x[2], c->clicked_y[2], c->threshold_misclick);
            }

            reset(c);
            return false;
        }
    }

    c->clicked_x[c->num_clicks] = x;
    c->clicked_y[c->num_clicks] = y;
    c->num_clicks++;

    if (c->verbose)
        printf("DEBUG: Adding click %i (X=%i, Y=%i)\n", c->num_clicks-1, x, y);

    return true;
}

inline bool along_axis(struct Calib* c, int xy, int x0, int y0)
{
    return ((abs(xy - x0) <= c->threshold_misclick) ||
            (abs(xy - y0) <= c->threshold_misclick));
}

bool finish(struct Calib* c, int width, int height)
{
    if (get_numclicks(c) != 4) {
        return false;
    }

    /* Should x and y be swapped? */
    const bool swap_xy = (abs (c->clicked_x [UL] - c->clicked_x [UR]) < abs (c->clicked_y [UL] - c->clicked_y [UR]));
    if (swap_xy) {
        SWAP(c->clicked_x[LL], c->clicked_x[UR]);
        SWAP(c->clicked_y[LL], c->clicked_y[UR]);
    }

    /* Compute min/max coordinates. */
    XYinfo axys = {-1, -1, -1, -1};
    /* These are scaled using the values of old_axys */
    const float scale_x = (c->old_axys.x_max - c->old_axys.x_min)/(float)width;
    axys.x_min = ((c->clicked_x[UL] + c->clicked_x[LL]) * scale_x/2) + c->old_axys.x_min;
    axys.x_max = ((c->clicked_x[UR] + c->clicked_x[LR]) * scale_x/2) + c->old_axys.x_min;
    const float scale_y = (c->old_axys.y_max - c->old_axys.y_min)/(float)height;
    axys.y_min = ((c->clicked_y[UL] + c->clicked_y[UR]) * scale_y/2) + c->old_axys.y_min;
    axys.y_max = ((c->clicked_y[LL] + c->clicked_y[LR]) * scale_y/2) + c->old_axys.y_min;

    /* Add/subtract the offset that comes from not having the points in the
     * corners (using the same coordinate system they are currently in)
     */
    const int delta_x = (axys.x_max - axys.x_min) / (float)(num_blocks - 2);
    axys.x_min -= delta_x;
    axys.x_max += delta_x;
    const int delta_y = (axys.y_max - axys.y_min) / (float)(num_blocks - 2);
    axys.y_min -= delta_y;
    axys.y_max += delta_y;


    /* If x and y has to be swapped we also have to swap the parameters */
    if (swap_xy) {
        SWAP(axys.x_min, axys.y_max);
        SWAP(axys.y_min, axys.x_max);
    }

    /* finish the data, driver/calibrator specific */
    return finish_data(c, axys, swap_xy);
}

const char* get_sysfs_name(struct Calib* c)
{
    if (is_sysfs_name(c, c->device_name))
        return c->device_name;

    /* TODO: more mechanisms */

    return NULL;
}

bool is_sysfs_name(struct Calib* c, const char* name) {
    const char* SYSFS_INPUT="/sys/class/input";
    const char* SYSFS_DEVNAME="device/name";

    DIR* dp = opendir(SYSFS_INPUT);
    if (dp == NULL)
        return false;

    while (dirent* ep = readdir(dp)) {
        if (strncmp(ep->d_name, "event", strlen("event")) == 0) {
            /* got event name, get its sysfs device name */
            char filename[40]; /* actually 35, but hey... */
            (void) sprintf(filename, "%s/%s/%s", SYSFS_INPUT, ep->d_name, SYSFS_DEVNAME);

            FILE *f = fopen(filename, "r");
            if (f != NULL) {
                int match = 0;
                char devname[100];
                match = fscanf(f, "%s", &devname);
                if (match == 1 && strcmp(devname, name) == 0) {
                    if (c->verbose)
                        printf("DEBUG: Found that '%s' is a sysfs name.\n", name);
                    return true;
                }
                fclose(f);
            }
        }
    }
    (void) closedir(dp);

    if (c->verbose)
        printf("DEBUG: Name '%s' does not match any in '%s/event*/%s'\n",
                    name, SYSFS_INPUT, SYSFS_DEVNAME);
    return false;
}

bool has_xorgconfd_support(struct Calib* c, Display* dpy) {
    bool has_support = false;

    Display* display = dpy;
    if (dpy == NULL) /* no connection to reuse */
        display = XOpenDisplay(NULL);

    if (display == NULL) {
        fprintf(stderr, "Unable to connect to X server\n");
        exit(1);
    }

    if (strstr(ServerVendor(display), "X.Org") &&
        VendorRelease(display) >= 10800000) {
        has_support = true;
    }

    if (dpy == NULL) /* no connection to reuse */
        XCloseDisplay(display);

    return has_support;
}

struct Calib* CalibratorXorgPrint(const char* const device_name0, const XYinfo& axys0, const bool verbose0, const int thr_misclick, const int thr_doubleclick, const OutputType output_type, const char* geometry)
{
    struct Calib* c = (struct Calib*)calloc(1, sizeof(struct Calib));
    c->device_name = device_name0;
    c->old_axys = axys0;
    c->verbose = verbose0;
    c->threshold_misclick = thr_misclick;
    c->threshold_doubleclick = thr_doubleclick;
    c->output_type = output_type;
    c->geometry = geometry;

    printf("Calibrating standard Xorg driver \"%s\"\n", c->device_name);
    printf("\tcurrent calibration values: min_x=%d, max_x=%d and min_y=%d, max_y=%d\n",
                c->old_axys.x_min, c->old_axys.x_max, c->old_axys.y_min, c->old_axys.y_max);
    printf("\tIf these values are estimated wrong, either supply it manually with the --precalib option, or run the 'get_precalib.sh' script to automatically get it (through HAL).\n");

    return c;
}

bool finish_data(struct Calib* c, const XYinfo new_axys, int swap_xy)
{
    bool success = true;

    /* we suppose the previous 'swap_xy' value was 0 */
    /* (unfortunately there is no way to verify this (yet)) */
    int new_swap_xy = swap_xy;

    printf("\n\n--> Making the calibration permanent <--\n");
    switch (c->output_type) {
        case OUTYPE_AUTO:
            /* xorg.conf.d or alternatively hal config */
            if (has_xorgconfd_support(c)) {
                success &= output_xorgconfd(c, new_axys, swap_xy, new_swap_xy);
            } else {
                success &= output_hal(c, new_axys, swap_xy, new_swap_xy);
            }
            break;
        case OUTYPE_XORGCONFD:
            success &= output_xorgconfd(c, new_axys, swap_xy, new_swap_xy);
            break;
        case OUTYPE_HAL:
            success &= output_hal(c, new_axys, swap_xy, new_swap_xy);
            break;
        default:
            fprintf(stderr, "ERROR: XorgPrint Calibrator does not support the supplied --output-type\n");
            success = false;
    }

    return success;
}

bool output_xorgconfd(struct Calib* c, const XYinfo new_axys, int swap_xy, int new_swap_xy)
{
    const char* sysfs_name = get_sysfs_name(c);
    bool not_sysfs_name = (sysfs_name == NULL);
    if (not_sysfs_name)
        sysfs_name = "!!Name_Of_TouchScreen!!";

    /* xorg.conf.d snippet */
    printf("  copy the snippet below into '/etc/X11/xorg.conf.d/99-calibration.conf'\n");
    printf("Section \"InputClass\"\n");
    printf("	Identifier	\"calibration\"\n");
    printf("	MatchProduct	\"%s\"\n", sysfs_name);
    printf("	Option	\"MinX\"	\"%d\"\n", new_axys.x_min);
    printf("	Option	\"MaxX\"	\"%d\"\n", new_axys.x_max);
    printf("	Option	\"MinY\"	\"%d\"\n", new_axys.y_min);
    printf("	Option	\"MaxY\"	\"%d\"\n", new_axys.y_max);
    if (swap_xy != 0)
        printf("	Option	\"SwapXY\"	\"%d\" # unless it was already set to 1\n", new_swap_xy);
    printf("EndSection\n");

    if (not_sysfs_name)
        printf("\nChange '%s' to your device's name in the config above.\n", sysfs_name);

    return true;
}

bool output_hal(struct Calib* c, const XYinfo new_axys, int swap_xy, int new_swap_xy)
{
    const char* sysfs_name = get_sysfs_name(c);
    bool not_sysfs_name = (sysfs_name == NULL);
    if (not_sysfs_name)
        sysfs_name = "!!Name_Of_TouchScreen!!";

    /* HAL policy output */
    printf("  copy the policy below into '/etc/hal/fdi/policy/touchscreen.fdi'\n\
<match key=\"info.product\" contains=\"%s\">\n\
  <merge key=\"input.x11_options.minx\" type=\"string\">%d</merge>\n\
  <merge key=\"input.x11_options.maxx\" type=\"string\">%d</merge>\n\
  <merge key=\"input.x11_options.miny\" type=\"string\">%d</merge>\n\
  <merge key=\"input.x11_options.maxy\" type=\"string\">%d</merge>\n"
     , sysfs_name, new_axys.x_min, new_axys.x_max, new_axys.y_min, new_axys.y_max);
    if (swap_xy != 0)
        printf("  <merge key=\"input.x11_options.swapxy\" type=\"string\">%d</merge>\n", new_swap_xy);
    printf("</match>\n");

    if (not_sysfs_name)
        printf("\nChange '%s' to your device's name in the config above.\n", sysfs_name);

    return true;
}

