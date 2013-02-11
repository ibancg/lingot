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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lingot-gui-config-dialog.h"
#include "lingot-gui-config-dialog-scale.h"
#include "lingot-msg.h"
#include "lingot-i18n.h"

enum {
	COLUMN_NAME = 0, COLUMN_SHIFT = 1, COLUMN_FREQUENCY = 2, NUM_COLUMNS = 3
};

void lingot_gui_config_dialog_scale_callback_change_deviation(GtkWidget *widget,
		LingotConfigDialog *dialog) {
	gtk_widget_queue_draw(GTK_WIDGET(dialog->scale_treeview) );
}

void lingot_gui_config_dialog_scale_tree_add_row_tree(gpointer data,
		GtkTreeView *treeview) {
	GtkTreeModel *model;
	GtkTreeStore *model_store;
	GtkTreeIter iter1, iter2;
	gdouble freq;

	model = gtk_tree_view_get_model(treeview);
	model_store = (GtkTreeStore *) model;

	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);

	if (gtk_tree_selection_count_selected_rows(selection) == 0) {
		gtk_tree_store_append(model_store, &iter2, NULL );
	} else {
		GList *list = gtk_tree_selection_get_selected_rows(selection, &model);

		int ipath = atoi(gtk_tree_path_to_string(list->data));
		GString *fixed_path = g_string_new("");
		g_string_printf(fixed_path, "%d", ipath);

		GtkTreePath *path = gtk_tree_path_new_from_string(fixed_path->str);
		g_string_free(fixed_path, TRUE);

		gboolean valid = FALSE;

		if (path) {
			if (gtk_tree_model_get_iter(model, &iter1, path)) { // get iter from specified path
				valid = TRUE;
			} else { // invalid path
				//			g_error("Error!!!\n");
				// TODO
			}
			gtk_tree_path_free(path);
		} else {
			//		g_error("Error!!!\n");
			// TODO
		}

		g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL );
		g_list_free(list);

		if (!valid)
			return;

		if (ipath == 0) {
			lingot_msg_add_warning(
					_("You cannot insert before the reference note."));
			return;
		}

		//	g_free(path_str);
		gtk_tree_store_insert_before(model_store, &iter2, NULL, &iter1);
	}

	gtk_tree_model_get_iter_first(model, &iter1);
	gtk_tree_model_get(model, &iter1, COLUMN_FREQUENCY, &freq, -1);

	gtk_tree_store_set(model_store, &iter2, COLUMN_NAME, "?", COLUMN_SHIFT,
			"1/1", COLUMN_FREQUENCY, freq, -1);
}

void lingot_gui_config_dialog_scale_tree_remove_selected_items(gpointer data,
		GtkTreeView *treeview) {
	GtkTreeStore *store;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);

	if (gtk_tree_selection_count_selected_rows(selection) == 0)
		return;

	GList *list = gtk_tree_selection_get_selected_rows(selection, &model);
	store = GTK_TREE_STORE(model);

	int nRemoved = 0;
	while (list) {
		int ipath = atoi(gtk_tree_path_to_string(list->data));
		ipath -= nRemoved;
		GString *fixed_path = g_string_new("");
		g_string_printf(fixed_path, "%d", ipath);

		GtkTreePath *path = gtk_tree_path_new_from_string(fixed_path->str);
		g_string_free(fixed_path, TRUE);
		if (path) {
			if (ipath != 0) {
				if (gtk_tree_model_get_iter(model, &iter, path)) { // get iter from specified path
					gtk_tree_store_remove(store, &iter); // remove item
					nRemoved++;
				} else { // invalid path
					//			g_error("Error!!!\n");
					// TODO
				}
			}
			gtk_tree_path_free(path);
		} else {
			//		g_error("Error!!!\n");
			// TODO
		}
		list = list->next;
	}
	g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL );
	g_list_free(list);
}

