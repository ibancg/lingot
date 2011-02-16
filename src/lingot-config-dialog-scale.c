/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2011  Ibán Cereijo Graña, Jairo Chapela Martínez.
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
#include <glade/glade.h>
#include <math.h>

#include "lingot-config-dialog.h"
#include "lingot-config-dialog-scale.h"
#include "lingot-error.h"

enum {
	COLUMN_NAME = 0, COLUMN_PITCH = 1, COLUMN_FREQUENCY = 2, NUM_COLUMNS = 3
};

void lingot_config_dialog_scale_tree_add_row_tree(gpointer data,
		GtkTreeView *treeview) {
	GtkTreeModel *model;
	GtkTreeStore *model_store;
	GtkTreeIter iter1, iter2;
	gdouble freq;

	model = gtk_tree_view_get_model(treeview);
	model_store = (GtkTreeStore *) model;

	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);

	if (gtk_tree_selection_count_selected_rows(selection) == 0) {
		gtk_tree_store_append(model_store, &iter2, NULL);
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

		g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(list);

		if (!valid)
			return;

		if (ipath == 0) {
			lingot_error_queue_push("Cannot insert before the reference pitch.");
			//		g_free(path_str);
			return;
		}

		//	g_free(path_str);
		gtk_tree_store_insert_before(model_store, &iter2, NULL, &iter1);
	}

	gtk_tree_model_get_iter_first(model, &iter1);
	gtk_tree_model_get(model, &iter1, COLUMN_FREQUENCY, &freq, -1);

	gtk_tree_store_set(model_store, &iter2, COLUMN_NAME, "", COLUMN_PITCH, 0.0,
			COLUMN_FREQUENCY, freq, -1);
}

void lingot_config_dialog_scale_tree_remove_selected_items(gpointer data,
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
		//	g_error("Caca!!!\n");
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
	g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(list);
}

void lingot_config_dialog_scale_tree_cell_edited_callback(
		GtkCellRendererText *cell, gchar *path_string, gchar *new_text,
		gpointer user_data) {
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeStore *model_store;
	GtkTreeIter iter, iter2;
	GtkCellRenderer *renderer;
	gdouble freq, stored_freq, base_freq, stored_pitch, pitch;
	//gchar* pitch_str;
	gdouble pitchf2, freq2;
	char* char_pointer;
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

	g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(list);

	if (!valid)
		return;

	renderer = &cell->parent;
	guint column_number = GPOINTER_TO_UINT(g_object_get_data(
					G_OBJECT(renderer), "my_column_num"));

	switch (column_number) {

	case COLUMN_NAME:
		char_pointer = strtok(new_text, delim);
		gtk_tree_store_set(model_store, &iter, COLUMN_NAME, (char_pointer
				== NULL) ? "?" : new_text, -1);
		break;

	case COLUMN_PITCH:

		pitch = lingot_config_scale_parse_pitch(new_text);

		if ((ipath == 0) && (fabs(pitch - 0.0) > 1e-10)) {
			//			lingot_error_queue_push(
			//					"You cannot change the first pitch, it must be 1/1.");
			// TODO
			break;
		}

		if ((pitch <= 0.0 - 1e-10) || (pitch > 1200.0)) {
			lingot_error_queue_push(
					"The pitch must be between 0 and 1200 cents.");
			g_print("PITCH %f\n", pitch);
			break;
		}

		gtk_tree_model_get(model, &iter, COLUMN_PITCH, &stored_pitch,
				COLUMN_FREQUENCY, &stored_freq, -1);
		gtk_tree_store_set(model_store, &iter, COLUMN_PITCH, pitch,
				COLUMN_FREQUENCY, stored_freq * pow(2.0, (pitch - stored_pitch)
						/ 1200.0), -1);
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

		gtk_tree_model_get(model, &iter, COLUMN_PITCH, &pitch,
				COLUMN_FREQUENCY, &stored_freq, -1);

		freq *= pow(2.0, -gtk_spin_button_get_value_as_float(
				config_dialog->root_frequency_error) / 1200.0);
		base_freq = freq * pow(2.0, -pitch / 1200.0);

		gtk_tree_model_get_iter_first(model, &iter2);

		index = 0;
		do {
			gtk_tree_model_get(model, &iter2, COLUMN_PITCH, &pitchf2, -1);
			freq2 = base_freq * pow(2.0, pitchf2 / 1200.0);
			gtk_tree_store_set(model_store, &iter2, COLUMN_FREQUENCY, (index
					== ipath) ? freq : freq2, -1);
			index++;
		} while (gtk_tree_model_iter_next(model, &iter2));

		//gtk_spin_button_set_value(config_dialog->root_frequency_error, 0.0);

		break;
	}

}

