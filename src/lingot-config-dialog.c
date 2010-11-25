/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2010  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <glade/glade.h>

#include "lingot-defs.h"

#include "lingot-core.h"
#include "lingot-config.h"
#include "lingot-mainframe.h"
#include "lingot-config-dialog.h"
#include "lingot-i18n.h"
#include "lingot-audio-jack.h"

#ifdef JACK
#include <jack/jack.h>
#endif

enum {
	COLUMN_NAME = 0, COLUMN_PITCH = 1, COLUMN_FREQUENCY = 2, NUM_COLUMNS = 3
};


int lingot_config_dialog_apply(LingotConfigDialog*);
void lingot_config_dialog_close(LingotConfigDialog*);
void lingot_config_dialog_rewrite(LingotConfigDialog*);
void lingot_config_dialog_combo_select_value(GtkWidget* combo, int value);
audio_system_t lingot_config_dialog_get_audio_system(GtkComboBox* combo);

gchar* filechooser_last_folder = NULL;

/* button press event attention routine. */

void lingot_config_dialog_callback_button_cancel(GtkButton *button,
		LingotConfigDialog* dialog) {
	lingot_config_dialog_close(dialog);
}

void lingot_config_dialog_callback_button_ok(GtkButton *button,
		LingotConfigDialog* dialog) {
	if (lingot_config_dialog_apply(dialog)) {
		// dumps the current config to the config file
		lingot_config_save(dialog->conf, CONFIG_FILE_NAME);
		// establish the current config as the old config, so the close rollback
		// will do nothing.
		lingot_config_copy(dialog->conf_old, dialog->conf);
		lingot_config_dialog_close(dialog);
	}
}

void lingot_config_dialog_callback_button_apply(GtkButton *button,
		LingotConfigDialog* dialog) {
	if (lingot_config_dialog_apply(dialog)) {
		lingot_config_dialog_rewrite(dialog);
	}
}

void lingot_config_dialog_callback_button_default(GtkButton *button,
		LingotConfigDialog* dialog) {
	lingot_config_restore_default_values(dialog->conf);
	lingot_config_dialog_rewrite(dialog);
}

void lingot_config_dialog_callback_cancel(GtkWidget *widget,
		LingotConfigDialog* dialog) {
	//lingot_mainframe_change_config(dialog->mainframe, dialog->conf_old); // restore old configuration.
	lingot_config_dialog_close(dialog);
}

void lingot_config_dialog_callback_close(GtkWidget *widget,
		LingotConfigDialog *dialog) {
	lingot_mainframe_change_config(dialog->mainframe, dialog->conf_old); // restore old configuration.
	gtk_widget_destroy(dialog->win);
	lingot_config_dialog_destroy(dialog);
}

void lingot_config_dialog_callback_change_sample_rate(GtkWidget *widget,
		LingotConfigDialog *dialog) {

	gchar* text = gtk_entry_get_text(GTK_ENTRY(
			GTK_BIN(dialog->sample_rate)->child));

	int sr;
	if (text != NULL) {
		sr = atoi(text);
	} else {
		sr = 44100;
		g_print("WARNING: cannot get sample rate, assuming 44100\n");
	}

	char buff1[20];
	char buff2[20];
	gdouble srf = sr;
	gdouble os = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(
			dialog->oversampling));
	sprintf(buff1, "%d", sr);
	sprintf(buff2, " = %0.1f", srf / os);
	gtk_label_set_text(dialog->label_sample_rate1, buff1);
	//gtk_label_set_text(dialog->jack_label_sample_rate1, buff1);
	gtk_label_set_text(dialog->label_sample_rate2, buff2);
}

void lingot_config_dialog_callback_change_input_system(GtkWidget *widget,
		LingotConfigDialog *dialog) {

	char buff[10];
	char* text = gtk_combo_box_get_active_text(dialog->input_system);
	audio_system_t audio_system = str_to_audio_system_t(text);
	free(text);

	LingotAudioSystemProperties* properties =
			lingot_audio_get_audio_system_properties(audio_system);

	if ((properties->forced_sample_rate) && (properties->n_sample_rates > 0)) {
		sprintf(buff, "%d", properties->sample_rates[0]);
		gtk_entry_set_text(GTK_ENTRY(GTK_BIN(dialog->sample_rate)->child), buff);
	}

	gtk_widget_set_sensitive(dialog->sample_rate,
			(gboolean) !properties->forced_sample_rate);

	GtkListStore* model = GTK_LIST_STORE(gtk_combo_box_get_model(
			dialog->sample_rate));
	gtk_list_store_clear(model);

	int i;
	for (i = 0; i < properties->n_sample_rates; i++) {
		sprintf(buff, "%d", properties->sample_rates[i]);
		gtk_combo_box_append_text(dialog->sample_rate, buff);
	}

	// TODO: devices

	gtk_entry_set_text(GTK_ENTRY(GTK_BIN(dialog->input_dev)->child),
			dialog->conf->audio_dev[audio_system]);
	gtk_widget_set_sensitive(dialog->input_dev, (gboolean) (audio_system
			!= AUDIO_SYSTEM_JACK));

	lingot_audio_audio_system_properties_destroy(properties);
}

