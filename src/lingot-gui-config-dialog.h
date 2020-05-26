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
 * along with lingot; if not, write to the Free Software Foundation,
 * Inc. 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef LINGOT_GUI_CONFIG_DIALOG_H
#define LINGOT_GUI_CONFIG_DIALOG_H

#include <gtk/gtk.h>

#include "lingot-config.h"

// callback when the config dialog is closed
typedef void (*lingot_gui_config_dialog_callback_closed_t)(void* param);

// callback when the config dialog changes the config
typedef void (*lingot_gui_config_dialog_callback_config_changed_t)(lingot_config_t* config,
                                                                   void* param);

typedef struct _lingot_config_dialog_t {

    // widgets that contains configuration information.
    GtkComboBoxText* input_system;
    GtkComboBoxText* input_dev;
    GtkScale* calculation_rate;
    GtkScale* visualization_rate;
    GtkScale* noise_threshold;
    GtkComboBoxText* fft_size;
    GtkSpinButton* temporal_window;
    GtkSpinButton* root_frequency_error;
    GtkComboBoxText* minimum_frequency;
    GtkComboBoxText* maximum_frequency;
    GtkComboBoxText* octave;
    GtkCheckButton* optimize_check_button;
    GtkLabel* fft_size_label;
    GtkLabel* fft_size_units_label;
    GtkLabel* temporal_window_label;
    GtkLabel* temporal_window_units_label;

    GtkNotebook* notebook;

    GtkButton* button_scale_add;
    GtkButton* button_scale_del;

    GtkEntry* scale_name;
    GtkTreeView* scale_treeview;

    lingot_config_t conf; // provisional configuration.
    lingot_config_t conf_old; // restoration point for cancel.

    GtkWidget* win; // window

    lingot_gui_config_dialog_callback_closed_t callback_closed;
    lingot_gui_config_dialog_callback_config_changed_t callback_config_changed;
    void* callback_param;

} lingot_config_dialog_t;

lingot_config_dialog_t *lingot_gui_config_dialog_create(lingot_config_t* config,
                                                        lingot_config_t* old_config,
                                                        lingot_gui_config_dialog_callback_closed_t callback_closed,
                                                        lingot_gui_config_dialog_callback_config_changed_t callback_config_changed,
                                                        void* callback_param);
void lingot_gui_config_dialog_destroy(lingot_config_dialog_t*);

#endif // LINGOT_GUI_CONFIG_DIALOG_H
