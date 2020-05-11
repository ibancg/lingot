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

static int spectrum_size_x = 0;
static int spectrum_size_y = 0;

static LINGOT_FLT spectrum_bottom_margin;
static LINGOT_FLT spectrum_top_margin;
static LINGOT_FLT spectrum_left_margin;
static LINGOT_FLT spectrum_right_margin;

static LINGOT_FLT spectrum_inner_x;
static LINGOT_FLT spectrum_inner_y;

static const LINGOT_FLT spectrum_min_db = 0; // TODO
static const LINGOT_FLT spectrum_max_db = 52;
static LINGOT_FLT spectrum_db_density = 0;

static char* lingot_gui_spectrum_format_frequency(LINGOT_FLT freq, char* buff) {
    if (freq == 0.0) {
        sprintf(buff, "0 Hz");
    } else if (floor(freq) == freq)
        sprintf(buff, "%0.0f kHz", freq);
    else if (floor(10 * freq) == 10 * freq) {
        if (freq <= 1000.0)
            sprintf(buff, "%0.0f Hz", 1e3 * freq);
        else
            sprintf(buff, "%0.1f kHz", freq);
    } else {
        if (freq <= 100.0)
            sprintf(buff, "%0.0f Hz", 1e3 * freq);
        else
            sprintf(buff, "%0.2f kHz", freq);
    }

    return buff;
}

static LINGOT_FLT lingot_gui_spectrum_get_in_bounds(LINGOT_FLT value, LINGOT_FLT min, LINGOT_FLT max) {
    if (value < min) {
        value = min;
    } else if (value > max) {
        value = max;
    }
    return value - min;
}

