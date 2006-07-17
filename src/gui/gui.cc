//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004, 2005  IbÃ¡n Cereijo GraÃ±a, Jairo Chapela MartÃ­nez.

  This file is part of lingot.

  lingot is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  lingot is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with lingot; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "defs.h"

#include "filter.h"
#include "gui.h"
#include "dialog_config.h"
#include "events.h"

#include "../background.xpm"

void callbackRedraw(GtkWidget *w, GdkEventExpose *e, void *data)
{
  ((GUI*)data)->redraw();
}

void callbackDestroy(GtkWidget *w, void *data)
{
/*  int tout_handle = 0;

  gtk_timeout_remove(tout_handle);
  ((GUI*)data)->quit = true;*/
  gtk_main_quit();
}

void callbackAbout(GtkWidget *w, void *data)
{
  quick_message(gettext("about lingot"), 
		gettext("\nlingot " VERSION ", (c) 2006\n"
			"\n"
			"Ibán Cereijo Graña <ibancg@gmail.com>\n"
			"Jairo Chapela Martínez <jairochapela@terra.es>\n\n"));
}

GUI::GUI() : Core()
{

  dc = NULL;
  quit = false;
  pix_stick = NULL;

//   cogerConfig(&conf);

  gauge_value = conf.VRP; // gauge in rest situation

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
  // acceleration of gauge position 's(t)' linealy depends on the difference
  // respect to the input stimulus 'e(t)' (destination position). Inserting
  // a friction coefficient 'q', acceleration proportionaly diminish with
  // velocity (typical friction in mechanics). 'k' is the adaptation constant,
  // and depends on the gauge mass.
  //
  // with the following derivatives approximation (valid for high sample rate):
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

  FLT k = 60; // adaptation constant.
  FLT q = 6;  // friction coefficient.

  FLT gauge_filter_a[] = {
    k + GAUGE_RATE*(q + GAUGE_RATE),
    -GAUGE_RATE*(q + 2.0*GAUGE_RATE),  
      GAUGE_RATE*GAUGE_RATE };
  FLT gauge_filter_b[] = { k };

  gauge_filter = new Filter( 2, 0, gauge_filter_a, gauge_filter_b );

  // ----- FREQUENCY FILTER CONFIGURATION ------

  // low pass IIR filter.
  FLT freq_filter_a[] = { 1.0, -0.5 };
  FLT freq_filter_b[] = { 0.5 };
  
  freq_filter = new Filter( 1, 0, freq_filter_a, freq_filter_b );
    
  // ---------------------------------------------------

  gtk_rc_parse_string(
 		      "style \"title\""
 		      "{"
		      "font = \"-*-helvetica-bold-r-normal--32-*-*-*-*-*-*-*\""
 		      "}"
		      "widget \"*label_nota\" style \"title\""
 		      );


  // creates the window
  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (win), gettext("lingot"));

  gtk_container_set_border_width(GTK_CONTAINER(win), 6);

  // fixed size
  gtk_window_set_policy(GTK_WINDOW(win), FALSE, TRUE, FALSE);  

  // tab organization by following container
  vb = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(win), vb);


  ////////////////////////////////////////////////////
  /*  GtkItemFactory *item_factory;
      GtkAccelGroup *accel_group;
      gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
      accel_group = gtk_accel_group_new();
      item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);
      gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);
      gtk_window_add_accel_group(GTK_WINDOW(win), accel_group);
      menu = gtk_item_factory_get_widget(item_factory, "<main>");
      gtk_menu_bar_set_shadow_type(GTK_MENU_BAR(menu), GTK_SHADOW_NONE);
      // lo primero que metemos en la caja vertical, es la barra de menÃº
      gtk_box_pack_start_defaults(GTK_BOX(vb), menu);*/

  /* menu */

  GtkWidget* tuner_menu = gtk_menu_new();
  GtkWidget* help_menu = gtk_menu_new();

  
  /* menu elements */
  GtkWidget* options_item = gtk_menu_item_new_with_label(gettext("Options"));
  GtkWidget* quit_item = gtk_menu_item_new_with_label(gettext("Quit"));

  GtkWidget* about_item = gtk_menu_item_new_with_label(gettext("About"));


  /* addition */
  gtk_menu_append( GTK_MENU(tuner_menu), options_item);
  gtk_menu_append( GTK_MENU(tuner_menu), quit_item);
  gtk_menu_append( GTK_MENU(help_menu), about_item);


  GtkAccelGroup *accel_group;

  accel_group = gtk_accel_group_new ();

  gtk_widget_add_accelerator (options_item, "activate", accel_group,
                              'o', GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (quit_item, "activate", accel_group,
                              'q', GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

  gtk_window_add_accel_group (GTK_WINDOW (win), accel_group);

  gtk_signal_connect( GTK_OBJECT(options_item), "activate",
		      GTK_SIGNAL_FUNC(dialog_config_cb), this);

  gtk_signal_connect( GTK_OBJECT(quit_item), "activate",
		      GTK_SIGNAL_FUNC(callbackDestroy), this);

  gtk_signal_connect( GTK_OBJECT(about_item), "activate",
		      GTK_SIGNAL_FUNC(callbackAbout), this);
  
  GtkWidget* menu_bar = gtk_menu_bar_new();
  gtk_widget_show( menu_bar );
  gtk_box_pack_start_defaults(GTK_BOX(vb), menu_bar);

  GtkWidget* tuner_item = gtk_menu_item_new_with_label(gettext("Tuner"));
  GtkWidget* help_item  = gtk_menu_item_new_with_label(gettext("Help"));
  gtk_menu_item_right_justify( GTK_MENU_ITEM(help_item));
  
  gtk_menu_item_set_submenu( GTK_MENU_ITEM(tuner_item), tuner_menu );
  gtk_menu_item_set_submenu( GTK_MENU_ITEM(help_item), help_menu );

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
  frame1 = gtk_frame_new(gettext("Deviation"));
  //  gtk_fixed_put(GTK_FIXED(fix), frame1, 0, 0);
  gtk_box_pack_start_defaults(GTK_BOX(hb), frame1);

  
  // note frame
  frame3 = gtk_frame_new(gettext("Note"));
  //   gtk_fixed_put(GTK_FIXED(fix), frame3, 164, 0);
  gtk_box_pack_start_defaults(GTK_BOX(hb), frame3);

  // spectrum frame at bottom
  frame4 = gtk_frame_new(gettext("Spectrum"));
  gtk_box_pack_end_defaults(GTK_BOX(vb), frame4);
  
  // for gauge drawing
  gauge = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(gauge), 160, 100);
  gtk_container_add(GTK_CONTAINER(frame1), gauge);
  
  // for spectrum drawing
  spectrum = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(spectrum), 256, 64);
  gtk_container_add(GTK_CONTAINER(frame4), spectrum);
  
  // for note and frequency displaying
  vbinfo = gtk_vbox_new(FALSE, 0);
  //   gtk_widget_set_usize(GTK_WIDEST(vbinfo), 80, 100);
  gtk_container_add(GTK_CONTAINER(frame3), vbinfo);

  freq_info = gtk_label_new(gettext("freq"));
  gtk_box_pack_start_defaults(GTK_BOX(vbinfo), freq_info);

  error_info = gtk_label_new(gettext("err"));
  gtk_box_pack_end_defaults(GTK_BOX(vbinfo), error_info);

  note_info = gtk_label_new(gettext("nota"));
  gtk_widget_set_name(note_info, "label_nota");
  gtk_box_pack_end_defaults(GTK_BOX(vbinfo), note_info);

  // show all
  gtk_widget_show_all(win);

  // two pixmaps for double buffer un gauge and spectrum drawing 
  // (virtual screen)
  gdk_pixmap_new(gauge->window, 160, 100, -1);
  pix_spectrum = gdk_pixmap_new(gauge->window, 256, 64, -1);

  // GTK signals
  gtk_signal_connect(GTK_OBJECT(gauge), "expose_event",
 		     (GtkSignalFunc)callbackRedraw, this);
  gtk_signal_connect(GTK_OBJECT(spectrum), "expose_event",
		     (GtkSignalFunc)callbackRedraw, this);
  gtk_signal_connect(GTK_OBJECT(win), "destroy",
 		     (GtkSignalFunc)callbackDestroy, this);

  // periodical representation temporization
  //  tout_handle = gtk_timeout_add(50, (GtkFunction)callbackTout, this);
}


