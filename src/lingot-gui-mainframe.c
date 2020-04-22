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
#include <errno.h>

#include "lingot-defs.h"

#include "lingot-config.h"
#include "lingot-gui-mainframe.h"
#include "lingot-gui-gauge.h"
#include "lingot-gui-spectrum.h"
#include "lingot-gui-config-dialog.h"
#include "lingot-i18n.h"
#include "lingot-io-config.h"
#include "lingot-msg.h"

void lingot_gui_mainframe_draw_labels(const lingot_main_frame_t*);

static gchar* filechooser_config_last_folder = NULL;

static const gdouble aspect_ratio_spectrum_visible = 1.14;
static const gdouble aspect_ratio_spectrum_invisible = 2.07;

void lingot_gui_mainframe_callback_destroy(GtkWidget* w, lingot_main_frame_t* frame) {
    (void)w;                //  Unused parameter.
    g_source_remove(frame->visualization_timer_uid);
    g_source_remove(frame->freq_computation_timer_uid);
    g_source_remove(frame->gauge_computation_uid);

    lingot_core_thread_stop(&frame->core);
    lingot_core_destroy(&frame->core);

    lingot_filter_destroy(&frame->gauge_filter);
    lingot_filter_destroy(&frame->freq_filter);
    lingot_config_destroy(&frame->conf);
    if (frame->config_dialog) {
        lingot_gui_config_dialog_destroy(frame->config_dialog);
    }

    free(frame);
    gtk_main_quit();
}

void lingot_gui_mainframe_callback_about(GtkWidget* w, lingot_main_frame_t* frame) {
    (void)w;                //  Unused parameter.
    (void)frame;            //  Unused parameter.
    static const gchar* authors[] = {
        "Iban Cereijo <ibancg@gmail.com>",
        "Jairo Chapela <jairochapela@gmail.com>",
        NULL };

    char buff[512];
    snprintf(buff, sizeof(buff), "Matthew Blissett (%s)", _("Logo design"));
    const gchar* artists[] = { buff, NULL };

    GError *error = NULL;
    GtkIconTheme *icon_theme = NULL;
    GdkPixbuf *pixbuf = NULL;

    // we use the property "logo" instead of "logo-icon-name", so we can specify
    // here at what size we want to scale the icon in this dialog
    icon_theme = gtk_icon_theme_get_default ();
    pixbuf = gtk_icon_theme_load_icon (icon_theme,
                                       "org.nongnu.lingot", // icon name
                                       80, // icon size
                                       0,  // flags
                                       &error);

    if (error) {
        g_warning("Couldn’t load icon: %s", error->message);
        g_error_free(error);
    }

    gtk_show_about_dialog(NULL,
                          "name", "Lingot",
                          "version", VERSION,
                          "copyright", "\xC2\xA9 2004-2020 Iban Cereijo\n\xC2\xA9 2004-2019 Jairo Chapela",
                          "comments", _("Accurate and easy to use musical instrument tuner"),
                          "authors", authors,
                          "artists", artists,
                          "website-label", "https://www.nongnu.org/lingot/",
                          "website", "https://www.nongnu.org/lingot/",
                          "license-type", GTK_LICENSE_GPL_2_0,
                          "translator-credits", _("translator-credits"),
                          //"logo-icon-name", "org.nongnu.lingot",
                          "logo", pixbuf,
                          NULL);

    if (pixbuf) {
        g_object_unref(pixbuf);
    }
}

void lingot_gui_mainframe_callback_view_spectrum(GtkWidget* w, lingot_main_frame_t* frame) {
    (void)w;                //  Unused parameter.
    gboolean visible = gtk_check_menu_item_get_active(
                GTK_CHECK_MENU_ITEM(frame->view_spectrum_item));

    GtkAllocation alloc;
    gtk_widget_get_allocation(frame->win, &alloc);
    GdkGeometry hints;
    gdouble aspect_ratio =
            visible ?
                aspect_ratio_spectrum_visible :
                aspect_ratio_spectrum_invisible;
    hints.min_aspect = aspect_ratio;
    hints.max_aspect = aspect_ratio;
    gtk_window_set_geometry_hints(GTK_WINDOW(frame->win), frame->win, &hints, GDK_HINT_ASPECT);
    gtk_widget_set_visible(frame->spectrum_frame, visible);
}

// callback from the config dialof when it is closed
static void lingot_gui_mainframe_callback_config_dialog_closed(lingot_main_frame_t *frame) {
    frame->config_dialog = NULL;
}