void lingot_gui_config_dialog_scale_tree_cell_edited_callback(
		GtkCellRendererText *cell, gchar *path_string, gchar *new_text,
		gpointer user_data) {
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeStore *model_store;
	GtkTreeIter iter, iter2;
	GtkCellRenderer *renderer;
	char* shift_char;
	char* stored_shift_char;
	gdouble freq, stored_freq, base_freq, stored_shift;
	double shift_cents;
	short int shift_numerator, shift_denominator;
	gdouble shiftf2, freq2;
	char* char_pointer;
	char buff[80];
	const char* delim = " \t\n";
	LingotConfigDialog* config_dialog = (LingotConfigDialog*) user_data;
	int index;

	treeview = config_dialog->scale_treeview;
	model_store = (GtkTreeStore *) gtk_tree_view_get_model(treeview);

	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);

	if (gtk_tree_selection_count_selected_rows(selection) != 1)
		return;

	GList *list = gtk_tree_selection_get_selected_rows(selection, &model);
	//	model_store = GTK_TREE_STORE ( model );

	int ipath = atoi(gtk_tree_path_to_string(list->data));
	GString *fixed_path = g_string_new("");
	g_string_printf(fixed_path, "%d", ipath);

	GtkTreePath *path = gtk_tree_path_new_from_string(fixed_path->str);
	g_string_free(fixed_path, TRUE);

	gboolean valid = FALSE;

	if (path) {
		if (gtk_tree_model_get_iter(model, &iter, path)) { // get iter from specified path
			valid = TRUE;
		} else { // invalid path
			//			g_error("Error!!!\n");
			// TODO
		}
		gtk_tree_path_free(path);
	} else {
		//		g_error("Error!!!\n");
		// TODO
	}

	g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL );
	g_list_free(list);

	if (!valid)
		return;

	renderer = &cell->parent;
	guint column_number = GPOINTER_TO_UINT(g_object_get_data(
					G_OBJECT(renderer), "my_column_num"));

	switch (column_number) {

	case COLUMN_NAME:
		if (strchr(new_text, ' ') || strchr(new_text, '\t')) {
			lingot_msg_add_warning(
					_("Do not use space characters for the note names."));
		} else if (strchr(new_text, '\n') || strchr(new_text, '{')
				|| strchr(new_text, '}')) {
			lingot_msg_add_warning(_("The name contains illegal characters."));
		} else {
			char_pointer = strtok(new_text, delim);
			gtk_tree_store_set(model_store, &iter, COLUMN_NAME,
					(char_pointer == NULL )? "?" : new_text, -1);
				}
				break;

				case COLUMN_SHIFT:

				lingot_config_scale_parse_shift(new_text, &shift_cents,
						&shift_numerator, &shift_denominator);

				// TODO: full validation

				if ((ipath == 0) && (fabs(shift_cents -0.0) > 1e-10)) {
					lingot_msg_add_warning(
							_("You cannot change the first shift, it must be 1/1."));
					break;
				}

				if (isnan(shift_cents) || (shift_cents <= 0.0 - 1e-10) || (shift_cents
								> 1200.0)) {
					lingot_msg_add_warning(
							_("The shift must be between 0 and 1200 cents, or between 1/1 and 2/1."));
					break;
				}

				gtk_tree_model_get(model, &iter, COLUMN_SHIFT, &stored_shift_char,
						COLUMN_FREQUENCY, &stored_freq, -1);
				lingot_config_scale_parse_shift(stored_shift_char, &stored_shift, NULL,
						NULL);
				free(stored_shift_char);
				lingot_config_scale_format_shift(buff, shift_cents, shift_numerator,
						shift_denominator);
				gtk_tree_store_set(model_store, &iter, COLUMN_SHIFT, buff,
						COLUMN_FREQUENCY, stored_freq * pow(2.0, (shift_cents
										- stored_shift) / 1200.0), -1);
				break;

				case COLUMN_FREQUENCY:

				if (!strcmp(new_text, "mid-A")) {
					freq = MID_A_FREQUENCY;
				} else if (!strcmp(new_text, "mid-C")) {
					freq = MID_C_FREQUENCY;
				} else {
					sscanf(new_text, "%lg", &freq);
					// TODO: validation
				}

				gtk_tree_model_get(model, &iter, COLUMN_SHIFT, &shift_char,
						COLUMN_FREQUENCY, &stored_freq, -1);
				lingot_config_scale_parse_shift(shift_char, &shift_cents, NULL, NULL);
				free(shift_char);

				freq *= pow(2.0, -gtk_spin_button_get_value(
								config_dialog->root_frequency_error) / 1200.0);
				base_freq = freq * pow(2.0, -shift_cents / 1200.0);

				gtk_tree_model_get_iter_first(model, &iter2);

				index = 0;
				do {
					gtk_tree_model_get(model, &iter2, COLUMN_SHIFT, &shift_char, -1);
					lingot_config_scale_parse_shift(shift_char, &shiftf2, NULL, NULL);
					free(shift_char);
					freq2 = base_freq * pow(2.0, shiftf2 / 1200.0);
					gtk_tree_store_set(model_store, &iter2, COLUMN_FREQUENCY, (index
									== ipath) ? freq : freq2, -1);
					index++;
				}while (gtk_tree_model_iter_next(model, &iter2));

				//gtk_spin_button_set_value(config_dialog->root_frequency_error, 0.0);

				break;
			}

}

