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

#include "lingot-background.xpm"
#include "lingot-logo.xpm"

void lingot_gui_mainframe_draw_gauge(const LingotMainFrame*);
void lingot_gui_mainframe_draw_spectrum(const LingotMainFrame*);
void lingot_gui_mainframe_draw_labels(const LingotMainFrame*);

// sizes

int gauge_size_x = 160;
int gauge_size_y = 100;

int spectrum_size_y = 64;

// spectrum area margins
int spectrum_bottom_margin = 16;
int spectrum_top_margin = 12;
int spectrum_x_margin = 15;

gchar* filechooser_config_last_folder = NULL;

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

unsigned short int lingot_gui_mainframe_get_closest_note_index(FLT freq,
		LingotScale* scale, FLT deviation, FLT* error_cents) {
	unsigned short note_index = 0;
	unsigned short int index;

	FLT offset = 1200.0 * log2(freq / scale->base_frequency) - deviation;
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

	if (!frame->core->running || isnan(frame->core->freq)
			|| (frame->core->freq < frame->conf->min_frequency)) {
		lingot_gauge_compute(frame->gauge, frame->conf->gauge_rest_value);
	} else {
		lingot_gui_mainframe_get_closest_note_index(frame->core->freq,
				frame->conf->scale, frame->conf->root_frequency_error,
				&error_cents);
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
	gdk_pixmap_new(frame->gauge_area->window, gauge_size_x, gauge_size_y, -1);
	frame->pix_stick = gdk_pixmap_create_from_xpm_d(frame->gauge_area->window,
			NULL, NULL, background2_xpm);

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

void lingot_gui_mainframe_draw_gauge(const LingotMainFrame* frame) {
	GdkGC * gc =
			gtk_widget_get_style(frame->gauge_area)->fg_gc[gtk_widget_get_state(
					frame->gauge_area)];
	GdkWindow* w = frame->gauge_area->window;

	static FLT gauge_size = 90.0;
	FLT max = 1.0;

	// TODO: keep?
	cairo_t* cr = NULL;

	cr = gdk_cairo_create(w);
	gdk_cairo_set_source_pixmap(cr, frame->pix_stick, 0, 0);
	GdkRectangle r;
	r.x = 0;
	r.y = 0;
	r.width = 160;
	r.height = 100;
	gdk_cairo_rectangle(cr, &r);
	cairo_fill(cr);

	// and draws gauge
	FLT normalized_error = frame->gauge->position
			/ frame->conf->scale->max_offset_rounded;
	FLT angle = normalized_error * M_PI / (1.5 * max);

	cairo_set_source_rgb(cr, 0.75, 0.0, 0.12);
	cairo_set_line_width(cr, 1.5);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_move_to(cr, gauge_size_x >> 1, gauge_size_y - 1);
	cairo_line_to(cr, (gauge_size_x >> 1) + gauge_size * sin(angle),
			gauge_size_y - 1 - gauge_size * cos(angle));
	cairo_stroke(cr);

	static char buff1[10];
	static char buff2[10];
	sprintf(buff1,
			(frame->conf->scale->max_offset_rounded > 1.0) ?
					"-%.0f c" : "-%.1f c",
			0.5 * frame->conf->scale->max_offset_rounded);
	sprintf(buff2,
			(frame->conf->scale->max_offset_rounded > 1.0) ?
					"+%.0f c" : "+%.1f c",
			0.5 * frame->conf->scale->max_offset_rounded);

	cairo_text_extents_t te;
	cairo_set_source_rgb(cr, 0.15, 0.15, 0.15);
	cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 10.0);
	cairo_text_extents(cr, buff1, &te);
	cairo_move_to(cr, 0.1 * gauge_size_x - te.width / 2 - te.x_bearing,
			0.8 * gauge_size_y - te.height / 2 - te.y_bearing);
	cairo_show_text(cr, buff1);
	cairo_text_extents(cr, buff2, &te);
	cairo_move_to(cr, 0.85 * gauge_size_x - te.width / 2 - te.x_bearing,
			0.8 * gauge_size_y - te.height / 2 - te.y_bearing);
	cairo_show_text(cr, buff2);

	// TODO: reuse?
	cairo_destroy(cr);
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
	GdkWindow* w = frame->spectrum_area->window;

	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
			spectrum_size_x + 2 * spectrum_x_margin,
			spectrum_size_y + spectrum_bottom_margin + spectrum_top_margin);
	cairo_t *cr = cairo_create(surface);

	// clear all
	int sizex = spectrum_size_x + 2 * spectrum_x_margin;
	int sizey = spectrum_size_y + spectrum_bottom_margin + spectrum_top_margin;

	cairo_set_source_rgb(cr, 0.06, 0.2, 0.06);
	GdkRectangle r;
	r.x = 0;
	r.y = 0;
	r.width = sizex;
	r.height = sizey;
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

	char* current_tone;
	static char error_string[30], freq_string[30];
	unsigned short note_index;

	// draw note, error and frequency labels

	// ignore continuous component
	if (!frame->core->running || isnan(frame->core->freq)
			|| (frame->core->freq < 10.0)) {
		current_tone = "---";
		strcpy(error_string, "e = ---");
		strcpy(freq_string, "f = ---");
	} else {
		FLT error_cents; // do not use, unfiltered
		note_index = lingot_gui_mainframe_get_closest_note_index(
				frame->core->freq, frame->conf->scale,
				frame->conf->root_frequency_error, &error_cents);
		if (note_index == frame->conf->scale->notes) {
			note_index = 0;
		}

		current_tone = frame->conf->scale->note_name[note_index];
		sprintf(error_string, "e = %+2.0f cents", frame->gauge->position);
		sprintf(freq_string, "f = %6.2f Hz",
				lingot_filter_filter_sample(frame->freq_filter,
						frame->core->freq));
	}

	gtk_label_set_text(GTK_LABEL(frame->freq_label), freq_string);
	gtk_label_set_text(GTK_LABEL(frame->error_label), error_string);

	char* markup = g_markup_printf_escaped(
			"<span size=\"xx-large\" weight=\"bold\">%s</span>", current_tone);
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