void lingot_config_dialog_callback_change_deviation(GtkWidget *widget,
		LingotConfigDialog *dialog) {
	gtk_widget_queue_draw(GTK_WIDGET(dialog->scale_treeview));
}

void lingot_config_dialog_set_audio_system(GtkComboBox* combo,
		audio_system_t audio_system) {
	const char* token = audio_system_t_to_str(audio_system);
	GtkTreeModel* model = gtk_combo_box_get_model(combo);
	GtkTreeIter iter;

	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

	while (valid) {
		gchar *str_data;
		gtk_tree_model_get(model, &iter, 0, &str_data, -1);
		if (!strcmp(str_data, token))
			gtk_combo_box_set_active_iter(combo, &iter);
		g_free(str_data);
		valid = gtk_tree_model_iter_next(model, &iter);
	}
}

audio_system_t lingot_config_dialog_get_audio_system(GtkComboBox* combo) {
	char* text = gtk_combo_box_get_active_text(combo);
	int result = str_to_audio_system_t(text);
	free(text);
	return result;
}

void lingot_config_dialog_combo_select_value(GtkWidget* combo, int value) {

	GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
	GtkTreeIter iter;

	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

	while (valid) {
		gchar *str_data;
		gtk_tree_model_get(model, &iter, 0, &str_data, -1);
		if (atoi(str_data) == value)
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), &iter);
		g_free(str_data);
		valid = gtk_tree_model_iter_next(model, &iter);
	}
}

void lingot_config_dialog_rewrite_scale(LingotConfigDialog* dialog,
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

void lingot_config_dialog_rewrite(LingotConfigDialog* dialog) {
	LingotConfig* conf = dialog->conf;

	lingot_config_dialog_set_audio_system(dialog->input_system,
			conf->audio_system);
	//		gtk_entry_set_text(dialog->oss_input_dev, conf->audio_dev);
	//		gtk_entry_set_text(dialog->alsa_input_dev, conf->audio_dev_alsa);
	gtk_entry_set_text(GTK_ENTRY(GTK_BIN(dialog->input_dev)->child),
			conf->audio_dev[conf->audio_system]);

	gtk_range_set_value(GTK_RANGE(dialog->calculation_rate),
			conf->calculation_rate);
	gtk_range_set_value(GTK_RANGE(dialog->visualization_rate),
			conf->visualization_rate);
	gtk_range_set_value(GTK_RANGE(dialog->noise_threshold),
			conf->noise_threshold_db);
	gtk_range_set_value(GTK_RANGE(dialog->gain), conf->gain);
	gtk_spin_button_set_value(dialog->oversampling, conf->oversampling);
	//	lingot_config_dialog_set_root_reference_note(
	//			dialog->combo_root_frequency_reference_note,
	//			conf->root_frequency_referente_note);
	gtk_spin_button_set_value(dialog->root_frequency_error,
			conf->root_frequency_error);
	gtk_spin_button_set_value(dialog->temporal_window, conf->temporal_window);
	gtk_spin_button_set_value(dialog->dft_number, conf->dft_number);
	gtk_spin_button_set_value(dialog->dft_size, conf->dft_size);
	gtk_spin_button_set_value(dialog->peak_number, conf->peak_number);
	gtk_spin_button_set_value(dialog->peak_halfwidth, conf->peak_half_width);
	gtk_spin_button_set_value(dialog->minimum_frequency, conf->min_frequency);
	gtk_range_set_value(GTK_RANGE(dialog->rejection_peak_relation),
			conf->peak_rejection_relation_db);
	lingot_config_dialog_combo_select_value(GTK_WIDGET(dialog->fft_size),
			(int) conf->fft_size);

	char buff[10];
	sprintf(buff, "%d", conf->sample_rate);
	gtk_entry_set_text(GTK_ENTRY(GTK_BIN(dialog->sample_rate)->child), buff);

	lingot_config_dialog_rewrite_scale(dialog, conf->scale);
}

void lingot_config_dialog_destroy(LingotConfigDialog* dialog) {
	dialog->mainframe->config_dialog = NULL;
	lingot_config_destroy(dialog->conf);
	lingot_config_destroy(dialog->conf_old);
	free(dialog);
}

int lingot_config_dialog_apply(LingotConfigDialog* dialog) {

	GtkWidget* message_dialog;
	gchar* text;
	LingotConfig* conf = dialog->conf;

	GtkTreeIter iter;
	GtkTreeModel* model = gtk_tree_view_get_model(dialog->scale_treeview);

	gchar* name;
	gdouble pitch, freq, last_pitch;

	gtk_tree_model_get_iter_first(model, &iter);

	int i = 0;
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

	conf->audio_system = lingot_config_dialog_get_audio_system(
			dialog->input_system);
	sprintf(conf->audio_dev[conf->audio_system], "%s", gtk_entry_get_text(
			GTK_ENTRY(GTK_BIN(dialog->input_dev)->child)));
	conf->root_frequency_error = gtk_spin_button_get_value_as_float(
			dialog->root_frequency_error);
	conf->calculation_rate = gtk_range_get_value(GTK_RANGE(
			dialog->calculation_rate));
	conf->visualization_rate = gtk_range_get_value(GTK_RANGE(
			dialog->visualization_rate));
	conf->temporal_window = gtk_spin_button_get_value_as_float(
			dialog->temporal_window);
	conf->noise_threshold_db = gtk_range_get_value(GTK_RANGE(
			dialog->noise_threshold));
	conf->gain = gtk_range_get_value(GTK_RANGE(dialog->gain));
	conf->oversampling = gtk_spin_button_get_value_as_int(dialog->oversampling);
	conf->dft_number = gtk_spin_button_get_value_as_int(dialog->dft_number);
	conf->dft_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
			dialog->dft_size));
	conf->peak_number = gtk_spin_button_get_value_as_int(dialog->peak_number);
	conf->peak_half_width = gtk_spin_button_get_value_as_int(
			dialog->peak_halfwidth);
	conf->peak_rejection_relation_db = gtk_range_get_value(GTK_RANGE(
			dialog->rejection_peak_relation));
	conf->min_frequency = gtk_spin_button_get_value_as_int(
			dialog->minimum_frequency);
	text = gtk_combo_box_get_active_text(dialog->fft_size);
	conf->fft_size = atoi(text);
	g_free(text);
	text = gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dialog->sample_rate)->child));
	conf->sample_rate = atoi(text);

	LingotScale* scale = conf->scale;
	lingot_config_scale_destroy(scale);

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

	lingot_config_update_internal_params(conf);
	lingot_mainframe_change_config(dialog->mainframe, conf);

	return 1;
}