// callback from the config dialof when the config has been changed
static void lingot_gui_mainframe_callback_config_dialog_changed_config(lingot_config_t* conf,
                                                                       lingot_main_frame_t *frame) {
    lingot_core_thread_stop(&frame->core);
    lingot_core_destroy(&frame->core);

    // dup.
    lingot_config_copy(&frame->conf, conf);

    lingot_core_new(&frame->core, &frame->conf);
    lingot_core_thread_start(&frame->core);

    // some parameters may have changed
    lingot_config_copy(conf, &frame->conf);
}

static void lingot_gui_mainframe_open_config_dialog(lingot_main_frame_t* frame,
                                                    lingot_config_t* config) {

    if (!frame->config_dialog) {
        frame->config_dialog = lingot_gui_config_dialog_create(config,
                                                               &frame->conf,
                                                               (lingot_gui_config_dialog_callback_closed_t) lingot_gui_mainframe_callback_config_dialog_closed,
                                                               (lingot_gui_config_dialog_callback_config_changed_t) lingot_gui_mainframe_callback_config_dialog_changed_config,
                                                               frame);
        gtk_window_set_icon(GTK_WINDOW(frame->config_dialog->win),
                            gtk_window_get_icon(GTK_WINDOW(frame->win)));
    } else {
        gtk_window_present(GTK_WINDOW(frame->config_dialog->win));
    }
}

void lingot_gui_mainframe_callback_config_dialog(GtkWidget* w,
                                                 lingot_main_frame_t* frame) {
    (void)w;                //  Unused parameter.
    lingot_gui_mainframe_open_config_dialog(frame, &frame->conf);

}

/* timeout for gauge and labels visualization */
gboolean lingot_gui_mainframe_callback_tout_visualization(gpointer data) {

    lingot_main_frame_t* frame = (lingot_main_frame_t*) data;
    gtk_widget_queue_draw(frame->gauge_area);

    return 1;
}

/* timeout for spectrum computation and display */
gboolean lingot_gui_mainframe_callback_tout_spectrum_computation_display(gpointer data) {

    lingot_main_frame_t* frame = (lingot_main_frame_t*) data;

    gtk_widget_queue_draw(frame->spectrum_area);
    lingot_gui_mainframe_draw_labels(frame);

    return 1;
}

/* timeout for a new gauge position computation */
gboolean lingot_gui_mainframe_callback_gauge_computation(gpointer data) {
    lingot_main_frame_t* frame = (lingot_main_frame_t*) data;

    FLT freq = lingot_core_thread_get_result_frequency(&frame->core);

    // ignore continuous component
    if (!lingot_core_thread_is_running(&frame->core) || isnan(freq)
            || (freq <= frame->conf.internal_min_frequency)) {
        frame->frequency = 0.0;
        frame->gauge_pos = lingot_filter_filter_sample(&frame->gauge_filter, frame->conf.gauge_rest_value);
    } else {
        FLT error_cents; // do not use, unfiltered
        frame->frequency = lingot_filter_filter_sample(&frame->freq_filter,
                                                       freq);
        frame->closest_note_index = lingot_config_scale_get_closest_note_index(
                    &frame->conf.scale, freq, frame->conf.root_frequency_error, &error_cents);
        if (!isnan(error_cents)) {
            frame->gauge_pos = lingot_filter_filter_sample(&frame->gauge_filter, error_cents);
        }
    }

    return 1;
}