void lingot_gui_config_dialog_scale_tree_frequency_cell_data_function(
		GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model,
		GtkTreeIter *iter, gpointer user_data) {
	gdouble freq;
	gchar buf[20];
	LingotConfigDialog* config_dialog = (LingotConfigDialog*) user_data;

	gtk_tree_model_get(model, iter, COLUMN_FREQUENCY, &freq, -1);
	freq *= pow(2.0,
			gtk_spin_button_get_value(config_dialog->root_frequency_error)
					/ 1200.0);

	if (fabs(freq - MID_A_FREQUENCY) < 1e-3) {
		g_object_set(renderer, "text", "mid-A", NULL );
	} else if (fabs(freq - MID_C_FREQUENCY) < 1e-3) {
		g_object_set(renderer, "text", "mid-C", NULL );
	} else {
		g_snprintf(buf, sizeof(buf), "%.4f", freq);
		g_object_set(renderer, "text", buf, NULL );
	}
}

void lingot_gui_config_dialog_scale_tree_add_column(
		LingotConfigDialog* config_dialog) {
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeView *treeview = config_dialog->scale_treeview;

	column = gtk_tree_view_column_new();

	gtk_tree_view_column_set_title(column, _("Name"));

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", COLUMN_NAME,
			NULL );
	g_object_set(renderer, "editable", TRUE, NULL );
	g_object_set_data(G_OBJECT(renderer), "my_column_num", GUINT_TO_POINTER(
			COLUMN_NAME) );
	g_signal_connect( renderer, "edited",
			(GCallback) lingot_gui_config_dialog_scale_tree_cell_edited_callback,
			config_dialog);

	gtk_tree_view_append_column(treeview, column);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Shift"));

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", COLUMN_SHIFT,
			NULL );
	g_object_set(renderer, "editable", TRUE, NULL );
	g_object_set_data(G_OBJECT(renderer), "my_column_num", GUINT_TO_POINTER(
			COLUMN_SHIFT) );
	//	gtk_tree_view_column_set_cell_data_func(column, renderer,
	//			lingot_config_dialog_scale_tree_shift_cell_data_function, NULL,
	//			NULL);

	g_signal_connect( renderer, "edited",
			(GCallback) lingot_gui_config_dialog_scale_tree_cell_edited_callback,
			config_dialog);

	gtk_tree_view_append_column(treeview, column);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Frequency [Hz]"));

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text",
			COLUMN_FREQUENCY, NULL );
	g_object_set(renderer, "editable", TRUE, NULL );
	g_object_set_data(G_OBJECT(renderer), "my_column_num", GUINT_TO_POINTER(
			COLUMN_FREQUENCY) );
	gtk_tree_view_column_set_cell_data_func(column, renderer,
			lingot_gui_config_dialog_scale_tree_frequency_cell_data_function,
			config_dialog, NULL );

	g_signal_connect( renderer, "edited",
			(GCallback) lingot_gui_config_dialog_scale_tree_cell_edited_callback,
			config_dialog);

	gtk_tree_view_append_column(treeview, column);
}

