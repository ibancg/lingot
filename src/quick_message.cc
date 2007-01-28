//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004, 2005  Ibán Cereijo Graña, Jairo Chapela Martínez.

  This file is part of lingot.

  lingot is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  lingot is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with lingot; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* needed includes for internacionalization */
#include <libintl.h>
#include <locale.h>
#include <langinfo.h>

#include "quick_message.h"

void quick_message(gchar *title, gchar *message) {

  GtkWidget *dialog, *label, *okay_button;
  
  /* Create the widgets */
  dialog = gtk_dialog_new();
  label = gtk_label_new (message);

  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);

  okay_button = gtk_button_new_with_label(gettext("Ok"));
  
  /* Ensure that the dialog box is destroyed when the user clicks ok. */
  
  gtk_signal_connect_object (GTK_OBJECT (okay_button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT(dialog));
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area),
		     okay_button);

  /* Add the label, and show everything we've added to the dialog. */
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
		     label);
  gtk_widget_show_all (dialog);
}
