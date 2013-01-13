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

#include "lingot-config.h"
#include "lingot-gui-mainframe.h"
#include "lingot-gui-config-dialog.h"
#include "lingot-gauge.h"
#include "lingot-i18n.h"

#include "lingot-msg.h"

#include "lingot-logo.xpm"

void lingot_gui_mainframe_draw_gauge(const LingotMainFrame*);
void lingot_gui_mainframe_draw_spectrum(const LingotMainFrame*);
void lingot_gui_mainframe_draw_labels(const LingotMainFrame*);

// sizes

static const int gauge_size_x = 160;
static const int gauge_size_y = 100;

static const int spectrum_size_y = 64;

// spectrum area margins
static const int spectrum_bottom_margin = 16;
static const int spectrum_top_margin = 12;
static const int spectrum_x_margin = 15;

static gchar* filechooser_config_last_folder = NULL;

// TODO: keep here?
static int closest_note_index = 0;
static FLT frequency = 0.0;

void lingot_gui_mainframe_callback_redraw_gauge(GtkWidget* w, GdkEventExpose* e,
		LingotMainFrame* frame) {
	lingot_gui_mainframe_draw_gauge(frame);
}

void lingot_gui_mainframe_callback_redraw_spectrum(GtkWidget* w,
		GdkEventExpose* e, LingotMainFrame* frame) {
	lingot_gui_mainframe_draw_spectrum(frame);
}

void lingot_gui_mainframe_callback_destroy(GtkWidget* w, LingotMainFrame* frame) {
	g_source_remove(frame->visualization_timer_uid);
	g_source_remove(frame->freq_computation_timer_uid);
	g_source_remove(frame->gauge_computation_uid);
	gtk_main_quit();
}

void lingot_gui_mainframe_callback_about(GtkWidget* w, LingotMainFrame* frame) {
	static const gchar* authors[] = { "Ibán Cereijo Graña <ibancg@gmail.com>",
			"Jairo Chapela Martínez <jairochapela@gmail.com>", NULL };

	char buff[100];
	sprintf(buff, "Matthew Blissett (%s)", _("Logo design"));
	const gchar* artists[] = { buff, NULL };

	gtk_show_about_dialog(NULL, "name", "Lingot", "version", VERSION,
			"copyright",
			"\xC2\xA9 2004-2013 Ibán Cereijo Graña\n\xC2\xA9 2004-2013 Jairo Chapela Martínez",
			"comments", _("Accurate and easy to use musical instrument tuner"),
			"authors", authors, "artists", artists, "website-label",
			"http://lingot.nongnu.org/", "website", "http://lingot.nongnu.org/",
			"translator-credits", _("translator-credits"), "logo-icon-name",
			"lingot-logo", NULL );
}

void lingot_gui_mainframe_callback_view_spectrum(GtkWidget* w,
		LingotMainFrame* frame) {
	if (gtk_check_menu_item_get_active(
			GTK_CHECK_MENU_ITEM(frame->view_spectrum_item) )) {
		gtk_widget_show(frame->spectrum_frame);
	} else {
		gtk_widget_hide(frame->spectrum_frame);
	}
}

void lingot_gui_mainframe_callback_config_dialog(GtkWidget* w,
		LingotMainFrame* frame) {
	lingot_gui_config_dialog_show(frame, NULL );
}

static short int lingot_gui_mainframe_get_closest_note_index(FLT freq,
		LingotScale* scale, FLT deviation, FLT* error_cents) {
	short note_index = 0;
	short int index;

	FLT offset = 1200.0 * log2(freq / scale->base_frequency) - deviation;
	int octave = ((int) (offset)) / 1200;
	if (offset < 0) {
		octave--;
	}
	offset = fmod(offset, 1200.0);
	if (offset < 0.0) {
		offset += 1200.0;
	}

	index = floor(scale->notes * offset / 1200.0);

	FLT pitch_inf;
	FLT pitch_sup;
	int n = 0;
	for (;;) {
		n++;
		pitch_inf = scale->offset_cents[index];
		pitch_sup =
				((index + 1) < scale->notes) ?
						scale->offset_cents[index + 1] : 1200.0;

		if (offset > pitch_sup) {
			index++;
			continue;
		}

		if (offset < pitch_inf) {
			index--;
			continue;
		}

		break;
	};

	if (fabs(offset - pitch_inf) < fabs(offset - pitch_sup)) {
		note_index = index;
		*error_cents = offset - pitch_inf;
	} else {
		note_index = index + 1;
		*error_cents = offset - pitch_sup;
	}

	if (note_index == scale->notes) {
		note_index = 0;
		octave++;
	}

	note_index += octave * scale->notes;

	return note_index;
}

/* timeout for gauge and labels visualization */
gboolean lingot_gui_mainframe_callback_tout_visualization(gpointer data) {
	unsigned int period;

	LingotMainFrame* frame = (LingotMainFrame*) data;

	period = 1000 / frame->conf->visualization_rate;
	frame->visualization_timer_uid = g_timeout_add(period,
			lingot_gui_mainframe_callback_tout_visualization, frame);

	gtk_widget_queue_draw(frame->gauge_area);

	return 0;
}

/* timeout for spectrum computation and display */
gboolean lingot_gui_mainframe_callback_tout_spectrum_computation_display(
		gpointer data) {
	unsigned int period;

	LingotMainFrame* frame = (LingotMainFrame*) data;

	period = 1000 / frame->conf->calculation_rate;
	frame->freq_computation_timer_uid = g_timeout_add(period,
			lingot_gui_mainframe_callback_tout_spectrum_computation_display,
			frame);

	gtk_widget_queue_draw(frame->spectrum_area);
	lingot_gui_mainframe_draw_labels(frame);
//	lingot_gui_mainframe_draw_spectrum_and_labels(frame);
	//lingot_core_compute_fundamental_fequency(frame->core);
	return 0;
}