void lingot_config_dialog_scale_tree_frequency_cell_data_function(
		GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model,
		GtkTreeIter *iter, gpointer user_data) {
	gdouble freq;
	gchar buf[20];
	LingotConfigDialog* config_dialog = (LingotConfigDialog*) user_data;

	gtk_tree_model_get(model, iter, COLUMN_FREQUENCY, &freq, -1);
	freq *= pow(2.0, gtk_spin_button_get_value_as_float(
			config_dialog->root_frequency_error) / 1200.0);

	if (fabs(freq - MID_A_FREQUENCY) < 1e-3) {
		g_object_set(renderer, "text", "mid-A", NULL);
	} else if (fabs(freq - MID_C_FREQUENCY) < 1e-3) {
		g_object_set(renderer, "text", "mid-C", NULL);
	} else {
		g_snprintf(buf, sizeof(buf), "%.4f", freq);
		g_object_set(renderer, "text", buf, NULL);
	}
}

void lingot_config_dialog_scale_tree_pitch_cell_data_function(
		GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model,
		GtkTreeIter *iter, gpointer user_data) {
	gdouble pitch;
	gchar buf[20];

	gtk_tree_model_get(model, iter, COLUMN_PITCH, &pitch, -1);
	if (fabs(pitch - 0.0) < 1e-10) {
		g_object_set(renderer, "text", "1/1", NULL);
		// TODO: more
	} else {
		g_snprintf(buf, sizeof(buf), "%.4f", pitch);
		g_object_set(renderer, "text", buf, NULL);
	}
}

void lingot_config_dialog_scale_tree_add_column(
		LingotConfigDialog* config_dialog) {
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeView *treeview = config_dialog->scale_treeview;

	column = gtk_tree_view_column_new();

	gtk_tree_view_column_set_title(column, "Name");

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", COLUMN_NAME,
			NULL);
	g_object_set(renderer, "editable", TRUE, NULL);
	g_object_set_data(G_OBJECT(renderer), "my_column_num", GUINT_TO_POINTER(
			COLUMN_NAME));
	g_signal_connect(renderer, "edited",
			(GCallback) lingot_config_dialog_scale_tree_cell_edited_callback,
			config_dialog);

	gtk_tree_view_append_column(treeview, column);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "Pitch");

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", COLUMN_PITCH,
			NULL);
	g_object_set(renderer, "editable", TRUE, NULL);
	g_object_set_data(G_OBJECT(renderer), "my_column_num", GUINT_TO_POINTER(
			COLUMN_PITCH));
	gtk_tree_view_column_set_cell_data_func(column, renderer,
			lingot_config_dialog_scale_tree_pitch_cell_data_function, NULL,
			NULL);

	g_signal_connect(renderer, "edited",
			(GCallback) lingot_config_dialog_scale_tree_cell_edited_callback,
			config_dialog);

	gtk_tree_view_append_column(treeview, column);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "Frequency [Hz]");

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text",
			COLUMN_FREQUENCY, NULL);
	g_object_set(renderer, "editable", TRUE, NULL);
	g_object_set_data(G_OBJECT(renderer), "my_column_num", GUINT_TO_POINTER(
			COLUMN_FREQUENCY));
	gtk_tree_view_column_set_cell_data_func(column, renderer,
			lingot_config_dialog_scale_tree_frequency_cell_data_function,
			config_dialog, NULL);

	g_signal_connect(renderer, "edited",
			(GCallback) lingot_config_dialog_scale_tree_cell_edited_callback,
			config_dialog);

	gtk_tree_view_append_column(treeview, column);
}

int lingot_config_dialog_scale_validate(LingotConfigDialog* dialog,
		LingotScale* scale) {

	GtkTreeIter iter;
	GtkTreeModel* model = gtk_tree_view_get_model(dialog->scale_treeview);

	gdouble pitch, last_pitch;

	gtk_tree_model_get_iter_first(model, &iter);

	last_pitch = -1.0;

	do {
		gtk_tree_model_get(model, &iter, COLUMN_PITCH, &pitch, -1);

		//		if (pitch < 0.0) {
		//			lingot_error_queue_push(
		//					"There are invalid pitches in the scale: negative pitches not allowed");
		//			return;
		//		}

		if (pitch < last_pitch) {
			lingot_error_queue_push(
					"There are invalid pitches in the scale: the scale should be ordered");
			return 0;
		}

		if (pitch >= 1200.0) {
			lingot_error_queue_push(
					"There are invalid pitches in the scale: all the pitches should be in the same octave");
			return 0;
		}

		last_pitch = pitch;
	} while (gtk_tree_model_iter_next(model, &iter));

	return 1;
}

