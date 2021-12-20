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
 * along with lingot; if not, write to the Free Software Foundation,
 * Inc. 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "lingot-defs-internal.h"

#include "lingot-config.h"
#include "lingot-gui-mainframe.h"
#include "lingot-gui-gauge.h"
#include "lingot-gui-strobe-disc.h"
#include "lingot-gui-spectrum.h"
#include "lingot-gui-config-dialog.h"
#include "lingot-gui-i18n.h"
#include "lingot-io-config.h"
#include "lingot-io-ui-settings.h"
#include "lingot-msg.h"

void lingot_gui_mainframe_draw_labels(const lingot_main_frame_t*);

static gchar* filechooser_config_last_folder = NULL;

void lingot_gui_mainframe_callback_show(GtkWidget* w,
                                        GtkAllocation *allocation,
                                        const lingot_main_frame_t* frame) {

    (void)allocation;   //  Unused parameter.
    (void)w;            //  Unused parameter.

    if (ui_settings.horizontal_paned_pos > 0) {
        gtk_paned_set_position(frame->horizontal_paned, ui_settings.horizontal_paned_pos);
        ui_settings.horizontal_paned_pos = -1;
    }

    if (ui_settings.vertical_paned_pos > 0) {
        gtk_paned_set_position(frame->vertical_paned, ui_settings.vertical_paned_pos);
        ui_settings.vertical_paned_pos = -1;
    }
}

void lingot_gui_mainframe_callback_hide(GtkWidget* w, const lingot_main_frame_t* frame) {
    (void)w;                //  Unused parameter.

    gint win_width;
    gint win_height;
    gint win_pos_x;
    gint win_pos_y;
    gtk_window_get_size(frame->win, &win_width, &win_height);
    gtk_window_get_position(frame->win, &win_pos_x, &win_pos_y);
    ui_settings.win_pos_x = win_pos_x;
    ui_settings.win_pos_y = win_pos_y;
    ui_settings.win_width = win_width;
    ui_settings.win_height = win_height;
    ui_settings.spectrum_visible = gtk_check_menu_item_get_active(frame->view_spectrum_item);
    ui_settings.gauge_visible = gtk_check_menu_item_get_active(frame->view_gauge_item);

    ui_settings.horizontal_paned_pos = gtk_paned_get_position(frame->horizontal_paned);
    ui_settings.vertical_paned_pos = gtk_paned_get_position(frame->vertical_paned);
}

void lingot_gui_mainframe_callback_destroy(GtkWidget* w, lingot_main_frame_t* frame) {
    (void)w;                //  Unused parameter.

    g_source_remove(frame->visualization_timer_uid);
    g_source_remove(frame->error_dispatch_timer_uid);
    g_source_remove(frame->gauge_sampling_timer_uid);

    lingot_core_thread_stop(&frame->core);
    lingot_core_destroy(&frame->core);

    lingot_filter_destroy(&frame->gauge_filter);
    lingot_filter_destroy(&frame->freq_filter);
    lingot_config_destroy(&frame->conf);
    if (frame->config_dialog) {
        lingot_gui_config_dialog_destroy(frame->config_dialog);
    }

    lingot_io_ui_settings_save();
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
    gboolean visible = gtk_check_menu_item_get_active(frame->view_spectrum_item);
    gtk_widget_set_visible(frame->spectrum_frame, visible);
}

void lingot_gui_mainframe_update_gauge_area_tooltip(lingot_main_frame_t* frame) {
    gtk_widget_set_tooltip_text(frame->gauge_area,
                                gtk_check_menu_item_get_active(frame->view_gauge_item) ?
                                    _("Shows the error in cents in a visual way. The range will depend on the maximum distance between each two notes in the scale defined in the Lingot settings. Try to provide scales with low maximum distance, i.e. with enough notes, to have a higher resolution in this gauge (12 notes per scale is a safe option).") :
                                    _("Shows the error as a rotating disc which speed depends on the error in cents to the desired note. The disc will be still then the played note is in tune."));
}

void lingot_gui_mainframe_callback_view_gauge(GtkWidget* w, lingot_main_frame_t* frame) {
    (void)w;                //  Unused parameter.
    gboolean on = gtk_check_menu_item_get_active(frame->view_gauge_item);
    if (on) {
      gtk_widget_set_visible(frame->gauge_frame, 1);
      gtk_check_menu_item_set_active(frame->view_strobe_disc_item, 0);
      gtk_check_menu_item_set_active(frame->view_none_item, 0);
    }
    lingot_gui_mainframe_update_gauge_area_tooltip(frame);
}


