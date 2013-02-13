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

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "lingot-defs.h"

#include "lingot-core.h"
#include "lingot-config.h"
#include "lingot-gui-mainframe.h"
#include "lingot-gui-config-dialog.h"
#include "lingot-i18n.h"
#include "lingot-config.h"
#include "lingot-gui-config-dialog-scale.h"
#include "lingot-msg.h"

int lingot_gui_config_dialog_apply(LingotConfigDialog*);
void lingot_gui_config_dialog_close(LingotConfigDialog*);
void lingot_gui_config_dialog_rewrite(LingotConfigDialog*);
void lingot_gui_config_dialog_combo_select_value(GtkWidget* combo, int value);
audio_system_t lingot_gui_config_dialog_get_audio_system(GtkComboBoxText* combo);
void lingot_gui_config_dialog_set_audio_device(GtkComboBoxText* combo,
		const gchar* device_name);

static const unsigned short frequency_combo_n_octaves = 6;
static const unsigned short frequency_combo_first_octave = 1;

/* button press event attention routine. */

void lingot_gui_config_dialog_callback_button_cancel(GtkButton *button,
		LingotConfigDialog* dialog) {
	lingot_gui_config_dialog_close(dialog);
}

void lingot_gui_config_dialog_callback_button_ok(GtkButton *button,
		LingotConfigDialog* dialog) {
	if (lingot_gui_config_dialog_apply(dialog)) {
		// dumps the current config to the config file
		lingot_config_save(dialog->conf, CONFIG_FILE_NAME);
		// establish the current config as the old config, so the close rollback
		// will do nothing.
		lingot_config_copy(dialog->conf_old, dialog->conf);
		lingot_gui_config_dialog_close(dialog);
	}
}

void lingot_gui_config_dialog_callback_button_apply(GtkButton *button,
		LingotConfigDialog* dialog) {
	if (lingot_gui_config_dialog_apply(dialog)) {
		lingot_gui_config_dialog_rewrite(dialog);
	}
}

void lingot_gui_config_dialog_callback_button_default(GtkButton *button,
		LingotConfigDialog* dialog) {
	lingot_config_restore_default_values(dialog->conf);
	lingot_gui_config_dialog_rewrite(dialog);
}

void lingot_gui_config_dialog_callback_cancel(GtkWidget *widget,
		LingotConfigDialog* dialog) {
	//lingot_mainframe_change_config(dialog->mainframe, dialog->conf_old); // restore old configuration.
	lingot_gui_config_dialog_close(dialog);
}

void lingot_gui_config_dialog_callback_close(GtkWidget *widget,
		LingotConfigDialog *dialog) {
	lingot_gui_mainframe_change_config(dialog->mainframe, dialog->conf_old); // restore old configuration.
	gtk_widget_destroy(dialog->win);
	lingot_gui_config_dialog_destroy(dialog);
}

void lingot_gui_config_dialog_callback_change_sample_rate(GtkWidget *widget,
		LingotConfigDialog *dialog) {

	const gchar* text = gtk_entry_get_text(GTK_ENTRY(
			gtk_bin_get_child(GTK_BIN(dialog->sample_rate))) );

	int sr;
	if (text != NULL ) {
		sr = atoi(text);
	} else {
		sr = 44100;
	}

	char buff1[20];
	char buff2[20];
	gdouble srf = sr;
	gdouble os = gtk_spin_button_get_value(GTK_SPIN_BUTTON(
			dialog->oversampling) );
	sprintf(buff1, "%d", sr);
	sprintf(buff2, " = %0.1f", srf / os);
	gtk_label_set_text(dialog->label_sample_rate1, buff1);
	//gtk_label_set_text(dialog->jack_label_sample_rate1, buff1);
	gtk_label_set_text(dialog->label_sample_rate2, buff2);
}