void lingot_config_dialog_close(LingotConfigDialog* dialog) {
	dialog->mainframe->config_dialog = NULL;
	gtk_widget_destroy(dialog->win);
}

static void lingot_config_dialog_scale_tree_add_row_tree(gpointer data,
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

static void lingot_config_dialog_scale_tree_add_column(
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

static void lingot_config_dialog_import_scl(gpointer data,
		LingotConfigDialog* config_dialog) {
	GtkWidget * dialog = gtk_file_chooser_dialog_new("Open File",
			config_dialog->win, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	GtkFileFilter *filefilter;
	filefilter = gtk_file_filter_new();

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
			lingot_config_dialog_rewrite_scale(config_dialog, scale);
		}

		g_free(filename);
	}
	gtk_widget_destroy(dialog);
	//g_free(filefilter);
}

void lingot_config_dialog_show(LingotMainFrame* frame) {
	GladeXML* _gladeXML = NULL;

	if (frame->config_dialog == NULL) {

		LingotConfigDialog* dialog = malloc(sizeof(LingotConfigDialog));

		dialog->mainframe = frame;
		dialog->conf = lingot_config_new();
		dialog->conf_old = lingot_config_new();

		// config copy
		lingot_config_copy(dialog->conf, frame->conf);
		lingot_config_copy(dialog->conf_old, frame->conf);

		// TODO: obtain glade files installation dir by other way

		FILE* fd = fopen("src/glade/lingot-config-dialog.glade", "r");
		if (fd != NULL) {
			fclose(fd);
			_gladeXML = glade_xml_new("src/glade/lingot-config-dialog.glade",
					"dialog1", NULL);
		} else {
_gladeXML		= glade_xml_new(
				LINGOT_GLADEDIR "lingot-config-dialog.glade", "dialog1",
				NULL);
	}

	dialog->win = glade_xml_get_widget(_gladeXML, "dialog1");

	gtk_window_set_icon(GTK_WINDOW(dialog->win), gtk_window_get_icon(
					GTK_WINDOW(frame->win)));
	//gtk_window_set_position(GTK_WINDOW(dialog->win), GTK_WIN_POS_CENTER_ALWAYS);
	dialog->mainframe->config_dialog = dialog;

	dialog->input_system
	= GTK_COMBO_BOX(glade_xml_get_widget(_gladeXML, "input_system"));
	dialog->input_dev
	= GTK_COMBO_BOX_ENTRY(glade_xml_get_widget(_gladeXML, "input_dev"));
	dialog->sample_rate
	= GTK_COMBO_BOX_ENTRY(glade_xml_get_widget(_gladeXML, "sample_rate"));
	dialog->calculation_rate
	= GTK_HSCALE(glade_xml_get_widget(_gladeXML, "calculation_rate"));
	dialog->visualization_rate
	= GTK_HSCALE(glade_xml_get_widget(_gladeXML, "visualization_rate"));
	dialog->noise_threshold
	= GTK_HSCALE(glade_xml_get_widget(_gladeXML, "noise_threshold"));
	dialog->gain = GTK_HSCALE(glade_xml_get_widget(_gladeXML, "gain"));
	dialog->oversampling
	= GTK_SPIN_BUTTON(glade_xml_get_widget(_gladeXML, "oversampling"));
	dialog->fft_size
	= GTK_COMBO_BOX(glade_xml_get_widget(_gladeXML, "fft_size"));
	dialog->temporal_window
	= GTK_SPIN_BUTTON(glade_xml_get_widget(_gladeXML, "temporal_window"));
	dialog->root_frequency_error
	= GTK_SPIN_BUTTON(glade_xml_get_widget(_gladeXML,
					"root_frequency_error"));
	dialog->dft_number
	= GTK_SPIN_BUTTON(glade_xml_get_widget(_gladeXML, "dft_number"));
	dialog->dft_size
	= GTK_SPIN_BUTTON(glade_xml_get_widget(_gladeXML, "dft_size"));
	dialog->peak_number
	= GTK_SPIN_BUTTON(glade_xml_get_widget(_gladeXML, "peak_number"));
	dialog->peak_halfwidth
	= GTK_SPIN_BUTTON(glade_xml_get_widget(_gladeXML, "peak_halfwidth"));
	dialog->rejection_peak_relation
	= GTK_HSCALE(glade_xml_get_widget(_gladeXML,
					"rejection_peak_relation"));
	dialog->label_sample_rate1 = GTK_LABEL(glade_xml_get_widget(_gladeXML,
					"label_sample_rate1"));
	dialog->label_sample_rate2 = GTK_LABEL(glade_xml_get_widget(_gladeXML,
					"label_sample_rate2"));
	dialog->minimum_frequency
	= GTK_SPIN_BUTTON(glade_xml_get_widget(_gladeXML,
					"minimum_frequency"));

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
	gtk_container_add(GTK_CONTAINER(scroll),
			GTK_WIDGET(dialog->scale_treeview));

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

	gtk_signal_connect(GTK_OBJECT(dialog->input_system), "changed",
			GTK_SIGNAL_FUNC (lingot_config_dialog_callback_change_input_system), dialog);
	gtk_signal_connect(GTK_ENTRY(GTK_BIN(dialog->sample_rate)->child), "changed",
			GTK_SIGNAL_FUNC (lingot_config_dialog_callback_change_sample_rate), dialog);

	gtk_signal_connect (GTK_OBJECT (dialog->oversampling), "value_changed",
			GTK_SIGNAL_FUNC (lingot_config_dialog_callback_change_sample_rate), dialog);
	gtk_signal_connect (GTK_OBJECT (dialog->root_frequency_error), "value_changed",
			GTK_SIGNAL_FUNC (lingot_config_dialog_callback_change_deviation), dialog);

	g_signal_connect(GTK_OBJECT(glade_xml_get_widget(_gladeXML, "button_default")), "clicked", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_button_default), dialog);
	g_signal_connect(GTK_OBJECT(glade_xml_get_widget(_gladeXML, "button_apply")), "clicked", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_button_apply), dialog);
	g_signal_connect(GTK_OBJECT(glade_xml_get_widget(_gladeXML, "button_accept")), "clicked", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_button_ok), dialog);
	g_signal_connect(GTK_OBJECT(glade_xml_get_widget(_gladeXML, "button_cancel")), "clicked", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_button_cancel), dialog);
	g_signal_connect(GTK_OBJECT(dialog->win), "destroy", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_close), dialog);

	g_object_unref(_gladeXML);
	lingot_config_dialog_rewrite(dialog);

	gtk_widget_show(dialog->win);
	gtk_widget_show_all(scroll);
} else {
	gtk_window_present(GTK_WINDOW(frame->config_dialog->win));
}
}