GUI::~GUI()
{
  delete gauge_filter;
  delete freq_filter;
  if (dc) delete dc;
}

// ---------------------------------------------------------------------------

void GUI::allocColor(GdkColor *col, int r, int g, int b)
{
  static GdkColormap* cmap = gdk_colormap_get_system();
  col->red   = r;
  col->green = g;
  col->blue  = b;    	    
  gdk_color_alloc(cmap, col);    
}

void GUI::fgColor(GdkGC *gc, int r, int g, int b)
{
  GdkColor col;
  allocColor(&col, r, g, b);
  gdk_gc_set_foreground(gc, &col);
}

void GUI::bgColor(GdkGC *gc, int r, int g, int b)
{
  GdkColor col;
  allocColor(&col, r, g, b);
  gdk_gc_set_background(gc, &col);
}

void GUI::redraw()
{
  drawGauge();
  drawSpectrum();
}

// ---------------------------------------------------------------------------

void GUI::drawGauge()
{ 
  GdkGC* gc = gauge->style->fg_gc[gauge->state];
  GdkWindow* w = gauge->window;
  GdkGCValues gv;

  gdk_gc_get_values(gc, &gv);
  
  FLT max = 1.0;

  // draws background
  if(!pix_stick) {
    pix_stick = gdk_pixmap_create_from_xpm_d(gauge->window, NULL, NULL,
					  background2_xpm);
  }
  gdk_draw_pixmap(gauge->window, gc, pix_stick, 0, 0, 0, 0, 160, 100);
  
  // and draws gauge
  fgColor(gc, 0xC000, 0x0000, 0x2000);

  gdk_draw_line(w, gc, 80, 99,
 		80 + (int)rint(90.0*sin(gauge_value*M_PI/(1.5*max))),
 		99 - (int)rint(90.0*cos(gauge_value*M_PI/(1.5*max))));

  // black edge.  
  fgColor(gc, 0x0000, 0x0000, 0x0000);
  gdk_draw_rectangle(w, gc, FALSE, 0, 0, 159, 99);

  gdk_draw_pixmap(gauge->window, gc, w, 0, 0, 0, 0, 160, 100);
  gdk_flush();
  //   Flush();
}


