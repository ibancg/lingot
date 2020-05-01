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

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "lingot-defs-internal.h"

#include "lingot-audio.h"
#include "lingot-core.h"
#include "lingot-config.h"
#include "lingot-gui-mainframe.h"
#include "lingot-gui-config-dialog.h"
#include "lingot-gui-i18n.h"
#include "lingot-config.h"
#include "lingot-io-config.h"
#include "lingot-gui-config-dialog-scale.h"
#include "lingot-msg.h"


int  lingot_gui_config_dialog_apply(lingot_config_dialog_t*);
void lingot_gui_config_dialog_data_to_gui(lingot_config_dialog_t*, const lingot_config_t*);
void lingot_gui_config_dialog_combo_select_value(GtkWidget* combo, int value);
int  lingot_gui_config_dialog_get_audio_system(GtkComboBoxText* combo);
void lingot_gui_config_dialog_set_audio_device(GtkComboBoxText* combo, const char* device_name);

// some static settings
static const unsigned short frequency_combo_n_octaves = 6;
static const unsigned short frequency_combo_first_octave = 1;

/* button press event attention routine. */

void lingot_gui_config_dialog_callback_button_cancel(GtkButton *button,
                                                     lingot_config_dialog_t* dialog) {
    (void)button;           //  Unused parameter.
    gtk_widget_destroy(dialog->win);
}

void lingot_gui_config_dialog_callback_button_ok(GtkButton *button,
                                                 lingot_config_dialog_t* dialog) {
    (void)button;           //  Unused parameter.
    if (lingot_gui_config_dialog_apply(dialog)) {
        // dumps the current config to the config file
        lingot_io_config_save(&dialog->conf, LINGOT_CONFIG_FILE_NAME);
        // we already applied the config, we disable the rollback
        dialog->callback_config_changed = NULL;
        gtk_widget_destroy(dialog->win);
    }
}

void lingot_gui_config_dialog_callback_button_apply(GtkButton *button,
                                                    lingot_config_dialog_t* dialog) {
    (void)button;           //  Unused parameter.
    if (lingot_gui_config_dialog_apply(dialog)) {
        lingot_gui_config_dialog_data_to_gui(dialog, &dialog->conf);
    }
}

void lingot_gui_config_dialog_callback_button_default(GtkButton *button,
                                                      lingot_config_dialog_t* dialog) {
    (void)button;           //  Unused parameter.
    lingot_config_restore_default_values(&dialog->conf);
    lingot_gui_config_dialog_data_to_gui(dialog, &dialog->conf);
    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->octave), 4);
}

void lingot_gui_config_dialog_callback_close(GtkWidget *widget,
                                             lingot_config_dialog_t *dialog) {
    (void)widget;           //  Unused parameter.
    if (dialog->callback_config_changed) {
        dialog->callback_config_changed(&dialog->conf_old, dialog->callback_param);
    }
    lingot_config_destroy(&dialog->conf);
    lingot_config_destroy(&dialog->conf_old);
    if (dialog->callback_closed) {
        dialog->callback_closed(dialog->callback_param);
    }
    free(dialog);
}

void lingot_gui_config_dialog_callback_change_input_system(GtkWidget *widget,
                                                           lingot_config_dialog_t *dialog) {
    (void)widget;           //  Unused parameter.
    gchar* text = gtk_combo_box_text_get_active_text(dialog->input_system);
    int audio_system_index = lingot_audio_system_find_by_name(text);
    g_free(text);

    lingot_audio_system_properties_t properties;

    if (lingot_audio_system_get_properties(&properties, audio_system_index) == 0) {
        GtkListStore* input_dev_model = GTK_LIST_STORE(
                    gtk_combo_box_get_model(GTK_COMBO_BOX(dialog->input_dev)));
        gtk_list_store_clear(input_dev_model);

        if (properties.devices != NULL) {
            int i;
            for (i = 0; i < properties.n_devices; i++)
                if (properties.devices[i] != NULL) {
                    gtk_combo_box_text_append_text(dialog->input_dev,
                                                   properties.devices[i]);
                }
        }

        lingot_gui_config_dialog_set_audio_device(dialog->input_dev,
                                                  dialog->conf.audio_dev[audio_system_index]);

        lingot_audio_system_properties_destroy(&properties);
    } else {
        gtk_entry_set_text(
                    GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->input_dev))), "");
        gtk_widget_set_sensitive(GTK_WIDGET(dialog->input_dev), FALSE);
    }
}

