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

static void lingot_gui_mainframe_draw_gauge_background(
		const LingotMainFrame* frame);
static void lingot_gui_mainframe_draw_spectrum_background(
		const LingotMainFrame* frame);
void lingot_gui_mainframe_draw_gauge(const LingotMainFrame*);
void lingot_gui_mainframe_draw_spectrum(const LingotMainFrame*);
void lingot_gui_mainframe_draw_labels(const LingotMainFrame*);

// sizes

static int gauge_size_x = 0;
static int gauge_size_y = 0;

static int spectrum_size_x = 0;
static int spectrum_size_y = 0;

static FLT spectrum_bottom_margin;
static FLT spectrum_top_margin;
static FLT spectrum_left_margin;
static FLT spectrum_right_margin;

static FLT spectrum_inner_x;
static FLT spectrum_inner_y;

static FLT spectrum_db_density;
static FLT spectrum_min_db;
static FLT spectrum_max_db;

static int labelsbox_size_x = 0;
static int labelsbox_size_y = 0;

static gchar* filechooser_config_last_folder = NULL;

static const gdouble aspect_ratio_spectrum_visible = 1.14;
static const gdouble aspect_ratio_spectrum_invisible = 2.07;

// TODO: keep here?
static int closest_note_index = 0;
static FLT frequency = 0.0;

static cairo_surface_t *gauge_background = 0x0;
static cairo_surface_t *spectrum_background = 0x0;

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
	gtk_window_set_geometry_hints(GTK_WINDOW(frame->win), frame->win, &hints,
			GDK_HINT_ASPECT);

	gtk_widget_set_visible(frame->spectrum_frame, visible);
}

void lingot_gui_mainframe_callback_config_dialog(GtkWidget* w,
		LingotMainFrame* frame) {
	lingot_gui_config_dialog_show(frame, NULL );
}

static short int lingot_gui_mainframe_get_closest_note_index(FLT freq,
		const LingotScale* scale, FLT deviation, FLT* error_cents) {
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
			GtkWindow* parent = GTK_WINDOW((frame->config_dialog != NULL )?
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
					gtk_window_get_icon(GTK_WINDOW(frame->win)));
			gtk_dialog_run(GTK_DIALOG(message_dialog));
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

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if (filechooser_config_last_folder != NULL )
			free(filechooser_config_last_folder);
		filechooser_config_last_folder = strdup(
				gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog)));
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

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if (filechooser_config_last_folder != NULL )
			free(filechooser_config_last_folder);
		filechooser_config_last_folder = strdup(
				gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog)));
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

void lingot_gui_mainframe_callback_window_resize(GtkWidget *widget,
		GtkAllocation *allocation, void *data) {

	const LingotMainFrame* frame = (LingotMainFrame*) data;

	GtkAllocation req;
	gtk_widget_get_allocation(frame->gauge_area, &req);

	if ((req.width != gauge_size_x) || (req.height != gauge_size_y)) {
		gauge_size_x = req.width;
		gauge_size_y = req.height;
		lingot_gui_mainframe_draw_gauge_background(frame);
	}

	gtk_widget_get_allocation(frame->spectrum_area, &req);

	if ((req.width != spectrum_size_x) || (req.height != spectrum_size_y)) {
		spectrum_size_x = req.width;
		spectrum_size_y = req.height;
		lingot_gui_mainframe_draw_spectrum_background(frame);
	}

	gtk_widget_get_allocation(frame->labelsbox, &req);

	labelsbox_size_x = req.width;
	labelsbox_size_y = req.height;
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
//	gtk_set_locale();

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

	frame->gauge_area = GTK_WIDGET(
			gtk_builder_get_object(builder, "gauge_area"));
	frame->spectrum_area = GTK_WIDGET(
			gtk_builder_get_object(builder, "spectrum_area"));

	frame->freq_label = GTK_WIDGET(
			gtk_builder_get_object(builder, "freq_label"));
	frame->tone_label = GTK_WIDGET(
			gtk_builder_get_object(builder, "tone_label"));
	frame->error_label = GTK_WIDGET(
			gtk_builder_get_object(builder, "error_label"));

	frame->spectrum_frame = GTK_WIDGET(
			gtk_builder_get_object(builder, "spectrum_frame"));
	frame->view_spectrum_item = GTK_WIDGET(
			gtk_builder_get_object(builder, "spectrum_item"));
	frame->labelsbox = GTK_WIDGET(gtk_builder_get_object(builder, "labelsbox"));

	gtk_check_menu_item_set_active(
			GTK_CHECK_MENU_ITEM(frame->view_spectrum_item), TRUE);

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
			G_CALLBACK(lingot_gui_mainframe_callback_redraw_gauge), frame);
	g_signal_connect(frame->spectrum_area, "draw",
			G_CALLBACK(lingot_gui_mainframe_callback_redraw_spectrum), frame);
	g_signal_connect(frame->win, "destroy",
			G_CALLBACK(lingot_gui_mainframe_callback_destroy), frame);

	// TODO: remove
	g_signal_connect(frame->win, "size-allocate",
			G_CALLBACK(lingot_gui_mainframe_callback_window_resize), frame);

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

