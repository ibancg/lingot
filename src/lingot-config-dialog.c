/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2009  Ibán Cereijo Graña, Jairo Chapela Martínez.
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

#include "lingot-defs.h"

#include "lingot-core.h"
#include "lingot-config.h"
#include "lingot-mainframe.h"
#include "lingot-config-dialog.h"
#include "lingot-i18n.h"

#ifdef JACK
#include <jack/jack.h>
#endif

void lingot_config_dialog_rewrite(LingotConfigDialog*);
void lingot_config_dialog_combo_select_value(GtkWidget* combo, int value);

/* button press event attention routine. */

void lingot_config_dialog_callback_button_cancel(GtkButton *button,
		LingotConfigDialog* dialog) {
	lingot_mainframe_change_config(dialog->mainframe, dialog->conf_old); // restore old configuration.
	lingot_config_dialog_close(dialog);
}

void lingot_config_dialog_callback_button_ok(GtkButton *button,
		LingotConfigDialog* dialog) {
	lingot_config_dialog_apply(dialog);
	lingot_config_save(dialog->conf, CONFIG_FILE_NAME);
	lingot_config_dialog_close(dialog);
}

void lingot_config_dialog_callback_button_apply(GtkButton *button,
		LingotConfigDialog* dialog) {
	lingot_config_dialog_apply(dialog);
	lingot_config_dialog_rewrite(dialog);
}

void lingot_config_dialog_callback_button_default(GtkButton *button,
		LingotConfigDialog* dialog) {
	lingot_config_reset(dialog->conf);
	lingot_config_dialog_rewrite(dialog);
}

void lingot_config_dialog_callback_close(GtkWidget *widget,
		LingotConfigDialog *dialog) {
	gtk_widget_destroy(dialog->win);
	lingot_config_dialog_destroy(dialog);
}

void lingot_config_dialog_change_sample_rate_labels(LingotConfigDialog *dialog,
		int sr) {
	char buff1[20];
	char buff2[20];
	float srf = sr;
	float os = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(dialog->oversampling));
	sprintf(buff1, "%d", sr);
	sprintf(buff2, " = %0.1f", srf / os);
	gtk_label_set_text(dialog->label_sample_rate1, buff1);
	gtk_label_set_text(dialog->jack_label_sample_rate1, buff1);
	gtk_label_set_text(dialog->label_sample_rate2, buff2);
}

void lingot_config_dialog_callback_change_sample_rate(GtkWidget *widget,
		LingotConfigDialog *dialog) {
	const char* text = (lingot_config_dialog_get_audio_system(
			dialog->input_system) == AUDIO_SYSTEM_JACK) ? gtk_label_get_text(
			dialog->jack_label_sample_rate1) : gtk_combo_box_get_active_text(
			dialog->sample_rate);
	int sr = (text != NULL) ? atoi(text) : 44100;
	lingot_config_dialog_change_sample_rate_labels(dialog, sr);
}

void lingot_config_dialog_callback_change_input_system(GtkWidget *widget,
		LingotConfigDialog *dialog) {
	char* text = gtk_combo_box_get_active_text(dialog->input_system);
	int core_running;

	if (!strcmp("JACK", text)) {

		core_running = dialog->mainframe->core->running;
		if (core_running) {
			lingot_core_stop(dialog->mainframe->core);
			lingot_core_destroy(dialog->mainframe->core);
		}

		dialog->conf->sample_rate = lingot_audio_jack_get_sample_rate();

		if (core_running) {
			dialog->mainframe->core = lingot_core_new(dialog->mainframe->conf);
			lingot_core_start(dialog->mainframe->core);
		}

		lingot_config_dialog_change_sample_rate_labels(dialog,
				dialog->conf->sample_rate);
		gtk_widget_hide(GTK_WIDGET(dialog->oss_alsa_label_sample_rate0));
		gtk_widget_hide(GTK_WIDGET(dialog->sample_rate));
		gtk_widget_hide(GTK_WIDGET(dialog->oss_alsa_label_sample_rate2));
		gtk_widget_hide(GTK_WIDGET(dialog->oss_alsa_label_input_dev0));
		gtk_widget_hide(GTK_WIDGET(dialog->input_dev));
		gtk_widget_hide(GTK_WIDGET(dialog->oss_alsa_label_input_dev2));
		gtk_widget_show(GTK_WIDGET(dialog->jack_label_sample_rate0));
		gtk_widget_show(GTK_WIDGET(dialog->jack_label_sample_rate1));
		gtk_widget_show(GTK_WIDGET(dialog->jack_label_sample_rate2));
	} else {
		gtk_widget_show(GTK_WIDGET(dialog->oss_alsa_label_sample_rate0));
		gtk_widget_show(GTK_WIDGET(dialog->sample_rate));
		gtk_widget_show(GTK_WIDGET(dialog->oss_alsa_label_sample_rate2));
		gtk_widget_show(GTK_WIDGET(dialog->oss_alsa_label_input_dev0));
		gtk_widget_show(GTK_WIDGET(dialog->input_dev));
		gtk_widget_show(GTK_WIDGET(dialog->oss_alsa_label_input_dev2));
		gtk_widget_hide(GTK_WIDGET(dialog->jack_label_sample_rate0));
		gtk_widget_hide(GTK_WIDGET(dialog->jack_label_sample_rate1));
		gtk_widget_hide(GTK_WIDGET(dialog->jack_label_sample_rate2));
		if (gtk_combo_box_get_active(dialog->sample_rate) < 0) {
			lingot_config_dialog_combo_select_value(GTK_WIDGET(dialog->sample_rate), 44100);
		}
	}
	lingot_config_dialog_callback_change_sample_rate(NULL, dialog);
}

