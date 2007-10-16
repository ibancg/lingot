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
GdkColor noise_threshold_color;
GdkColor grid_color;
GdkColor freq_color;

// sizes

int gauge_size_x = 160;
int gauge_size_y = 100;

int spectrum_size_y = 64;

// spectrum area margins 
int spectrum_bottom_margin = 16;
int spectrum_top_margin = 12;
int spectrum_x_margin = 15;

GtkWidget* view_spectrum_item;
GtkWidget* spectrum_frame;

PangoFontDescription* spectrum_legend_font_desc;

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
        static const gchar* authors[] = {
        	"Ibán Cereijo Graña <ibancg@gmail.com>",
        	"Jairo Chapela Martínez <jairochapela@gmail.com>",
        	NULL
        };
        
        const gchar* artists[] = { 
        	"Matthew Blissett (Logo design)",
        	NULL
        };

        gtk_show_about_dialog (NULL,
			"name",		"Lingot",
			"version",	VERSION,
			"copyright",	"\xC2\xA9 2004-2007 Ibán Cereijo Graña\n\xC2\xA9 2004-2007 Jairo Chapela Martínez",
			"comments",	_("Accurate and easy to use musical instrument tuner"),
			"authors",	authors,
			"artists", artists,
			"translator-credits",	_("translator-credits"),
			"logo-icon-name", "lingot-logo",
			NULL);
      }
  }

void lingot_mainframe_callback_view_spectrum(GtkWidget* w,
    LingotMainFrame* frame)
  {
    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(view_spectrum_item)))
      {
        gtk_widget_hide(spectrum_frame);
      }
    else
      {
        gtk_widget_show(spectrum_frame);
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
        else
          {
            // TODO: activate config dialog
          }
      }
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

        period = 1000/frame->conf->visualization_rate;
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

        period = 1000/frame->conf->calculation_rate;
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

        period = 1000/GAUGE_RATE;
        gtk_timeout_add(period, lingot_mainframe_callback_frequency, frame);
      }

    return 0;
  }

void lingot_mainframe_color(GdkColor* color, int red, int green, int blue)
  {
    color->red = red;
    color->green = green;
    color->blue = blue;
  }

