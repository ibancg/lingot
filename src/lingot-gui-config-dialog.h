/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2013  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#ifndef __LINGOT_GUI_CONFIG_DIALOG_H__
#define __LINGOT_GUI_CONFIG_DIALOG_H__

#include <gtk/gtk.h>

#include "lingot-config.h"

typedef struct _LingotConfigDialog LingotConfigDialog;

struct _LingotConfigDialog {

	// widgets that contains configuration information.
	GtkComboBoxText* input_system;
	GtkComboBoxText* input_dev;
	GtkComboBoxText* sample_rate;
	GtkHScale* calculation_rate;
	GtkHScale* visualization_rate;
	GtkHScale* noise_threshold;
	GtkHScale* gain;
	GtkSpinButton* oversampling;
	GtkComboBoxText* fft_size;
	GtkSpinButton* temporal_window;
	GtkSpinButton* root_frequency_error;
	GtkSpinButton* dft_number;
	GtkSpinButton* dft_size;
	GtkSpinButton* peak_number;
	GtkSpinButton* peak_halfwidth;
	GtkComboBoxText* minimum_frequency;
	GtkComboBoxText* maximum_frequency;
	GtkHScale* rejection_peak_relation;
	GtkLabel* label_sample_rate1;
	GtkLabel* label_sample_rate2;
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

	LingotConfig* conf; // provisional configuration.
	LingotConfig* conf_old; // restoration point for cancel.

	LingotMainFrame* mainframe;

	GtkWidget* win; // window
};

//LingotConfigDialog* lingot_config_dialog_new(LingotMainFrame*);
void lingot_gui_config_dialog_destroy(LingotConfigDialog*);
void lingot_gui_config_dialog_show(LingotMainFrame* frame, LingotConfig* config);

#endif // __LINGOT_GUI_CONFIG_DIALOG_H__