void lingot_config_dialog_callback_change_a_frequence(GtkWidget *widget,
		LingotConfigDialog *dialog) {
	char buff[20];

	float root_freq = 440.0 * pow(2.0, gtk_spin_button_get_value_as_float
			(GTK_SPIN_BUTTON(dialog->root_frequency_error)) / 1200.0);
	sprintf(buff, "%%    = %0.2f", root_freq);
	gtk_label_set_text(GTK_LABEL(dialog->label_root_frequency), buff);
}

void lingot_config_dialog_set_audio_system(GtkComboBox* combo, int audio_system) {
	GtkTreeModel* model = gtk_combo_box_get_model(combo);
	GtkTreeIter iter;

	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
	char* tokens[] = { "OSS", "ALSA", "JACK" };

	while (valid) {
		gchar *str_data;
		gtk_tree_model_get(model, &iter, 0, &str_data, -1);
		if (!strcmp(str_data, tokens[audio_system]))
			gtk_combo_box_set_active_iter(combo, &iter);
		g_free(str_data);
		valid = gtk_tree_model_iter_next(model, &iter);
	}
}

int lingot_config_dialog_get_audio_system(GtkComboBox* combo) {
	char* text = gtk_combo_box_get_active_text(combo);
	if (text != NULL)
		if (!strcmp("OSS", text))
			return AUDIO_SYSTEM_OSS;
		else if (!strcmp("ALSA", text))
			return AUDIO_SYSTEM_ALSA;
		else if (!strcmp("JACK", text))
			return AUDIO_SYSTEM_JACK;

	return -1;
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

void lingot_config_dialog_rewrite(LingotConfigDialog* dialog) {
	LingotConfig* conf = dialog->conf;
	lingot_config_dialog_set_audio_system(dialog->input_system,
			conf->audio_system);
	gtk_entry_set_text(dialog->input_dev, conf->audio_dev);
	gtk_range_set_value(GTK_RANGE(dialog->calculation_rate), conf->calculation_rate);
	gtk_range_set_value(GTK_RANGE(dialog->visualization_rate), conf->visualization_rate);
	gtk_range_set_value(GTK_RANGE(dialog->noise_threshold), conf->noise_threshold_db);
	gtk_range_set_value(GTK_RANGE(dialog->gain), conf->gain);
	gtk_spin_button_set_value(dialog->oversampling, conf->oversampling);
	gtk_spin_button_set_value(dialog->root_frequency_error,
			conf->root_frequency_error);
	gtk_spin_button_set_value(dialog->temporal_window, conf->temporal_window);
	gtk_spin_button_set_value(dialog->dft_number, conf->dft_number);
	gtk_spin_button_set_value(dialog->dft_size, conf->dft_size);
	gtk_spin_button_set_value(dialog->peak_number, conf->peak_number);
	gtk_spin_button_set_value(dialog->peak_halfwidth, conf->peak_half_width);
	gtk_range_set_value(GTK_RANGE(dialog->rejection_peak_relation),
	conf->peak_rejection_relation_db);
	lingot_config_dialog_combo_select_value(GTK_WIDGET(dialog->fft_size), (int) conf->fft_size);
	lingot_config_dialog_combo_select_value(GTK_WIDGET(dialog->sample_rate),
	(int) conf->sample_rate);
	lingot_config_dialog_callback_change_sample_rate(NULL, dialog);
	lingot_config_dialog_callback_change_a_frequence(NULL, dialog);
}

void lingot_config_dialog_destroy(LingotConfigDialog* dialog) {
	dialog->mainframe->config_dialog = NULL;
	lingot_config_destroy(dialog->conf);
	lingot_config_destroy(dialog->conf_old);

	//gtk_widget_destroy(dialog->win);
	free(dialog);
}

void lingot_config_dialog_apply(LingotConfigDialog* dialog) {
	GtkWidget* message_dialog;

	dialog->conf->audio_system = lingot_config_dialog_get_audio_system(
			dialog->input_system);
	sprintf(dialog->conf->audio_dev, "%s",
			gtk_entry_get_text(dialog->input_dev));
	dialog->conf->root_frequency_error = gtk_spin_button_get_value_as_float(
			dialog->root_frequency_error);
	dialog->conf->calculation_rate = gtk_range_get_value(GTK_RANGE(dialog->calculation_rate));
	dialog->conf->visualization_rate = gtk_range_get_value(GTK_RANGE(dialog->visualization_rate));
	dialog->conf->temporal_window = gtk_spin_button_get_value_as_float(
			dialog->temporal_window);
	dialog->conf->noise_threshold_db = gtk_range_get_value(GTK_RANGE(dialog->noise_threshold));
	dialog->conf->gain = gtk_range_get_value(GTK_RANGE(dialog->gain));
	dialog->conf->oversampling = gtk_spin_button_get_value_as_int(
			dialog->oversampling);
	dialog->conf->dft_number = gtk_spin_button_get_value_as_int(
			dialog->dft_number);
	dialog->conf->dft_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->dft_size));
	dialog->conf->peak_number = gtk_spin_button_get_value_as_int(
			dialog->peak_number);
	dialog->conf->peak_half_width = gtk_spin_button_get_value_as_int(
			dialog->peak_halfwidth);
	dialog->conf->peak_rejection_relation_db = gtk_range_get_value(GTK_RANGE(dialog->rejection_peak_relation));
	dialog->conf->fft_size = atoi(gtk_combo_box_get_active_text(
			dialog->fft_size));
	dialog->conf->sample_rate = atoi((dialog->conf->audio_system
			== AUDIO_SYSTEM_JACK) ? gtk_label_get_text(
			dialog->jack_label_sample_rate1) : gtk_combo_box_get_active_text(
			dialog-> sample_rate));

	int result = lingot_config_update_internal_params(dialog->conf);

	lingot_mainframe_change_config(dialog->mainframe, dialog->conf);

	if (!result) {
		message_dialog
				= gtk_message_dialog_new(
						GTK_WINDOW(dialog->win),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_INFO,
						GTK_BUTTONS_CLOSE,
						_("Temporal buffer is smaller than FFT size. It has been increased to %0.3f seconds"),
						dialog->conf->temporal_window);
		gtk_window_set_title(GTK_WINDOW(message_dialog), _("Warning"));
		gtk_window_set_icon(GTK_WINDOW(message_dialog), gtk_window_get_icon(GTK_WINDOW(dialog->mainframe->win)));
		gtk_dialog_run(GTK_DIALOG(message_dialog));
		gtk_widget_destroy(message_dialog);
	}
}

