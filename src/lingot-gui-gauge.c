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

static int gauge_size_x = 0;
static int gauge_size_y = 0;

static void lingot_gui_mainframe_cairo_set_source_argb(cairo_t *cr,
                                                       unsigned int color) {
    cairo_set_source_rgba(cr,
                          0.00392156862745098 * ((color >> 16) & 0xff),
                          0.00392156862745098 * ((color >> 8) & 0xff),
                          0.00392156862745098 * (color & 0xff),
                          1.0 - 0.00392156862745098 * ((color >> 24) & 0xff));
}

typedef struct {
    LINGOT_FLT x;
    LINGOT_FLT y;
} point_t;

static void lingot_gui_gauge_draw_tic(cairo_t *cr,
                                      const point_t* gaugeCenter, double radius1, double radius2,
                                      double angle) {
    cairo_move_to(cr,
                  gaugeCenter->x + radius1 * sin(angle),
                  gaugeCenter->y - radius1 * cos(angle));
    cairo_rel_line_to(cr,
                      (radius2 - radius1) * sin(angle),
                      (radius1 - radius2) * cos(angle));
    cairo_stroke(cr);
}

static void lingot_gui_gauge_redraw_background(cairo_t *cr,
                                               lingot_main_frame_t* frame) {

    // normalized dimensions
    static const LINGOT_FLT gauge_gaugeCenterY = 0.94;
    static const LINGOT_FLT gauge_centsBarStroke = 0.025;
    static const LINGOT_FLT gauge_centsBarRadius = 0.75;
    static const LINGOT_FLT gauge_centsBarMajorTicRadius = 0.04;
    static const LINGOT_FLT gauge_centsBarMinorTicRadius = 0.03;
    static const LINGOT_FLT gauge_centsBarMajorTicStroke = 0.03;
    static const LINGOT_FLT gauge_centsBarMinorTicStroke = 0.01;
    static const LINGOT_FLT gauge_centsTextSize = 0.09;
    static const LINGOT_FLT gauge_frequencyBarStroke = 0.025;
    static const LINGOT_FLT gauge_frequencyBarRadius = 0.78;
    static const LINGOT_FLT gauge_frequencyBarMajorTicRadius = 0.04;
    static const LINGOT_FLT gauge_okBarStroke = 0.07;
    static const LINGOT_FLT gauge_okBarRadius = 0.48;

    static const LINGOT_FLT overtureAngle = 65.0 * M_PI / 180.0;

    // colors
    static const unsigned int gauge_centsBarColor = 0x333355;
    static const unsigned int gauge_frequencyBarColor = 0x555533;
    static const unsigned int gauge_okColor = 0x99dd99;
    static const unsigned int gauge_koColor = 0xddaaaa;

    const int width = gauge_size_x;
    int height = gauge_size_y;

    // dimensions applied to the current size
    point_t gaugeCenter = { .x = width / 2, .y = height * gauge_gaugeCenterY };

    if (width < 1.6 * height) {
        height = (int) (width / 1.6);
        gaugeCenter.y = 0.5 * (gauge_size_y - height)
                + height * gauge_gaugeCenterY;
    }

    const LINGOT_FLT centsBarRadius = height * gauge_centsBarRadius;
    const LINGOT_FLT centsBarStroke = height * gauge_centsBarStroke;
    const LINGOT_FLT centsBarMajorTicRadius = centsBarRadius
            - height * gauge_centsBarMajorTicRadius;
    const LINGOT_FLT centsBarMinorTicRadius = centsBarRadius
            - height * gauge_centsBarMinorTicRadius;
    const LINGOT_FLT centsBarMajorTicStroke = height * gauge_centsBarMajorTicStroke;
    const LINGOT_FLT centsBarMinorTicStroke = height * gauge_centsBarMinorTicStroke;
    const LINGOT_FLT centsTextSize = height * gauge_centsTextSize;
    const LINGOT_FLT frequencyBarRadius = height * gauge_frequencyBarRadius;
    const LINGOT_FLT frequencyBarMajorTicRadius = frequencyBarRadius
            + height * gauge_frequencyBarMajorTicRadius;
    const LINGOT_FLT frequencyBarStroke = height * gauge_frequencyBarStroke;
    const LINGOT_FLT okBarRadius = height * gauge_okBarRadius;
    const LINGOT_FLT okBarStroke = height * gauge_okBarStroke;

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_save(cr);
    const GdkRectangle r = { .x = 0, .y = 0, .width = gauge_size_x, .height =
                             gauge_size_y };
    gdk_cairo_rectangle(cr, &r);
    cairo_fill_preserve(cr);
    cairo_restore(cr);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_stroke(cr);

    // draw ok/ko bar
    cairo_set_line_width(cr, okBarStroke);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    lingot_gui_mainframe_cairo_set_source_argb(cr, gauge_koColor);
    cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, okBarRadius,
              -0.5 * M_PI - overtureAngle, -0.5 * M_PI + overtureAngle);
    cairo_stroke(cr);
    lingot_gui_mainframe_cairo_set_source_argb(cr, gauge_okColor);
    cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, okBarRadius,
              -0.5 * M_PI - 0.1 * overtureAngle,
              -0.5 * M_PI + 0.1 * overtureAngle);
    cairo_stroke(cr);

    // draw cents bar
    cairo_set_line_width(cr, centsBarStroke);
    lingot_gui_mainframe_cairo_set_source_argb(cr, gauge_centsBarColor);
    cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, centsBarRadius,
              -0.5 * M_PI - 1.05 * overtureAngle,
              -0.5 * M_PI + 1.05 * overtureAngle);
    cairo_stroke(cr);

    // cent tics
    const double gaugeRange = frame->conf.gauge_range;
    static const int maxMinorDivisions = 20;
    double centsPerMinorDivision = gaugeRange / maxMinorDivisions;
    const double base = pow(10.0, floor(log10(centsPerMinorDivision)));
    double normalizedCentsPerDivision = centsPerMinorDivision / base;
    if (normalizedCentsPerDivision >= 6.0) {
        normalizedCentsPerDivision = 10.0;
    } else if (normalizedCentsPerDivision >= 2.5) {
        normalizedCentsPerDivision = 5.0;
    } else if (normalizedCentsPerDivision >= 1.2) {
        normalizedCentsPerDivision = 2.0;
    } else {
        normalizedCentsPerDivision = 1.0;
    }
    centsPerMinorDivision = normalizedCentsPerDivision * base;
    const double centsPerMajorDivision = 5.0 * centsPerMinorDivision;

    // minor tics
    cairo_set_line_width(cr, centsBarMinorTicStroke);
    int maxIndex = (int) floor(0.5 * gaugeRange / centsPerMinorDivision);
    double angleStep = 2.0 * overtureAngle * centsPerMinorDivision
            / gaugeRange;
    int index;
    for (index = -maxIndex; index <= maxIndex; index++) {
        const double angle = index * angleStep;
        lingot_gui_gauge_draw_tic(cr, &gaugeCenter,
                                  centsBarMinorTicRadius, centsBarRadius, angle);
    }

    // major tics
    maxIndex = (int) floor(0.5 * gaugeRange / centsPerMajorDivision);
    angleStep = 2.0 * overtureAngle * centsPerMajorDivision / gaugeRange;
    cairo_set_line_width(cr, centsBarMajorTicStroke);
    for (index = -maxIndex; index <= maxIndex; index++) {
        double angle = index * angleStep;
        lingot_gui_gauge_draw_tic(cr, &gaugeCenter,
                                  centsBarMajorTicRadius, centsBarRadius, angle);
    }

    // cents text
    cairo_set_line_width(cr, 1.0);
    double oldAngle = 0.0;

    cairo_save(cr);

    static char buff[10];

    cairo_text_extents_t te;
    cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, centsTextSize);

    sprintf(buff, "%s", "cent");
    cairo_text_extents(cr, buff, &te);
    cairo_move_to(cr, gaugeCenter.x - te.width / 2 - te.x_bearing,
                  gaugeCenter.y - 0.81 * centsBarMajorTicRadius - te.height / 2
                  - te.y_bearing);
    cairo_show_text(cr, buff);

    cairo_translate(cr, gaugeCenter.x, gaugeCenter.y);
    for (index = -maxIndex; index <= maxIndex; index++) {
        double angle = index * angleStep;
        cairo_rotate(cr, angle - oldAngle);
        int cents = (int) (index * centsPerMajorDivision);
        sprintf(buff, "%s%i", ((cents > 0) ? "+" : ""), cents);
        cairo_text_extents(cr, buff, &te);
        cairo_move_to(cr, -te.width / 2 - te.x_bearing,
                      -0.92 * centsBarMajorTicRadius - te.height / 2 - te.y_bearing);
        cairo_show_text(cr, buff);
        oldAngle = angle;
    }
    cairo_restore(cr);
    cairo_stroke(cr);

    // draw frequency bar
    cairo_set_line_width(cr, frequencyBarStroke);
    lingot_gui_mainframe_cairo_set_source_argb(cr, gauge_frequencyBarColor);
    cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, frequencyBarRadius,
              -0.5 * M_PI - 1.05 * overtureAngle,
              -0.5 * M_PI + 1.05 * overtureAngle);
    cairo_stroke(cr);

    // frequency tics
    lingot_gui_gauge_draw_tic(cr, &gaugeCenter,
                              frequencyBarMajorTicRadius, frequencyBarRadius, 0.0);
}