void GUI::drawSpectrum()
{
  GdkGC* gc = spectrum->style->fg_gc[spectrum->state];
  GdkWindow* w = pix_spectrum; //spectrum->window;
  GdkGCValues gv;
  gdk_gc_get_values(gc, &gv);
  
  // clear all
  fgColor(gc, 0x1111, 0x3333, 0x1111);
  //   gdk_gc_set_foreground(gc, &spectrum->style->bg[spectrum->state]);
  gdk_draw_rectangle(w, gc, TRUE, 0, 0, 256, 64);
  
  // grid
  int Nlx = 9;
  int Nly = 3;

  fgColor(gc, 0x7000, 0x7000, 0x7000);
  //   gdk_gc_set_foreground(gc, &spectrum->style->dark[spectrum->state]);
  for (int i = 0; i <= Nly; i++)
    gdk_draw_line(w, gc, 0, i*63/Nly, 255, i*63/Nly);
  for (int i = 0; i <= Nlx; i++)
    gdk_draw_line(w, gc, i*255/Nlx, 0, i*255/Nlx, 63);

  fgColor(gc, 0x2222, 0xFFFF, 0x2222);
  //   fgColor(gc, 0x4000, 0x4000, 0x4000);
  //   gdk_gc_set_foreground(gc, &spectrum->style->fg[spectrum->state]);
   
# define PLOT_GAIN  8
   
  // spectrum drawing.
  for (register unsigned int i = 0; (i < conf.FFT_SIZE) && (i < 256); i++) {    
    int j = (X[i] > 1.0) ? (int) (64 - PLOT_GAIN*log10(X[i])) : 64;   // dB.
    gdk_draw_line(w, gc, i, 63, i, j);
  }

  if (freq != 0.0) {

    // fundamental frequency marking with a red point. 
    fgColor(gc, 0xFFFF, 0x2222, 0x2222);
    // index of closest sample to fundamental frequency.
    int i = (int) rint(freq*conf.FFT_SIZE*conf.OVERSAMPLING/conf.SAMPLE_RATE);
    int j = (X[i] > 1.0) ? (int) (64 - PLOT_GAIN*log10(X[i])) : 64;   // dB.
    gdk_draw_rectangle(w, gc, TRUE, i-1, j-1, 3, 3);
  }

# undef  PLOT_GAIN

  fgColor(gc, 0x0000, 0x0000, 0x0000);

  //   Flush();
  //   Flip();
  gdk_draw_pixmap(spectrum->window, gc, w, 0, 0, 0, 0, 256, 64);
  gdk_flush();
}


void GUI::putFrequency()
{
  // note indexation by semitone.
  static char* note_string[] = 
    { "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#" };
  const FLT Log2 = log(2.0);

  FLT   fret_f, error;
  int   fret;
  char* current_note;
 
  static char error_string[30], freq_string[30];

  if (freq < 10.0) {
  
    current_note = "---";
    strcpy(error_string, "e = ---");
    strcpy(freq_string, "f = ---");
    error = gauge_filter->filter(conf.VRP);
    //error = conf.VRP;

  } else {
  
    // bring up some octaves to avoid negative frets.
    fret_f = log(freq/conf.ROOT_FREQUENCY)/Log2*12.0 + 12e2;

    error = gauge_filter->filter(fret_f - rint(fret_f));
    //error = fret_f - rint(fret_f);
  
    fret = ((int) rint(fret_f)) % 12;
  
    current_note = note_string[fret];
    sprintf(error_string, "e = %+4.2f%%", error*100.0);
    sprintf(freq_string, "f = %6.2f Hz", freq_filter->filter(freq));
  }

  gauge_value = error;  

  gtk_label_set_text(GTK_LABEL(freq_info), freq_string);
  gtk_label_set_text(GTK_LABEL(error_info), error_string);
  char labeltext_current_note[100];
  sprintf(labeltext_current_note, "<big><b>%s</b></big>", current_note);
  gtk_label_set_markup(GTK_LABEL(note_info), labeltext_current_note);
}

