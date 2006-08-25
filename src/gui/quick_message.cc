
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