/* timeout for a new gauge position computation */
gboolean lingot_gui_mainframe_callback_gauge_computation(gpointer data) {
	unsigned int period;
	double error_cents;
	LingotMainFrame* frame = (LingotMainFrame*) data;

	period = 1000 / GAUGE_RATE;
	frame->gauge_computation_uid = g_timeout_add(period,
			lingot_gui_mainframe_callback_gauge_computation, frame);

	// ignore continuous component
	if (!frame->core->running || isnan(frame->core->freq)
			|| (frame->core->freq < frame->conf->min_frequency)) {
		frequency = 0.0;
		lingot_gauge_compute(frame->gauge, frame->conf->gauge_rest_value);
	} else {
		FLT error_cents; // do not use, unfiltered
		frequency = lingot_filter_filter_sample(frame->freq_filter,
				frame->core->freq);
		closest_note_index = lingot_gui_mainframe_get_closest_note_index(
				frame->core->freq, frame->conf->scale,
				frame->conf->root_frequency_error, &error_cents);
		lingot_gauge_compute(frame->gauge, error_cents);
	}

	return 0;
}

/* timeout for dispatching the error queue */
gboolean lingot_gui_mainframe_callback_error_dispatcher(gpointer data) {
	unsigned int period;
	GtkWidget* message_dialog;
	LingotMainFrame* frame = (LingotMainFrame*) data;

	char* error_message = NULL;
	message_type_t message_type;
	int error_code;
	int more_messages;

	do {
		more_messages = lingot_msg_get(&error_message, &message_type,
				&error_code);

		if (more_messages) {
			GtkWindow* parent = GTK_WINDOW(
					(frame->config_dialog != NULL) ?
					frame->config_dialog->win : frame->win);
			GtkButtonsType buttonsType;

			char message[2000];
			char* message_pointer = message;

			message_pointer += sprintf(message_pointer, "%s", error_message);

			if (error_code == EBUSY) {
				message_pointer +=
						sprintf(message_pointer, "\n\n%s",
								_("Please check that there are not other processes locking the requested device. Also, consider that some audio servers can sometimes hold the resources for a few seconds since the last time they were used. In such a case, you can try again."));
			}

			if ((message_type == ERROR) && !frame->core->running) {
				buttonsType = GTK_BUTTONS_OK;
				message_pointer +=
						sprintf(message_pointer, "\n\n%s",
								_("The core is not running, you must check your configuration."));
			} else {
				buttonsType = GTK_BUTTONS_OK;
			}

			message_dialog = gtk_message_dialog_new(parent,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					(message_type == ERROR) ?
							GTK_MESSAGE_ERROR :
							((message_type == WARNING) ?
									GTK_MESSAGE_WARNING : GTK_MESSAGE_INFO),
					buttonsType, "%s", message);

			gtk_window_set_title(GTK_WINDOW(message_dialog),
					(message_type == ERROR) ?
							_("Error") :
							((message_type == WARNING) ?
									_("Warning") : _("Info") ));
			gtk_window_set_icon(GTK_WINDOW(message_dialog),
					gtk_window_get_icon(GTK_WINDOW(frame->win) ));
			gtk_dialog_run(GTK_DIALOG(message_dialog) );
			gtk_widget_destroy(message_dialog);
			free(error_message);

//			if ((message_type == ERROR) && !frame->core->running) {
//				lingot_gui_mainframe_callback_config_dialog(NULL, frame);
//			}

		}
	} while (more_messages);

	period = 1000 / ERROR_DISPATCH_RATE;
	frame->error_dispatcher_uid = g_timeout_add(period,
			lingot_gui_mainframe_callback_error_dispatcher, frame);

	return 0;
}

void lingot_gui_mainframe_callback_open_config(gpointer data,
		LingotMainFrame* frame) {
	GtkWidget * dialog = gtk_file_chooser_dialog_new(
			_("Open Configuration File"), GTK_WINDOW(frame->win),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL );
	GtkFileFilter *filefilter;
	LingotConfig* config = NULL;
	filefilter = gtk_file_filter_new();

	gtk_file_filter_set_name(filefilter,
			(const gchar *) _("Lingot configuration files"));
	gtk_file_filter_add_pattern(filefilter, "*.conf");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filefilter);
	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);

	if (filechooser_config_last_folder != NULL ) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
				filechooser_config_last_folder);
	}

	if (gtk_dialog_run(GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog) );
		if (filechooser_config_last_folder != NULL )
			free(filechooser_config_last_folder);
		filechooser_config_last_folder = strdup(
				gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog) ));
		config = lingot_config_new();
		lingot_config_load(config, filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
	//g_free(filefilter);

	if (config != NULL ) {
		lingot_gui_config_dialog_show(frame, config);
	}
}