static void lingot_gui_spectrum_redraw_background(cairo_t *cr, lingot_main_frame_t *frame) {

    const LINGOT_FLT font_size = 8 + spectrum_size_y / 30;

    static char buff[10];
    static char buff2[10];
    cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size);
    sprintf(buff, "%0.0f", spectrum_min_db);
    sprintf(buff2, "%0.0f", spectrum_max_db);
    cairo_text_extents_t te;
    cairo_text_extents_t te2;
    cairo_text_extents(cr, buff, &te);
    cairo_text_extents(cr, buff2, &te2);
    if (te2.width > te.width) {
        te = te2;
    }

    // spectrum area margins
    spectrum_bottom_margin = 1.6 * te.height;
    spectrum_top_margin = spectrum_bottom_margin;
    spectrum_left_margin = te.width * 1.5;
    spectrum_right_margin = 0.03 * spectrum_size_x;
    if (spectrum_right_margin > 0.8 * spectrum_left_margin) {
        spectrum_right_margin = 0.8 * spectrum_left_margin;
    }
    spectrum_inner_x = spectrum_size_x - spectrum_left_margin - spectrum_right_margin;
    spectrum_inner_y = spectrum_size_y - spectrum_bottom_margin - spectrum_top_margin;

    sprintf(buff, "000 Hz");
    cairo_text_extents(cr, buff, &te);
    // minimum grid size in pixels
    const int minimum_grid_width = (int) (1.5 * te.width);
    const int minimum_grid_height = (int) (3.0 * te.height);

    // clear all
    cairo_set_source_rgba(cr, 0.06, 0.2, 0.06, 1.0);
    GdkRectangle r = { .x = 0, .y = 0, .width = spectrum_size_x, .height = spectrum_size_y };
    gdk_cairo_rectangle(cr, &r);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, 0.56, 0.56, 0.56, 1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_move_to(cr, spectrum_left_margin,
                  spectrum_size_y - spectrum_bottom_margin);
    cairo_rel_line_to(cr, spectrum_inner_x, 0);
    cairo_stroke(cr);

    // choose scale factor
    const LINGOT_FLT spectrum_max_frequency = 0.5 * frame->conf.sample_rate
            / frame->conf.oversampling;

    // scale factors (in KHz) to draw the grid. We will choose the smaller
    // factor that respects the minimum_grid_width
    static const double scales[] = { 0.01, 0.05, 0.1, 0.2, 0.5, 1, 2, 4, 11, 22, -1.0 };

    int i;
    for (i = 0; scales[i + 1] > 0.0; i++) {
        if ((1e3 * scales[i] * spectrum_inner_x / spectrum_max_frequency)
                > minimum_grid_width)
            break;
    }

    LINGOT_FLT scale = scales[i];
    LINGOT_FLT grid_width = 1e3 * scales[i] * spectrum_inner_x
            / spectrum_max_frequency;

    LINGOT_FLT freq = 0.0;
    LINGOT_FLT x;
    for (x = 0.0; x <= spectrum_inner_x; x += grid_width) {
        cairo_move_to(cr, spectrum_left_margin + x, spectrum_top_margin);
        cairo_rel_line_to(cr, 0, spectrum_inner_y + 3); // TODO: proportion
        cairo_stroke(cr);

        lingot_gui_spectrum_format_frequency(freq, buff);

        cairo_text_extents(cr, buff, &te);
        cairo_move_to(cr,
                      spectrum_left_margin + x + 6 - te.width / 2 - te.x_bearing,
                      spectrum_size_y - 0.5 * spectrum_bottom_margin - te.height / 2
                      - te.y_bearing);
        cairo_show_text(cr, buff);
        freq += scale;
    }

    spectrum_db_density = (spectrum_inner_y)
            / (spectrum_max_db - spectrum_min_db);

    sprintf(buff, "dB");

    cairo_text_extents(cr, buff, &te);
    cairo_move_to(cr, spectrum_left_margin - te.x_bearing,
                  0.5 * spectrum_top_margin - te.height / 2 - te.y_bearing);
    cairo_show_text(cr, buff);

    // scale factors (in KHz) to draw the grid. We will choose the smallest
    // factor that respects the minimum_grid_width
    static const int db_scales[] = { 5, 10, 20, 25, 50, 75, 100, -1 };
    for (i = 0; db_scales[i + 1] > 0; i++) {
        if ((db_scales[i] * spectrum_db_density) > minimum_grid_height)
            break;
    }

    const int db_scale = db_scales[i];

    LINGOT_FLT y = 0;
    int i0 = (int) ceil(spectrum_min_db / db_scale);
    if (spectrum_min_db < 0.0) {
        i0--;
    }
    int i1 = (int) ceil(spectrum_max_db / db_scale);
    for (i = i0; i <= i1; i++) {
        y = spectrum_db_density * (i * db_scale - spectrum_min_db);
        if ((y < 0.0) || (y > spectrum_inner_y)) {
            continue;
        }
        sprintf(buff, "%d", i * db_scale);

        cairo_text_extents(cr, buff, &te);
        cairo_move_to(cr,
                      0.45 * spectrum_left_margin - te.width / 2 - te.x_bearing,
                      spectrum_size_y - spectrum_bottom_margin - y - te.height / 2
                      - te.y_bearing);
        cairo_show_text(cr, buff);

        cairo_move_to(cr, spectrum_left_margin - 3,
                      spectrum_size_y - spectrum_bottom_margin - y);
        cairo_rel_line_to(cr, spectrum_inner_x + 3, 0);
        cairo_stroke(cr);
    }
}