/* timeout for dispatching the error queue */
gboolean lingot_gui_mainframe_callback_error_dispatcher(gpointer data) {
    GtkWidget* message_dialog;
    lingot_main_frame_t* frame = (lingot_main_frame_t*) data;

    char error_message[LINGOT_MSG_MAX_SIZE + 1];
    lingot_msg_type_t message_type;
    int error_code;
    int more_messages;

    do {
        more_messages = lingot_msg_pop(error_message, &message_type, &error_code);

        if (more_messages) {
            GtkWindow* parent =
                    GTK_WINDOW((frame->config_dialog != NULL) ? frame->config_dialog->win : frame->win);
            GtkButtonsType buttonsType;

            char message[2 * LINGOT_MSG_MAX_SIZE];
            char* message_pointer = message;

            message_pointer += snprintf(message_pointer,
                                        (size_t) (message - message_pointer) + sizeof(message), "%s",
                                        error_message);

            if (error_code == EBUSY) {
                message_pointer +=
                        snprintf(message_pointer,
                                 (size_t) (message - message_pointer) + sizeof(message),
                                 "\n\n%s",
                                 _("Please check that there are not other processes locking the requested device. Also, consider that some audio servers can sometimes hold the resources for a few seconds since the last time they were used. In such a case, you can try again."));
            }

            if ((message_type == LINGOT_MSG_ERROR) && !lingot_core_thread_is_running(&frame->core)) {
                buttonsType = GTK_BUTTONS_OK;
                message_pointer +=
                        snprintf(message_pointer,
                                 (size_t) (message - message_pointer) + sizeof(message),
                                 "\n\n%s",
                                 _("The core is not running, you must check your configuration."));
            } else {
                buttonsType = GTK_BUTTONS_OK;
            }

            message_dialog = gtk_message_dialog_new(parent,
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    (message_type == LINGOT_MSG_ERROR) ? GTK_MESSAGE_ERROR :
                                                                                         ((message_type == LINGOT_MSG_WARNING) ? GTK_MESSAGE_WARNING : GTK_MESSAGE_INFO),
                                                    buttonsType, "%s", message);

            gtk_window_set_title(GTK_WINDOW(message_dialog),
                                 (message_type == LINGOT_MSG_ERROR) ? _("Error") :
                                                                      ((message_type == LINGOT_MSG_WARNING) ? _("Warning") : _("Info")));
            gtk_window_set_icon(GTK_WINDOW(message_dialog),
                                gtk_window_get_icon(GTK_WINDOW(frame->win)));
            gtk_dialog_run(GTK_DIALOG(message_dialog));
            gtk_widget_destroy(message_dialog);

            //			if ((message_type == ERROR) && !frame->core.running) {
            //				lingot_gui_mainframe_callback_config_dialog(NULL, frame);
            //			}

        }
    } while (more_messages);

    return 1;
}

void lingot_gui_mainframe_callback_open_config(gpointer data,
                                               lingot_main_frame_t* frame) {
    (void)data;             //  Unused parameter.
    GtkWidget * dialog = gtk_file_chooser_dialog_new(
                _("Open Configuration File"), GTK_WINDOW(frame->win),
                GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL,
                "_Open", GTK_RESPONSE_ACCEPT, NULL);
    char config_used = 0;
    lingot_config_t config;

    GtkFileFilter *filefilter = gtk_file_filter_new();
    gtk_file_filter_set_name(filefilter, (const gchar *) _("Lingot configuration files"));
    gtk_file_filter_add_pattern(filefilter, "*.conf");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filefilter); // ownership transferer to file_chooser
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);

    if (filechooser_config_last_folder != NULL) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            filechooser_config_last_folder);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        g_free(filechooser_config_last_folder);
        filechooser_config_last_folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
        gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        lingot_config_new(&config);
        lingot_io_config_load(&config, filename);
        config_used = 1;
        g_free(filename);
    }
    gtk_widget_destroy(dialog);

    if (config_used) {
        if (frame->config_dialog) {
            lingot_gui_config_dialog_destroy(frame->config_dialog);
            frame->config_dialog = NULL;
        }

        lingot_gui_mainframe_open_config_dialog(frame, &config);
        lingot_config_destroy(&config);
    }
}