void lingot_gui_config_dialog_callback_change_input_system(GtkWidget *widget,
		LingotConfigDialog *dialog) {

	char buff[10];
	char* text = gtk_combo_box_text_get_active_text(dialog->input_system);
	audio_system_t audio_system = str_to_audio_system_t(text);
	free(text);

	LingotAudioSystemProperties* properties =
			lingot_audio_get_audio_system_properties(audio_system);

	if (properties != NULL ) {
		if (properties->forced_sample_rate) {
			if (properties->n_sample_rates > 0) {
				sprintf(buff, "%d", properties->sample_rates[0]);
				gtk_entry_set_text(
						GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->sample_rate))),
						buff);
			}
		}

		gtk_widget_set_sensitive(GTK_WIDGET(dialog->sample_rate),
				(gboolean) !properties->forced_sample_rate);

		GtkListStore* model = GTK_LIST_STORE(gtk_combo_box_get_model(
						GTK_COMBO_BOX(dialog->sample_rate)));
		gtk_list_store_clear(model);

		int i;
		for (i = 0; i < properties->n_sample_rates; i++) {
			sprintf(buff, "%d", properties->sample_rates[i]);
			gtk_combo_box_text_append_text(dialog->sample_rate, buff);
		}

		GtkListStore* input_dev_model = GTK_LIST_STORE(gtk_combo_box_get_model(
						GTK_COMBO_BOX(dialog->input_dev)));
		gtk_list_store_clear(input_dev_model);

		if (properties->devices != NULL ) {
			for (i = 0; i < properties->n_devices; i++)
				if (properties->devices[i] != NULL ) {
					gtk_combo_box_text_append_text(dialog->input_dev,
							properties->devices[i]);
				}
		}

		lingot_gui_config_dialog_set_audio_device(dialog->input_dev,
				dialog->conf->audio_dev[audio_system]);

		lingot_audio_audio_system_properties_destroy(properties);
	} else {
		gtk_entry_set_text(
				GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->input_dev))), "");
		gtk_widget_set_sensitive(GTK_WIDGET(dialog->input_dev), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(dialog->sample_rate), FALSE);
	}
}

void lingot_gui_config_dialog_set_audio_system(GtkComboBoxText* combo,
		audio_system_t audio_system) {
	const char* token = audio_system_t_to_str(audio_system);
	GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo) );
	GtkTreeIter iter;

	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

	while (valid) {
		gchar *str_data;
		gtk_tree_model_get(model, &iter, 0, &str_data, -1);
		if (!strcmp(str_data, token))
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), &iter);
		g_free(str_data);
		valid = gtk_tree_model_iter_next(model, &iter);
	}
}

audio_system_t lingot_gui_config_dialog_get_audio_system(GtkComboBoxText* combo) {
	char* text = gtk_combo_box_text_get_active_text(combo);
	int result = str_to_audio_system_t(text);
	free(text);
	return result;
}

// extracts the string between <>
static const gchar* lingot_gui_config_dialog_get_string(const gchar* str) {
	static char buffer[1024];

	const char* delim = "<>";
	char* str_ = strdup(str);
	char* token = strtok(str_, delim);
	if ((token == str_) && (strlen(token) != strlen(str))) {
		token = strtok(NULL, delim);
	}

	if (token != 0x0) {
		strcpy(buffer, token);
	} else {
		strcpy(buffer, str);
	}
	free(str_);
	return buffer;
}

// extracts the audio device name from the text introduced in the entry (which
// includes description)
const gchar* lingot_gui_config_dialog_get_audio_device(const gchar* str) {
	return lingot_gui_config_dialog_get_string(str);
}

void lingot_gui_config_dialog_set_audio_device(GtkComboBoxText* combo,
		const gchar* device_name) {

	GtkTreeModel* model = GTK_TREE_MODEL(gtk_combo_box_get_model(
					GTK_COMBO_BOX(combo)));
	GtkTreeIter iter;
	const gchar* item;
	const gchar* filtered_item;
	const gchar* filtered_name = device_name;
	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			gtk_tree_model_get(model, &iter, 0, &item, -1);
			filtered_item = lingot_gui_config_dialog_get_audio_device(item);
			if (!strcmp(device_name, filtered_item)) {
				filtered_name = item;
				break;
			}
		} while (gtk_tree_model_iter_next(model, &iter));
	}

	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo))),
			filtered_name);
}

