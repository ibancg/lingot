/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2020  Iban Cereijo.
 * Copyright (C) 2004-2008  Jairo Chapela.

 *
 * This file is part of lingot.
 *
 * lingot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * lingot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with lingot; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtk/gtk.h>
#include <math.h>

#include "lingot-defs-internal.h"
#include "lingot-gui-mainframe.h"

static int strobe_disc_size_x = 0;
static int strobe_disc_size_y = 0;

double strobe_angle = 0.0;
double strobe_angle_computation_rate = 0.0;
cairo_surface_t* strobe_image = NULL;

extern GResource *lingot_get_resource (void);

static cairo_status_t read_png(void* closure,
                               unsigned char* data,
                               unsigned int length) {
    GInputStream* is = closure;
    gsize res = g_input_stream_read (is, data, length, NULL, NULL);
    return res ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_READ_ERROR;
}

void lingot_gui_strobe_disc_init(double computation_rate) {

    GInputStream* is = g_resources_open_stream("/org/nongnu/lingot/lingot-strobe.png",
                                               G_RESOURCE_LOOKUP_FLAGS_NONE,
                                               NULL);
    strobe_angle_computation_rate = computation_rate;
    strobe_image = cairo_image_surface_create_from_png_stream(read_png, is);
    g_object_unref(is);
}

void lingot_gui_strobe_disc_set_error(double error) {
    strobe_angle += 1e-1 * error / strobe_angle_computation_rate;
}

void lingot_gui_strobe_disc_redraw(GtkWidget *widget, cairo_t *cr, lingot_main_frame_t* frame) {

    (void) frame; //  Unused parameter.

    GtkAllocation req;
    gtk_widget_get_allocation(widget, &req);

    if ((req.width != strobe_disc_size_x) || (req.height != strobe_disc_size_y)) {
        strobe_disc_size_x = req.width;
        strobe_disc_size_y = req.height;
    }

    const int width = strobe_disc_size_x;
    int height = strobe_disc_size_y;

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    const GdkRectangle r = { .x = 0, .y = 0, .width = strobe_disc_size_x, .height = strobe_disc_size_y };
    gdk_cairo_rectangle(cr, &r);
    cairo_fill_preserve(cr);

    int w = cairo_image_surface_get_width(strobe_image);
    int h = cairo_image_surface_get_height(strobe_image);

    cairo_translate(cr, width / 2.0, 1.5 * height);
    cairo_rotate(cr, strobe_angle);
    double s = 1.5 * width / w;
    cairo_scale(cr, s, s);
    cairo_translate(cr, -w / 2.0, -h / 2.0);
    cairo_set_source_surface(cr, strobe_image, 0, 0);
    cairo_paint(cr);
}