void lingot_gui_mainframe_callback_save_config(gpointer data,
		LingotMainFrame* frame) {
	GtkWidget *dialog = gtk_file_chooser_dialog_new(
			_("Save Configuration File"), GTK_WINDOW(frame->win),
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL );
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
			TRUE);

	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),
			"untitled.conf");
	GtkFileFilter* filefilter = gtk_file_filter_new();

	gtk_file_filter_set_name(filefilter,
			(const gchar *) _("Lingot configuration files"));
	gtk_file_filter_add_pattern(filefilter, "*.conf");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filefilter);
	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);

	if (filechooser_config_last_folder != NULL ) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
				filechooser_config_last_folder);
	}

	if (gtk_dialog_run(GTK_DIALOG(dialog) ) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog) );
		if (filechooser_config_last_folder != NULL )
			free(filechooser_config_last_folder);
		filechooser_config_last_folder = strdup(
				gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog) ));
		lingot_config_save(frame->conf, filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

void lingot_gui_mainframe_color(GdkColor* color, int red, int green, int blue) {
	color->red = red;
	color->green = green;
	color->blue = blue;
}

void lingot_gui_mainframe_create(int argc, char *argv[]) {

	LingotMainFrame* frame;
	LingotConfig* conf;

	if (filechooser_config_last_folder == NULL ) {
		char buff[1000];
		sprintf(buff, "%s/%s", getenv("HOME"), CONFIG_DIR_NAME);
		filechooser_config_last_folder = strdup(buff);
	}

	frame = malloc(sizeof(LingotMainFrame));

	frame->config_dialog = NULL;

	frame->conf = lingot_config_new();
	conf = frame->conf;
	lingot_config_load(conf, CONFIG_FILE_NAME);

	frame->gauge = lingot_gauge_new(conf->gauge_rest_value); // gauge in rest situation

	// ----- FREQUENCY FILTER CONFIGURATION ------

	// low pass IIR filter.
	FLT freq_filter_a[] = { 1.0, -0.5 };
	FLT freq_filter_b[] = { 0.5 };

	frame->freq_filter = lingot_filter_new(1, 0, freq_filter_a, freq_filter_b);

	// ---------------------------------------------------

	gtk_init(&argc, &argv);
	gtk_set_locale();

	GtkBuilder* builder = gtk_builder_new();

	// TODO: obtain glade files installation dir by other way
	// TODO: add dev condition to check for the file name in $PWD
#	define FILE_NAME "lingot-mainframe.glade"
	FILE* fd = fopen("src/glade/" FILE_NAME, "r");
	if (fd != NULL ) {
		fclose(fd);
		gtk_builder_add_from_file(builder, "src/glade/" FILE_NAME, NULL );
	} else {
		gtk_builder_add_from_file(builder, LINGOT_GLADEDIR FILE_NAME, NULL );
	}
#	undef FILE_NAME

	frame->win = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));

	GdkPixbuf* logo = gdk_pixbuf_new_from_xpm_data(lingotlogo);
	gtk_icon_theme_add_builtin_icon("lingot-logo", 64, logo);

	gtk_window_set_icon(GTK_WINDOW(frame->win), logo);

	frame->gauge_area =
			GTK_WIDGET(gtk_builder_get_object(builder, "gauge_area"));
	frame->spectrum_area =
			GTK_WIDGET(gtk_builder_get_object(builder, "spectrum_area"));

	frame->freq_label =
			GTK_WIDGET(gtk_builder_get_object(builder, "freq_label"));
	frame->tone_label =
			GTK_WIDGET(gtk_builder_get_object(builder, "tone_label"));
	frame->error_label =
			GTK_WIDGET(gtk_builder_get_object(builder, "error_label"));

	frame->spectrum_frame =
			GTK_WIDGET(gtk_builder_get_object(builder, "spectrum_frame"));
	frame->spectrum_scroll = GTK_SCROLLED_WINDOW(
			gtk_builder_get_object(builder, "scrolledwindow1"));
	frame->view_spectrum_item = GTK_WIDGET(gtk_builder_get_object(builder,
					"spectrum_item"));

	gtk_check_menu_item_set_active(
			GTK_CHECK_MENU_ITEM(frame->view_spectrum_item), TRUE);

	// show all
	gtk_widget_show_all(frame->win);

	int x = ((conf->fft_size > 256) ? (conf->fft_size >> 1) : 256)
			+ 2 * spectrum_x_margin;
	int y = spectrum_size_y + spectrum_bottom_margin + spectrum_top_margin;

	// two pixmaps for double buffer in gauge and spectrum drawing
	// (virtual screen)
	gdk_pixmap_new(gtk_widget_get_window(frame->gauge_area), gauge_size_x,
			gauge_size_y, -1);

	// GTK signals
	gtk_signal_connect(
			GTK_OBJECT(gtk_builder_get_object(builder, "preferences_item")),
			"activate",
			GTK_SIGNAL_FUNC(lingot_gui_mainframe_callback_config_dialog),
			frame);
	gtk_signal_connect(GTK_OBJECT(gtk_builder_get_object(builder, "quit_item")),
			"activate", GTK_SIGNAL_FUNC(lingot_gui_mainframe_callback_destroy),
			frame);
	gtk_signal_connect(
			GTK_OBJECT(gtk_builder_get_object(builder, "about_item")),
			"activate", GTK_SIGNAL_FUNC(lingot_gui_mainframe_callback_about),
			frame);
	gtk_signal_connect(
			GTK_OBJECT(gtk_builder_get_object(builder, "spectrum_item")),
			"activate",
			GTK_SIGNAL_FUNC(lingot_gui_mainframe_callback_view_spectrum),
			frame);
	gtk_signal_connect(
			GTK_OBJECT(gtk_builder_get_object(builder, "open_config_item")),
			"activate",
			GTK_SIGNAL_FUNC(lingot_gui_mainframe_callback_open_config), frame);
	gtk_signal_connect(
			GTK_OBJECT(gtk_builder_get_object(builder, "save_config_item")),
			"activate",
			GTK_SIGNAL_FUNC(lingot_gui_mainframe_callback_save_config), frame);

	gtk_signal_connect(GTK_OBJECT(frame->gauge_area), "expose_event",
			GTK_SIGNAL_FUNC(lingot_gui_mainframe_callback_redraw_gauge), frame);
	gtk_signal_connect(GTK_OBJECT(frame->spectrum_area), "expose_event",
			GTK_SIGNAL_FUNC(lingot_gui_mainframe_callback_redraw_spectrum),
			frame);
	gtk_signal_connect(GTK_OBJECT(frame->win), "destroy",
			GTK_SIGNAL_FUNC(lingot_gui_mainframe_callback_destroy), frame);

	GtkAccelGroup* accel_group = gtk_accel_group_new();
	gtk_widget_add_accelerator(
			GTK_WIDGET(gtk_builder_get_object(builder, "preferences_item")),
			"activate", accel_group, 'p', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_window_add_accel_group(GTK_WINDOW(frame->win), accel_group);

	unsigned int period;
	period = 1000 / conf->visualization_rate;
	frame->visualization_timer_uid = g_timeout_add(period,
			lingot_gui_mainframe_callback_tout_visualization, frame);

	period = 1000 / conf->calculation_rate;
	frame->freq_computation_timer_uid = g_timeout_add(period,
			lingot_gui_mainframe_callback_tout_spectrum_computation_display,
			frame);

	period = 1000 / GAUGE_RATE;
	frame->gauge_computation_uid = g_timeout_add(period,
			lingot_gui_mainframe_callback_gauge_computation, frame);

	period = 1000 / ERROR_DISPATCH_RATE;
	frame->error_dispatcher_uid = g_timeout_add(period,
			lingot_gui_mainframe_callback_error_dispatcher, frame);

	frame->core = lingot_core_new(conf);
	lingot_core_start(frame->core);

	g_object_unref(builder);

	gtk_main();
}

