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
#include <ctype.h>
#include <string.h>
#include <dirent.h>

#include <X11/extensions/XInput.h>

#include "gui_gtk.h"
#include "main.h"

/**
 * find a calibratable touchscreen device (using XInput)
 *
 * if pre_device is NULL, the last calibratable device is selected.
 * retuns number of devices found,
 * the data of the device is returned in the last 3 function parameters
 */
int find_device(const char* pre_device, bool verbose, bool list_devices,
        XID* device_id, const char** device_name, XYinfo* device_axys)
{
    bool pre_device_is_id = true;
    int found = 0;

    Display* display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Unable to connect to X server\n");
        exit(1);
    }

    int xi_opcode, event, error;
    if (!XQueryExtension(display, "XInputExtension", &xi_opcode, &event, &error)) {
        fprintf(stderr, "X Input extension not available.\n");
        exit(1);
    }

    /* verbose, get Xi version */
    if (verbose) {
        XExtensionVersion *version = XGetExtensionVersion(display, INAME);

        if (version && (version != (XExtensionVersion*) NoSuchExtension)) {
            printf("DEBUG: %s version is %i.%i\n",
                INAME, version->major_version, version->minor_version);
            XFree(version);
        }
    }

    if (pre_device != NULL) {
        /* check whether the pre_device is an ID (only digits) */
        int len = strlen(pre_device);
        int loop;
        for (loop=0; loop<len; loop++) {
	        if (!isdigit(pre_device[loop])) {
	            pre_device_is_id = false;
	            break;
	        }
        }
    }


    if (verbose)
        printf("DEBUG: Skipping virtual master devices and devices without axis valuators.\n");
    int ndevices;
    XDeviceInfoPtr list, slist;
    slist=list=(XDeviceInfoPtr) XListInputDevices (display, &ndevices);
    int i;
    for (i=0; i<ndevices; i++, list++)
    {
        if (list->use == IsXKeyboard || list->use == IsXPointer) /* virtual master device */
            continue;

        /* if we are looking for a specific device */
        if (pre_device != NULL) {
            if ((pre_device_is_id && list->id == (XID) atoi(pre_device)) ||
                (!pre_device_is_id && strcmp(list->name, pre_device) == 0)) {
                /* OK, fall through */
            } else {
                /* skip, not this device */
                continue;
            }
        }

        XAnyClassPtr any = (XAnyClassPtr) (list->inputclassinfo);
        int j;
        for (j=0; j<list->num_classes; j++)
        {

            if (any->class == ValuatorClass)
            {
                XValuatorInfoPtr V = (XValuatorInfoPtr) any;
                XAxisInfoPtr ax = (XAxisInfoPtr) V->axes;

                if (V->mode != Absolute) {
                    if (verbose)
                        printf("DEBUG: Skipping device '%s' id=%i, does not report Absolute events.\n",
                            list->name, (int)list->id);
                } else if (V->num_axes < 2 ||
                    (ax[0].min_value == -1 && ax[0].max_value == -1) ||
                    (ax[1].min_value == -1 && ax[1].max_value == -1)) {
                    if (verbose)
                        printf("DEBUG: Skipping device '%s' id=%i, does not have two calibratable axes.\n",
                            list->name, (int)list->id);
                } else {
                    /* a calibratable device (has 2 axis valuators) */
                    found++;
                    *device_id = list->id;
                    *device_name = my_strdup(list->name);
                    device_axys->x_min = ax[0].min_value;
                    device_axys->x_max = ax[0].max_value;
                    device_axys->y_min = ax[1].min_value;
                    device_axys->y_max = ax[1].max_value;

                    if (list_devices)
                        printf("Device \"%s\" id=%i\n", *device_name, (int)*device_id);
                }

            }

            /*
             * Increment 'any' to point to the next item in the linked
             * list.  The length is in bytes, so 'any' must be cast to
             * a character pointer before being incremented.
             */
            any = (XAnyClassPtr) ((char *) any + any->length);
        }

    }
    XFreeDeviceList(slist);
    XCloseDisplay(display);

    return found;
}