static void lingot_gui_mainframe_cairo_set_source_argb(cairo_t *cr,
		unsigned int color) {
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

static void lingot_gui_mainframe_draw_gauge_background(
		const LingotMainFrame* frame) {

	// normalized dimensions
	static const FLT gauge_gaugeCenterY = 0.94;
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
	static const FLT gauge_okBarStroke = 0.07;
	static const FLT gauge_okBarRadius = 0.48;

	static const FLT overtureAngle = 65.0 * M_PI / 180.0;

	// colors
	static const unsigned int gauge_centsBarColor = 0x333355;
	static const unsigned int gauge_frequencyBarColor = 0x555533;
	static const unsigned int gauge_okColor = 0x99dd99;
	static const unsigned int gauge_koColor = 0xddaaaa;

	int width = gauge_size_x;
	int height = gauge_size_y;

	// dimensions applied to the current size
	point_t gaugeCenter = { .x = width / 2, .y = height * gauge_gaugeCenterY };

	if (width < 1.6 * height) {
		height = width / 1.6;
		gaugeCenter.y = 0.5 * (gauge_size_y - height)
				+ height * gauge_gaugeCenterY;
	}

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
	FLT frequencyBarStroke = height * gauge_frequencyBarStroke;
	FLT okBarRadius = height * gauge_okBarRadius;
	FLT okBarStroke = height * gauge_okBarStroke;

	if (gauge_background != 0x0) {
		cairo_surface_destroy(gauge_background);
	}

	gauge_background = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
			gauge_size_x, gauge_size_y);
	cairo_t *cr = cairo_create(gauge_background);

	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_save(cr);
	GdkRectangle r = { .x = 0, .y = 0, .width = gauge_size_x, .height =
			gauge_size_y };
	gdk_cairo_rectangle(cr, &r);
	cairo_fill_preserve(cr);
	cairo_restore(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_stroke(cr);

// draw ok/ko bar
	cairo_set_line_width(cr, okBarStroke);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
	lingot_gui_mainframe_cairo_set_source_argb(cr, gauge_koColor);
	cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, okBarRadius,
			-0.5 * M_PI - overtureAngle, -0.5 * M_PI + overtureAngle);
	cairo_stroke(cr);
	lingot_gui_mainframe_cairo_set_source_argb(cr, gauge_okColor);
	cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, okBarRadius,
			-0.5 * M_PI - 0.1 * overtureAngle,
			-0.5 * M_PI + 0.1 * overtureAngle);
	cairo_stroke(cr);

// draw cents bar
	cairo_set_line_width(cr, centsBarStroke);
	lingot_gui_mainframe_cairo_set_source_argb(cr, gauge_centsBarColor);
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
				-0.92 * centsBarMajorTicRadius - te.height / 2 - te.y_bearing);
		cairo_show_text(cr, buff);
		oldAngle = angle;
	}
	cairo_restore(cr);
	cairo_stroke(cr);