void lingot_gui_mainframe_destroy(LingotMainFrame* frame) {

	lingot_core_stop(frame->core);
	lingot_core_destroy(frame->core);

	lingot_gauge_destroy(frame->gauge);
	lingot_filter_destroy(frame->freq_filter);
	lingot_config_destroy(frame->conf);
	if (frame->config_dialog)
		lingot_gui_config_dialog_destroy(frame->config_dialog);

	free(frame);
}

// ---------------------------------------------------------------------------

static void cairo_set_source_argb(cairo_t *cr, unsigned int color) {
	cairo_set_source_rgba(cr, 0.00392156862745098 * ((color >> 16) & 0xff),
			0.00392156862745098 * ((color >> 8) & 0xff),
			0.00392156862745098 * (color & 0xff),
			1.0 - 0.00392156862745098 * ((color >> 24) & 0xff));
}

typedef struct {
	FLT x;
	FLT y;
} point_t;

static void lingot_gui_mainframe_draw_gauge_tic(cairo_t *cr,
		const point_t* gaugeCenter, double radius1, double radius2,
		double angle) {
	cairo_move_to(cr, gaugeCenter->x + radius1 * sin(angle),
			gaugeCenter->y - radius1 * cos(angle));
	cairo_rel_line_to(cr, (radius2 - radius1) * sin(angle),
			(radius1 - radius2) * cos(angle));
	cairo_stroke(cr);
}