// extracts the frequency from the text introduced in the entry (which may
// include the note name)
double lingot_gui_config_dialog_get_frequency(const gchar* str) {
	char* endptr;
	const char* double_str = lingot_gui_config_dialog_get_string(str);
	double result = strtod(double_str, &endptr);

	if ((result == 0.0) && (endptr == double_str)) {
		result = -1.0;
	} else if ((result < 0.0) || (result >= 20000.0)) {
		// TODO: limits
		// TODO: warning to the user?
		result = -1.0;
	}

	printf("frequency '%s' = %f\n", str, result);
	return result;
}

void lingot_gui_config_dialog_set_frequency(LingotConfigDialog* dialog,
		GtkComboBoxText* combo, double frequency) {

	if (frequency >= 0.0) {
		static char buffer[20];

		// TODO: iterate over the scale, not the combo
		double error_cents;
		int closest_index = lingot_config_scale_get_closest_note_index(
				dialog->conf->scale, frequency,
				dialog->conf->root_frequency_error, &error_cents);

		double freq2 = lingot_config_scale_get_frequency(dialog->conf->scale,
				closest_index); // TODO: deviation
		if (fabs(frequency - freq2) < 1e-1) {
			int index = lingot_config_scale_get_note_index(dialog->conf->scale,
					closest_index);
			int octave = lingot_config_scale_get_octave(dialog->conf->scale,
					closest_index) + 4;
			// TODO: note name
			sprintf(buffer, "%s %d <%0.2f>",
					dialog->conf->scale->note_name[index], octave, frequency);
		} else {
			sprintf(buffer, "%0.2f", frequency);
		}

		printf("closest index to frequency %f is %d, freq %f\n", frequency,
				closest_index, freq2);

		printf("frequency name for %f = '%s'\n", frequency, buffer);
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo))),
				buffer);
	}
}

void lingot_gui_config_dialog_populate_frequency_combos(
		LingotConfigDialog* dialog) {

	// TODO: only when necessary
	printf("lingot_gui_config_dialog_callback_change_scale()...\n");

	static char buff[100];

	// Unset the model first and perform the operation over a new Combo to avoid
	// bug 673079 in gnome: https://bugzilla.gnome.org/show_bug.cgi?id=673079
	// TODO: another way?
	gtk_combo_box_set_model(GTK_COMBO_BOX(dialog->minimum_frequency), NULL );
	gtk_combo_box_set_model(GTK_COMBO_BOX(dialog->maximum_frequency), NULL );

	GtkComboBoxText* combo =
			GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new_with_entry());

	LingotConfig* config = dialog->conf;

	int i;
	int octave_index;
	for (i = 0; i < config->scale->notes; i++) {
		for (octave_index = frequency_combo_first_octave;
				octave_index
						< frequency_combo_first_octave
								+ frequency_combo_n_octaves; octave_index++) {
			sprintf(buff,
					"<span font_desc=\"%d\">%s</span><span font_desc=\"%d\">%i</span>",
					12, config->scale->note_name[i], 7, octave_index);
			gtk_combo_box_text_append_text(combo, buff);
		}
	}

	gtk_combo_box_set_model(GTK_COMBO_BOX(dialog->minimum_frequency),
			gtk_combo_box_get_model(GTK_COMBO_BOX(combo) ));
	gtk_combo_box_set_model(GTK_COMBO_BOX(dialog->maximum_frequency),
			gtk_combo_box_get_model(GTK_COMBO_BOX(combo) ));
	gtk_widget_destroy(GTK_WIDGET(combo) );
	printf("ok ... lingot_gui_config_dialog_callback_change_scale()\n");
}