void lingot_gui_mainframe_callback_view_none(GtkWidget* w, lingot_main_frame_t* frame) {
    (void)w;                //  Unused parameter.
    gboolean on = gtk_check_menu_item_get_active(frame->view_none_item);
    if (on) {
      gtk_check_menu_item_set_active(frame->view_strobe_disc_item, 0);
      gtk_check_menu_item_set_active(frame->view_gauge_item, 0);
      gtk_widget_set_visible(frame->gauge_frame, 0);

    }
    lingot_gui_mainframe_update_gauge_area_tooltip(frame);
}

void lingot_gui_mainframe_callback_view_strobe_disc(GtkWidget* w, lingot_main_frame_t* frame) {
    (void)w;                //  Unused parameter.
    gboolean on = gtk_check_menu_item_get_active(frame->view_strobe_disc_item);
    if (on) {
      gtk_widget_set_visible(frame->gauge_frame, 1);
      gtk_check_menu_item_set_active(frame->view_none_item, 0);
      gtk_check_menu_item_set_active(frame->view_gauge_item, 0);
    }
    lingot_gui_mainframe_update_gauge_area_tooltip(frame);
}

// callback from the config dialog when it is closed
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
                            gtk_window_get_icon(frame->win));
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
    gtk_widget_queue_draw(frame->spectrum_area);
    lingot_gui_mainframe_draw_labels(frame);

    return 1; // repeat event
}

/* timeout for a new gauge position computation */
gboolean lingot_gui_mainframe_callback_gauge_sampling(gpointer data) {
    lingot_main_frame_t* frame = (lingot_main_frame_t*) data;

    LINGOT_FLT freq = lingot_core_thread_get_result_frequency(&frame->core);

    // ignore continuous component
    if (!lingot_core_thread_is_running(&frame->core) || isnan(freq)
            || (freq <= frame->conf.internal_min_frequency)) {
        frame->frequency = 0.0;
        frame->gauge_pos = lingot_filter_filter_sample(&frame->gauge_filter, frame->conf.gauge_rest_value);
        // lingot_gauge_compute(&frame->gauge, 0.0); // strobe still
    } else {
        LINGOT_FLT error_cents; // do not use, unfiltered
        frame->frequency = lingot_filter_filter_sample(&frame->freq_filter,
                                                       freq);
        frame->closest_note_index = lingot_config_scale_get_closest_note_index(
                    &frame->conf.scale, freq, frame->conf.root_frequency_error, &error_cents);
        if (!isnan(error_cents)) {
            frame->gauge_pos = lingot_filter_filter_sample(&frame->gauge_filter, error_cents);
        }
    }

    lingot_gui_strobe_disc_set_error(frame->gauge_pos);
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
                    (frame->config_dialog != NULL) ? GTK_WINDOW(frame->config_dialog->win) : frame->win;
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
                                gtk_window_get_icon(frame->win));
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
                _("Open Configuration File"), frame->win,
                GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL,
                "_Open", GTK_RESPONSE_ACCEPT, NULL);
    int config_used = 0;
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
        config_used = lingot_io_config_load(&config, filename);
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
                _("Save Configuration File"), frame->win,
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

gboolean lingot_gui_gauge_strobe_disc_redraw(GtkWidget *w, cairo_t *cr, lingot_main_frame_t* frame) {
//    lingot_gui_mainframe_callback_show(w, frame);
    if (gtk_check_menu_item_get_active(frame->view_gauge_item)) {
        lingot_gui_gauge_redraw(w, cr, frame);
    } else if (gtk_check_menu_item_get_active(frame->view_strobe_disc_item)){
        lingot_gui_strobe_disc_redraw(w, cr, frame);
    }

    return FALSE;
}

