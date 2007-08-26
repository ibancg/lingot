//-*- C++ -*-
/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2007  Ibán Cereijo Graña, Jairo Chapela Martínez.
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
#include "lingot-mainframe.h"
#include "lingot-config-dialog.h"
#include "lingot-gauge.h"
#include "lingot-i18n.h"

#include "lingot-background.xpm"
#include "lingot-logo.xpm"

void lingot_mainframe_redraw(LingotMainFrame*);
void lingot_mainframe_put_frequency(LingotMainFrame*);
void lingot_mainframe_draw_gauge(LingotMainFrame*);
void lingot_mainframe_draw_spectrum(LingotMainFrame*);

GdkColor black_color;
GdkColor gauge_color;
GdkColor spectrum_background_color;
GdkColor spectrum_color;
GdkColor grid_color;
GdkColor freq_color;

LingotMainFrame* main_frame_instance= NULL;

void lingot_mainframe_callback_redraw(GtkWidget* w, GdkEventExpose* e,
    LingotMainFrame* frame)
  {
    if (frame->core->running)
      lingot_mainframe_redraw(frame);
  }

void lingot_mainframe_callback_destroy(GtkWidget* w, LingotMainFrame* frame)
  {
    if (frame->core->running)
      {
        lingot_core_stop(frame->core);

        gtk_timeout_remove(frame->visualization_timer_uid);
        gtk_timeout_remove(frame->calculation_timer_uid);
        gtk_timeout_remove(frame->freq_timer_uid);

        gtk_main_quit();
        lingot_mainframe_destroy(frame);
      }
  }

void lingot_mainframe_callback_about(GtkWidget* w, LingotMainFrame* frame)
  {
    if (frame->core->running)
      {
        GtkWidget* about = gtk_about_dialog_new();
        const gchar** authors = malloc(3*sizeof(gchar*));
        const gchar** artists = malloc(2*sizeof(gchar*));

        gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about), "Lingot");
        gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), VERSION);
        gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about), "http://lingot.nongnu.org/");
        gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about), _("Accurate and easy to use musical instrument tuner"));
        authors[0] = "Ibán Cereijo Graña <ibancg@gmail.com>";
        authors[1] = "Jairo Chapela Martínez <jairochapela@gmail.com>";
        authors[2] = NULL;
        artists[0] = "Matthew Blissett (Logo design)";
        artists[1] = NULL;
        gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);
        gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(about), artists);
        gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), "Copyright (C) 2004-2007 Ibán Cereijo Graña\nCopyright (C) 2004-2007 Jairo Chapela Martínez");
        GdkPixbuf* icon = gdk_pixbuf_new_from_xpm_data(lingotlogo);
        gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about), icon);
        //gtk_show_about_dialog(GTK_WINDOW(w), "Lingot", NULL);
        gtk_dialog_run (GTK_DIALOG (about));
        gtk_widget_destroy (about);
        free(authors);
        free(artists);
      }
  }

void lingot_mainframe_callback_config_dialog(GtkWidget* w,
    LingotMainFrame* frame)
  {
    if (frame->core->running)
      {
        if (!frame->config_dialog)
          {
            frame->config_dialog = lingot_config_dialog_new(frame);
          }
      }
    //gtk_widget_activate(gui->config_dialog->win);
  }

/* Callback for visualization */
gboolean lingot_mainframe_callback_visualization(gpointer data)
  {
    //printf("visualization callback!\n");

    unsigned int period;
    LingotMainFrame* frame = (LingotMainFrame*) data;

    if (frame->core->running)
      {
        lingot_mainframe_draw_gauge(frame);

        period = (int) (1.0e3/frame->conf->visualization_rate);
        gtk_timeout_add(period, lingot_mainframe_callback_visualization, frame);
      }

    return 0;
  }

/* Callback for calculation */
gboolean lingot_mainframe_callback_calculation(gpointer data)
  {
    unsigned int period;
    LingotMainFrame* frame = (LingotMainFrame*) data;

    if (frame->core->running)
      {
        lingot_mainframe_draw_spectrum(frame);

        period = (int) (1.0e3/frame->conf->calculation_rate);
        gtk_timeout_add(period, lingot_mainframe_callback_calculation, frame);
      }

    return 0;
  }