static void usage(char* cmd, unsigned thr_misclick)
{
    fprintf(stderr, "Usage: %s [-h|--help] [-v|--verbose] [--list] [--device <device name or id>] [--precalib <minx> <maxx> <miny> <maxy>] [--misclick <nr of pixels>] [--output-type <auto|xorg.conf.d|hal|xinput>] [--fake] [--geometry <w>x<h>]\n", cmd);
    fprintf(stderr, "\t-h, --help: print this help message\n");
    fprintf(stderr, "\t-v, --verbose: print debug messages during the process\n");
    fprintf(stderr, "\t--list: list calibratable input devices and quit\n");
    fprintf(stderr, "\t--device <device name or id>: select a specific device to calibrate\n");
    fprintf(stderr, "\t--precalib: manually provide the current calibration setting (eg. the values in xorg.conf)\n");
    fprintf(stderr, "\t--misclick: set the misclick threshold (0=off, default: %i pixels)\n",
        thr_misclick);
    fprintf(stderr, "\t--fake: emulate a fake device (for testing purposes)\n");
    fprintf(stderr, "\t--geometry: manually provide the geometry (width and height) for the calibration window\n");
}

struct Calib* main_common(int argc, char** argv)
{
    bool verbose = false;
    bool list_devices = false;
    bool fake = false;
    bool precalib = false;
    XYinfo pre_axys = {-1, -1, -1, -1};
    const char* pre_device = NULL;
    const char* geometry = NULL;
    unsigned thr_misclick = 15;
    unsigned thr_doubleclick = 7;

    /* parse input */
    if (argc > 1) {
        int i;
        for (i=1; i!=argc; i++) {
            /* Display help ? */
            if (strcmp("-h", argv[i]) == 0 ||
                strcmp("--help", argv[i]) == 0) {
                fprintf(stderr, "xinput_calibratior, v%s\n\n", "0.0.0");
                usage(argv[0], thr_misclick);
                exit(0);
            } else

            /* Verbose output ? */
            if (strcmp("-v", argv[i]) == 0 ||
                strcmp("--verbose", argv[i]) == 0) {
                verbose = true;
            } else

            /* Just list devices ? */
            if (strcmp("--list", argv[i]) == 0) {
                list_devices = true;
            } else

            /* Select specific device ? */
            if (strcmp("--device", argv[i]) == 0) {
                if (argc > i+1)
                    pre_device = argv[++i];
                else {
                    fprintf(stderr, "Error: --device needs a device name or id as argument; use --list to list the calibratable input devices.\n\n");
                    usage(argv[0], thr_misclick);
                    exit(1);
                }
            } else

            /* Get pre-calibration ? */
            if (strcmp("--precalib", argv[i]) == 0) {
                precalib = true;
                if (argc > i+1)
                    pre_axys.x_min = atoi(argv[++i]);
                if (argc > i+1)
                    pre_axys.x_max = atoi(argv[++i]);
                if (argc > i+1)
                    pre_axys.y_min = atoi(argv[++i]);
                if (argc > i+1)
                    pre_axys.y_max = atoi(argv[++i]);
            } else

            /* Get mis-click threshold ? */
            if (strcmp("--misclick", argv[i]) == 0) {
                if (argc > i+1)
                    thr_misclick = atoi(argv[++i]);
                else {
                    fprintf(stderr, "Error: --misclick needs a number (the pixel threshold) as argument. Set to 0 to disable mis-click detection.\n\n");
                    usage(argv[0], thr_misclick);
                    exit(1);
                }
            } else

            /* specify window geometry? */
            if (strcmp("--geometry", argv[i]) == 0) {
                geometry = argv[++i];
                /* sscanf(argv[++i],"%dx%d",&win_width,&win_height); */
            } else

            /* Fake calibratable device ? */
            if (strcmp("--fake", argv[i]) == 0) {
                fake = true;
            }
            
            /* unknown option */
            else {
                fprintf(stderr, "Unknown option: %s\n\n", argv[i]);
                usage(argv[0], thr_misclick);
                exit(0);
            }
        }
    }
    