void lingot_gui_config_dialog_set_audio_system(GtkComboBoxText* combo,
                                               int audio_system_index) {
    const char* token = lingot_audio_system_get_name(audio_system_index);
    GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    GtkTreeIter iter;

    gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

    while (valid) {
        gchar *str_data;
        gtk_tree_model_get(model, &iter,
                           0, &str_data,
                           -1);
        if (!strcmp(str_data, token)) {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), &iter);
        }
        g_free(str_data);
        valid = gtk_tree_model_iter_next(model, &iter);
    }
}

int lingot_gui_config_dialog_get_audio_system(GtkComboBoxText* combo) {
    gchar* text = gtk_combo_box_text_get_active_text(combo);
    int result = lingot_audio_system_find_by_name(text);
    g_free(text);
    return result;
}

// extracts the string between <> markers
static const gchar* lingot_gui_config_dialog_get_string(const gchar* str) {
    static char buffer[1024];

    const char* delim = "<>";
    char* str_ = _strdup(str);
    char* token = strtok(str_, delim);
    if ((token == str_) && (strlen(token) != strlen(str))) {
        token = strtok(NULL, delim);
    }

    strcpy(buffer, token ? token : str);
    free(str_);
    return buffer;
}

// extracts the audio device name from the text introduced in the entry (which
// includes description)
const gchar* lingot_gui_config_dialog_get_audio_device(const gchar* str) {
    return lingot_gui_config_dialog_get_string(str);
}

void lingot_gui_config_dialog_set_audio_device(GtkComboBoxText* combo,
                                               const char* device_name) {

    GtkTreeModel* model = GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(combo)));
    GtkTreeIter iter;
    char* filtered_name = _strdup(device_name);
    if (gtk_tree_model_get_iter_first(model, &iter)) {
        do {
            gchar* item;
            gtk_tree_model_get(model, &iter,
                               0, &item,
                               -1);
            // if the audio device ID is already in the popup list, we select the text in the list
            const gchar* filtered_item = lingot_gui_config_dialog_get_audio_device(item);
            if (!strcmp(device_name, filtered_item)) {
                free(filtered_name);
                filtered_name = _strdup(item); // we strdup because we must g_free(), not free()
                g_free(item);
                break;
            }
            g_free(item);
        } while (gtk_tree_model_iter_next(model, &iter));
    }

    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo))), filtered_name);
    free(filtered_name);
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

    //printf("frequency '%s' = %f\n", str, result);
    return result;
}

void lingot_gui_config_dialog_set_frequency(lingot_config_dialog_t* dialog,
                                            GtkComboBoxText* combo, double frequency) {

    if (frequency >= 0.0) {
        static char buffer[20];

        double error_cents;
        int closest_index = lingot_config_scale_get_closest_note_index(&dialog->conf.scale,
                                                                       frequency,
                                                                       dialog->conf.root_frequency_error, &error_cents);

        double freq2 = lingot_config_scale_get_frequency(&dialog->conf.scale,
                                                         closest_index); // TODO: deviation
        if (fabs(frequency - freq2) < 1e-1) {
            int index = lingot_config_scale_get_note_index(&dialog->conf.scale,
                                                           closest_index);
            int octave = lingot_config_scale_get_octave(&dialog->conf.scale,
                                                        closest_index) + 4;
            // TODO: note name
            snprintf(buffer, sizeof(buffer), "%s %d <%0.2f>",
                     dialog->conf.scale.note_name[index], octave, frequency);
        } else {
            snprintf(buffer, sizeof(buffer), "%0.2f", frequency);
        }

        //		printf("closest index to frequency %f is %d, freq %f\n", frequency,
        //				closest_index, freq2);
        //
        //		printf("frequency name for %f = '%s'\n", frequency, buffer);
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo))),
                           buffer);
    }
}

