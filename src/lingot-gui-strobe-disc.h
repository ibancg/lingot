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

#ifndef LINGOT_GUI_STROBE_DISCE_H
#define LINGOT_GUI_STROBE_DISCE_H

// Drawing routines for the strobe disc widget.

#include <gtk/gtk.h>
#include "lingot-gui-mainframe.h"

void lingot_gui_strobe_disc_init(double computation_rate);
void lingot_gui_strobe_disc_set_error(double error);
gboolean lingot_gui_strobe_disc_redraw(GtkWidget *w, cairo_t *cr, lingot_main_frame_t *);

#endif // LINGOT_GUI_STROBE_DISCE_H