void lingot_gui_mainframe_draw_gauge(const LingotMainFrame* frame) {
	GdkGC * gc =
			gtk_widget_get_style(frame->gauge_area)->fg_gc[gtk_widget_get_state(
					frame->gauge_area)];
	GdkWindow* w = gtk_widget_get_window(frame->gauge_area);

// normalized dimensions
	static const FLT gauge_gaugeCenterY = 0.94;
	static const FLT gauge_gaugeLength = 0.85;
	static const FLT gauge_gaugeLengthBack = 0.08;
	static const FLT gauge_gaugeCenterRadius = 0.05;
	static const FLT gauge_gaugeStroke = 0.01;
	static const FLT gauge_gaugeShadowOffsetX = 0.015;
	static const FLT gauge_gaugeShadowOffsetY = 0.01;
	static const FLT gauge_centsBarStroke = 0.025;
	static const FLT gauge_centsBarRadius = 0.75;
	static const FLT gauge_centsBarMajorTicRadius = 0.04;
	static const FLT gauge_centsBarMinorTicRadius = 0.03;
	static const FLT gauge_centsBarMajorTicStroke = 0.03;
	static const FLT gauge_centsBarMinorTicStroke = 0.01;
	static const FLT gauge_centsTextSize = 0.09;
	static const FLT gauge_frequencyBarStroke = 0.025;
	static const FLT gauge_frequencyBarRadius = 0.78;
	static const FLT gauge_frequencyBarMajorTicRadius = 0.04;
	static const FLT gauge_frequencyBarMinorTicRadius = 0.03;
	static const FLT gauge_frequencyBarMajorTicStroke = 0.03;
	static const FLT gauge_frequencyBarMinorTicStroke = 0.01;
	static const FLT gauge_frequencyTextRadius = 0.1;
	static const FLT gauge_frequencyTextSize = 0.09;
	static const FLT gauge_notesRadius = 0.80;
	static const FLT gauge_notesTextSize = 0.14;
	static const FLT gauge_notesSubindexTextSize = 0.06;
	static const FLT gauge_notesSecondaryTextSize = 0.14;
	static const FLT gauge_okBarStroke = 0.07;
	static const FLT gauge_okBarRadius = 0.48;

	static const FLT overtureAngle = 65.0 * M_PI / 180.0;

// colors
	static const unsigned int gauge_backgroundColor = 0xdddddd;
	static const unsigned int gauge_gaugeColor = 0xaa3333;
	static const unsigned int gauge_gaugeShadowColor = 0x44000000;
	static const unsigned int gauge_centsBarColor = 0x333355;
	static const unsigned int gauge_frequencyBarColor = 0x555533;
	static const unsigned int gauge_okColor = 0x99dd99;
	static const unsigned int gauge_koColor = 0xddaaaa;

	int width = gauge_size_x;
	int height = gauge_size_y;

// dimensions applied to the current size
	point_t gaugeCenter = { .x = width >> 1, .y = height * gauge_gaugeCenterY };
	point_t gaugeShadowCenter = { .x = gaugeCenter.x
			+ height * gauge_gaugeShadowOffsetX, .y = gaugeCenter.y
			+ height * gauge_gaugeShadowOffsetY };
	FLT gaugeLength = height * gauge_gaugeLength;
	FLT gaugeLengthBack = height * gauge_gaugeLengthBack;
	FLT gaugeCenterRadius = height * gauge_gaugeCenterRadius;
	FLT gaugeStroke = height * gauge_gaugeStroke;
	FLT centsBarRadius = height * gauge_centsBarRadius;
	FLT centsBarStroke = height * gauge_centsBarStroke;
	FLT centsBarMajorTicRadius = centsBarRadius
			- height * gauge_centsBarMajorTicRadius;
	FLT centsBarMinorTicRadius = centsBarRadius
			- height * gauge_centsBarMinorTicRadius;
	FLT centsBarMajorTicStroke = height * gauge_centsBarMajorTicStroke;
	FLT centsBarMinorTicStroke = height * gauge_centsBarMinorTicStroke;
	FLT centsTextSize = height * gauge_centsTextSize;
	FLT frequencyBarRadius = height * gauge_frequencyBarRadius;
	FLT frequencyBarMajorTicRadius = frequencyBarRadius
			+ height * gauge_frequencyBarMajorTicRadius;
	FLT frequencyBarMajorTicStroke = height * gauge_frequencyBarMajorTicStroke;
	FLT frequencyBarMinorTicRadius = frequencyBarRadius
			+ height * gauge_frequencyBarMinorTicRadius;
	FLT frequencyBarMinorTicStroke = height * gauge_frequencyBarMinorTicStroke;
	FLT frequencyBarStroke = height * gauge_frequencyBarStroke;
	FLT frequencyTextRadius = frequencyBarRadius
			+ height * gauge_frequencyTextRadius;
	FLT frequencyTextSize = height * gauge_frequencyTextSize;
	FLT notesRadius = height * gauge_notesRadius;
	FLT notesTextSize = height * gauge_notesTextSize;
	FLT notesSubindexTextSize = height * gauge_notesSubindexTextSize;
	FLT notesSecondaryTextSize = height * gauge_notesSecondaryTextSize;
	FLT okBarRadius = height * gauge_okBarRadius;
	FLT okBarStroke = height * gauge_okBarStroke;

	static FLT gauge_size = 90.0;
	FLT max = 1.0;

// TODO: keep?
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
			gauge_size_x, gauge_size_y);
	cairo_t *cr = cairo_create(surface);

	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	GdkRectangle r = { .x = 0, .y = 0, .width = gauge_size_x, .height =
			gauge_size_y };
	gdk_cairo_rectangle(cr, &r);
	cairo_fill(cr);

	// draw ok/ko bar
	cairo_set_line_width(cr, okBarStroke);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
	cairo_set_source_argb(cr, gauge_koColor);
	cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, okBarRadius,
			-0.5 * M_PI - overtureAngle, -0.5 * M_PI + overtureAngle);
	cairo_stroke(cr);
	cairo_set_source_argb(cr, gauge_okColor);
	cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, okBarRadius,
			-0.5 * M_PI - 0.1 * overtureAngle,
			-0.5 * M_PI + 0.1 * overtureAngle);
	cairo_stroke(cr);

	// draw cents bar
	cairo_set_line_width(cr, centsBarStroke);
	cairo_set_source_argb(cr, gauge_centsBarColor);
	cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, centsBarRadius,
			-0.5 * M_PI - 1.05 * overtureAngle,
			-0.5 * M_PI + 1.05 * overtureAngle);
	cairo_stroke(cr);

	// cent tics
	double maxOffsetRounded = frame->conf->scale->max_offset_rounded;
	const int maxMinorDivisions = 20;
	double centsPerMinorDivision = maxOffsetRounded / maxMinorDivisions;
	double base = pow(10.0, floor(log10(centsPerMinorDivision)));
	double normalizedCentsPerDivision = centsPerMinorDivision / base;
	if (normalizedCentsPerDivision >= 6.0) {
		normalizedCentsPerDivision = 10.0;
	} else if (normalizedCentsPerDivision >= 2.5) {
		normalizedCentsPerDivision = 5.0;
	} else if (normalizedCentsPerDivision >= 1.2) {
		normalizedCentsPerDivision = 2.0;
	} else {
		normalizedCentsPerDivision = 1.0;
	}
	centsPerMinorDivision = normalizedCentsPerDivision * base;
	double centsPerMajorDivision = 5.0 * centsPerMinorDivision;

	// minor tics
	cairo_set_line_width(cr, centsBarMinorTicStroke);
	int maxIndex = (int) floor(0.5 * maxOffsetRounded / centsPerMinorDivision);
	double angleStep = 2.0 * overtureAngle * centsPerMinorDivision
			/ maxOffsetRounded;
	int index;
	for (index = -maxIndex; index <= maxIndex; index++) {
		double angle = index * angleStep;
		lingot_gui_mainframe_draw_gauge_tic(cr, &gaugeCenter,
				centsBarMinorTicRadius, centsBarRadius, angle);
	}

	// major tics
	maxIndex = (int) floor(0.5 * maxOffsetRounded / centsPerMajorDivision);
	angleStep = 2.0 * overtureAngle * centsPerMajorDivision / maxOffsetRounded;
	cairo_set_line_width(cr, centsBarMajorTicStroke);
	for (index = -maxIndex; index <= maxIndex; index++) {
		double angle = index * angleStep;
		lingot_gui_mainframe_draw_gauge_tic(cr, &gaugeCenter,
				centsBarMajorTicRadius, centsBarRadius, angle);
	}

	// cents text
	cairo_set_line_width(cr, 1.0);
	double oldAngle = 0.0;
	float textSize;

	cairo_save(cr);

	static char buff[10];

	cairo_text_extents_t te;
	cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, centsTextSize);

	sprintf(buff, "%s", "cent");
	cairo_text_extents(cr, buff, &te);
	cairo_move_to(cr, gaugeCenter.x - te.width / 2 - te.x_bearing,
			gaugeCenter.y - 0.81 * centsBarMajorTicRadius - te.height / 2
					- te.y_bearing);
	cairo_show_text(cr, buff);

	cairo_translate(cr, gaugeCenter.x, gaugeCenter.y);
	for (index = -maxIndex; index <= maxIndex; index++) {
		double angle = index * angleStep;
		cairo_rotate(cr, angle - oldAngle);
		int cents = (int) (index * centsPerMajorDivision);
		sprintf(buff, "%s%i", ((cents > 0) ? "+" : ""), cents);
		cairo_text_extents(cr, buff, &te);
		cairo_move_to(cr, -te.width / 2 - te.x_bearing,
				-0.9 * centsBarMajorTicRadius - te.height / 2 - te.y_bearing);
		cairo_show_text(cr, buff);
		oldAngle = angle;
	}
	cairo_restore(cr);
	cairo_stroke(cr);

	// draw frequency bar
	cairo_set_line_width(cr, frequencyBarStroke);
	cairo_set_source_argb(cr, gauge_frequencyBarColor);
	cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, frequencyBarRadius,
			-0.5 * M_PI - 1.05 * overtureAngle,
			-0.5 * M_PI + 1.05 * overtureAngle);
	cairo_stroke(cr);

	// frequency tics
	lingot_gui_mainframe_draw_gauge_tic(cr, &gaugeCenter,
			frequencyBarMajorTicRadius, frequencyBarRadius, 0.0);
