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

#ifndef LINGOT_GUI_CONFIG_DIALOG_SCALE_H_
#define LINGOT_GUI_CONFIG_DIALOG_SCALE_H_

#include <gtk/gtk.h>

struct LingotConfigDialog;
struct LingotScale;
struct GtkBuilder;

// initialize and show the components
void lingot_gui_config_dialog_scale_show(LingotConfigDialog*, GtkBuilder*);

// validate the information stored in the table
int lingot_gui_config_dialog_scale_validate(LingotConfigDialog* dialog,
		LingotScale* scale);

// copies the information stores in the table to the internal data structure
void lingot_gui_config_dialog_scale_apply(LingotConfigDialog* dialog,
		LingotScale* scale);

// fills the table with the information carried by the structure
void lingot_gui_config_dialog_scale_rewrite(LingotConfigDialog* dialog,
		LingotScale* scale);

void lingot_gui_config_dialog_scale_callback_change_deviation(GtkWidget *widget,
		LingotConfigDialog *dialog);

#endif /* LINGOT_GUI_CONFIG_DIALOG_SCALE_H_ */