void lingot_gui_mainframe_callback_save_config(gpointer data, lingot_main_frame_t* frame) {
    (void)data;             //  Unused parameter.
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
                _("Save Configuration File"), GTK_WINDOW(frame->win),
                GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel", GTK_RESPONSE_CANCEL,
                "_Save", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), _("untitled.conf"));

    GtkFileFilter *filefilter = gtk_file_filter_new();
    gtk_file_filter_set_name(filefilter, (const gchar *) _("Lingot configuration files"));
    gtk_file_filter_add_pattern(filefilter, "*.conf");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filefilter); // ownership transferer to file_chooser
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);

    if (filechooser_config_last_folder != NULL) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            filechooser_config_last_folder);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        g_free(filechooser_config_last_folder);
        filechooser_config_last_folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
        gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        lingot_io_config_save(&frame->conf, filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

lingot_main_frame_t* lingot_gui_mainframe_create() {

    lingot_main_frame_t* frame;

    if (filechooser_config_last_folder == NULL) {
        char buff[1000];
        snprintf(buff, sizeof(buff), "%s/%s", getenv("HOME"), CONFIG_DIR_NAME);
        filechooser_config_last_folder = _strdup(buff);
    }

    frame = malloc(sizeof(lingot_main_frame_t));

    frame->config_dialog = NULL;

    lingot_config_t* const conf = &frame->conf;
    lingot_config_new(conf);
    lingot_io_config_load(conf, CONFIG_FILE_NAME);

    //
    // ----- ERROR GAUGE FILTER CONFIGURATION -----
    //
    // dynamic model of the gauge:
    //
    //               2
    //              d                              d
    //              --- s(t) = k (e(t) - s(t)) - q -- s(t)
    //                2                            dt
    //              dt
    //
    // acceleration of gauge position 's(t)' linearly depends on the difference
    // respect to the input stimulus 'e(t)' (destination position). Inserting
    // a friction coefficient 'q', the acceleration proportionally diminishes with
    // the velocity (typical friction in mechanics). 'k' is the adaptation
    // constant, and depends on the gauge mass.
    //
    // with the following derivative approximations (valid for high sample rate):
    //
    //                 d
    //                 -- s(t) ~= (s[n] - s[n - 1])*fs
    //                 dt
    //
    //            2
    //           d                                            2
    //           --- s(t) ~= (s[n] - 2*s[n - 1] + s[n - 2])*fs
    //             2
    //           dt
    //
    // we can obtain a difference equation, and implement it with an IIR digital
    // filter.
    //

    const FLT gauge_filter_k = 60; // adaptation constant.
    const FLT gauge_filter_q = 6; // friction coefficient.
    const FLT gauge_filter_a[] = { gauge_filter_k + GAUGE_RATE * (gauge_filter_q + GAUGE_RATE), -GAUGE_RATE
                                   * (gauge_filter_q + 2.0 * GAUGE_RATE), GAUGE_RATE * GAUGE_RATE };
    const FLT gauge_filter_b[] = { gauge_filter_k };

    lingot_filter_new(&frame->gauge_filter, 2, 0, gauge_filter_a, gauge_filter_b);
    frame->gauge_pos = lingot_filter_filter_sample(&frame->gauge_filter, conf->gauge_rest_value);

    // ----- FREQUENCY FILTER CONFIGURATION ------

    // low pass IIR filter.
    FLT freq_filter_a[] = { 1.0, -0.5 };
    FLT freq_filter_b[] = { 0.5 };

    lingot_filter_new(&frame->freq_filter, 1, 0, freq_filter_a, freq_filter_b);

    // ---------------------------------------------------


    GtkBuilder* builder = gtk_builder_new();

    gtk_builder_add_from_resource(builder, "/org/nongnu/lingot/lingot-gui-mainframe.glade", NULL);

    frame->win = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));

    gtk_window_set_default_icon_name("org.nongnu.lingot");
    gtk_window_set_icon_name(GTK_WINDOW(frame->win), "org.nongnu.lingot");

    frame->gauge_area = GTK_WIDGET(gtk_builder_get_object(builder, "gauge_area"));
    frame->spectrum_area = GTK_WIDGET(gtk_builder_get_object(builder, "spectrum_area"));

    frame->freq_label = GTK_WIDGET(gtk_builder_get_object(builder, "freq_label"));
    frame->tone_label = GTK_WIDGET(gtk_builder_get_object(builder, "tone_label"));
    frame->error_label = GTK_WIDGET(gtk_builder_get_object(builder, "error_label"));

    frame->spectrum_frame = GTK_WIDGET(gtk_builder_get_object(builder, "spectrum_frame"));
    frame->view_spectrum_item = GTK_WIDGET(gtk_builder_get_object(builder, "spectrum_item"));
    frame->labelsbox = GTK_WIDGET(gtk_builder_get_object(builder, "labelsbox"));

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(frame->view_spectrum_item), TRUE);

    // show all
    gtk_widget_show_all(frame->win);

    GtkAllocation alloc;
    gtk_widget_get_allocation(frame->win, &alloc);
    GdkGeometry hints;
    gdouble aspect_ratio = aspect_ratio_spectrum_visible;
    hints.min_aspect = aspect_ratio;
    hints.max_aspect = aspect_ratio;
    gtk_window_set_geometry_hints(GTK_WINDOW(frame->win), frame->win, &hints,
                                  GDK_HINT_ASPECT);

    // GTK signals
    g_signal_connect(gtk_builder_get_object(builder, "preferences_item"),
                     "activate", G_CALLBACK(lingot_gui_mainframe_callback_config_dialog),
                     frame);
    g_signal_connect(gtk_builder_get_object(builder, "quit_item"), "activate",
                     G_CALLBACK(lingot_gui_mainframe_callback_destroy), frame);
    g_signal_connect(gtk_builder_get_object(builder, "about_item"), "activate",
                     G_CALLBACK(lingot_gui_mainframe_callback_about), frame);
    g_signal_connect(gtk_builder_get_object(builder, "spectrum_item"),
                     "activate", G_CALLBACK(lingot_gui_mainframe_callback_view_spectrum),
                     frame);
    g_signal_connect(gtk_builder_get_object(builder, "open_config_item"),
                     "activate", G_CALLBACK(lingot_gui_mainframe_callback_open_config),
                     frame);
    g_signal_connect(gtk_builder_get_object(builder, "save_config_item"),
                     "activate", G_CALLBACK(lingot_gui_mainframe_callback_save_config),
                     frame);

    g_signal_connect(frame->gauge_area, "draw",
                     G_CALLBACK(lingot_gui_gauge_redraw), frame);
    g_signal_connect(frame->spectrum_area, "draw",
                     G_CALLBACK(lingot_gui_spectrum_redraw), frame);
    g_signal_connect(frame->win, "destroy",
                     G_CALLBACK(lingot_gui_mainframe_callback_destroy), frame);

    GtkAccelGroup* accel_group = gtk_accel_group_new();
    gtk_widget_add_accelerator(GTK_WIDGET(gtk_builder_get_object(builder, "preferences_item")),
                               "activate", accel_group, 'p', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_window_add_accel_group(GTK_WINDOW(frame->win), accel_group);

    unsigned int period;
    period = (unsigned int) (1000 / conf->visualization_rate);
    frame->visualization_timer_uid = g_timeout_add(period,
                                                   lingot_gui_mainframe_callback_tout_visualization, frame);

    period = (unsigned int) (1000 / conf->calculation_rate);
    frame->freq_computation_timer_uid = g_timeout_add(period,
                                                      lingot_gui_mainframe_callback_tout_spectrum_computation_display,
                                                      frame);

    period = (unsigned int) (1000 / GAUGE_RATE);
    frame->gauge_computation_uid = g_timeout_add(period,
                                                 lingot_gui_mainframe_callback_gauge_computation, frame);

    period = (unsigned int) (1000 / ERROR_DISPATCH_RATE);
    frame->error_dispatcher_uid = g_timeout_add(period,
                                                lingot_gui_mainframe_callback_error_dispatcher, frame);

    frame->closest_note_index = 0;
    frame->frequency = 0;

    lingot_core_new(&frame->core, conf);
    lingot_core_thread_start(&frame->core);

    g_object_unref(builder);

    return frame;
}