//	double baseOffset = lingot_config_scale_get_absolute_offset(
//			frame->conf->scale, closest_note_index);
//	for (index = closest_note_index - 1;; index--) {
//		double offset = lingot_config_scale_get_absolute_offset(
//				frame->conf->scale, index);
//		if (baseOffset - offset < 0.5 * maxOffsetRounded) {
//			double angle = 2 * ((offset - baseOffset) * overtureAngle)
//					/ (maxOffsetRounded);
//			lingot_gui_mainframe_draw_gauge_tic(cr, &gaugeCenter,
//					frequencyBarMajorTicRadius, frequencyBarRadius, angle);
//		} else {
//			break;
//		}
//	}

//	// note name text
//	cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL,
//			CAIRO_FONT_WEIGHT_BOLD);
//	cairo_set_font_size(cr, notesTextSize);
//	sprintf(buff, "%s",
//			frame->conf->scale->note_name[lingot_config_scale_get_note_index(
//					frame->conf->scale, closest_note_index)]);
//	cairo_text_extents(cr, buff, &te);
//	cairo_move_to(cr, gaugeCenter.x - te.width / 2 - te.x_bearing,
//			gaugeCenter.y - notesRadius - te.height / 2 - te.y_bearing);
//	cairo_show_text(cr, buff);
//
//	cairo_set_font_size(cr, notesSubindexTextSize);
//	sprintf(buff, "%i",
//			lingot_config_scale_get_octave(frame->conf->scale,
//					closest_note_index));
//	cairo_text_extents_t te2;
//	cairo_text_extents(cr, buff, &te2);
//	cairo_move_to(cr, gaugeCenter.x + te.width / 2 + te.x_bearing,
//			gaugeCenter.y - notesRadius);
//	cairo_show_text(cr, buff);

//	// frequency text
//	if (frequency > 0.0) {
//		cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL,
//				CAIRO_FONT_WEIGHT_NORMAL);
//		cairo_set_font_size(cr, frequencyTextSize);
//		sprintf(buff, "%0.1f Hz",
//				lingot_config_scale_get_frequency(frame->conf->scale,
//						closest_note_index));
//		cairo_text_extents(cr, buff, &te);
//		cairo_move_to(cr, gaugeCenter.x - te.width / 2 - te.x_bearing,
//				gaugeCenter.y - frequencyTextRadius - te.height / 2
//						- te.y_bearing);
//		cairo_show_text(cr, buff);
//	}

	// draws gauge
	double normalized_error = frame->gauge->position
			/ frame->conf->scale->max_offset_rounded;
	double angle = 2.0 * normalized_error * overtureAngle;
	cairo_set_line_width(cr, gaugeStroke);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
	cairo_set_source_argb(cr, gauge_gaugeShadowColor);
	lingot_gui_mainframe_draw_gauge_tic(cr, &gaugeShadowCenter,
			-gaugeLengthBack, gaugeLength, angle);
	cairo_arc(cr, gaugeShadowCenter.x, gaugeShadowCenter.y, gaugeCenterRadius,
			0, 2 * M_PI);
	cairo_fill(cr);
	cairo_set_source_argb(cr, gauge_gaugeColor);
	lingot_gui_mainframe_draw_gauge_tic(cr, &gaugeCenter, -gaugeLengthBack,
			gaugeLength, angle);
	cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, gaugeCenterRadius, 0, 2 * M_PI);
	cairo_fill(cr);

	cairo_destroy(cr);

	cairo_t *cr2 = gdk_cairo_create(w);
	cairo_set_source_surface(cr2, surface, 0, 0);
	cairo_paint(cr2);

	cairo_destroy(cr2);

	cairo_surface_destroy(surface);

}

void lingot_gui_mainframe_draw_spectrum(const LingotMainFrame* frame) {

	GtkWidget* widget = NULL;
	PangoLayout* layout;

	const int spectrum_size_x = (
			(frame->conf->fft_size > 256) ? (frame->conf->fft_size >> 1) : 256);

// minimum grid size in pixels
	static const int minimum_grid_width = 50;

// scale factors (in KHz) to draw the grid. We will choose the smaller
// factor that respects the minimum_grid_width
	static const double scales[] = { 0.01, 0.05, 0.1, 0.2, 0.5, 1, 2, 4, 11, 22,
			-1.0 };

// spectrum drawing mode
	static gboolean spectrum_drawing_filled = TRUE;

// grid division in dB
	static FLT grid_db_height = 25;

	register unsigned int i;
	int j;
	int old_j;

	GdkGC * gc =
			gtk_widget_get_style(frame->spectrum_area)->fg_gc[gtk_widget_get_state(
					frame->spectrum_area)];
	GdkWindow* w = gtk_widget_get_window(frame->spectrum_area);

	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
			spectrum_size_x + 2 * spectrum_x_margin,
			spectrum_size_y + spectrum_bottom_margin + spectrum_top_margin);
	cairo_t *cr = cairo_create(surface);