gboolean lingot_gui_spectrum_redraw(GtkWidget *w, cairo_t *cr, lingot_main_frame_t* frame) {

    unsigned int i;

    GtkAllocation req;
    gtk_widget_get_allocation(w, &req);
    if ((req.width != spectrum_size_x) || (req.height != spectrum_size_y)) {
        spectrum_size_x = req.width;
        spectrum_size_y = req.height;
    }

    lingot_gui_spectrum_redraw_background(cr, frame);

    // spectrum drawing.
    if (lingot_core_thread_is_running(&frame->core)) {

        cairo_set_line_width(cr, 1.0);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);

        LINGOT_FLT x;
        LINGOT_FLT y = -1;

        const unsigned int min_index = 0;
        const unsigned int max_index = frame->conf.fft_size / 2;

        LINGOT_FLT index_density = spectrum_inner_x / max_index;
        // TODO: step
        const unsigned int index_step = 1;

        static const double dashed1[] = { 5.0, 5.0 };
        static int len1 = sizeof(dashed1) / sizeof(dashed1[0]);

        const LINGOT_FLT x0 = spectrum_left_margin;
        const LINGOT_FLT y0 = spectrum_size_y - spectrum_bottom_margin;

        LINGOT_FLT dydxm1 = 0;

        cairo_set_source_rgba(cr, 0.13, 1.0, 0.13, 1.0);

        cairo_translate(cr, x0, y0);
        cairo_rectangle(cr, 1.0, -1.0, spectrum_inner_x - 2,
                        -(spectrum_inner_y - 2));
        cairo_clip(cr);
        cairo_new_path(cr); // path not consumed by clip()

        LINGOT_FLT* spd = lingot_core_thread_get_result_spd(&frame->core);

        y = -spectrum_db_density
                * lingot_gui_spectrum_get_in_bounds(spd[min_index],
                                                     spectrum_min_db, spectrum_max_db); // dB.

        cairo_move_to(cr, 0, 0);
        cairo_line_to(cr, 0, y);

        LINGOT_FLT yp1 = -spectrum_db_density
                * lingot_gui_spectrum_get_in_bounds(spd[min_index + 1],
                spectrum_min_db, spectrum_max_db);
        LINGOT_FLT ym1 = y;

        for (i = index_step; i < max_index - 1; i += index_step) {

            x = index_density * i;
            ym1 = y;
            y = yp1;
            yp1 = -spectrum_db_density
                    * lingot_gui_spectrum_get_in_bounds(spd[i + 1],
                    spectrum_min_db, spectrum_max_db);
            LINGOT_FLT dydx = (yp1 - ym1) / (2 * index_density);
            static const LINGOT_FLT dx = 0.4;
            LINGOT_FLT x1 = x - (1 - dx) * index_density;
            LINGOT_FLT x2 = x - dx * index_density;
            LINGOT_FLT y1 = ym1 + dydxm1 * dx;
            LINGOT_FLT y2 = y - dydx * dx;

            dydxm1 = dydx;
            cairo_curve_to(cr, x1, y1, x2, y2, x, y);
            //			cairo_line_to(cr, x, y);
        }

        y = -spectrum_db_density
                * lingot_gui_spectrum_get_in_bounds(spd[max_index - 1],
                spectrum_min_db, spectrum_max_db); // dB.
        cairo_line_to(cr, index_density * max_index, y);
        cairo_line_to(cr, index_density * max_index, 0);
        cairo_close_path(cr);
        cairo_fill_preserve(cr);
        //		cairo_restore(cr);
        cairo_stroke(cr);

        LINGOT_FLT freq = lingot_core_thread_get_result_frequency(&frame->core);
        LINGOT_FLT target_freq = lingot_config_scale_get_frequency(&frame->conf.scale,
                                                                   frame->closest_note_index);

        if (freq != 0.0) {

            // target frequency mark with a blue vertical line.
            // cairo_set_dash(cr, dashed1, len1, 0);
            cairo_set_source_rgba(cr, 0.23, 0.83, 1.0, 1.0);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

            cairo_set_line_width(cr, 1.0);

            // index of closest sample to fundamental frequency.
            x = index_density * target_freq * frame->conf.fft_size
                    * frame->conf.oversampling / frame->conf.sample_rate;
            cairo_move_to(cr, x, 0);
            cairo_rel_line_to(cr, 0.0, -spectrum_inner_y);
            cairo_stroke(cr);


            // fundamental frequency mark with a red vertical line.
            cairo_set_dash(cr, dashed1, len1, 0);
            cairo_set_source_rgba(cr, 1.0, 0.13, 0.13, 1.0);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

            cairo_set_line_width(cr, 2.0);

            // index of closest sample to fundamental frequency.
            x = index_density * freq * frame->conf.fft_size
                    * frame->conf.oversampling / frame->conf.sample_rate;
            cairo_move_to(cr, x, 0);
            cairo_rel_line_to(cr, 0.0, -spectrum_inner_y);
            cairo_stroke(cr);

            cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
            cairo_set_line_width(cr, 1.0);
        }

        cairo_set_dash(cr, dashed1, len1, 0);
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 1.0);

        LINGOT_FLT y_min = -spectrum_db_density
                * lingot_gui_spectrum_get_in_bounds(frame->conf.min_overall_SNR,
                                                     spectrum_min_db,
                                                     spectrum_max_db); // dB.
        y = y_min;
        cairo_move_to(cr, 0, y);
        // noise threshold drawing.
        for (i = min_index + index_step; i < max_index - 1; i += index_step) {

            x = index_density * i;
            y = y_min;
            cairo_line_to(cr, x, y);
        }
        cairo_stroke(cr);

    }

    return FALSE;
}