int lingot_gui_config_dialog_scale_validate(LingotConfigDialog* dialog,
		LingotScale* scale) {

	GtkTreeIter iter, iter2;
	GtkTreeModel* model = gtk_tree_view_get_model(dialog->scale_treeview);

	char* name;
	char* name2;
	char* shift_char;
	gdouble shift, last_shift;
	int empty_names = 0;

	gtk_tree_model_get_iter_first(model, &iter);

	last_shift = -1.0;

	// TODO: full validation

	int row1 = 0;

	do {
		gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, COLUMN_SHIFT,
				&shift_char, -1);
		lingot_config_scale_parse_shift(shift_char, &shift, NULL, NULL );
		free(shift_char);

		if (!strcmp(name, "") || !strcmp(name, "?")) {
			empty_names = 1;
		}

		gtk_tree_model_get_iter_first(model, &iter2);
		int row2 = 0;
		do {
			gtk_tree_model_get(model, &iter2, COLUMN_NAME, &name2, -1);
			if ((row1 != row2) && !strcmp(name, name2)) {
				lingot_msg_add_error(_("There are notes with the same name"));
				// TODO: select the conflictive line
				free(name);
				free(name2);
				return 0;
			}
			free(name2);
			row2++;
		} while (gtk_tree_model_iter_next(model, &iter2));

		free(name);

		if (shift < last_shift) {
			lingot_msg_add_error(
					_("There are invalid values in the scale: the notes should be ordered by frequency / shift"));
			// TODO: select the conflictive line
			return 0;
		}

		if (shift >= 1200.0) {
			lingot_msg_add_error(
					_("There are invalid values in the scale: all the notes should be in the same octave"));
			return 0;
		}

		last_shift = shift;
		row1++;
	} while (gtk_tree_model_iter_next(model, &iter));

	if (empty_names) {
		lingot_msg_add_warning(_("There are notes without name"));
		return 0;
	}

	return 1;
}

void lingot_gui_config_dialog_scale_apply(LingotConfigDialog* dialog,
		LingotScale* scale) {
	GtkTreeIter iter;
	GtkTreeModel* model = gtk_tree_view_get_model(dialog->scale_treeview);
	gdouble freq, shift;
	short int shift_num, shift_den;
	char* shift_char;
	gchar* name;
	int i = 0;

	int rows = gtk_tree_model_iter_n_children(model, NULL );

	gtk_tree_model_get_iter_first(model, &iter);
	gtk_tree_model_get(model, &iter, COLUMN_FREQUENCY, &freq, -1);

	scale->name = strdup(gtk_entry_get_text(dialog->scale_name));
	scale->base_frequency = freq;
	lingot_config_scale_allocate(scale, rows);

	do {
		gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, COLUMN_SHIFT,
				&shift_char, -1);
		lingot_config_scale_parse_shift(shift_char, &shift, &shift_num,
				&shift_den);
		free(shift_char);

		scale->note_name[i] = name;
		scale->offset_cents[i] = shift;
		scale->offset_ratios[0][i] = shift_num;
		scale->offset_ratios[1][i] = shift_den;
		i++;
	} while (gtk_tree_model_iter_next(model, &iter));
}