// draw frequency bar
	cairo_set_line_width(cr, frequencyBarStroke);
	lingot_gui_mainframe_cairo_set_source_argb(cr, gauge_frequencyBarColor);
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
	cairo_destroy(cr);

}

void lingot_gui_mainframe_draw_gauge(const LingotMainFrame* frame) {
	GdkWindow* w = gtk_widget_get_window(frame->gauge_area);

	// normalized dimensions
	static const FLT gauge_gaugeCenterY = 0.94;
	static const FLT gauge_gaugeLength = 0.85;
	static const FLT gauge_gaugeLengthBack = 0.08;
	static const FLT gauge_gaugeCenterRadius = 0.045;
	static const FLT gauge_gaugeStroke = 0.01;
	static const FLT gauge_gaugeShadowOffsetX = 0.015;
	static const FLT gauge_gaugeShadowOffsetY = 0.01;

	static const FLT overtureAngle = 65.0 * M_PI / 180.0;

	// colors
	static const unsigned int gauge_gaugeColor = 0xaa3333;
	static const unsigned int gauge_gaugeShadowColor = 0x44000000;

	int width = gauge_size_x;
	int height = gauge_size_y;

	// dimensions applied to the current size
	point_t gaugeCenter = { .x = width / 2, .y = height * gauge_gaugeCenterY };

	if (width < 1.6 * height) {
		height = width / 1.6;
		gaugeCenter.y = 0.5 * (gauge_size_y - height)
				+ height * gauge_gaugeCenterY;
	}

	point_t gaugeShadowCenter = { .x = gaugeCenter.x
			+ height * gauge_gaugeShadowOffsetX, .y = gaugeCenter.y
			+ height * gauge_gaugeShadowOffsetY };
	FLT gaugeLength = height * gauge_gaugeLength;
	FLT gaugeLengthBack = height * gauge_gaugeLengthBack;
	FLT gaugeCenterRadius = height * gauge_gaugeCenterRadius;
	FLT gaugeStroke = height * gauge_gaugeStroke;

	if (gauge_background == 0x0) {
		lingot_gui_mainframe_draw_gauge_background(frame);
	}

	cairo_t *cr = gdk_cairo_create(w);
	cairo_set_source_surface(cr, gauge_background, 0, 0);
	cairo_paint(cr);

// draws gauge
	double normalized_error = frame->gauge->position
			/ frame->conf->scale->max_offset_rounded;
	double angle = 2.0 * normalized_error * overtureAngle;
	cairo_set_line_width(cr, gaugeStroke);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
	lingot_gui_mainframe_cairo_set_source_argb(cr, gauge_gaugeShadowColor);
	lingot_gui_mainframe_draw_gauge_tic(cr, &gaugeShadowCenter,
			-gaugeLengthBack, -0.99 * gaugeCenterRadius, angle);
	lingot_gui_mainframe_draw_gauge_tic(cr, &gaugeShadowCenter,
			0.99 * gaugeCenterRadius, gaugeLength, angle);
	cairo_arc(cr, gaugeShadowCenter.x, gaugeShadowCenter.y, gaugeCenterRadius,
			0, 2 * M_PI);
	cairo_fill(cr);
	lingot_gui_mainframe_cairo_set_source_argb(cr, gauge_gaugeColor);
	lingot_gui_mainframe_draw_gauge_tic(cr, &gaugeCenter, -gaugeLengthBack,
			gaugeLength, angle);
	cairo_arc(cr, gaugeCenter.x, gaugeCenter.y, gaugeCenterRadius, 0, 2 * M_PI);
	cairo_fill(cr);

	cairo_destroy(cr);

//	cairo_surface_destroy(surface);

}

static const int showSNR = 1;
static const int gain = 40;

FLT lingot_gui_mainframe_get_signal(const LingotMainFrame* frame, int i,
		FLT min, FLT max) {
	FLT signal = frame->core->SPL[i];
	if (showSNR) {
		signal -= frame->core->noise_level[i];
	} else {
		signal += gain;
	}
	if (signal < min) {
		signal = min;
	} else if (signal > max) {
		signal = max;
	}
	return signal;
}