/* Callback for frequency calculation */
gboolean lingot_mainframe_callback_frequency(gpointer data)
  {
    unsigned int period;
    LingotMainFrame* frame = (LingotMainFrame*) data;

    if (frame->core->running)
      {
        lingot_mainframe_put_frequency(frame);

        period = (int) (1.0e3/GAUGE_RATE);
        gtk_timeout_add(period, lingot_mainframe_callback_frequency, frame);
      }

    return 0;
  }

LingotMainFrame* lingot_mainframe_new(int argc, char *argv[])
  {
    GtkWidget* vb;
    GtkWidget* tuner_menu;
    GtkWidget* help_menu;
    GtkWidget* options_item;
    GtkWidget* quit_item;
    GtkWidget* about_item;
    GtkAccelGroup *accel_group;
    GtkWidget* menu_bar;
    GtkWidget* tuner_item;
    GtkWidget* help_item;
    GtkWidget* hb;
    GtkWidget* frame1;
    GtkWidget* frame3;
    GtkWidget* frame4;
    GtkWidget* vbinfo;
    LingotMainFrame* frame;
    unsigned int period;

    frame = malloc(sizeof(LingotMainFrame));

    frame->config_dialog = NULL;
    frame->quit = 0;
    frame->pix_stick = NULL;

    frame->conf = lingot_config_new();
    lingot_config_load(frame->conf, CONFIG_FILE_NAME);

    frame->gauge = lingot_gauge_new(frame->conf->vr); // gauge in rest situation	
    frame->core = lingot_core_new(frame->conf);

    // ----- FREQUENCY FILTER CONFIGURATION ------

    // low pass IIR filter.
    FLT freq_filter_a[] =
      { 1.0, -0.5};
    FLT freq_filter_b[] =
      { 0.5};

    frame->freq_filter= lingot_filter_new( 1, 0, freq_filter_a, freq_filter_b );

    // ---------------------------------------------------

    gtk_init(&argc, &argv);
    gtk_set_locale();

    gtk_rc_parse_string("style \"title\""
      "{"
      "font = \"-*-helvetica-bold-r-normal--32-*-*-*-*-*-*-*\""
      "}"
      "widget \"*label_nota\" style \"title\"");

    // creates the window
    frame->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title(GTK_WINDOW (frame->win), _("lingot"));

    gtk_container_set_border_width(GTK_CONTAINER(frame->win), 6);

    // fixed size
    gtk_window_set_resizable(GTK_WINDOW(frame->win), FALSE);

    // tab organization by following container
    vb = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame->win), vb);

    tuner_menu = gtk_menu_new();
    help_menu = gtk_menu_new();

    /* menu elements */
    options_item = gtk_menu_item_new_with_label(_("Options"));
    quit_item = gtk_menu_item_new_with_label(_("Quit"));

    about_item = gtk_menu_item_new_with_label(_("About"));

    /* addition */
    gtk_menu_append( GTK_MENU(tuner_menu), options_item);
    gtk_menu_append( GTK_MENU(tuner_menu), quit_item);
    gtk_menu_append( GTK_MENU(help_menu), about_item);

    accel_group = gtk_accel_group_new ();

    gtk_widget_add_accelerator (options_item, "activate", accel_group, 'o',
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator (quit_item, "activate", accel_group, 'q',
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_window_add_accel_group (GTK_WINDOW (frame->win), accel_group);

    gtk_signal_connect( GTK_OBJECT(options_item), "activate",
        GTK_SIGNAL_FUNC(lingot_mainframe_callback_config_dialog), frame);

    gtk_signal_connect( GTK_OBJECT(quit_item), "activate",
        GTK_SIGNAL_FUNC(lingot_mainframe_callback_destroy), frame);

    gtk_signal_connect( GTK_OBJECT(about_item), "activate",
        GTK_SIGNAL_FUNC(lingot_mainframe_callback_about), frame);

    menu_bar = gtk_menu_bar_new();
    gtk_widget_show(menu_bar );
    gtk_box_pack_start_defaults(GTK_BOX(vb), menu_bar);

    tuner_item = gtk_menu_item_new_with_label(_("Tuner"));
    help_item = gtk_menu_item_new_with_label(_("Help"));
    gtk_menu_item_right_justify( GTK_MENU_ITEM(help_item));

    gtk_menu_item_set_submenu( GTK_MENU_ITEM(tuner_item), tuner_menu);
    gtk_menu_item_set_submenu( GTK_MENU_ITEM(help_item), help_menu);

    gtk_menu_bar_append( GTK_MENU_BAR (menu_bar), tuner_item );
    gtk_menu_bar_append( GTK_MENU_BAR (menu_bar), help_item );

#ifdef GTK12
    gtk_menu_bar_set_shadow_type( GTK_MENU_BAR (menu_bar), GTK_SHADOW_NONE );
#endif

    ////////////////////////////////////////////////////

    // a fixed container to put the two upper frames in fixed positions
    hb = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start_defaults(GTK_BOX(vb), hb);

    // gauge frame
    frame1 = gtk_frame_new(_("Deviation"));
    //  gtk_fixed_put(GTK_FIXED(fix), frame1, 0, 0);
    gtk_box_pack_start_defaults(GTK_BOX(hb), frame1);

    // note frame
    frame3 = gtk_frame_new(_("Note"));
    //gtk_fixed_put(GTK_BOX(hb), frame3, 164, 0);
    gtk_box_pack_start(GTK_BOX(hb), frame3, TRUE, TRUE, 1);

    // spectrum frame at bottom
    frame4 = gtk_frame_new(_("Spectrum"));
    gtk_box_pack_end_defaults(GTK_BOX(vb), frame4);

    // for gauge drawing
    frame->gauge_area = gtk_drawing_area_new();
    gtk_drawing_area_size(GTK_DRAWING_AREA(frame->gauge_area), 160, 100);
    gtk_container_add(GTK_CONTAINER(frame1), frame->gauge_area);

    // for spectrum drawing
    frame->spectrum_area = gtk_drawing_area_new();
    gtk_drawing_area_size(GTK_DRAWING_AREA(frame->spectrum_area), 256, 64);
    gtk_container_add(GTK_CONTAINER(frame4), frame->spectrum_area);

    // for note and frequency displaying
    vbinfo = gtk_vbox_new(FALSE, 0);
    gtk_widget_set_size_request(vbinfo, 90, 100);
    gtk_container_add(GTK_CONTAINER(frame3), vbinfo);

    frame->freq_label = gtk_label_new(_("freq"));
    gtk_box_pack_start_defaults(GTK_BOX(vbinfo), frame->freq_label);

    frame->error_label = gtk_label_new(_("err"));
    gtk_box_pack_end_defaults(GTK_BOX(vbinfo), frame->error_label);

    frame->note_label = gtk_label_new(_("note"));
    gtk_widget_set_name(frame->note_label, "label_nota");
    gtk_box_pack_end_defaults(GTK_BOX(vbinfo), frame->note_label);

    // show all
    gtk_widget_show_all(frame->win);

    // two pixmaps for double buffer in gauge and spectrum drawing 
    // (virtual screen)
    gdk_pixmap_new(frame->gauge_area->window, 160, 100, -1);
    frame->pix_spectrum= gdk_pixmap_new(frame->gauge_area->window, 256, 64, -1);

    // GTK signals
    gtk_signal_connect(GTK_OBJECT(frame->gauge_area), "expose_event",
        (GtkSignalFunc) lingot_mainframe_callback_redraw, frame);
    gtk_signal_connect(GTK_OBJECT(frame->spectrum_area), "expose_event",
        (GtkSignalFunc) lingot_mainframe_callback_redraw, frame);
    gtk_signal_connect(GTK_OBJECT(frame->win), "destroy",
        (GtkSignalFunc) lingot_mainframe_callback_destroy, frame);

    period = (int) (1.0e3/frame->conf->visualization_rate);
    frame->visualization_timer_uid = gtk_timeout_add(period,
        lingot_mainframe_callback_visualization, frame);

    period = (int) (1.0e3/frame->conf->calculation_rate);
    frame->calculation_timer_uid = gtk_timeout_add(period,
        lingot_mainframe_callback_calculation, frame);

    period = (int) (1.0e3/GAUGE_RATE);
    frame->freq_timer_uid = gtk_timeout_add(period,
        lingot_mainframe_callback_frequency, frame);

    gauge_color.red = 0xC000;
    gauge_color.green = 0x0000;
    gauge_color.blue = 0x2000;

    spectrum_background_color.red = 0x1111;
    spectrum_background_color.green = 0x3333;
    spectrum_background_color.blue = 0x1111;

    spectrum_color.red = 0x2222;
    spectrum_color.green = 0xFFFF;
    spectrum_color.blue = 0x2222;

    grid_color.red = 0x7000;
    grid_color.green = 0x7000;
    grid_color.blue = 0x7000;

    freq_color.red = 0xFFFF;
    freq_color.green = 0x2222;
    freq_color.blue = 0x2222;

    gdk_color_alloc(gdk_colormap_get_system(), &gauge_color);
    gdk_color_alloc(gdk_colormap_get_system(), &spectrum_color);
    gdk_color_alloc(gdk_colormap_get_system(), &spectrum_background_color);
    gdk_color_alloc(gdk_colormap_get_system(), &grid_color);
    gdk_color_alloc(gdk_colormap_get_system(), &freq_color);
    gdk_color_black(gdk_colormap_get_system(), &black_color);

    main_frame_instance = frame;
    return frame;
  }

void lingot_mainframe_destroy(LingotMainFrame* frame)
  {
    lingot_gauge_destroy(frame->gauge);
    lingot_core_destroy(frame->core);
    lingot_filter_destroy(frame->freq_filter);
    free(frame->conf);
    if (frame->config_dialog)
      lingot_config_dialog_destroy(frame->config_dialog);

    /*
     gdk_color_free(&gauge_color);
     gdk_color_free(&spectrum_color);
     gdk_color_free(&spectrum_background_color);
     gdk_color_free(&grid_color);
     gdk_color_free(&freq_color);
     */

    free(frame);
  }

void lingot_mainframe_run(LingotMainFrame* frame)
  {

    lingot_core_start(frame->core);
    gtk_main();
  }
// ---------------------------------------------------------------------------

void lingot_mainframe_redraw(LingotMainFrame* frame)
  {
    lingot_mainframe_draw_gauge(frame);
    lingot_mainframe_draw_spectrum(frame);
  }

// ---------------------------------------------------------------------------

void lingot_mainframe_draw_gauge(LingotMainFrame* frame)
  {
    GdkGC* gc = frame->gauge_area->style->fg_gc[frame->gauge_area->state];
    GdkWindow* w = frame->gauge_area->window;
    GdkGCValues gv;

    gdk_gc_get_values(gc, &gv);

    FLT max = 1.0;

    // draws background
    if (!frame->pix_stick)
      {
        frame->pix_stick = gdk_pixmap_create_from_xpm_d(
            frame->gauge_area->window, NULL, 
            NULL, background2_xpm);
      }
    gdk_draw_pixmap(frame->gauge_area->window, gc, frame->pix_stick, 0, 0, 0, 0, 160, 100);

    // and draws gauge
    gdk_gc_set_foreground(gc, &gauge_color);

    gdk_draw_line(w, gc, 80, 99, 80 + (int)rint(90.0*sin(frame->gauge->position
        *M_PI/(1.5*max))), 99 - (int)rint(90.0*cos(frame->gauge->position*M_PI
        /(1.5*max))));

    // black edge.  
    gdk_gc_set_foreground(gc, &black_color);
    gdk_draw_rectangle(w, gc, FALSE, 0, 0, 159, 99);

    gdk_draw_pixmap(frame->gauge_area->window, gc, w, 0, 0, 0, 0, 160, 100);
    gdk_flush();
  }

void lingot_mainframe_draw_spectrum(LingotMainFrame* frame)
  {
    register unsigned int i;
    GdkGC* gc = frame->spectrum_area->style->fg_gc[frame->spectrum_area->state];
    GdkWindow* w = frame->pix_spectrum; //spectrum->window;
    GdkGCValues gv;
    gdk_gc_get_values(gc, &gv);

    // clear all
    gdk_gc_set_foreground(gc, &spectrum_background_color);

    gdk_draw_rectangle(w, gc, TRUE, 0, 0, 256, 64);

    // grid
    int Nlx = 9;
    int Nly = 3;

    gdk_gc_set_foreground(gc, &grid_color);

    for (i = 0; i <= Nly; i++)
      gdk_draw_line(w, gc, 0, i*63/Nly, 255, i*63/Nly);
    for (i = 0; i <= Nlx; i++)
      gdk_draw_line(w, gc, i*255/Nlx, 0, i*255/Nlx, 63);

    gdk_gc_set_foreground(gc, &spectrum_color);

# define PLOT_GAIN  8

    // spectrum drawing.
    for (i = 0; (i < frame->conf->fft_size) && (i < 256); i++)
      {
        int j = (frame->core->X[i] > 1.0) ? (int) (64 - PLOT_GAIN*log10(frame->core->X[i])) : 64; // dB.
        gdk_draw_line(w, gc, i, 63, i, j);
      }

    if (frame->core->freq != 0.0)
      {

        // fundamental frequency mark with a red point. 
        gdk_gc_set_foreground(gc, &freq_color);

        // index of closest sample to fundamental frequency.
        int i = (int) rint(frame->core->freq*frame->conf->fft_size
            *frame->conf->oversampling/frame->conf->sample_rate);
        int j = (frame->core->X[i] > 1.0) ? (int) (64 - PLOT_GAIN*log10(frame->core->X[i])) : 64; // dB.
        gdk_draw_rectangle(w, gc, TRUE, i-1, j-1, 3, 3);
      }

# undef  PLOT_GAIN

    gdk_gc_set_foreground(gc, &black_color);

    gdk_draw_pixmap(frame->spectrum_area->window, gc, w, 0, 0, 0, 0, 256, 64);
    gdk_flush();
  }

void lingot_mainframe_put_frequency(LingotMainFrame* frame)
  {
    // note indexation by semitone.
    static char* note_string[] =
      { "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#" };
    const FLT Log2 = log(2.0);

    FLT fret_f;
    int fret;
    char* current_note;

    static char error_string[30], freq_string[30];

    if (frame->core->freq < 10.0)
      {

        current_note = "---";
        strcpy(error_string, "e = ---");
        strcpy(freq_string, "f = ---");
        lingot_gauge_compute(frame->gauge, frame->conf->vr);
        //error = conf->VRP;

      }
    else
      {

        // bring up some octaves to avoid negative frets.
        fret_f = log(frame->core->freq/frame->conf->root_frequency)/Log2*12.0
            + 12e2;

        lingot_gauge_compute(frame->gauge, fret_f - rint(fret_f));
        //error = fret_f - rint(fret_f);

        fret = ((int) rint(fret_f)) % 12;

        current_note = note_string[fret];
        sprintf(error_string, "e = %+4.2f%%", frame->gauge->position*100.0);
        sprintf(freq_string, "f = %6.2f Hz", lingot_filter_filter_sample(
            frame->freq_filter, frame->core->freq));
      }

    gtk_label_set_text(GTK_LABEL(frame->freq_label), freq_string);
    gtk_label_set_text(GTK_LABEL(frame->error_label), error_string);
    char labeltext_current_note[100];
    sprintf(labeltext_current_note, "<big><b>%s</b></big>", current_note);
    gtk_label_set_markup(GTK_LABEL(frame->note_label), labeltext_current_note);
  }

void lingot_mainframe_change_config(LingotMainFrame* frame, LingotConfig* conf)
  {
    lingot_core_stop(frame->core);
    lingot_core_destroy(frame->core);

    // dup.
    *frame->conf = *conf;

    frame->core = lingot_core_new(frame->conf);
    lingot_core_start(frame->core);
  }
