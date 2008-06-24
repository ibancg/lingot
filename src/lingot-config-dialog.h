//-*- C++ -*-
/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2007  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#ifndef __LINGOT_CONFIG_DIALOG_H__
#define __LINGOT_CONFIG_DIALOG_H__

#include <gtk/gtk.h>

#include "lingot-config.h"

typedef struct _LingotConfigDialog LingotConfigDialog;

struct _LingotConfigDialog
  {

    // widgets that contains configuration information.
    GtkWidget* spin_calculation_rate;
    GtkWidget* spin_visualization_rate;
    GtkWidget* spin_oversampling;
    GtkWidget* spin_root_frequency_error;
    GtkWidget* spin_temporal_window;
    GtkWidget* spin_noise_threshold;
    GtkWidget* spin_dft_number;
    GtkWidget* spin_dft_size;
    GtkWidget* spin_peak_number;
    GtkWidget* spin_peak_half_width;
    GtkWidget* spin_peak_rejection_relation;
    GtkWidget* combo_fft_size;
    GtkWidget* combo_sample_rate;

    GtkWidget* button_ok;
    GtkWidget* button_cancel;
    GtkWidget* button_apply;
    GtkWidget* button_default;

    GtkWidget* label_sample_rate;
    GtkWidget* label_root_frequency;

    LingotConfig* conf; // provisional configuration.
    LingotConfig* conf_old; // restoration point for cancel.

    LingotMainFrame* mainframe;
    
    GtkWidget* win; // window
  };

LingotConfigDialog* lingot_config_dialog_new(LingotMainFrame*);
void lingot_config_dialog_destroy(LingotConfigDialog*);

void lingot_config_dialog_rewrite(LingotConfigDialog*);
void lingot_config_dialog_apply(LingotConfigDialog*);
void lingot_config_dialog_close(LingotConfigDialog*);

#endif // __LINGOT_CONFIG_DIALOG_H__