FLT lingot_gui_mainframe_get_noise(const LingotMainFrame* frame, int i, FLT min,
		FLT max) {
	FLT noise = frame->conf->noise_threshold_db;
	if (!showSNR) {
		noise += frame->core->noise_level[i] + gain;
	}
	if (noise < min) {
		noise = min;
	} else if (noise > max) {
		noise = max;
	}
	return noise;
}

static char* lingot_gui_mainframe_format_frequency(FLT freq, char* buff) {
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

	return buff;
}

void lingot_gui_mainframe_draw_spectrum_background(const LingotMainFrame* frame) {

	// grid division in dB
	static FLT grid_db_height = 25;

	if (spectrum_background != 0x0) {
		cairo_surface_destroy(spectrum_background);
	}

	spectrum_background = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
			spectrum_size_x, spectrum_size_y);

	cairo_t *cr = cairo_create(spectrum_background);

	const FLT font_size = 8 + spectrum_size_y / 30;

	static char buff[10];
	cairo_text_extents_t te;
	cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, font_size);
	sprintf(buff, "75");
	cairo_text_extents(cr, buff, &te);

// spectrum area margins
	spectrum_bottom_margin = 1.6 * te.height;
	spectrum_top_margin = spectrum_bottom_margin;
	spectrum_left_margin = te.width * 1.5;
	spectrum_right_margin = 0.8 * spectrum_left_margin;
	spectrum_inner_x = spectrum_size_x - spectrum_left_margin
			- spectrum_right_margin;
	spectrum_inner_y = spectrum_size_y - spectrum_bottom_margin
			- spectrum_top_margin;

	sprintf(buff, "000 Hz");
	cairo_text_extents(cr, buff, &te);
	// minimum grid size in pixels
	const int minimum_grid_width = 1.5 * te.width;

	// clear all
	cairo_set_source_rgba(cr, 0.06, 0.2, 0.06, 1.0);
	GdkRectangle r = { .x = 0, .y = 0, .width = spectrum_size_x, .height =
			spectrum_size_y };
	gdk_cairo_rectangle(cr, &r);
	cairo_fill(cr);

	cairo_set_source_rgba(cr, 0.56, 0.56, 0.56, 1.0);
	cairo_set_line_width(cr, 1.0);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
	cairo_move_to(cr, spectrum_left_margin,
			spectrum_size_y - spectrum_bottom_margin);
	cairo_rel_line_to(cr, spectrum_inner_x, 0);
	cairo_stroke(cr);

	// choose scale factor
	const FLT max_frequency = 0.5 * frame->conf->sample_rate
			/ frame->conf->oversampling;

	// scale factors (in KHz) to draw the grid. We will choose the smaller
	// factor that respects the minimum_grid_width
	static const double scales[] = { 0.01, 0.05, 0.1, 0.2, 0.5, 1, 2, 4, 11, 22,
			-1.0 };

	unsigned int i;
	for (i = 0; scales[i] > 0.0; i++) {
		if ((1e3 * scales[i] * spectrum_inner_x / max_frequency)
				> minimum_grid_width)
			break;
	}

	if (scales[i] < 0.0) {
		i--;
	}

	FLT scale = scales[i];
	FLT grid_width = 1e3 * scales[i] * spectrum_inner_x / max_frequency;

	FLT freq = 0.0;
	for (i = 0; i <= spectrum_inner_x; i += grid_width) {
		cairo_move_to(cr, spectrum_left_margin + i, spectrum_top_margin);
		cairo_rel_line_to(cr, 0, spectrum_inner_y + 3); // TODO: proportion
		cairo_stroke(cr);

		lingot_gui_mainframe_format_frequency(freq, buff);

		cairo_text_extents(cr, buff, &te);
		cairo_move_to(cr,
				spectrum_left_margin + i + 6 - te.width / 2 - te.x_bearing,
				spectrum_size_y - 0.5 * spectrum_bottom_margin - te.height / 2
						- te.y_bearing);
		cairo_show_text(cr, buff);
		freq += scale;
	}

	spectrum_min_db = 0; // TODO
	spectrum_max_db = 50;
	spectrum_db_density = (spectrum_inner_y)
			/ (spectrum_max_db - spectrum_min_db);

	sprintf(buff, "dB");

	cairo_text_extents(cr, buff, &te);
	cairo_move_to(cr, spectrum_left_margin - te.x_bearing,
			0.5 * spectrum_top_margin - te.height / 2 - te.y_bearing);
	cairo_show_text(cr, buff);

	FLT grid_height = grid_db_height * spectrum_db_density;
	FLT y = 0;
	for (i = 0; i <= spectrum_inner_y; i += grid_height) {
		if (y == 0)
			sprintf(buff, " %0.0f", y);
		else
			sprintf(buff, "%0.0f", y);

		cairo_text_extents(cr, buff, &te);
		cairo_move_to(cr,
				0.45 * spectrum_left_margin - te.width / 2 - te.x_bearing,
				spectrum_size_y - spectrum_bottom_margin - i - te.height / 2
						- te.y_bearing);
		cairo_show_text(cr, buff);

		cairo_move_to(cr, spectrum_left_margin - 3,
				spectrum_size_y - spectrum_bottom_margin - i);
		cairo_rel_line_to(cr, spectrum_inner_x + 3, 0);
		cairo_stroke(cr);

		y += grid_db_height;
	}

	cairo_destroy(cr);
}