void lingot_gui_config_dialog_scale_rewrite(LingotConfigDialog* dialog,
		LingotScale* scale) {
	gtk_entry_set_text(dialog->scale_name, scale->name);
	GtkTreeStore* store = (GtkTreeStore *) gtk_tree_view_get_model(
			dialog->scale_treeview);
	gtk_tree_store_clear(store);
	GtkTreeIter iter2;
	char buff[80];

	int i;
	for (i = 0; i < scale->notes; i++) {
		gtk_tree_store_append(store, &iter2, NULL );
		FLT freq = scale->base_frequency
				* pow(2.0, scale->offset_cents[i] / 1200.0);
		lingot_config_scale_format_shift(buff, scale->offset_cents[i],
				scale->offset_ratios[0][i], scale->offset_ratios[1][i]);
		gtk_tree_store_set(store, &iter2, COLUMN_NAME, scale->note_name[i],
				COLUMN_SHIFT, buff, COLUMN_FREQUENCY, freq, -1);
	}
}

void lingot_gui_config_dialog_import_scl(gpointer data,
		LingotConfigDialog* config_dialog) {
	GtkWidget * dialog = gtk_file_chooser_dialog_new(_("Open Scale File"),
			GTK_WINDOW(config_dialog->win), GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
			GTK_RESPONSE_ACCEPT, NULL );
	GtkFileFilter *filefilter;
	filefilter = gtk_file_filter_new();
	static gchar* filechooser_last_folder = NULL;

	gtk_file_filter_set_name(filefilter, (const gchar *) _("Scala files"));
	gtk_file_filter_add_pattern(filefilter, "*.scl");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filefilter);

	if (filechooser_last_folder != NULL ) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
				filechooser_last_folder);
	}

	if (gtk_dialog_run(GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog) );
		if (filechooser_last_folder != NULL ) {
			free(filechooser_last_folder);
			filechooser_last_folder = NULL;
		}

		if (gtk_file_chooser_get_current_folder(
				GTK_FILE_CHOOSER(dialog) ) != NULL) {
			filechooser_last_folder = strdup(
					gtk_file_chooser_get_current_folder(
							GTK_FILE_CHOOSER(dialog) ));
		}

		// TODO
		LingotScale* scale = lingot_config_scale_new();
		if (lingot_config_scale_load_scl(scale, filename)) {
			lingot_gui_config_dialog_scale_rewrite(config_dialog, scale);
		}

		lingot_config_scale_destroy(scale);
		free(scale);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
	//g_free(filefilter);
}

gint lingot_gui_config_dialog_scale_key_press_cb(GtkWidget *widget,
		GdkEventKey *kevent, gpointer data) {

	LingotConfigDialog* dialog = (LingotConfigDialog*) data;
	if (kevent->type == GDK_KEY_PRESS) {
		if (kevent->keyval == 0xffff) {
			g_signal_emit_by_name(G_OBJECT(dialog->button_scale_del), "clicked",
					NULL );
		} else if (kevent->keyval == 0xff63) {
			g_signal_emit_by_name(G_OBJECT(dialog->button_scale_add), "clicked",
					NULL );
		}
	}

	return TRUE;
}

gint lingot_gui_config_dialog_scale_tree_view_column_get_index(
		GtkTreeViewColumn *column) {
	GtkTreeView *tree =
			GTK_TREE_VIEW(gtk_tree_view_column_get_tree_view(column));
	GList *cols = gtk_tree_view_get_columns(tree);
	int counter = 0;

	while (cols != NULL ) {
		if (column == GTK_TREE_VIEW_COLUMN(cols->data) ) {
			g_list_free(cols);
			return counter;
		}
		cols = cols->next;
		counter++;
	}

	g_list_free(cols);
	return -1;
}

