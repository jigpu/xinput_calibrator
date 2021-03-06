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

#ifndef _calibrator_h
#define _calibrator_h

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
#define NUM_BLOCKS 8

/* Names of the points */
enum
{
	UL = 0, /* Upper-left  */
	UR = 1, /* Upper-right */
	LL = 2, /* Lower-left  */
	LR = 3  /* Lower-right */
};

/* struct to hold min/max info of the X and Y axis */
typedef struct
{
	int x_min;
	int x_max;
	int y_min;
	int y_max;
} XYinfo;

typedef enum
{
	false = 0,
	true  = 1
} bool;

struct Calib
{
    /* original axys values */
    XYinfo old_axys;

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

    /* manually specified geometry string */
    const char* geometry;
};

void reset      (struct Calib *c);
bool add_click  (struct Calib *c,
                 int           x,
                 int           y);
bool along_axis (struct Calib *c,
                 int           xy,
                 int           x0,
                 int           y0);
bool finish     (struct Calib *c,
                 int           width,
                 int           height,
                 XYinfo       *new_axys,
                 bool         *swap);

#endif /* _calibrator_h */
