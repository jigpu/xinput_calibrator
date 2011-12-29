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

#ifndef _calibrator_hpp
#define _calibrator_hpp

#include <X11/Xlib.h>

/*
 * Number of blocks. We partition the screen into 'num_blocks' x 'num_blocks'
 * rectangles of equal size. We then ask the user to press points that are
 * located at the corner closes to the center of the four blocks in the corners
 * of the screen. The following ascii art illustrates the situation. We partition
 * the screen into 8 blocks in each direction. We then let the user press the
 * points marked with 'O'.
 *
 *   +--+--+--+--+--+--+--+--+
 *   |  |  |  |  |  |  |  |  |
 *   +--O--+--+--+--+--+--O--+
 *   |  |  |  |  |  |  |  |  |
 *   +--+--+--+--+--+--+--+--+
 *   |  |  |  |  |  |  |  |  |
 *   +--+--+--+--+--+--+--+--+
 *   |  |  |  |  |  |  |  |  |
 *   +--+--+--+--+--+--+--+--+
 *   |  |  |  |  |  |  |  |  |
 *   +--+--+--+--+--+--+--+--+
 *   |  |  |  |  |  |  |  |  |
 *   +--+--+--+--+--+--+--+--+
 *   |  |  |  |  |  |  |  |  |
 *   +--O--+--+--+--+--+--O--+
 *   |  |  |  |  |  |  |  |  |
 *   +--+--+--+--+--+--+--+--+
 */
const int num_blocks = 8;

/* Names of the points */
enum {
    UL = 0, /* Upper-left  */
    UR = 1, /* Upper-right */
    LL = 2, /* Lower-left  */
    LR = 3  /* Lower-right */
};

/* Output types */
typedef enum {
    OUTYPE_AUTO,
    OUTYPE_XORGCONFD,
    OUTYPE_HAL,
    OUTYPE_XINPUT
} OutputType;

/* struct to hold min/max info of the X and Y axis */
typedef struct {
    int x_min;
    int x_max;
    int y_min;
    int y_max;
} XYinfo;

struct Calib {
    /* name of the device (driver) */
    const char* device_name;

    /* original axys values */
    XYinfo old_axys;

    /* be verbose or not */
    bool verbose;

    /* nr of clicks registered */
    int num_clicks;

    /* click coordinates */
    int clicked_x[4], clicked_y[4];

    /* Threshold to keep the same point from being clicked twice.
     * Set to zero if you don't want this check
     */
    int threshold_doubleclick;

    /* Threshold to detect mis-clicks (clicks not along axes)
     * A lower value forces more precise calibration
     * Set to zero if you don't want this check
     */
    int threshold_misclick;

    /* Type of output */
    OutputType output_type;

    /* manually specified geometry string */
    const char* geometry;
};

/* set the doubleclick treshold */
void set_threshold_doubleclick(struct Calib*, int t);

/* set the misclick treshold */
void set_threshold_misclick(struct Calib*, int t);

/* get the number of clicks already registered */
int get_numclicks(struct Calib*);

/* return geometry string or NULL */
const char* get_geometry(struct Calib*);

/* reset clicks */
void reset(struct Calib*);

/* add a click with the given coordinates */
bool add_click(struct Calib*, int x, int y);

/* calculate and apply the calibration */
bool finish(struct Calib*, int width, int height);

/* get the sysfs name of the device,
 * returns NULL if it can not be found
 */
const char* get_sysfs_name(struct Calib*);

/* check whether the coordinates are along the respective axis */
bool along_axis(struct Calib*, int xy, int x0, int y0);

/* Check whether the given name is a sysfs device name */
bool is_sysfs_name(struct Calib*, const char* name);

/* Check whether the X server has xorg.conf.d support */
bool has_xorgconfd_support(struct Calib*, Display* display=NULL);

struct Calib* CalibratorXorgPrint(const char* const device_name, const XYinfo& axys,
        const bool verbose, const int thr_misclick, const int thr_doubleclick,
        const OutputType output_type, const char* geometry);

bool finish_data(struct Calib*, const XYinfo new_axys, int swap_xy);
bool output_xorgconfd(struct Calib*, const XYinfo new_axys, int swap_xy, int new_swap_xy);
bool output_hal(struct Calib*, const XYinfo new_axys, int swap_xy, int new_swap_xy);

#endif