void lingot_gui_mainframe_draw_spectrum(const LingotMainFrame* frame) {

	unsigned int i;

	GdkWindow* w = gtk_widget_get_window(frame->spectrum_area);

	if (spectrum_background == 0x0) {
		lingot_gui_mainframe_draw_spectrum_background(frame);
	}

	// TODO: change access to frame->core->X
	// spectrum drawing.
	if (frame->core->running) {

		cairo_t *cr = gdk_cairo_create(w);
		cairo_set_source_surface(cr, spectrum_background, 0, 0);
		cairo_paint(cr);

		cairo_set_line_width(cr, 1.0);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);

		FLT x;
		FLT y = -1;

		const int min = 0;
		const int max = frame->conf->fft_size / 2;

		FLT index_density = spectrum_inner_x / max;
		// TODO: step
		int index_step = 1;

		static const double dashed1[] = { 5.0, 5.0 };
		static int len1 = sizeof(dashed1) / sizeof(dashed1[0]);

		const FLT x0 = spectrum_left_margin;
		const FLT y0 = spectrum_size_y - spectrum_bottom_margin;

		y = y0
				- spectrum_db_density
						* lingot_gui_mainframe_get_signal(frame, min,
								spectrum_min_db, spectrum_max_db); // dB.
		FLT dydxm1 = 0;

		cairo_set_source_rgba(cr, 0.13, 1.0, 0.13, 1.0);
	//		cairo_mask_surface(cr, surface, 20.0, 20.);
	//		cairo_rectangle(cr, spectrum_left_margin, spectrum_top_margin,
	//				spectrum_inner_x, spectrum_inner_y);
		cairo_move_to(cr, x0, y0);
		cairo_line_to(cr, x0, y);

		FLT yp1 = y0
				- spectrum_db_density
						* lingot_gui_mainframe_get_signal(frame, min + 1,
								spectrum_min_db, spectrum_max_db);
		FLT ym1 = y;

		for (i = index_step; i < max - 1; i += index_step) {

			x = x0 + index_density * i;
			ym1 = y;
			y = yp1;
			yp1 = y0
					- spectrum_db_density
							* lingot_gui_mainframe_get_signal(frame, i + 1,
									spectrum_min_db, spectrum_max_db);
			FLT dydx = (yp1 - ym1) / (2 * index_density);
			static const FLT dx = 0.4;
			FLT x1 = x - (1 - dx) * index_density;
			FLT x2 = x - dx * index_density;
			FLT y1 = ym1 + dydxm1 * dx;
			FLT y2 = y - dydx * dx;

			dydxm1 = dydx;
			cairo_curve_to(cr, x1, y1, x2, y2, x, y);
		}

		y = y0
				- spectrum_db_density
						* lingot_gui_mainframe_get_signal(frame, max - 1,
								spectrum_min_db, spectrum_max_db); // dB.
		cairo_line_to(cr, x0 + index_density * max, y);
		cairo_line_to(cr, x0 + index_density * max, y0);
		cairo_close_path(cr);
		cairo_fill_preserve(cr);
//		cairo_restore(cr);
		cairo_stroke(cr);

		cairo_set_dash(cr, dashed1, len1, 0);

		if (frame->core->freq != 0.0) {

			// fundamental frequency mark with a red vertical line.
			cairo_set_source_rgba(cr, 1.0, 0.13, 0.13, 1.0);
			cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
			cairo_set_line_width(cr, 1.5);

			// index of closest sample to fundamental frequency.
			x = x0
					+ index_density * frame->core->freq * frame->conf->fft_size
							* frame->conf->oversampling
							/ frame->conf->sample_rate;
			//			s = lingot_gui_mainframe_get_signal(frame, i, min_db, max_db);
//			y = y0 - spectrum_db_density * s; // dB.
			cairo_move_to(cr, x, y0);
			cairo_rel_line_to(cr, 0.0, -spectrum_inner_y);
			cairo_stroke(cr);

			cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
			cairo_set_line_width(cr, 1.0);
		}

		cairo_set_source_rgba(cr, 0.86, 0.83, 0.0, 1.0);

		y = y0
				- spectrum_db_density
						* lingot_gui_mainframe_get_noise(frame, 0,
								spectrum_min_db, spectrum_max_db); // dB.
		cairo_move_to(cr, x0, y);
		// noise threshold drawing.
		for (i = min + index_step; i < max - 1; i += index_step) {

			x = x0 + index_density * i;
			y = y0
					- spectrum_db_density
							* lingot_gui_mainframe_get_noise(frame, i,
									spectrum_min_db, spectrum_max_db); // dB.
			cairo_line_to(cr, x, y);
		}
		cairo_stroke(cr);

		cairo_destroy(cr);

	}

}