lingot_main_frame_t* lingot_gui_mainframe_create() {

    lingot_main_frame_t* frame;

    if (filechooser_config_last_folder == NULL) {
        char buff[1000];
        snprintf(buff, sizeof(buff), "%s/%s", getenv("HOME"), LINGOT_CONFIG_DIR_NAME);
        filechooser_config_last_folder = _strdup(buff);
    }

    frame = malloc(sizeof(lingot_main_frame_t));

    frame->config_dialog = NULL;

    lingot_config_t* const conf = &frame->conf;
    lingot_config_new(conf);
    lingot_io_config_load(conf, LINGOT_CONFIG_FILE_NAME);

    lingot_io_ui_settings_init();

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
    // respect to the input stimulus 'e(t)' (target position). Inserting a
    // friction coefficient 'q', the acceleration proportionally diminishes with
    // the velocity (typical friction in mechanics). 'k' is the adaptation
    // constant, and depends on the gauge mass.
    //
    // with the following derivative approximations, valid for high sample rate:
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

    double gauge_rate = ui_settings.gauge_sampling_rate;

    // Adaptation constant. The bigger this value is, the quicker the gauge moves to its target.
    const LINGOT_FLT gauge_filter_k = ui_settings.gauge_adaptation_constant;

    // Friction coefficient. The bigger the value, the less the "bouncing" effect on the gauge.
    const LINGOT_FLT gauge_filter_q = ui_settings.gauge_damping_constant;

    const LINGOT_FLT gauge_filter_a[] = { gauge_filter_k + gauge_rate * (gauge_filter_q + gauge_rate),
                                          -gauge_rate * (gauge_filter_q + 2.0 * gauge_rate),
                                          gauge_rate * gauge_rate };
    const LINGOT_FLT gauge_filter_b[] = { gauge_filter_k };

    lingot_filter_new(&frame->gauge_filter, 2, 0, gauge_filter_a, gauge_filter_b);
    frame->gauge_pos = lingot_filter_filter_sample(&frame->gauge_filter, conf->gauge_rest_value);

    // ----- FREQUENCY FILTER CONFIGURATION ------

    // low pass IIR filter.
    LINGOT_FLT freq_filter_a[] = { 1.0, -0.5 };
    LINGOT_FLT freq_filter_b[] = { 0.5 };

    lingot_filter_new(&frame->freq_filter, 1, 0, freq_filter_a, freq_filter_b);

    // ---------------------------------------------------


    GtkBuilder* builder = gtk_builder_new();

    gtk_builder_add_from_resource(builder, "/org/nongnu/lingot/lingot-gui-mainframe.glade", NULL);

    frame->win = GTK_WINDOW(gtk_builder_get_object(builder, "window1"));

    gtk_window_set_default_icon_name("org.nongnu.lingot");
    gtk_window_set_icon_name(frame->win, "org.nongnu.lingot");

    lingot_gui_strobe_disc_init(gauge_rate);

    frame->gauge_area = GTK_WIDGET(gtk_builder_get_object(builder, "gauge_area"));
    frame->spectrum_area = GTK_WIDGET(gtk_builder_get_object(builder, "spectrum_area"));

    frame->freq_label = GTK_LABEL(gtk_builder_get_object(builder, "freq_label"));
    frame->tone_label = GTK_LABEL(gtk_builder_get_object(builder, "tone_label"));
    frame->error_label = GTK_LABEL(gtk_builder_get_object(builder, "error_label"));

    frame->spectrum_frame = GTK_WIDGET(gtk_builder_get_object(builder, "spectrum_frame"));
    frame->gauge_frame = GTK_WIDGET(gtk_builder_get_object(builder, "gauge_frame"));

    frame->view_spectrum_item = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "spectrum_item"));
    frame->view_gauge_item = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "gauge_item"));
    frame->view_none_item = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "none_item"));
    frame->view_strobe_disc_item = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "strobe_disc_item"));
    frame->labelsbox = GTK_WIDGET(gtk_builder_get_object(builder, "labelsbox"));
    frame->horizontal_paned = GTK_PANED(gtk_builder_get_object(builder, "horizontal_paned"));
    frame->vertical_paned = GTK_PANED(gtk_builder_get_object(builder, "vertical_paned"));

    // GTK signals
    g_signal_connect(gtk_builder_get_object(builder, "preferences_item"),
                     "activate", G_CALLBACK(lingot_gui_mainframe_callback_config_dialog),
                     frame);
    g_signal_connect(gtk_builder_get_object(builder, "quit_item"), "activate",
                     G_CALLBACK(lingot_gui_mainframe_callback_destroy), frame);
    g_signal_connect(gtk_builder_get_object(builder, "about_item"), "activate",
                     G_CALLBACK(lingot_gui_mainframe_callback_about), frame);
    g_signal_connect(frame->view_spectrum_item,
                     "activate", G_CALLBACK(lingot_gui_mainframe_callback_view_spectrum),
                     frame);
    g_signal_connect(frame->view_gauge_item,
                     "activate", G_CALLBACK(lingot_gui_mainframe_callback_view_gauge),
                     frame);
    g_signal_connect(frame->view_strobe_disc_item,
                     "activate", G_CALLBACK(lingot_gui_mainframe_callback_view_strobe_disc),
                     frame);
    g_signal_connect(frame->view_none_item,
                    "activate", G_CALLBACK(lingot_gui_mainframe_callback_view_none),
                    frame);
    g_signal_connect(gtk_builder_get_object(builder, "open_config_item"),
                     "activate", G_CALLBACK(lingot_gui_mainframe_callback_open_config),
                     frame);
    g_signal_connect(gtk_builder_get_object(builder, "save_config_item"),
                     "activate", G_CALLBACK(lingot_gui_mainframe_callback_save_config),
                     frame);

    g_signal_connect(frame->gauge_area, "draw",
                     G_CALLBACK(lingot_gui_gauge_strobe_disc_redraw), frame);
    g_signal_connect(frame->spectrum_area, "draw",
                     G_CALLBACK(lingot_gui_spectrum_redraw), frame);
    g_signal_connect(frame->win, "hide",
                     G_CALLBACK(lingot_gui_mainframe_callback_hide), frame);
    g_signal_connect(frame->win, "size-allocate", // TODO: "show" does not work
                     G_CALLBACK(lingot_gui_mainframe_callback_show), frame);
    g_signal_connect(frame->win, "destroy",
                     G_CALLBACK(lingot_gui_mainframe_callback_destroy), frame);

    GtkAccelGroup* accel_group = gtk_accel_group_new();
    gtk_widget_add_accelerator(GTK_WIDGET(gtk_builder_get_object(builder, "preferences_item")),
                               "activate", accel_group, 'p', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_window_add_accel_group(frame->win, accel_group);

    gtk_check_menu_item_set_active(frame->view_spectrum_item, ui_settings.spectrum_visible);
    gtk_check_menu_item_set_active(frame->view_gauge_item, ui_settings.gauge_visible);
    gtk_check_menu_item_set_active(frame->view_none_item, ui_settings.none_visible);


    if (ui_settings.win_width > 0) {
        gtk_window_resize(frame->win, ui_settings.win_width, ui_settings.win_height);
    }
    //if (ui_settings.win_pos_x > 0) {
    //    gtk_window_move(frame->win,
    //                    ui_settings.win_pos_x,
    //                    ui_settings.win_pos_y);
    //}

    // show all
    gtk_widget_show(GTK_WIDGET(frame->win));


    unsigned int period;

    period = (unsigned int) (1000 / ui_settings.error_dispatch_rate);
    frame->error_dispatch_timer_uid = g_timeout_add(period,
                                                lingot_gui_mainframe_callback_error_dispatcher, frame);

    period = (unsigned int) (1000 / ui_settings.gauge_sampling_rate);
    frame->gauge_sampling_timer_uid = g_timeout_add(period,
                                                 lingot_gui_mainframe_callback_gauge_sampling, frame);

    // TODO: move visualization rate to UI settings.
    period = (unsigned int) (1000 / ui_settings.visualization_rate);
    frame->visualization_timer_uid = g_timeout_add(period,
                                                   lingot_gui_mainframe_callback_tout_visualization, frame);

    frame->closest_note_index = 0;
    frame->frequency = 0;

    lingot_core_new(&frame->core, conf);
    lingot_core_thread_start(&frame->core);

    g_object_unref(builder);

    // welcome message showig changes in this version
    if (!ui_settings.app_version) {
        char buff[1000];
        snprintf(buff, sizeof(buff),
                 _("Welcome to Lingot %s. \n\nWe have added a new experimental strobe disc display, among other improvements, which you can find under the 'View' menu.\n\nEnjoy!"), VERSION);
        lingot_msg_add_info(buff);
    }

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
    gtk_label_set_markup(frame->freq_label, markup);
    g_free(markup);
    markup = g_markup_printf_escaped("<span font_desc=\"%d\">%s</span>",
                                     font_size, error_string);
    gtk_label_set_markup(frame->error_label, markup);
    g_free(markup);

    font_size = 10 + req.width / 22;
    markup =
            g_markup_printf_escaped(
                "<span font_desc=\"%d\" weight=\"bold\">%s</span><span font_desc=\"%d\" weight=\"bold\"><sub>%s</sub></span>",
                font_size, note_string, (int) (0.75 * font_size),
                octave_string);
    gtk_label_set_markup(frame->tone_label, markup);
    g_free(markup);
}