LingotMainFrame* lingot_mainframe_new(int argc, char *argv[])
  {
    GtkWidget* vertical_box;
    GtkWidget* horizontal_box;
    GtkWidget* file_menu;
    GtkWidget* edit_menu;
    GtkWidget* help_menu;
    GtkWidget* preferences_item;
    GtkWidget* quit_item;
    GtkWidget* about_item;
    GtkAccelGroup* accel_group;
    GtkWidget* menu_bar;
    GtkWidget* file_item;
    GtkWidget* edit_item;
    GtkWidget* help_item;
    GtkWidget* frame1;
    GtkWidget* frame3;
    GtkWidget* vbinfo;
    LingotMainFrame* frame;
    unsigned int period;

    frame = malloc(sizeof(LingotMainFrame));

    frame->config_dialog = NULL;
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
    gtk_window_set_position(GTK_WINDOW(frame->win), GTK_WIN_POS_CENTER);

    GdkPixbuf* logo = gdk_pixbuf_new_from_xpm_data(lingotlogo);
    gtk_icon_theme_add_builtin_icon("lingot-logo", 64, logo);
    
    gtk_window_set_icon(GTK_WINDOW(frame->win), logo);

//    g_object_unref(logo);

    gtk_window_set_title(GTK_WINDOW (frame->win), _("lingot"));

    gtk_container_set_border_width(GTK_CONTAINER(frame->win), 6);

    // fixed size
    gtk_window_set_resizable(GTK_WINDOW(frame->win), FALSE);
    // tab organization by following container
    vertical_box = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame->win), vertical_box);

    file_menu = gtk_menu_new();
    edit_menu = gtk_menu_new();
    help_menu = gtk_menu_new();
    GtkWidget* view_menu = gtk_menu_new();

    accel_group = gtk_accel_group_new ();

    /* menu elements */
    preferences_item
        = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, accel_group);
    quit_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group);
    about_item
        = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, accel_group);
    view_spectrum_item= gtk_check_menu_item_new_with_label(_("Spectrum"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(view_spectrum_item), TRUE);

    /* addition */
    gtk_menu_append( GTK_MENU(file_menu), quit_item);
    gtk_menu_append( GTK_MENU(edit_menu), preferences_item);
    gtk_menu_append( GTK_MENU(view_menu), view_spectrum_item);
    gtk_menu_append( GTK_MENU(help_menu), about_item);

    gtk_widget_add_accelerator (preferences_item, "activate", accel_group, 'o',
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator (quit_item, "activate", accel_group, 'q',
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_window_add_accel_group (GTK_WINDOW (frame->win), accel_group);

    gtk_signal_connect( GTK_OBJECT(preferences_item), "activate",
        GTK_SIGNAL_FUNC(lingot_mainframe_callback_config_dialog), frame);

    gtk_signal_connect( GTK_OBJECT(quit_item), "activate",
        GTK_SIGNAL_FUNC(lingot_mainframe_callback_destroy), frame);

    gtk_signal_connect( GTK_OBJECT(about_item), "activate",
        GTK_SIGNAL_FUNC(lingot_mainframe_callback_about), frame);

    gtk_signal_connect( GTK_OBJECT(view_spectrum_item), "activate",
        GTK_SIGNAL_FUNC(lingot_mainframe_callback_view_spectrum), frame);

    menu_bar = gtk_menu_bar_new();
    gtk_widget_show(menu_bar);
    gtk_box_pack_start_defaults(GTK_BOX(vertical_box), menu_bar);

    file_item = gtk_menu_item_new_with_mnemonic(_("_File"));
    edit_item = gtk_menu_item_new_with_mnemonic(_("_Edit"));
    GtkWidget* view_item = gtk_menu_item_new_with_mnemonic(_("_View"));
    help_item = gtk_menu_item_new_with_mnemonic(_("_Help"));
    gtk_menu_item_right_justify( GTK_MENU_ITEM(help_item));

    gtk_menu_item_set_submenu( GTK_MENU_ITEM(file_item), file_menu);
    gtk_menu_item_set_submenu( GTK_MENU_ITEM(edit_item), edit_menu);
    gtk_menu_item_set_submenu( GTK_MENU_ITEM(view_item), view_menu);
    gtk_menu_item_set_submenu( GTK_MENU_ITEM(help_item), help_menu);

    gtk_menu_bar_append( GTK_MENU_BAR (menu_bar), file_item );
    gtk_menu_bar_append( GTK_MENU_BAR (menu_bar), edit_item );
    gtk_menu_bar_append( GTK_MENU_BAR (menu_bar), view_item );
    gtk_menu_bar_append( GTK_MENU_BAR (menu_bar), help_item );

#ifdef GTK12
    gtk_menu_bar_set_shadow_type( GTK_MENU_BAR (menu_bar), GTK_SHADOW_NONE );
#endif

    ////////////////////////////////////////////////////

    // a fixed container to put the two upper frames in fixed positions
    horizontal_box = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start_defaults(GTK_BOX(vertical_box), horizontal_box);

    // gauge frame
    frame1 = gtk_frame_new(_("Deviation"));
    //  gtk_fixed_put(GTK_FIXED(fix), frame1, 0, 0);
    gtk_box_pack_start(GTK_BOX(horizontal_box), frame1, FALSE, FALSE, 0);

    // note frame
    frame3 = gtk_frame_new(_("Note"));
    //gtk_fixed_put(GTK_BOX(hb), frame3, 164, 0);
    gtk_box_pack_start(GTK_BOX(horizontal_box), frame3, TRUE, TRUE, 0);

    // spectrum frame at bottom
    spectrum_frame = gtk_frame_new(_("Spectrum"));
    gtk_box_pack_end_defaults(GTK_BOX(vertical_box), spectrum_frame);

    // for gauge drawing
    frame->gauge_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(GTK_WIDGET(frame->gauge_area), gauge_size_x, gauge_size_y);
    gtk_container_add(GTK_CONTAINER(frame1), frame->gauge_area);

    // for spectrum drawing
    GtkObject* adjust = gtk_adjustment_new (0.0, 0.0, ((frame->conf->fft_size
        > 256) ? 0.5 : 1.0)*frame->conf->sample_rate, 1.0, 100.0, 100.0);
    frame->spectrum_scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(GTK_ADJUSTMENT(adjust), NULL));
    frame->spectrum_area = gtk_drawing_area_new();

    int
        x = ((frame->conf->fft_size > 256) ? (frame->conf->fft_size >> 1) : 256)
            + 2*spectrum_x_margin;
    int y = spectrum_size_y + spectrum_bottom_margin + spectrum_top_margin;

    gtk_widget_set_size_request(GTK_WIDGET(frame->spectrum_area), x, y);
    gtk_scrolled_window_set_policy(frame->spectrum_scroll,
        (frame->conf->fft_size > 512) ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER,
        GTK_POLICY_NEVER);
    gtk_scrolled_window_add_with_viewport(frame->spectrum_scroll,
        frame->spectrum_area);
    gtk_widget_set_size_request(GTK_WIDGET(frame->spectrum_scroll), 260 + 2*spectrum_x_margin, spectrum_size_y + spectrum_bottom_margin + spectrum_top_margin + 4 + ((frame->conf->fft_size > 512) ? 16 : 0));
    gtk_container_add(GTK_CONTAINER(spectrum_frame), GTK_WIDGET(frame->spectrum_scroll));

    // for note and frequency displaying
    vbinfo = gtk_vbox_new(FALSE, 0);
    gtk_widget_set_size_request(GTK_WIDGET(vbinfo), 96 + 2*spectrum_x_margin, gauge_size_y);
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
    gdk_pixmap_new(frame->gauge_area->window, gauge_size_x, gauge_size_y, -1);
    frame->pix_spectrum= gdk_pixmap_new(frame->spectrum_area->window, x, y, -1);

    // GTK signals
    gtk_signal_connect(GTK_OBJECT(frame->gauge_area), "expose_event",
        GTK_SIGNAL_FUNC(lingot_mainframe_callback_redraw), frame);
    gtk_signal_connect(GTK_OBJECT(frame->spectrum_area), "expose_event",
        GTK_SIGNAL_FUNC(lingot_mainframe_callback_redraw), frame);
    gtk_signal_connect(GTK_OBJECT(frame->win), "destroy",
        GTK_SIGNAL_FUNC(lingot_mainframe_callback_destroy), frame);

    period = 1000/frame->conf->visualization_rate;
    frame->visualization_timer_uid = gtk_timeout_add(period,
        lingot_mainframe_callback_visualization, frame);

    period = 1000/frame->conf->calculation_rate;
    frame->calculation_timer_uid = gtk_timeout_add(period,
        lingot_mainframe_callback_calculation, frame);

    period = 1000/GAUGE_RATE;
    frame->freq_timer_uid = gtk_timeout_add(period,
        lingot_mainframe_callback_frequency, frame);

    lingot_mainframe_color(&gauge_color, 0xC000, 0x0000, 0x2000);
    lingot_mainframe_color(&spectrum_background_color, 0x1111, 0x3333, 0x1111);
    lingot_mainframe_color(&spectrum_color, 0x2222, 0xEEEE, 0x2222);
    lingot_mainframe_color(&noise_threshold_color, 0x8888, 0x8888, 0x2222);
    lingot_mainframe_color(&grid_color, 0x9000, 0x9000, 0x9000);
    lingot_mainframe_color(&freq_color, 0xFFFF, 0x2222, 0x2222);

    gdk_color_alloc(gdk_colormap_get_system(), &gauge_color);
    gdk_color_alloc(gdk_colormap_get_system(), &spectrum_color);
    gdk_color_alloc(gdk_colormap_get_system(), &spectrum_background_color);
    gdk_color_alloc(gdk_colormap_get_system(), &noise_threshold_color);
    gdk_color_alloc(gdk_colormap_get_system(), &grid_color);
    gdk_color_alloc(gdk_colormap_get_system(), &freq_color);
    gdk_color_black(gdk_colormap_get_system(), &black_color);

    spectrum_legend_font_desc
        =pango_font_description_from_string ("Helvetica Plain 7");

    return frame;
  }

void lingot_mainframe_destroy(LingotMainFrame* frame)
  {
    lingot_gauge_destroy(frame->gauge);
    lingot_core_destroy(frame->core);
    lingot_filter_destroy(frame->freq_filter);
    lingot_config_destroy(frame->conf);
    if (frame->config_dialog)
      lingot_config_dialog_destroy(frame->config_dialog);

    pango_font_description_free(spectrum_legend_font_desc);

    gtk_widget_destroy(frame->freq_label);
    gtk_widget_destroy(frame->error_label);
    gtk_widget_destroy(frame->note_label);

    gtk_widget_destroy(frame->win);

    // gdk_color_free(&gauge_color);
    // gdk_color_free(&spectrum_color);
    // gdk_color_free(&spectrum_background_color);
    // gdk_color_free(&noise_threshold_color);
    // gdk_color_free(&grid_color);
    // gdk_color_free(&freq_color);

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

    static FLT gauge_size = 90.0;
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

    gdk_draw_line(w, gc, gauge_size_x >> 1, gauge_size_y - 1,
        (gauge_size_x>> 1) + (int)rint(gauge_size*sin(frame->gauge->position
            *M_PI/(1.5*max))), gauge_size_y - 1- (int)rint(gauge_size
            *cos(frame->gauge->position*M_PI/(1.5*max))));

    // black edge.  
    gdk_gc_set_foreground(gc, &black_color);
    gdk_draw_rectangle(w, gc, FALSE, 0, 0, gauge_size_x - 1, gauge_size_y - 1);

    gdk_draw_pixmap(frame->gauge_area->window, gc, w, 0, 0, 0, 0, gauge_size_x, gauge_size_y);
    gdk_flush();
  }

void lingot_mainframe_draw_spectrum(LingotMainFrame* frame)
  {
    PangoLayout* layout;

    int
        spectrum_size_x = ((frame->conf->fft_size > 256) ? (frame->conf->fft_size
            >> 1)
            : 256);

    // minimum grid size in pixels
    static int minimum_grid_width = 50;

    /* scale factors to draw the grid. We will choose the smaller factor that
     respects the minimum_grid_width */
    static double scales[] =
      { 0.01, 0.05, 0.1, 0.2, 0.5, 1, 2, 4, 11, 22, -1.0 };

    // spectrum drawing mode
    static gboolean spectrum_drawing_filled = TRUE;

    // grid division in dB
    static FLT grid_db_height = 25;

    register unsigned int i;
    int j;
    int old_j;

    GdkGC* gc = frame->spectrum_area->style->fg_gc[frame->spectrum_area->state];
    GdkWindow* window = frame->pix_spectrum; //spectrum->window;
    GdkGCValues gv;
    gdk_gc_get_values(gc, &gv);

    // clear all
    gdk_gc_set_foreground(gc, &spectrum_background_color);
    gdk_draw_rectangle(window, gc, TRUE, 0, 0, spectrum_size_x + 2
        *spectrum_x_margin, spectrum_size_y + spectrum_bottom_margin
        + spectrum_top_margin);

    gdk_gc_set_foreground(gc, &grid_color);

    gdk_draw_line(window, gc, spectrum_x_margin, spectrum_size_y
        + spectrum_top_margin, spectrum_x_margin + spectrum_size_x,
        spectrum_size_y + spectrum_top_margin);

    // choose scale factor
    for (i = 0; scales[i] > 0.0; i++)
      {
        if ((1e3*scales[i]*frame->conf->fft_size*frame->conf->oversampling
            /frame->conf->sample_rate) > minimum_grid_width)
          break;
      }

    if (scales[i] < 0.0)
      i--;

    FLT scale = scales[i];

    int grid_width = 1e3*scales[i]*frame->conf->fft_size
        *frame->conf->oversampling/frame->conf->sample_rate;

    char buff[10];

    FLT freq = 0.0;
    for (i = 0; i <= spectrum_size_x; i += grid_width)
      {
        gdk_draw_line(window, gc, spectrum_x_margin + i, spectrum_top_margin,
            spectrum_x_margin + i, spectrum_size_y + spectrum_top_margin + 3);

        if (freq == 0.0)
          {
            sprintf(buff, "0 Hz");
          }
        else if (floor(freq) == freq)
          sprintf(buff, "%0.0f kHz", freq);
        else if (floor(10*freq) == 10*freq)
          {
            if (freq <= 1000.0)
              sprintf(buff, "%0.0f Hz", 1e3*freq);
            else
              sprintf(buff, "%0.1f kHz", freq);
          }
        else
          {
            if (freq <= 100.0)
              sprintf(buff, "%0.0f Hz", 1e3*freq);
            else
              sprintf(buff, "%0.2f kHz", freq);
          }

        layout = gtk_widget_create_pango_layout(frame->spectrum_area, buff);
        pango_layout_set_font_description (layout, spectrum_legend_font_desc);
        gdk_draw_layout(window, gc, spectrum_x_margin - 8 + i,
            spectrum_size_y + spectrum_top_margin + 5, layout);
        freq += scale;
      }

# define PLOT_GAIN  8

    sprintf(buff, "dB");

    layout = gtk_widget_create_pango_layout(frame->spectrum_area, buff);
    pango_layout_set_font_description (layout, spectrum_legend_font_desc);
    gdk_draw_layout(window, gc, spectrum_x_margin - 6, 2, layout);
    
    int grid_height = (int) (PLOT_GAIN*log10(pow(10.0, grid_db_height/10.0))); // dB.
    j = 0;
    for (i = 0; i <= spectrum_size_y; i += grid_height)
      {
        if (j == 0)
          sprintf(buff, " %i", j);
        else
         sprintf(buff, "%i", j);

        layout = gtk_widget_create_pango_layout(frame->spectrum_area, buff);
        pango_layout_set_font_description (layout, spectrum_legend_font_desc);
        gdk_draw_layout(window, gc, 2, spectrum_size_y
            + spectrum_top_margin - i - 5, layout);
        
        gdk_draw_line(window, gc, spectrum_x_margin, spectrum_size_y
            + spectrum_top_margin - i, spectrum_x_margin + spectrum_size_x,
            spectrum_size_y + spectrum_top_margin - i);
        
        j += grid_db_height;
      }

    gdk_gc_set_foreground(gc, &noise_threshold_color);

    // noise threshold drawing.
    j = -1;
    for (i = 0; (i < frame->conf->fft_size) && (i < spectrum_size_x); i++)
      {
        if ((i % 10) > 5)
          continue;

        FLT w = 2*M_PI*i/frame->conf->fft_size;
        //FLT noise = pow(10.0, (frame->conf->noise_threshold_db*(1.0 - 0.9*w/M_PI))/10.0);
        //FLT noise = lingot_signal_get_noise_threshold(frame->conf, w);
        FLT noise = frame->conf->noise_threshold_nu;
        old_j = j;
        j = (noise > 1.0) ? (int) (PLOT_GAIN*log10(noise)) : 0; // dB.
        if ((old_j >= 0) && (old_j < spectrum_size_y)&& (j >= 0)&& (j
            < spectrum_size_y))
          gdk_draw_line(window, gc, spectrum_x_margin + i - 1, spectrum_size_y
              + spectrum_top_margin - old_j, spectrum_x_margin + i,
              spectrum_size_y + spectrum_top_margin - j);
      }

    gdk_gc_set_foreground(gc, &spectrum_color);

    // spectrum drawing.
    j = -1;
    for (i = 0; (i < frame->conf->fft_size) && (i < spectrum_size_x); i++)
      {
        old_j = j;
        j = (frame->core->X[i] > 1.0) ? (int) (PLOT_GAIN*log10(frame->core->X[i])) : 0; // dB.
        if (spectrum_drawing_filled)
          {
            if (j < spectrum_size_y)
              gdk_draw_line(window, gc, spectrum_x_margin + i, spectrum_size_y
                  + spectrum_top_margin - 1, spectrum_x_margin + i,
                  spectrum_top_margin
                      + ((j< spectrum_size_y) ? (spectrum_size_y - j) : 0));
          }
        else if ((old_j >= 0) && (old_j < spectrum_size_y)&& (j >= 0)&& (j
            < spectrum_size_y))
          gdk_draw_line(window, gc, spectrum_x_margin + i - 1, spectrum_size_y
              + spectrum_top_margin - old_j, spectrum_x_margin + i,
              spectrum_size_y + spectrum_top_margin - j);
      }

    if (frame->core->freq != 0.0)
      {

        // fundamental frequency mark with a red point. 
        gdk_gc_set_foreground(gc, &freq_color);

        // index of closest sample to fundamental frequency.
        i = (int) rint(frame->core->freq*frame->conf->fft_size
            *frame->conf->oversampling/frame->conf->sample_rate);
        j = (frame->core->X[i] > 1.0) ? (int) (PLOT_GAIN*log10(frame->core->X[i])) : 0; // dB.
        if (j < spectrum_size_y - 1)
          gdk_draw_rectangle(window, gc, TRUE, spectrum_x_margin + i-1,
              spectrum_size_y + spectrum_top_margin - j - 1, 3, 3);
      }

# undef  PLOT_GAIN

    gdk_gc_set_foreground(gc, &black_color);

    gdk_draw_pixmap(frame->spectrum_area->window, gc, window, 0, 0, 0, 0, spectrum_size_x + 2*spectrum_x_margin, spectrum_size_y + spectrum_bottom_margin + spectrum_top_margin);
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
    char labeltext_current_note[100];

    static char error_string[30], freq_string[30];

	LingotCore* core1 = frame->core;

	// ignore continuous component
	// since 1.7.5: core is dumping NANs on some config changes, here we detect
	// it, but we don't know why the core is generating them. 
    if (isnan(frame->core->freq) || (frame->core->freq < 10.0))
      {
        current_note = "---";
        strcpy(error_string, "e = ---");
        strcpy(freq_string, "f = ---");
        lingot_gauge_compute(frame->gauge, frame->conf->vr);
      }
    else
      {
        // bring up some octaves to avoid negative frets.
        fret_f = log(frame->core->freq/frame->conf->root_frequency)/Log2*12.0
            + 12e2;

        lingot_gauge_compute(frame->gauge, fret_f - rint(fret_f));
        fret = ((int) rint(fret_f)) % 12;

        current_note = note_string[fret];
        sprintf(error_string, "e = %+4.2f%%", frame->gauge->position*100.0);
        sprintf(freq_string, "f = %6.2f Hz", lingot_filter_filter_sample(
            frame->freq_filter, frame->core->freq));
      }

    gtk_label_set_text(GTK_LABEL(frame->freq_label), freq_string);
    gtk_label_set_text(GTK_LABEL(frame->error_label), error_string);
    sprintf(labeltext_current_note,
        "<big><big><big><big><b>%s</b></big></big></big></big>", current_note);
    gtk_label_set_markup(GTK_LABEL(frame->note_label), labeltext_current_note);
  }

void lingot_mainframe_change_config(LingotMainFrame* frame, LingotConfig* conf)
  {
    lingot_core_stop(frame->core);
    lingot_core_destroy(frame->core);

    // dup.
    *frame->conf = *conf;

    // resize spectrum area
    g_object_unref(frame->pix_spectrum);

    int
        x = ((frame->conf->fft_size > 256) ? (frame->conf->fft_size >> 1) : 256)
            + 2*spectrum_x_margin;
    int y = spectrum_size_y + spectrum_top_margin + spectrum_bottom_margin;
    gtk_widget_set_size_request(GTK_WIDGET(frame->spectrum_area), x, y);
    frame->pix_spectrum= gdk_pixmap_new(frame->spectrum_area->window, x, y, -1);

    gtk_scrolled_window_set_policy(frame->spectrum_scroll,
        (frame->conf->fft_size > 512) ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER,
        GTK_POLICY_NEVER);
    gtk_widget_set_size_request(GTK_WIDGET(frame->spectrum_scroll), 260 + 2*spectrum_x_margin, spectrum_size_y + spectrum_bottom_margin + spectrum_top_margin + 4 + ((frame->conf->fft_size > 512) ? 16 : 0));

    frame->core = lingot_core_new(frame->conf);
    lingot_core_start(frame->core);
  }
