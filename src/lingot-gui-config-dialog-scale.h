/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2019  Iban Cereijo.
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

#ifndef LINGOT_GUI_CONFIG_DIALOG_SCALE_H
#define LINGOT_GUI_CONFIG_DIALOG_SCALE_H

#include <gtk/gtk.h>

#include "lingot-gui-config-dialog.h"

struct LingotConfigDialog;
struct LingotScale;
struct GtkBuilder;

// initialize and show the components
void lingot_gui_config_dialog_scale_show(LingotConfigDialog*, GtkBuilder*);

// validate the information stored in the table
int lingot_gui_config_dialog_scale_validate(const LingotConfigDialog* dialog,
                                            const LingotScale* scale);

// copies the information stores in the table to the internal data structure
void lingot_gui_config_dialog_scale_gui_to_data(const LingotConfigDialog* dialog,
                                                LingotScale* scale);

// fills the table with the information carried by the structure
void lingot_gui_config_dialog_scale_data_to_gui(LingotConfigDialog* dialog,
                                                const LingotScale* scale);

void lingot_gui_config_dialog_scale_callback_change_deviation(GtkWidget *widget,
                                                              LingotConfigDialog *dialog);
void lingot_gui_config_dialog_scale_callback_change_octave(GtkWidget *widget,
                                                           LingotConfigDialog *dialog);

#endif /* LINGOT_GUI_CONFIG_DIALOG_SCALE_H */