// clear all
	int sizex = spectrum_size_x + 2 * spectrum_x_margin;
	int sizey = spectrum_size_y + spectrum_bottom_margin + spectrum_top_margin;

	cairo_set_source_rgb(cr, 0.06, 0.2, 0.06);
	GdkRectangle r = { .x = 0, .y = 0, .width = sizex, .height = sizey };
	gdk_cairo_rectangle(cr, &r);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0.56, 0.56, 0.56);
	cairo_set_line_width(cr, 1.0);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
	cairo_move_to(cr, spectrum_x_margin, spectrum_size_y + spectrum_top_margin);
	cairo_line_to(cr, spectrum_x_margin + spectrum_size_x,
			spectrum_size_y + spectrum_top_margin);
	cairo_stroke(cr);

// choose scale factor
	for (i = 0; scales[i] > 0.0; i++) {
		if ((1e3 * scales[i] * frame->conf->fft_size * frame->conf->oversampling
				/ frame->conf->sample_rate) > minimum_grid_width)
			break;
	}

	if (scales[i] < 0.0)
		i--;

	FLT scale = scales[i];

	int grid_width = 1e3 * scales[i] * frame->conf->fft_size
			* frame->conf->oversampling / frame->conf->sample_rate;

	char buff[10];

	cairo_text_extents_t te;
	cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 9.5);

	FLT freq = 0.0;
	for (i = 0; i <= spectrum_size_x; i += grid_width) {
		cairo_move_to(cr, spectrum_x_margin + i, spectrum_top_margin);
		cairo_line_to(cr, spectrum_x_margin + i,
				spectrum_size_y + spectrum_top_margin + 3);
		cairo_stroke(cr);

		if (freq == 0.0) {
			sprintf(buff, "0 Hz");
		} else if (floor(freq) == freq)
			sprintf(buff, "%0.0f kHz", freq);
		else if (floor(10 * freq) == 10 * freq) {
			if (freq <= 1000.0)
				sprintf(buff, "%0.0f Hz", 1e3 * freq);
			else
				sprintf(buff, "%0.1f kHz", freq);
		} else {
			if (freq <= 100.0)
				sprintf(buff, "%0.0f Hz", 1e3 * freq);
			else
				sprintf(buff, "%0.2f kHz", freq);
		}

		cairo_text_extents(cr, buff, &te);
		cairo_move_to(cr,
				spectrum_x_margin + i + 6 - te.width / 2 - te.x_bearing,
				spectrum_size_y + spectrum_top_margin + 8 - te.height / 2
						- te.y_bearing);
		cairo_show_text(cr, buff);
		freq += scale;
	}

	static const FLT plot_gain = 8.0;

	sprintf(buff, "dB");

	cairo_text_extents(cr, buff, &te);
	cairo_move_to(cr, spectrum_x_margin - 6 - te.width / 2 - te.x_bearing,
			6 - te.height / 2 - te.y_bearing);
	cairo_show_text(cr, buff);

	int grid_height =
			(int) (plot_gain * log10(pow(10.0, grid_db_height / 10.0))); // dB.
	j = 0;
	for (i = 0; i <= spectrum_size_y; i += grid_height) {
		if (j == 0)
			sprintf(buff, " %i", j);
		else
			sprintf(buff, "%i", j);

		cairo_text_extents(cr, buff, &te);
		cairo_move_to(cr, 6 - te.width / 2 - te.x_bearing,
				spectrum_size_y + spectrum_top_margin - i - te.height / 2
						- te.y_bearing);
		cairo_show_text(cr, buff);

		cairo_move_to(cr, spectrum_x_margin,
				spectrum_size_y + spectrum_top_margin - i);
		cairo_line_to(cr, spectrum_x_margin + spectrum_size_x,
				spectrum_size_y + spectrum_top_margin - i);
		cairo_stroke(cr);

		j += grid_db_height;
	}

// TODO: specify the visible range