void lingot_gui_config_dialog_populate_frequency_combos(const lingot_config_dialog_t* dialog) {

    // TODO: only when necessary

    static char buff[512];

    // Unset the model first and perform the operation over a new Combo to avoid
    // bug 673079 in gnome: https://bugzilla.gnome.org/show_bug.cgi?id=673079
    // TODO: another way?
    gtk_combo_box_set_model(GTK_COMBO_BOX(dialog->minimum_frequency), NULL);
    gtk_combo_box_set_model(GTK_COMBO_BOX(dialog->maximum_frequency), NULL);

    GtkComboBoxText* combo = GTK_COMBO_BOX_TEXT(
                gtk_combo_box_text_new_with_entry());

    const lingot_config_t* const config = &dialog->conf;

    int i;
    int octave_index;
    for (i = 0; i < config->scale.notes; i++) {
        for (octave_index = frequency_combo_first_octave;
             octave_index
             < frequency_combo_first_octave
             + frequency_combo_n_octaves; octave_index++) {
            snprintf(buff, sizeof(buff),
                     "<span font_desc=\"%d\">%s</span><span font_desc=\"%d\">%i</span>",
                     12, config->scale.note_name[i], 7, octave_index);
            gtk_combo_box_text_append_text(combo, buff);
        }
    }

    gtk_combo_box_set_model(GTK_COMBO_BOX(dialog->minimum_frequency),
                            gtk_combo_box_get_model(GTK_COMBO_BOX(combo)));
    gtk_combo_box_set_model(GTK_COMBO_BOX(dialog->maximum_frequency),
                            gtk_combo_box_get_model(GTK_COMBO_BOX(combo)));
    gtk_widget_destroy(GTK_WIDGET(combo));
}

void lingot_gui_config_dialog_change_combo_frequency(GtkComboBoxText *combo,
                                                     lingot_config_dialog_t *dialog) {

    int index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    if (index >= 0) {
        // the base frequency of the scale corresponds to octave 4, typically C4
        int octave = frequency_combo_first_octave + (index % frequency_combo_n_octaves) - 4;
        int local_index = index / frequency_combo_n_octaves;
        int global_index = local_index + octave * dialog->conf.scale.notes;
        double frequency = lingot_config_scale_get_frequency(&dialog->conf.scale, global_index);
        lingot_gui_config_dialog_set_frequency(dialog, combo, frequency);
    }
}

void lingot_gui_config_dialog_change_min_frequency(GtkWidget *widget,
                                                   lingot_config_dialog_t *dialog) {
    lingot_gui_config_dialog_change_combo_frequency(GTK_COMBO_BOX_TEXT(widget),
                                                    dialog);
}

void lingot_gui_config_dialog_change_max_frequency(GtkWidget *widget,
                                                   lingot_config_dialog_t *dialog) {
    lingot_gui_config_dialog_change_combo_frequency(GTK_COMBO_BOX_TEXT(widget),
                                                    dialog);
}

// updates the enabled/disabled status of some widgets depending on the status of the check button
static void lingot_gui_config_dialog_optimize_check_toggled_update(lingot_config_dialog_t *dialog) {

    gboolean enabled = !gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(dialog->optimize_check_button));
    GtkWidget* widgets[] = { GTK_WIDGET(dialog->fft_size_label), GTK_WIDGET(
                             dialog->fft_size), GTK_WIDGET(dialog->fft_size_units_label),
                             GTK_WIDGET(dialog->temporal_window_label), GTK_WIDGET(
                             dialog->temporal_window), GTK_WIDGET(
                             dialog->temporal_window_units_label), 0x0 };
    unsigned int i;
    for (i = 0; widgets[i] != 0x0; i++) {
        gtk_widget_set_sensitive(widgets[i], enabled);
    }
}

void lingot_gui_config_dialog_optimize_check_toggled(GtkWidget *widget,
                                                     lingot_config_dialog_t *dialog) {
    (void)widget;           //  Unused parameter.
    lingot_gui_config_dialog_optimize_check_toggled_update(dialog);
}