void lingot_gui_config_dialog_change_combo_frequency(GtkComboBoxText *combo,
		LingotConfigDialog *dialog) {

	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo) );
	if (index >= 0) {
		int octave = frequency_combo_first_octave
				+ (index % frequency_combo_n_octaves) - 4;
		int local_index = index / frequency_combo_n_octaves;
		int global_index = local_index + octave * dialog->conf->scale->notes;
		double frequency = lingot_config_scale_get_frequency(
				dialog->conf->scale, global_index); // TODO: deviation
		lingot_gui_config_dialog_set_frequency(dialog, combo, frequency);
		printf("CHANGE global index %d, octave %d %d, local index %d, %d, %f\n",
				global_index, octave,
				lingot_config_scale_get_octave(dialog->conf->scale,
						global_index), local_index, index, frequency);
	}
}

void lingot_gui_config_dialog_change_min_frequency(GtkWidget *widget,
		LingotConfigDialog *dialog) {
	lingot_gui_config_dialog_change_combo_frequency(GTK_COMBO_BOX_TEXT(widget),
			dialog);
}

void lingot_gui_config_dialog_change_max_frequency(GtkWidget *widget,
		LingotConfigDialog *dialog) {
	lingot_gui_config_dialog_change_combo_frequency(GTK_COMBO_BOX_TEXT(widget),
			dialog);
}

void lingot_gui_config_dialog_combo_select_value(GtkWidget* combo, int value) {

	GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo) );
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

void lingot_gui_config_dialog_rewrite(LingotConfigDialog* dialog) {
	LingotConfig* conf = dialog->conf;

	lingot_gui_config_dialog_set_audio_system(dialog->input_system,
			conf->audio_system);
	lingot_gui_config_dialog_set_audio_device(dialog->input_dev,
			conf->audio_dev[conf->audio_system]);

	gtk_range_set_value(GTK_RANGE(dialog->calculation_rate),
			conf->calculation_rate);
	gtk_range_set_value(GTK_RANGE(dialog->visualization_rate),
			conf->visualization_rate);
	gtk_range_set_value(GTK_RANGE(dialog->noise_threshold),
			conf->noise_threshold_db);
//	gtk_range_set_value(GTK_RANGE(dialog->gain), conf->gain);
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
	gtk_range_set_value(GTK_RANGE(dialog->rejection_peak_relation),
			conf->peak_rejection_relation_db);
	lingot_gui_config_dialog_combo_select_value(GTK_WIDGET(dialog->fft_size),
			(int) conf->fft_size);
	lingot_gui_config_dialog_set_frequency(dialog, dialog->minimum_frequency,
			conf->min_frequency);
	lingot_gui_config_dialog_set_frequency(dialog, dialog->maximum_frequency,
			conf->max_frequency);

	char buff[10];
	sprintf(buff, "%d", conf->sample_rate);
	gtk_entry_set_text(
			GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->sample_rate))), buff);

	lingot_gui_config_dialog_scale_rewrite(dialog, conf->scale);
}

void lingot_gui_config_dialog_destroy(LingotConfigDialog* dialog) {
	dialog->mainframe->config_dialog = NULL;
	lingot_config_destroy(dialog->conf);
	lingot_config_destroy(dialog->conf_old);
	free(dialog);
}