    /* Choose the device to calibrate */
    XID         device_id   = (XID) -1;
    const char* device_name = NULL;
    XYinfo      device_axys = {-1, -1, -1, -1};
    if (fake) {
        /* Fake a calibratable device */
        device_name = "Fake_device";
        device_axys.x_min=0;
        device_axys.x_max=1000;
        device_axys.y_min=0;
        device_axys.y_max=1000;

        if (verbose) {
            printf("DEBUG: Faking device: %s\n", device_name);
        }
    } else {
        /* Find the right device */
        int nr_found = find_device(pre_device, verbose, list_devices, &device_id, &device_name, &device_axys);

        if (list_devices) {
            /* printed the list in find_device */
            if (nr_found == 0)
                printf("No calibratable devices found.\n");
            exit(0);
        }

        if (nr_found == 0) {
            if (pre_device == NULL)
                fprintf (stderr, "Error: No calibratable devices found.\n");
            else
                fprintf (stderr, "Error: Device \"%s\" not found; use --list to list the calibratable input devices.\n", pre_device);
            exit(1);

        } else if (nr_found > 1) {
            printf ("Warning: multiple calibratable devices found, calibrating last one (%s)\n\tuse --device to select another one.\n", device_name);
        }

        if (verbose) {
            printf("DEBUG: Selected device: %s\n", device_name);
        }
    }

    /* override min/max XY from command line ? */
    if (precalib) {
        if (pre_axys.x_min != -1)
            device_axys.x_min = pre_axys.x_min;
        if (pre_axys.x_max != -1)
            device_axys.x_max = pre_axys.x_max;
        if (pre_axys.y_min != -1)
            device_axys.y_min = pre_axys.y_min;
        if (pre_axys.y_max != -1)
            device_axys.y_max = pre_axys.y_max;

        if (verbose) {
            printf("DEBUG: Setting precalibration: %i, %i, %i, %i\n",
                device_axys.x_min, device_axys.x_max,
                device_axys.y_min, device_axys.y_max);
        }
    }

    /* lastly, presume a standard Xorg driver (evtouch, mutouch, ...) */
    return CalibratorXorgPrint(device_name, &device_axys,
            verbose, thr_misclick, thr_doubleclick, geometry);
}

struct Calib* CalibratorXorgPrint(const char* const device_name0, const XYinfo *axys0, const bool verbose0, const int thr_misclick, const int thr_doubleclick, const char* geometry)
{
    struct Calib* c = (struct Calib*)calloc(1, sizeof(struct Calib));
    c->old_axys = *axys0;
    c->threshold_misclick = thr_misclick;
    c->threshold_doubleclick = thr_doubleclick;
    c->geometry = geometry;

    printf("Calibrating standard Xorg driver \"%s\"\n", device_name0);
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
    success &= output_xorgconfd(c, new_axys, swap_xy, new_swap_xy);

    return success;
}

bool output_xorgconfd(struct Calib* c, const XYinfo new_axys, int swap_xy, int new_swap_xy)
{
    const char* sysfs_name = "!!Name_Of_TouchScreen!!";

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

    return true;
}

int main(int argc, char** argv)
{
    int success = 0;
    XYinfo axys;
    bool swap_xy;

    struct Calib* calibrator = main_common(argc, argv);

    /* GTK setup */
    gtk_init(&argc, &argv);

    success = run_gui(calibrator, &axys, &swap_xy);
    if (success)
        success = finish_data(calibrator, axys, swap_xy);

    if (!success) {
        /* TODO, in GUI ? */
        fprintf(stderr, "Error: unable to apply or save configuration values\n");
    }

    free(calibrator);
    return success;
}