void lingot_gui_config_dialog_combo_select_value(GtkWidget* combo, int value) {

    GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    GtkTreeIter iter;

    gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

    while (valid) {
        gchar *str_data;
        gtk_tree_model_get(model, &iter,
                           0, &str_data,
                           -1);
        if (atoi(str_data) == value) {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), &iter);
        }
        g_free(str_data);
        valid = gtk_tree_model_iter_next(model, &iter);
    }
}

void lingot_gui_config_dialog_data_to_gui(lingot_config_dialog_t* dialog, const lingot_config_t* conf) {

    lingot_gui_config_dialog_set_audio_system(dialog->input_system, conf->audio_system_index);
    lingot_gui_config_dialog_set_audio_device(dialog->input_dev, conf->audio_dev[conf->audio_system_index]);

    gtk_range_set_value(GTK_RANGE(dialog->calculation_rate), conf->calculation_rate);
    gtk_range_set_value(GTK_RANGE(dialog->visualization_rate), conf->visualization_rate);
    gtk_range_set_value(GTK_RANGE(dialog->noise_threshold), conf->min_overall_SNR);

    gtk_spin_button_set_value(dialog->root_frequency_error, conf->root_frequency_error);
    gtk_spin_button_set_value(dialog->temporal_window, conf->temporal_window);

    lingot_gui_config_dialog_combo_select_value(GTK_WIDGET(dialog->fft_size), (int) conf->fft_size);
    lingot_gui_config_dialog_set_frequency(dialog, dialog->minimum_frequency, conf->min_frequency);
    lingot_gui_config_dialog_set_frequency(dialog, dialog->maximum_frequency, conf->max_frequency);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->optimize_check_button),
                                 conf->optimize_internal_parameters);

    lingot_gui_config_dialog_scale_data_to_gui(dialog, &conf->scale);
}

int lingot_gui_config_dialog_gui_to_data(const lingot_config_dialog_t* dialog, lingot_config_t* conf) {

    gchar* text1;

    // validation

    if (!lingot_gui_config_dialog_scale_validate(dialog, &conf->scale)) {
        return 0;
    }

    const char* audio_device = lingot_gui_config_dialog_get_audio_device(
                gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->input_dev)))));

    lingot_config_parameter_spec_t audioDeviceSpec =
            lingot_io_config_get_parameter_spec(LINGOT_PARAMETER_ID_AUDIO_DEV);

    if (strlen(audio_device) >= audioDeviceSpec.str_max_len) {
        lingot_msg_add_error(_("Audio device identifier too long"));
        gtk_notebook_set_current_page(dialog->notebook, 0);
        return 0;
    }

    // TODO: value?
    int sample_rate = 44100;

    //	LingotConfigParameterSpec sampleRateSpec = lingot_config_get_parameter_spec(
    //			LINGOT_PARAMETER_ID_SAMPLE_RATE);
    //
    //	// TODO: generalize validation?
    //	if ((sample_rate < sampleRateSpec.int_min)
    //			|| (sample_rate > sampleRateSpec.int_max)) {
    //		char buff[1000];
    //		sprintf(buff, "%s %i - %i %s", _("Sample rate out of range"),
    //				sampleRateSpec.int_min, sampleRateSpec.int_max,
    //				sampleRateSpec.units);
    //		lingot_msg_add_error(buff);
    //		gtk_notebook_set_current_page(dialog->notebook, 0);
    //		return 0;
    //	}

    conf->audio_system_index = lingot_gui_config_dialog_get_audio_system(dialog->input_system);
    snprintf(conf->audio_dev[conf->audio_system_index],
            sizeof(conf->audio_dev[conf->audio_system_index]), "%s",
            lingot_gui_config_dialog_get_audio_device(
                gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->input_dev))))));
    conf->root_frequency_error = gtk_spin_button_get_value(dialog->root_frequency_error);
    conf->calculation_rate = gtk_range_get_value(GTK_RANGE(dialog->calculation_rate));
    conf->visualization_rate = gtk_range_get_value(GTK_RANGE(dialog->visualization_rate));
    conf->temporal_window = gtk_spin_button_get_value(dialog->temporal_window);
    conf->min_overall_SNR = gtk_range_get_value(GTK_RANGE(dialog->noise_threshold));
    conf->optimize_internal_parameters = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(dialog->optimize_check_button));
    double min_freq = lingot_gui_config_dialog_get_frequency(
                gtk_entry_get_text(
                    GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->minimum_frequency)))));
    double max_freq = lingot_gui_config_dialog_get_frequency(
                gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dialog->maximum_frequency)))));
    const lingot_config_parameter_spec_t minimumFrequencySpec =
            lingot_io_config_get_parameter_spec(LINGOT_PARAMETER_ID_MINIMUM_FREQUENCY);
    const lingot_config_parameter_spec_t maximumFrequencySpec =
            lingot_io_config_get_parameter_spec(LINGOT_PARAMETER_ID_MAXIMUM_FREQUENCY);

    if (min_freq <= max_freq) {
        if ((min_freq >= minimumFrequencySpec.float_min)
                && (min_freq <= minimumFrequencySpec.float_max)) {
            conf->min_frequency = min_freq;
        }
        if ((max_freq >= maximumFrequencySpec.float_min)
                && (max_freq <= maximumFrequencySpec.float_max)) {
            conf->max_frequency = max_freq;
        }
    }

    text1 = gtk_combo_box_text_get_active_text(dialog->fft_size);
    conf->fft_size = (unsigned int) atoi(text1);
    g_free(text1);
    conf->sample_rate = sample_rate;

    lingot_gui_config_dialog_scale_gui_to_data(dialog, &conf->scale);

    lingot_config_update_internal_params(conf);

    if (conf->gauge_range > 200) {
        lingot_msg_add_warning(
                    _("The provided scale contains wide gaps in frequency that increase the gauge range and produce a loss of visual accuracy. Consider providing scales with at least 12 tones, or with a maximum distance between adjacent notes below 200 cents."));
    }

    // TODO: outta here
    lingot_gui_config_dialog_populate_frequency_combos(dialog);

    return 1;
}