void lingot_config_dialog_scale_apply(LingotConfigDialog* dialog,
		LingotScale* scale) {
	GtkTreeIter iter;
	GtkTreeModel* model = gtk_tree_view_get_model(dialog->scale_treeview);
	gdouble freq, pitch;
	gchar* name;
	int i = 0;

	int rows = gtk_tree_model_iter_n_children(model, NULL);

	gtk_tree_model_get_iter_first(model, &iter);
	gtk_tree_model_get(model, &iter, COLUMN_FREQUENCY, &freq, -1);

	scale->name = strdup(gtk_entry_get_text(dialog->scale_name));
	scale->notes = rows;
	scale->base_frequency = freq;
	scale->note_name = (char**) malloc(scale->notes * sizeof(char*));
	scale->offset_cents = (FLT*) malloc(scale->notes * sizeof(FLT));

	do {
		gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, COLUMN_PITCH,
				&pitch, -1);
		scale->note_name[i] = name;
		scale->offset_cents[i] = pitch;
		i++;
	} while (gtk_tree_model_iter_next(model, &iter));
}

void lingot_config_dialog_scale_rewrite(LingotConfigDialog* dialog,
		LingotScale* scale) {
	gtk_entry_set_text(dialog->scale_name, scale->name);
	GtkTreeStore* store = (GtkTreeStore *) gtk_tree_view_get_model(
			dialog->scale_treeview);
	gtk_tree_store_clear(store);
	GtkTreeIter iter2;

	int i;
	for (i = 0; i < scale->notes; i++) {
		gtk_tree_store_append(store, &iter2, NULL);
		FLT freq = scale->base_frequency * pow(2.0, scale->offset_cents[i]
				/ 1200.0);
		gtk_tree_store_set(store, &iter2, COLUMN_NAME, scale->note_name[i],
				COLUMN_PITCH, scale->offset_cents[i], COLUMN_FREQUENCY, freq,
				-1);
	}
}

void lingot_config_dialog_import_scl(gpointer data,
		LingotConfigDialog* config_dialog) {
	GtkWidget * dialog = gtk_file_chooser_dialog_new("Open File",
			GTK_WINDOW(config_dialog->win), GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
			GTK_RESPONSE_ACCEPT, NULL);
	GtkFileFilter *filefilter;
	filefilter = gtk_file_filter_new();
	static gchar* filechooser_last_folder = NULL;

	gtk_file_filter_set_name(filefilter, (const gchar *) "Scala files");
	gtk_file_filter_add_pattern(filefilter, "*.scl");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filefilter);

	if (filechooser_last_folder != NULL) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
				filechooser_last_folder);
		free(filechooser_last_folder);
	}

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		filechooser_last_folder = strdup(gtk_file_chooser_get_current_folder(
				GTK_FILE_CHOOSER(dialog)));
		// TODO
		LingotScale* scale = lingot_config_scale_new();
		if (!lingot_config_scale_load(scale, filename)) {
			lingot_error_queue_push(
					"The scale cannot be imported: file format error");
			lingot_config_scale_destroy(scale);
			free(scale);
		} else {
			lingot_config_dialog_scale_rewrite(config_dialog, scale);
		}

		g_free(filename);
	}
	gtk_widget_destroy(dialog);
	//g_free(filefilter);
}

void lingot_config_dialog_scale_show(LingotConfigDialog* dialog,
		GladeXML* _gladeXML) {

	dialog->scale_name
			= GTK_ENTRY(glade_xml_get_widget(_gladeXML, "scale_name"));
	GtkWidget* scroll = glade_xml_get_widget(_gladeXML, "scrolledwindow1");

	/* crea el modelo del arbol */
	GtkTreeStore *model_store = gtk_tree_store_new(3, G_TYPE_STRING,
			G_TYPE_DOUBLE, G_TYPE_DOUBLE);
	GtkTreeModel* model = GTK_TREE_MODEL(model_store);

	/* crea un nuevo widget gtktreeview */
	dialog->scale_treeview = GTK_TREE_VIEW(gtk_tree_view_new());

	/* agrega columnas al modelo del arbol */
	lingot_config_dialog_scale_tree_add_column(dialog);

	/* asocia el modelo al gtkteeview */
	gtk_tree_view_set_model(dialog->scale_treeview, model);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(
			dialog->scale_treeview), GTK_SELECTION_MULTIPLE);

	g_object_unref(G_OBJECT(model));
	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(dialog->scale_treeview));

	GtkButton* button_del = GTK_BUTTON(glade_xml_get_widget(_gladeXML,
					"button_scale_del"));
	GtkButton* button_add = GTK_BUTTON(glade_xml_get_widget(_gladeXML,
					"button_scale_add"));
	GtkButton* button_import = GTK_BUTTON(glade_xml_get_widget(_gladeXML,
					"button_scale_import"));

	g_signal_connect(G_OBJECT(button_add), "clicked", G_CALLBACK(lingot_config_dialog_scale_tree_add_row_tree),
			dialog->scale_treeview);
	g_signal_connect(G_OBJECT(button_del), "clicked", G_CALLBACK(lingot_config_dialog_scale_tree_remove_selected_items),
			dialog->scale_treeview);
	g_signal_connect(G_OBJECT(button_import), "clicked", G_CALLBACK(lingot_config_dialog_import_scl),
			dialog);
}