int lingot_gui_config_dialog_apply(LingotConfigDialog* dialog) {

	gchar* text1;
	const gchar* text2;
	LingotConfig* conf = dialog->conf;

	// validation

	if (!lingot_gui_config_dialog_scale_validate(dialog, conf->scale)) {
		return 0;
	}

	const char* audio_device = lingot_gui_config_dialog_get_audio_device(
			gtk_entry_get_text(
					GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->input_dev))) ));

	LingotConfigParameterSpec audioDeviceSpec =
			lingot_config_get_parameter_spec(LINGOT_PARAMETER_ID_AUDIO_DEV);

	if (strlen(audio_device) >= audioDeviceSpec.str_max_len) {
		lingot_msg_add_error(_("Audio device identifier too long"));
		gtk_notebook_set_current_page(dialog->notebook, 0);
		return 0;
	}

	text2 = gtk_entry_get_text(
			GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->sample_rate))) );
	int sample_rate = atoi(text2);

	LingotConfigParameterSpec sampleRateSpec = lingot_config_get_parameter_spec(
			LINGOT_PARAMETER_ID_SAMPLE_RATE);

	// TODO: generalize validation?
	if ((sample_rate < sampleRateSpec.int_min)
			|| (sample_rate > sampleRateSpec.int_max)) {
		char buff[1000];
		sprintf(buff, "%s %i - %i %s", _("Sample rate out of range"),
				sampleRateSpec.int_min, sampleRateSpec.int_max,
				sampleRateSpec.units);
		lingot_msg_add_error(buff);
		gtk_notebook_set_current_page(dialog->notebook, 0);
		return 0;
	}

	conf->audio_system = lingot_gui_config_dialog_get_audio_system(
			dialog->input_system);
	sprintf(conf->audio_dev[conf->audio_system], "%s",
			lingot_gui_config_dialog_get_audio_device(
					gtk_entry_get_text(
							GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->input_dev))) )));
	conf->root_frequency_error = gtk_spin_button_get_value(
			dialog->root_frequency_error);
	conf->calculation_rate = gtk_range_get_value(GTK_RANGE(
			dialog->calculation_rate) );
	conf->visualization_rate = gtk_range_get_value(GTK_RANGE(
			dialog->visualization_rate) );
	conf->temporal_window = gtk_spin_button_get_value(dialog->temporal_window);
	conf->noise_threshold_db = gtk_range_get_value(GTK_RANGE(
			dialog->noise_threshold) );
//	conf->gain = gtk_range_get_value(GTK_RANGE(dialog->gain) );
	conf->oversampling = gtk_spin_button_get_value_as_int(dialog->oversampling);
	conf->dft_number = gtk_spin_button_get_value_as_int(dialog->dft_number);
	conf->dft_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
			dialog->dft_size) );
	conf->peak_number = gtk_spin_button_get_value_as_int(dialog->peak_number);
	conf->peak_half_width = gtk_spin_button_get_value_as_int(
			dialog->peak_halfwidth);
	conf->peak_rejection_relation_db = gtk_range_get_value(GTK_RANGE(
			dialog->rejection_peak_relation) );
	double min_freq = lingot_gui_config_dialog_get_frequency(
			gtk_entry_get_text(
					GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->minimum_frequency))) ));
	double max_freq = lingot_gui_config_dialog_get_frequency(
			gtk_entry_get_text(
					GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->maximum_frequency))) ));
	const LingotConfigParameterSpec minimumFrequencySpec = lingot_config_get_parameter_spec(
			LINGOT_PARAMETER_ID_MINIMUM_FREQUENCY);
	const LingotConfigParameterSpec maximumFrequencySpec = lingot_config_get_parameter_spec(
			LINGOT_PARAMETER_ID_MAXIMUM_FREQUENCY);

	if ((min_freq >= minimumFrequencySpec.float_min)
			&& (min_freq <= minimumFrequencySpec.float_max)) {
		conf->min_frequency = min_freq;
	}
	if ((max_freq >= maximumFrequencySpec.float_min)
			&& (max_freq <= maximumFrequencySpec.float_max)) {
		conf->max_frequency = max_freq;
	}

	text1 = gtk_combo_box_text_get_active_text(dialog->fft_size);
	conf->fft_size = atoi(text1);
	g_free(text1);
	conf->sample_rate = sample_rate;

	LingotScale* scale = conf->scale;
	lingot_config_scale_destroy(scale);

	lingot_gui_config_dialog_scale_apply(dialog, scale);

	lingot_config_update_internal_params(conf);

	lingot_gui_mainframe_change_config(dialog->mainframe, conf);

	if (scale->max_offset_rounded > 200) {
		lingot_msg_add_warning(
				_("The provided scale contains wide gaps in frequency that increase the gauge range and produce a loss of visual accuracy. Consider providing scales with at least 12 tones, or with a maximum distance between adjacent notes below 200 cents."));
	}

	lingot_gui_config_dialog_populate_frequency_combos(dialog);

	return 1;
}

