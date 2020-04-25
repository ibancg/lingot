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

#ifndef LINGOT_GUI_MAIN_FRAME_H
#define LINGOT_GUI_MAIN_FRAME_H

#include "lingot-defs-internal.h"
#include "lingot-core.h"
#include "lingot-config.h"
#include "lingot-filter.h"
#include "lingot-gui-config-dialog.h"

#include <gtk/gtk.h>

// Window that contains all controls, graphics, etc. of the tuner.

typedef struct _lingot_main_frame_t {

    // gtk widgets
    GtkWidget* gauge_area;
    GtkWidget* spectrum_area;
    GtkWidget* tone_label;
    GtkWidget* view_spectrum_item;
    GtkWidget* spectrum_frame;

    GtkWidget* freq_label;
    GtkWidget* error_label;

    GtkWidget* labelsbox;

    lingot_filter_t freq_filter;
    lingot_filter_t gauge_filter;

    lingot_core_t core;

    GtkWidget* win;

    GdkColor gauge_color;
    GdkColor spectrum_color;

    lingot_config_dialog_t* config_dialog;
    lingot_config_t conf;

    // timer uids
    guint visualization_timer_uid;
    guint freq_computation_timer_uid;
    guint gauge_computation_uid;
    guint error_dispatcher_uid;

    // filtered frequency and closest note index in the scale
    LINGOT_FLT frequency;
    // filtered position for the gauge
    LINGOT_FLT gauge_pos;

    int closest_note_index;
} lingot_main_frame_t;

lingot_main_frame_t* lingot_gui_mainframe_create();

#endif //LINGOT_GUI_MAIN_FRAME_H