// ---------------------------------------------------------------------------


void lingot_gui_mainframe_draw_labels(const lingot_main_frame_t* frame) {

    char* note_string;
    static char error_string[30];
    static char freq_string[30];
    static char octave_string[30];

    GtkAllocation req;
    gtk_widget_get_allocation(frame->labelsbox, &req);

    // draw note, error and frequency labels

    if (frame->frequency == 0.0) {
        note_string = "---";
        strcpy(error_string, "e = ---");
        strcpy(freq_string, "f = ---");
        strcpy(octave_string, "");
    } else {
        note_string =
                frame->conf.scale.note_name[lingot_config_scale_get_note_index(
                    &frame->conf.scale, frame->closest_note_index)];
        sprintf(error_string, "e = %+2.0f cents", frame->gauge_pos);
        sprintf(freq_string, "f = %6.2f Hz", frame->frequency);
        sprintf(octave_string, "%d",
                lingot_config_scale_get_octave(&frame->conf.scale,
                                               frame->closest_note_index) + 4);
    }

    int font_size = 9 + req.width / 80;
    char* markup = g_markup_printf_escaped("<span font_desc=\"%d\">%s</span>",
                                           font_size, freq_string);
    gtk_label_set_markup(GTK_LABEL(frame->freq_label), markup);
    g_free(markup);
    markup = g_markup_printf_escaped("<span font_desc=\"%d\">%s</span>",
                                     font_size, error_string);
    gtk_label_set_markup(GTK_LABEL(frame->error_label), markup);
    g_free(markup);

    font_size = 10 + req.width / 22;
    markup =
            g_markup_printf_escaped(
                "<span font_desc=\"%d\" weight=\"bold\">%s</span><span font_desc=\"%d\" weight=\"bold\"><sub>%s</sub></span>",
                font_size, note_string, (int) (0.75 * font_size),
                octave_string);
    gtk_label_set_markup(GTK_LABEL(frame->tone_label), markup);
    g_free(markup);
}