void lingot_gui_config_dialog_close(LingotConfigDialog* dialog) {
	dialog->mainframe->config_dialog = NULL;
	gtk_widget_destroy(dialog->win);
}

void lingot_gui_config_dialog_show(LingotMainFrame* frame, LingotConfig* config) {

	if (frame->config_dialog == NULL ) {

		LingotConfigDialog* dialog = malloc(sizeof(LingotConfigDialog));

		dialog->mainframe = frame;
		dialog->conf = lingot_config_new();
		dialog->conf_old = lingot_config_new();

		// config copy
		lingot_config_copy(dialog->conf,
				(config == NULL )? frame->conf : config);
		lingot_config_copy(dialog->conf_old, frame->conf);

		GtkBuilder* builder = gtk_builder_new();

		// TODO: obtain glade files installation dir by other way
		// TODO: add dev condition to check for the file name in $PWD
#	    define FILE_NAME "lingot-config-dialog.glade"
		FILE* fd = fopen("src/glade/" FILE_NAME, "r");
		if (fd != NULL ) {
			fclose(fd);
			gtk_builder_add_from_file(builder, "src/glade/" FILE_NAME, NULL );
		} else {
			gtk_builder_add_from_file(builder, LINGOT_GLADEDIR FILE_NAME,
					NULL );
		}
#		undef FILE_NAME

		dialog->win = GTK_WIDGET(gtk_builder_get_object(builder, "dialog1"));

		gtk_window_set_icon(GTK_WINDOW(dialog->win),
				gtk_window_get_icon(GTK_WINDOW(frame->win) ));
		//gtk_window_set_position(GTK_WINDOW(dialog->win), GTK_WIN_POS_CENTER_ALWAYS);
		dialog->mainframe->config_dialog = dialog;

		dialog->input_system =
				GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "input_system"));

		gtk_combo_box_text_append_text(dialog->input_system,
				audio_system_t_to_str(AUDIO_SYSTEM_OSS));
#ifdef ALSA
		gtk_combo_box_text_append_text(dialog->input_system,
				audio_system_t_to_str(AUDIO_SYSTEM_ALSA));
#endif
#ifdef JACK
		gtk_combo_box_text_append_text(dialog->input_system,
				audio_system_t_to_str(AUDIO_SYSTEM_JACK));
#endif
#ifdef PULSEAUDIO
		gtk_combo_box_text_append_text(dialog->input_system,
				audio_system_t_to_str(AUDIO_SYSTEM_PULSEAUDIO));