int lingot_gui_config_dialog_apply(lingot_config_dialog_t* dialog) {

    int result = lingot_gui_config_dialog_gui_to_data(dialog, &dialog->conf);

    if (result && dialog->callback_config_changed) {
        dialog->callback_config_changed(&dialog->conf, dialog->callback_param);
    }

    return result;
}

void lingot_gui_config_dialog_destroy(lingot_config_dialog_t* dialog) {
    gtk_widget_destroy(dialog->win);
}

lingot_config_dialog_t* lingot_gui_config_dialog_create(lingot_config_t* config,
                                                        lingot_config_t* old_config,
                                                        lingot_gui_config_dialog_callback_closed_t callback_closed,
                                                        lingot_gui_config_dialog_callback_config_changed_t callback_config_changed,
                                                        void* callback_param) {

    int i;

    lingot_config_dialog_t* dialog = malloc(sizeof(lingot_config_dialog_t));

    GtkBuilder* builder = gtk_builder_new();

    gtk_builder_add_from_resource(builder, "/org/nongnu/lingot/lingot-gui-config-dialog.glade", NULL);

    dialog->win = GTK_WIDGET(gtk_builder_get_object(builder, "dialog1"));

    //gtk_window_set_position(GTK_WINDOW(dialog->win), GTK_WIN_POS_CENTER_ALWAYS);

    dialog->input_system = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "input_system"));

    for (i = 0; i < lingot_audio_system_get_count(); ++i) {
        gtk_combo_box_text_append_text(dialog->input_system,
                                       lingot_audio_system_get_name(i));
    }

    dialog->input_dev = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "input_dev"));
    dialog->calculation_rate = GTK_SCALE(gtk_builder_get_object(builder, "calculation_rate"));
    dialog->visualization_rate = GTK_SCALE(gtk_builder_get_object(builder, "visualization_rate"));
    dialog->noise_threshold = GTK_SCALE(gtk_builder_get_object(builder, "noise_threshold"));
    dialog->fft_size = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "fft_size"));
    dialog->temporal_window = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "temporal_window"));
    dialog->root_frequency_error = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "root_frequency_error"));
    dialog->minimum_frequency = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "minimum_frequency"));
    dialog->maximum_frequency = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "maximum_frequency"));
    dialog->notebook = GTK_NOTEBOOK(gtk_builder_get_object(builder, "notebook1"));
    dialog->optimize_check_button = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "optimize_check_button"));
    dialog->fft_size_label = GTK_LABEL(gtk_builder_get_object(builder, "fft_size_label"));
    dialog->fft_size_units_label = GTK_LABEL(gtk_builder_get_object(builder, "fft_size_units_label"));
    dialog->temporal_window_label = GTK_LABEL(gtk_builder_get_object(builder, "temporal_window_label"));
    dialog->temporal_window_units_label = GTK_LABEL(gtk_builder_get_object(builder, "temporal_window_units_label"));
    dialog->octave = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "octave"));

    dialog->callback_closed = callback_closed;
    dialog->callback_config_changed = callback_config_changed;
    dialog->callback_param = callback_param;

    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->octave), 4);

    gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(dialog->minimum_frequency), frequency_combo_n_octaves);
    gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(dialog->maximum_frequency), frequency_combo_n_octaves);

    GList* cell_list = gtk_cell_layout_get_cells(
                GTK_CELL_LAYOUT(dialog->minimum_frequency));
    if (cell_list && cell_list->data) {
        gtk_cell_layout_set_attributes(
                    GTK_CELL_LAYOUT(dialog->minimum_frequency), cell_list->data,
                    "markup", 0, NULL);
    }
    cell_list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(dialog->maximum_frequency));
    if (cell_list && cell_list->data) {
        gtk_cell_layout_set_attributes(
                    GTK_CELL_LAYOUT(dialog->maximum_frequency), cell_list->data,
                    "markup", 0, NULL);
    }

    lingot_gui_config_dialog_scale_create(dialog, builder);

    g_signal_connect(dialog->input_system, "changed",
                     G_CALLBACK(lingot_gui_config_dialog_callback_change_input_system),
                     dialog);
    g_signal_connect(dialog->root_frequency_error, "value_changed",
                     G_CALLBACK(lingot_gui_config_dialog_scale_callback_change_deviation),
                     dialog);
    g_signal_connect(dialog->octave, "changed",
                     G_CALLBACK(lingot_gui_config_dialog_scale_callback_change_octave),
                     dialog);

    g_signal_connect(dialog->minimum_frequency, "changed",
                     G_CALLBACK(lingot_gui_config_dialog_change_min_frequency),
                     dialog);
    g_signal_connect(dialog->maximum_frequency, "changed",
                     G_CALLBACK(lingot_gui_config_dialog_change_max_frequency),
                     dialog);
    g_signal_connect(dialog->optimize_check_button, "toggled",
                     G_CALLBACK(lingot_gui_config_dialog_optimize_check_toggled),
                     dialog);

    g_signal_connect(gtk_builder_get_object(builder, "button_default"),
                     "clicked",
                     G_CALLBACK(lingot_gui_config_dialog_callback_button_default),
                     dialog);
    g_signal_connect(gtk_builder_get_object(builder, "button_apply"),
                     "clicked",
                     G_CALLBACK(lingot_gui_config_dialog_callback_button_apply),
                     dialog);
    g_signal_connect(gtk_builder_get_object(builder, "button_accept"),
                     "clicked",
                     G_CALLBACK(lingot_gui_config_dialog_callback_button_ok),
                     dialog);
    g_signal_connect(gtk_builder_get_object(builder, "button_cancel"),
                     "clicked",
                     G_CALLBACK(lingot_gui_config_dialog_callback_button_cancel),
                     dialog);
    g_signal_connect(dialog->win, "destroy",
                     G_CALLBACK(lingot_gui_config_dialog_callback_close), dialog);

    g_object_unref(builder);

    // config copy
    lingot_config_copy(&dialog->conf, config);
    lingot_config_copy(&dialog->conf_old, old_config ? old_config : config);
    lingot_gui_config_dialog_data_to_gui(dialog, &dialog->conf);
    lingot_gui_config_dialog_populate_frequency_combos(dialog);
    gtk_widget_show_all(dialog->win);

    return dialog;
}