void lingot_gui_mainframe_draw_labels(const LingotMainFrame* frame) {

	char* note_string;
	static char error_string[30];
	static char freq_string[30];
	static char octave_string[30];

// draw note, error and frequency labels

// ignore continuous component
	if (frequency == 0.0) {
		note_string = "---";
		strcpy(error_string, "e = ---");
		strcpy(freq_string, "f = ---");
		strcpy(octave_string, "");
	} else {
		note_string =
				frame->conf->scale->note_name[lingot_config_scale_get_note_index(
						frame->conf->scale, closest_note_index)];
		sprintf(error_string, "e = %+2.0f cents", frame->gauge->position);
		sprintf(freq_string, "f = %6.2f Hz", frequency);
		sprintf(octave_string, "%d",
				lingot_config_scale_get_octave(frame->conf->scale,
						closest_note_index) + 4);
	}

	int font_size = 9 + labelsbox_size_x / 80;
	char* markup = g_markup_printf_escaped("<span font_desc=\"%d\">%s</span>",
			font_size, freq_string);
	gtk_label_set_markup(GTK_LABEL(frame->freq_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<span font_desc=\"%d\">%s</span>",
			font_size, error_string);
	gtk_label_set_markup(GTK_LABEL(frame->error_label), markup);
	g_free(markup);

	font_size = 10 + labelsbox_size_x / 22;
	markup =
			g_markup_printf_escaped(
					"<span font_desc=\"%d\" weight=\"bold\">%s</span><span font_desc=\"%d\" weight=\"bold\"><sub>%s</sub></span>",
					font_size, note_string, (int) (0.75 * font_size),
					octave_string);
	gtk_label_set_markup(GTK_LABEL(frame->tone_label), markup);
	g_free(markup);
}

void lingot_gui_mainframe_change_config(LingotMainFrame* frame,
		LingotConfig* conf) {
	lingot_core_stop(frame->core);
	lingot_core_destroy(frame->core);

// dup.
	lingot_config_copy(frame->conf, conf);

	frame->core = lingot_core_new(frame->conf);
	lingot_core_start(frame->core);

// some parameters may have changed
	lingot_config_copy(conf, frame->conf);
}