// TODO: change access to frame->core->X
// spectrum drawing.
	if (frame->core->running) {

		cairo_set_source_rgb(cr, 0.13, 1.0, 0.13);

		j = -1;

		GtkAdjustment* adj = gtk_scrolled_window_get_hadjustment(
				frame->spectrum_scroll);
		int min = gtk_adjustment_get_value(adj) - spectrum_x_margin;
		int max = min + 256 + 3 * spectrum_x_margin;
		if (min < 0)
			min = 0;
		if (max >= spectrum_size_x)
			max = spectrum_size_x;

		for (i = min; i < max; i++) {

			FLT SNR = frame->core->SPL[i] - frame->core->noise_level[i];
			j = (SNR >= 0.0) ? (int) (plot_gain * SNR / 10.0) : 0; // dB.
			if (j >= spectrum_size_y)
				j = spectrum_size_y - 1;
			if (spectrum_drawing_filled) {
				cairo_move_to(cr, spectrum_x_margin + i,
						spectrum_size_y + spectrum_top_margin - 1);
				cairo_line_to(cr, spectrum_x_margin + i,
						spectrum_top_margin + spectrum_size_y - j);
				cairo_stroke(cr);
			} else {
				if ((old_j >= 0) && (old_j < spectrum_size_y) && (j >= 0)
						&& (j < spectrum_size_y))
					cairo_move_to(cr, spectrum_x_margin + i - 1,
							spectrum_size_y + spectrum_top_margin - old_j);
				cairo_line_to(cr, spectrum_x_margin + i,
						spectrum_size_y + spectrum_top_margin - j);
				cairo_stroke(cr);
				old_j = j;
			}
		}

		if (frame->core->freq != 0.0) {

			// fundamental frequency mark with a red point.
			cairo_set_source_rgb(cr, 1.0, 0.13, 0.13);
			cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
			cairo_set_line_width(cr, 4.0);

			// index of closest sample to fundamental frequency.
			i = (int) rint(
					frame->core->freq * frame->conf->fft_size
							* frame->conf->oversampling
							/ frame->conf->sample_rate);
			if ((i < frame->conf->fft_size - 1) && (i < spectrum_size_x - 1)) {

				FLT SNR = frame->core->SPL[i] - frame->core->noise_level[i];

				j = (SNR >= 0.0) ? (int) (plot_gain * SNR / 10.0) : 0; // dB.
				if (j < spectrum_size_y - 1) {
					cairo_move_to(cr, spectrum_x_margin + i - 1,
							spectrum_size_y + spectrum_top_margin - j - 1);
					cairo_rel_line_to(cr, 0.0, 0.0);
					cairo_stroke(cr);
				}
			}

			int k;
			for (k = 0; k < frame->core->markers_size; k++) {
				// index of closest sample to fundamental frequency.
				i = (int) rint(
						frame->core->markers[k] * frame->conf->fft_size
								* frame->conf->oversampling
								/ frame->conf->sample_rate);
				if ((i < frame->conf->fft_size - 1)
						&& (i < spectrum_size_x - 1)) {

					FLT SNR = frame->core->SPL[i] - frame->core->noise_level[i];

					j = (SNR >= 0.0) ? (int) (plot_gain * SNR / 10.0) : 0; // dB.
					if (j < spectrum_size_y - 1) {
						cairo_move_to(cr, spectrum_x_margin + i - 1,
								spectrum_size_y + spectrum_top_margin - j - 1);
						cairo_rel_line_to(cr, 0.0, 0.0);
						cairo_stroke(cr);
					}
				}
			}

			cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
			cairo_set_line_width(cr, 1.0);
		}

		cairo_set_source_rgb(cr, 0.66, 0.53, 1.0);

		// noise threshold drawing.
		j = -1;
		for (i = 0; (i < frame->conf->fft_size) && (i < spectrum_size_x); i++) {
			if ((i % 10) > 5)
				continue;

			FLT noise = frame->core->noise_level[i];
			old_j = j;
			j = (noise >= 0.0) ? (int) (plot_gain * noise / 10.0) : 0; // dB.
			if ((old_j >= 0) && (old_j < spectrum_size_y) && (j >= 0)
					&& (j < spectrum_size_y))
				cairo_move_to(cr, spectrum_x_margin + i - 1,
						spectrum_size_y + spectrum_top_margin - old_j);
			cairo_line_to(cr, spectrum_x_margin + i,
					spectrum_size_y + spectrum_top_margin - j);
			cairo_stroke(cr);
		}

	}

// TODO: keep?
	cairo_destroy(cr);

	cairo_t *cr2 = gdk_cairo_create(w);
	cairo_set_source_surface(cr2, surface, 0, 0);
	cairo_paint(cr2);

	cairo_destroy(cr2);

	cairo_surface_destroy(surface);
}

void lingot_gui_mainframe_draw_labels(const LingotMainFrame* frame) {

	char* note_name;
	static char error_string[30], freq_string[30];
	unsigned short note_index;

	// draw note, error and frequency labels

	// ignore continuous component
	if (frequency == 0.0) {
		note_name = "---";
		strcpy(error_string, "e = ---");
		strcpy(freq_string, "f = ---");
	} else {
		note_name =
				frame->conf->scale->note_name[lingot_config_scale_get_note_index(
						frame->conf->scale, closest_note_index)];
		sprintf(error_string, "e = %+2.0f cents", frame->gauge->position);
		sprintf(freq_string, "f = %6.2f Hz", frequency);
	}

	gtk_label_set_text(GTK_LABEL(frame->freq_label), freq_string);
	gtk_label_set_text(GTK_LABEL(frame->error_label), error_string);

	char* markup = g_markup_printf_escaped(
			"<span size=\"xx-large\" weight=\"bold\">%s</span>", note_name);
	gtk_label_set_markup(GTK_LABEL(frame->tone_label), markup);
	g_free(markup);
}

void lingot_gui_mainframe_change_config(LingotMainFrame* frame,
		LingotConfig* conf) {
	lingot_core_stop(frame->core);
	lingot_core_destroy(frame->core);

// dup.
	lingot_config_copy(frame->conf, conf);

	int x = ((frame->conf->fft_size > 256) ? (frame->conf->fft_size >> 1) : 256)
			+ 2 * spectrum_x_margin;
	int y = spectrum_size_y + spectrum_top_margin + spectrum_bottom_margin;
	gtk_widget_set_size_request(GTK_WIDGET(frame->spectrum_area), x, y);

	gtk_scrolled_window_set_policy(frame->spectrum_scroll,
			(frame->conf->fft_size > 512) ?
					GTK_POLICY_ALWAYS : GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	gtk_widget_set_size_request(GTK_WIDGET(frame->spectrum_scroll),
			260 + 2 * spectrum_x_margin,
			spectrum_size_y + spectrum_bottom_margin + spectrum_top_margin + 4
					+ ((frame->conf->fft_size > 512) ? 16 : 0));

	frame->core = lingot_core_new(frame->conf);
	lingot_core_start(frame->core);

// some parameters may have changed
	lingot_config_copy(conf, frame->conf);
}