void lingot_gui_gauge_redraw(GtkWidget *w, cairo_t *cr, lingot_main_frame_t* frame) {

    GtkAllocation req;
    gtk_widget_get_allocation(w, &req);

    if ((req.width != gauge_size_x) || (req.height != gauge_size_y)) {
        gauge_size_x = req.width;
        gauge_size_y = req.height;
    }

    // normalized dimensions
    static const LINGOT_FLT gauge_gaugeCenterY = 0.94;
    static const LINGOT_FLT gauge_gaugeLength = 0.85;
    static const LINGOT_FLT gauge_gaugeLengthBack = 0.08;
    static const LINGOT_FLT gauge_gaugeCenterRadius = 0.045;
    static const LINGOT_FLT gauge_gaugeStroke = 0.012;
    static const LINGOT_FLT gauge_gaugeShadowOffsetX = 0.015;
    static const LINGOT_FLT gauge_gaugeShadowOffsetY = 0.01;

    static const LINGOT_FLT overtureAngle = 65.0 * M_PI / 180.0;

    // colors
    static const unsigned int gauge_gaugeColor = 0xaa3333;
    static const unsigned int gauge_gaugeShadowColor = 0x88000000;

    const int width = gauge_size_x;
    int height = gauge_size_y;

    // dimensions applied to the current size
    point_t gaugeCenter = { .x = width / 2, .y = height * gauge_gaugeCenterY };

    if (width < 1.6 * height) {
        height = (int) (width / 1.6);
        gaugeCenter.y = 0.5 * (gauge_size_y - height) + height * gauge_gaugeCenterY;
    }

    const point_t gaugeShadowCenter = { .x = gaugeCenter.x
                                        + height * gauge_gaugeShadowOffsetX, .y = gaugeCenter.y
                                        + height * gauge_gaugeShadowOffsetY };
    const LINGOT_FLT gaugeLength = height * gauge_gaugeLength;
    const LINGOT_FLT gaugeLengthBack = height * gauge_gaugeLengthBack;
    const LINGOT_FLT gaugeCenterRadius = height * gauge_gaugeCenterRadius;
    const LINGOT_FLT gaugeStroke = height * gauge_gaugeStroke;

    lingot_gui_gauge_redraw_background(cr, frame);

    const double normalized_error = frame->gauge_pos / frame->conf.gauge_range;
    const double angle = 2.0 * normalized_error * overtureAngle;
    cairo_set_line_width(cr, gaugeStroke);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    lingot_gui_mainframe_cairo_set_source_argb(cr, gauge_gaugeShadowColor);
    lingot_gui_gauge_draw_tic(cr, &gaugeShadowCenter,
                              -gaugeLengthBack, -0.99 * gaugeCenterRadius, angle);
    lingot_gui_gauge_draw_tic(cr, &gaugeShadowCenter,
                              0.99 * gaugeCenterRadius, gaugeLength, angle);
    cairo_arc(cr, gaugeShadowCenter.x, gaugeShadowCenter.y, gaugeCenterRadius,
              0, 2 * M_PI);
    cairo_fill(cr);
    lingot_gui_mainframe_cairo_set_source_argb(cr, gauge_gaugeColor);
    lingot_gui_gauge_draw_tic(cr, &gaugeCenter, -gaugeLengthBack,
                              gaugeLength, angle);
    cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, gaugeCenterRadius, 0, 2 * M_PI);
    cairo_fill(cr);
}