#endif

		dialog->input_dev =
				GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "input_dev"));

		dialog->sample_rate =
				GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "sample_rate"));
		dialog->calculation_rate =
				GTK_HSCALE(gtk_builder_get_object(builder, "calculation_rate"));
		dialog->visualization_rate =
				GTK_HSCALE(gtk_builder_get_object(builder, "visualization_rate"));
		dialog->noise_threshold =
				GTK_HSCALE(gtk_builder_get_object(builder, "noise_threshold"));
		dialog->gain = GTK_HSCALE(gtk_builder_get_object(builder, "gain"));
		dialog->oversampling =
				GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "oversampling"));
		dialog->fft_size =
				GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "fft_size"));
		dialog->temporal_window =
				GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "temporal_window"));
		dialog->root_frequency_error =
				GTK_SPIN_BUTTON(gtk_builder_get_object(builder,
								"root_frequency_error"));
		dialog->dft_number =
				GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "dft_number"));
		dialog->dft_size =
				GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "dft_size"));
		dialog->peak_number =
				GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "peak_number"));
		dialog->peak_halfwidth =
				GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "peak_halfwidth"));
		dialog->rejection_peak_relation =
				GTK_HSCALE(gtk_builder_get_object(builder,
								"rejection_peak_relation"));
		dialog->label_sample_rate1 = GTK_LABEL(gtk_builder_get_object(builder,
						"label_sample_rate1"));
		dialog->label_sample_rate2 = GTK_LABEL(gtk_builder_get_object(builder,
						"label_sample_rate2"));
		dialog->minimum_frequency =
				GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder,
								"minimum_frequency"));
		dialog->notebook = GTK_NOTEBOOK(gtk_builder_get_object(builder,
						"notebook1"));
		dialog->maximum_frequency =
				GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder,
								"maximum_frequency"));
		GList* cell_list = gtk_cell_layout_get_cells(
				GTK_CELL_LAYOUT(dialog->minimum_frequency) );
		if (cell_list && cell_list->data) {
			gtk_cell_layout_set_attributes(
					GTK_CELL_LAYOUT(dialog->minimum_frequency), cell_list->data,
					"markup", 0, NULL );
		}
		cell_list = gtk_cell_layout_get_cells(
				GTK_CELL_LAYOUT(dialog->maximum_frequency) );
		if (cell_list && cell_list->data) {
			gtk_cell_layout_set_attributes(
					GTK_CELL_LAYOUT(dialog->maximum_frequency), cell_list->data,
					"markup", 0, NULL );
		}

		lingot_gui_config_dialog_scale_show(dialog, builder);

		g_signal_connect( dialog->input_system, "changed",
				G_CALLBACK (lingot_gui_config_dialog_callback_change_input_system),
				dialog);
		g_signal_connect( gtk_bin_get_child(GTK_BIN(dialog->sample_rate)),
				"changed",
				G_CALLBACK (lingot_gui_config_dialog_callback_change_sample_rate),
				dialog);

		g_signal_connect( dialog->oversampling, "value_changed",
				G_CALLBACK (lingot_gui_config_dialog_callback_change_sample_rate),
				dialog);
		g_signal_connect( dialog->root_frequency_error, "value_changed",
				G_CALLBACK (lingot_gui_config_dialog_scale_callback_change_deviation),
				dialog);

		g_signal_connect( dialog->minimum_frequency, "changed",
				G_CALLBACK (lingot_gui_config_dialog_change_min_frequency),
				dialog);
		g_signal_connect( dialog->maximum_frequency, "changed",
				G_CALLBACK (lingot_gui_config_dialog_change_max_frequency),
				dialog);

		g_signal_connect( gtk_builder_get_object(builder, "button_default"),
				"clicked",
				G_CALLBACK(lingot_gui_config_dialog_callback_button_default),
				dialog);
		g_signal_connect( gtk_builder_get_object(builder, "button_apply"),
				"clicked",
				G_CALLBACK(lingot_gui_config_dialog_callback_button_apply),
				dialog);
		g_signal_connect( gtk_builder_get_object(builder, "button_accept"),
				"clicked",
				G_CALLBACK(lingot_gui_config_dialog_callback_button_ok),
				dialog);
		g_signal_connect( gtk_builder_get_object(builder, "button_cancel"),
				"clicked",
				G_CALLBACK(lingot_gui_config_dialog_callback_button_cancel),
				dialog);
		g_signal_connect(dialog->win, "destroy",
				G_CALLBACK(lingot_gui_config_dialog_callback_close), dialog);

		lingot_gui_config_dialog_rewrite(dialog);

		gtk_widget_show(dialog->win);
		GtkWidget* scroll =
				GTK_WIDGET(gtk_builder_get_object(builder, "scrolledwindow1"));
		gtk_widget_show_all(scroll);
		g_object_unref(builder);

		lingot_gui_config_dialog_populate_frequency_combos(dialog);

	} else {
		if (config != NULL ) {
			lingot_config_copy(frame->config_dialog->conf, config);
			lingot_gui_config_dialog_rewrite(frame->config_dialog);
		}

		gtk_window_present(GTK_WINDOW(frame->config_dialog->win) );
	}

	if (config != NULL ) {
		lingot_config_destroy(config);
	}
}