gboolean lingot_gui_config_dialog_scale_table_query_tooltip(GtkWidget *widget,
		gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip,
		gpointer user_data) {
	GtkTreePath* path;
	GtkTreeViewColumn* col;
	gint cx, cy;

	gtk_tree_view_get_path_at_pos((GtkTreeView*) widget, x, y, &path, &col, &cx,
			&cy);

	gint column_index =
			lingot_gui_config_dialog_scale_tree_view_column_get_index(col);

	switch (column_index) {
	case COLUMN_NAME:
		gtk_tooltip_set_text(tooltip,
				_("Note identifier, free text that will be displayed in the main window when tuning close to the given note. Don't use space characters here."));
		break;
	case COLUMN_SHIFT:
		gtk_tooltip_set_text(tooltip,
				_("Shift. You can define it as a deviation in cents from the reference note (the first one), or as a frequency ratio, like '3/2' or '5/4'. All the values must be between 0 and 1200 cents, or between 1/1 and 2/1 (i.e., all the notes must be in the same octave), and they must be well ordered."));
		break;
	case COLUMN_FREQUENCY:
		gtk_tooltip_set_text(tooltip,
				_("Frequency. You can enter here the absolute frequency for a given note as a reference, and all the other frequencies will shift according to the deviations specified in the 'Shift' column. You can either use an absolute numeric value or the keywords 'mid-C' (261.625565 Hz) and 'mid-A' (440 Hz)."));
		break;
	}

	return TRUE;
}

void lingot_gui_config_dialog_scale_show(LingotConfigDialog* dialog,
		GtkBuilder* builder) {

	dialog->scale_name = GTK_ENTRY(
			gtk_builder_get_object(builder, "scale_name"));
	GtkWidget* scroll =
			GTK_WIDGET(gtk_builder_get_object(builder, "scrolledwindow1"));

	/* crea el modelo del arbol */
	GtkTreeStore *model_store = gtk_tree_store_new(3, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_DOUBLE );
	GtkTreeModel* model = GTK_TREE_MODEL(model_store);

	/* crea un nuevo widget gtktreeview */
	dialog->scale_treeview = GTK_TREE_VIEW(gtk_tree_view_new());

	/* agrega columnas al modelo del arbol */
	lingot_gui_config_dialog_scale_tree_add_column(dialog);

	/* asocia el modelo al gtkteeview */
	gtk_tree_view_set_model(dialog->scale_treeview, model);
	gtk_tree_selection_set_mode(
			gtk_tree_view_get_selection(dialog->scale_treeview),
			GTK_SELECTION_MULTIPLE);

	g_object_unref(G_OBJECT(model) );
	gtk_container_add(GTK_CONTAINER(scroll),
			GTK_WIDGET(dialog->scale_treeview) );

	dialog->button_scale_del = GTK_BUTTON(gtk_builder_get_object(builder,
					"button_scale_del"));
	dialog->button_scale_add = GTK_BUTTON(gtk_builder_get_object(builder,
					"button_scale_add"));
	GtkButton* button_import = GTK_BUTTON(gtk_builder_get_object(builder,
					"button_scale_import"));

	g_signal_connect(G_OBJECT(dialog->scale_treeview), "key_press_event",
			G_CALLBACK(lingot_gui_config_dialog_scale_key_press_cb), dialog);

	g_signal_connect(G_OBJECT(dialog->button_scale_add), "clicked",
			G_CALLBACK( lingot_gui_config_dialog_scale_tree_add_row_tree),
			dialog->scale_treeview);
	g_signal_connect( G_OBJECT(dialog->button_scale_del), "clicked",
			G_CALLBACK( lingot_gui_config_dialog_scale_tree_remove_selected_items),
			dialog->scale_treeview);
	g_signal_connect(G_OBJECT(button_import), "clicked",
			G_CALLBACK( lingot_gui_config_dialog_import_scl), dialog);

	gtk_widget_set_has_tooltip(GTK_WIDGET(dialog->scale_treeview), TRUE);
	g_signal_connect(G_OBJECT(dialog->scale_treeview), "query-tooltip",
			(GCallback) lingot_gui_config_dialog_scale_table_query_tooltip,
			NULL);
}