void lingot_config_dialog_close(LingotConfigDialog* dialog) {
	gtk_widget_destroy(dialog->win);
	dialog->mainframe->config_dialog = NULL;
}

void lingot_config_dialog_show(LingotMainFrame* frame) {
	GError *err = NULL;
	GtkBuilder *builder;

	if (frame->config_dialog == NULL) {

		LingotConfigDialog* dialog = malloc(sizeof(LingotConfigDialog));

		dialog->mainframe = frame;
		dialog->conf = lingot_config_new();
		dialog->conf_old = lingot_config_new();

		// config copy
		*dialog->conf = *frame->conf;
		*dialog->conf_old = *frame->conf;

		builder = gtk_builder_new();
		// TODO: obtain glade files installation dir by other way
		if (gtk_builder_add_from_file(
				builder,
				"" PACKAGE_LOCALE_DIR "/../lingot/glade/lingot-config-dialog.xml",
				&err) == 0) {
			//error_message(err->message);
			g_error_free(err);
			return;
		}
		dialog->win = GTK_WIDGET(gtk_builder_get_object(builder, "dialog1"));
		gtk_window_set_icon(GTK_WINDOW(dialog->win), gtk_window_get_icon(GTK_WINDOW(frame->win)));
		//gtk_window_set_position(GTK_WINDOW(dialog->win), GTK_WIN_POS_CENTER_ALWAYS);
		dialog->mainframe->config_dialog = dialog;

		dialog->input_system = GTK_COMBO_BOX(gtk_builder_get_object(builder, "input_system"));

		dialog->input_dev = GTK_ENTRY(gtk_builder_get_object(builder, "input_dev"));
		dialog->sample_rate = GTK_COMBO_BOX(gtk_builder_get_object(builder, "sample_rate"));
		dialog->calculation_rate = GTK_HSCALE(gtk_builder_get_object(builder, "calculation_rate"));
		dialog->visualization_rate = GTK_HSCALE(gtk_builder_get_object(builder, "visualization_rate"));
		dialog->noise_threshold = GTK_HSCALE(gtk_builder_get_object(builder, "noise_threshold"));
		dialog->gain = GTK_HSCALE(gtk_builder_get_object(builder, "gain"));
		dialog->oversampling = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "oversampling"));
		dialog->fft_size = GTK_COMBO_BOX(gtk_builder_get_object(builder, "fft_size"));
		dialog->temporal_window = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "temporal_window"));
		dialog->root_frequency_error = GTK_SPIN_BUTTON(gtk_builder_get_object(builder,
						"root_frequency_error"));
		dialog->dft_number = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "dft_number"));
		dialog->dft_size = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "dft_size"));
		dialog->peak_number = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "peak_number"));
		dialog->peak_halfwidth = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "peak_halfwidth"));
		dialog->rejection_peak_relation = GTK_HSCALE(gtk_builder_get_object(builder,
						"rejection_peak_relation"));
		dialog->label_sample_rate1 = GTK_LABEL(gtk_builder_get_object(builder,
						"label_sample_rate1"));
		dialog->label_sample_rate2 = GTK_LABEL(gtk_builder_get_object(builder,
						"label_sample_rate2"));
		dialog->label_root_frequency = GTK_LABEL(gtk_builder_get_object(builder,
						"label_root_frequency"));
		dialog->jack_label_sample_rate0 = GTK_LABEL(gtk_builder_get_object(builder,
						"jack_label_sample_rate0"));
		dialog->jack_label_sample_rate1 = GTK_LABEL(gtk_builder_get_object(builder,
						"jack_label_sample_rate1"));
		dialog->jack_label_sample_rate2 = GTK_LABEL(gtk_builder_get_object(builder,
						"jack_label_sample_rate2"));
		dialog->oss_alsa_label_sample_rate0 = GTK_LABEL(gtk_builder_get_object(builder,
						"oss_alsa_label_sample_rate0"));
		dialog->oss_alsa_label_sample_rate2 = GTK_LABEL(gtk_builder_get_object(builder,
						"oss_alsa_label_sample_rate2"));
		dialog->oss_alsa_label_input_dev0 = GTK_LABEL(gtk_builder_get_object(builder,
						"oss_alsa_label_input_dev0"));
		dialog->oss_alsa_label_input_dev2 = GTK_LABEL(gtk_builder_get_object(builder,
						"oss_alsa_label_input_dev2"));

		gtk_signal_connect(GTK_OBJECT(dialog->input_system), "changed",
				GTK_SIGNAL_FUNC (lingot_config_dialog_callback_change_input_system), dialog);
		gtk_signal_connect(GTK_OBJECT(dialog->sample_rate), "changed",
				GTK_SIGNAL_FUNC (lingot_config_dialog_callback_change_sample_rate), dialog);

		gtk_signal_connect (GTK_OBJECT (dialog->oversampling), "value_changed",
				GTK_SIGNAL_FUNC (lingot_config_dialog_callback_change_sample_rate), dialog);
		gtk_signal_connect (GTK_OBJECT (dialog->root_frequency_error), "value_changed",
				GTK_SIGNAL_FUNC (lingot_config_dialog_callback_change_a_frequence), dialog);

		g_signal_connect(GTK_OBJECT(gtk_builder_get_object(builder, "button_default")), "clicked", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_button_default), dialog);
		g_signal_connect(GTK_OBJECT(gtk_builder_get_object(builder, "button_apply")), "clicked", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_button_apply), dialog);
		g_signal_connect(GTK_OBJECT(gtk_builder_get_object(builder, "button_accept")), "clicked", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_button_ok), dialog);
		g_signal_connect(GTK_OBJECT(gtk_builder_get_object(builder, "button_cancel")), "clicked", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_button_cancel), dialog);

		g_signal_connect(GTK_OBJECT(dialog->win), "destroy", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_close), dialog);

		g_object_unref(builder);
		lingot_config_dialog_rewrite(dialog);

		gtk_widget_show(dialog->win);
	} else {
		gtk_window_present(GTK_WINDOW(frame->config_dialog->win));
	}
}