/* Callback for visualization */
gboolean visualization_callback(gpointer data) {
	//printf("visualization callback!\n");
	
	GUI* gui = (GUI*) data;

	gui->drawGauge();

	unsigned int period = int(1.0e3/gui->conf.VISUALIZATION_RATE);
	gtk_timeout_add(period, visualization_callback, gui);
	
	return 0; 
}

/* Callback for calculation */
gboolean calculation_callback(gpointer data) {
	//printf("calculation callback!\n");
	
	GUI* gui = (GUI*) data;

	gui->drawSpectrum();

	unsigned int period = int(1.0e3/gui->conf.CALCULATION_RATE);
	gtk_timeout_add(period, calculation_callback, gui);
	
	return 0; 
}

/* Callback for frequency calculation */
gboolean frequency_callback(gpointer data) {
	//printf("error calculation callback!\n");
	
	GUI* gui = (GUI*) data;

	gui->putFrequency();

	unsigned int period = int(1.0e3/GAUGE_RATE);
	gtk_timeout_add(period, frequency_callback, gui);
	
	return 0; 
}

void GUI::mainLoop()
{
/*  struct timeval  t_current, t_diff;

  t_event        e_gauge, e_gtk, e_vis, e_calculus;
  t_event*       next_tout;

  gettimeofday(&t_current, NULL);

  e_vis      = next_event(t_current, conf.VISUALIZATION_RATE);
  e_gauge    = next_event(t_current, GAUGE_RATE);
  e_gtk      = next_event(t_current, GTK_EVENTS_RATE);
  e_calculus = next_event(t_current, conf.CALCULATION_RATE);*/

//  ES.add(&e_gauge);
//  ES.add(&e_gtk);
//  ES.add(&e_vis);
//  ES.add(&e_calculus);

	unsigned int period = int(1.0e3/conf.VISUALIZATION_RATE);
	gtk_timeout_add(period, visualization_callback, this);

	period = int(1.0e3/conf.CALCULATION_RATE);
	gtk_timeout_add(period, calculation_callback, this);

	period = int(1.0e3/GAUGE_RATE);
	gtk_timeout_add(period, frequency_callback, this);

	return;
		
/*  while(!quit) {

    next_tout = ES.next(); // consult the next event.
    gettimeofday(&t_current, NULL);

    if (timercmp(next_tout, &t_current, >)) {
      timersub(next_tout, &t_current, &t_diff);
      select(0, NULL, NULL, NULL, &t_diff);
      gettimeofday(&t_current, NULL);
    }

    ES.remove(next_tout);

    if (next_tout == &e_vis) {
      e_vis   = next_event(t_current, conf.VISUALIZATION_RATE);
      ES.add(&e_vis);
      drawGauge();
    }

    if (next_tout == &e_gauge) {
      e_gauge   = next_event(t_current, GAUGE_RATE);
      ES.add(&e_gauge);	
      putFrequency();
    } 

    if (next_tout == &e_gtk) {
      e_gtk = next_event(t_current, GTK_EVENTS_RATE);
      ES.add(&e_gtk);	
      while(gtk_events_pending()) gtk_main_iteration_do(FALSE);
    }

    if (next_tout == &e_calculus) {
      e_calculus = next_event(t_current, conf.CALCULATION_RATE);
      ES.add(&e_calculus);	
      drawSpectrum();
    }
  }*/
}


void quick_message(gchar *title, gchar *message) {

  GtkWidget *dialog, *label, *okay_button;
  
  /* Create the widgets */
  dialog = gtk_dialog_new();
  label = gtk_label_new (message);

  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);

  okay_button = gtk_button_new_with_label(gettext("Ok"));
  
  /* Ensure that the dialog box is destroyed when the user clicks ok. */
  
  gtk_signal_connect_object (GTK_OBJECT (okay_button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT(dialog));
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area),
		     okay_button);

  /* Add the label, and show everything we've added to the dialog. */
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
		     label);
  gtk_widget_show_all (dialog);
}
