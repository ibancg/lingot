//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004-2007  IbÃ¡n Cereijo GraÃ±a, Jairo Chapela MartÃ­nez.

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

#include "iir.h"
#include "config.h"
#include "gui.h"
#include "dialog_config.h"
#include "gauge.h"

//#include "../config.h"

#define _(x) gettext(x)

#include "background.xpm"
#include "lingot-icon.xpm"

void callbackRedraw(GtkWidget* w, GdkEventExpose *e, void *data)
{
  ((GUI*)data)->redraw();
}

void callbackDestroy(GtkWidget* w, GUI* gui)
{
	gui->core->stop();
  gtk_main_quit();
}

void callbackAbout(GtkWidget* w, void *data)
{
	GtkWidget* about = gtk_about_dialog_new();
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about), "Lingot");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), VERSION);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about), "http://lingot.nongnu.org/");
	
	const gchar** authors = new const gchar*[3];
	authors[0] = "Ibán Cereijo Graña <ibancg@gmail.com>";
	authors[1] = "Jairo Chapela Martínez <jairochapela@gmail.com>";
	authors[2] = NULL;
	const gchar** artists = new const gchar*[2];
	artists[0] = "Matthew Blissett (Logo design)";
	artists[1] = NULL;
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);
	gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(about), artists);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), "(c) Copyright 2004-2007\nIbán Cereijo Graña <ibancg AT gmail.com>\nJairo Chapela Martínez <jairochapela AT gmail.com>");
	GdkPixbuf* icon = gdk_pixbuf_new_from_xpm_data(lingotlogo);
	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about), icon);
	//gtk_show_about_dialog(GTK_WINDOW(w), "Lingot", NULL);
  gtk_dialog_run (GTK_DIALOG (about));
	gtk_widget_destroy (about);
}

void dialog_config_cb(GtkWidget* w, GUI *gui)
{
  if (!gui->dialog_config) {
	  gui->dialog_config = new DialogConfig(gui);	  
  }
  
  //gtk_widget_activate(gui->dialog_config->win);
}


/* Callback for visualization */
gboolean visualization_callback(gpointer data) {
	//printf("visualization callback!\n");
	
	GUI* gui = (GUI*) data;

	gui->drawGauge();

	unsigned int period = int(1.0e3/gui->conf->VISUALIZATION_RATE);
	gtk_timeout_add(period, visualization_callback, gui);
	
	return 0; 
}

/* Callback for calculation */
gboolean calculation_callback(gpointer data) {
	//printf("calculation callback!\n");
	
	GUI* gui = (GUI*) data;

	gui->drawSpectrum();

	unsigned int period = int(1.0e3/gui->conf->CALCULATION_RATE);
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

/* runs the core object. */
void run_core(Core* core) {
	core->start();	
  pthread_exit(NULL);
}


GUI::GUI(int argc, char *argv[])
{

  dialog_config = NULL;
  quit = false;
  pix_stick = NULL;

	conf = new Config();
	conf->parseConfigFile(CONFIG_FILE_NAME);
		
	gauge = new Gauge(conf->VRP); // gauge in rest situation	

	core = new Core(conf);
	core->start();

  // ----- FREQUENCY FILTER CONFIGURATION ------

  // low pass IIR filter.
  FLT freq_filter_a[] = { 1.0, -0.5 };
  FLT freq_filter_b[] = { 0.5 };
  
  freq_filter = new IIR( 1, 0, freq_filter_a, freq_filter_b );
    
  // ---------------------------------------------------

	gtk_init(&argc, &argv);
	gtk_set_locale();

  gtk_rc_parse_string(
 		      "style \"title\""
 		      "{"
		      "font = \"-*-helvetica-bold-r-normal--32-*-*-*-*-*-*-*\""
 		      "}"
		      "widget \"*label_nota\" style \"title\""
 		      );


  // creates the window
  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW (win), _("lingot"));

  gtk_container_set_border_width(GTK_CONTAINER(win), 6);

  // fixed size
	gtk_window_set_resizable(GTK_WINDOW(win), FALSE);

  // tab organization by following container
  GtkWidget* vb = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(win), vb);

  GtkWidget* tuner_menu = gtk_menu_new();
  GtkWidget* help_menu = gtk_menu_new();
  
  /* menu elements */
  GtkWidget* options_item = gtk_menu_item_new_with_label(_("Options"));
  GtkWidget* quit_item = gtk_menu_item_new_with_label(_("Quit"));

  GtkWidget* about_item = gtk_menu_item_new_with_label(_("About"));


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

  GtkWidget* tuner_item = gtk_menu_item_new_with_label(_("Tuner"));
  GtkWidget* help_item  = gtk_menu_item_new_with_label(_("Help"));
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
  GtkWidget* hb = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(vb), hb);
  
  // gauge frame
  GtkWidget* frame1 = gtk_frame_new(_("Deviation"));
  //  gtk_fixed_put(GTK_FIXED(fix), frame1, 0, 0);
  gtk_box_pack_start_defaults(GTK_BOX(hb), frame1);

  
  // note frame
  GtkWidget* frame3 = gtk_frame_new(_("Note"));
  //gtk_fixed_put(GTK_BOX(hb), frame3, 164, 0);
  gtk_box_pack_start(GTK_BOX(hb), frame3, TRUE, TRUE, 1);

  // spectrum frame at bottom
  GtkWidget* frame4 = gtk_frame_new(_("Spectrum"));
  gtk_box_pack_end_defaults(GTK_BOX(vb), frame4);
  
  // for gauge drawing
  gauge_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(gauge_area), 160, 100);
  gtk_container_add(GTK_CONTAINER(frame1), gauge_area);
  
  // for spectrum drawing
  spectrum_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(spectrum_area), 256, 64);
  gtk_container_add(GTK_CONTAINER(frame4), spectrum_area);
  
  // for note and frequency displaying
  GtkWidget* vbinfo = gtk_vbox_new(FALSE, 0);
  gtk_widget_set_size_request(vbinfo, 90, 100);
  gtk_container_add(GTK_CONTAINER(frame3), vbinfo);

  freq_label = gtk_label_new(_("freq"));
  gtk_box_pack_start_defaults(GTK_BOX(vbinfo), freq_label);

  error_label = gtk_label_new(_("err"));
  gtk_box_pack_end_defaults(GTK_BOX(vbinfo), error_label);

  note_label = gtk_label_new(_("nota"));
  gtk_widget_set_name(note_label, "label_nota");
  gtk_box_pack_end_defaults(GTK_BOX(vbinfo), note_label);

  // show all
  gtk_widget_show_all(win);

  // two pixmaps for double buffer in gauge and spectrum drawing 
  // (virtual screen)
  gdk_pixmap_new(gauge_area->window, 160, 100, -1);
  pix_spectrum = gdk_pixmap_new(gauge_area->window, 256, 64, -1);

  // GTK signals
  gtk_signal_connect(GTK_OBJECT(gauge_area), "expose_event",
 		     (GtkSignalFunc) callbackRedraw, this);
  gtk_signal_connect(GTK_OBJECT(spectrum_area), "expose_event",
		     (GtkSignalFunc) callbackRedraw, this);
  gtk_signal_connect(GTK_OBJECT(win), "destroy",
 		     (GtkSignalFunc) callbackDestroy, this);

  unsigned int period;
  period = int(1.0e3/conf->VISUALIZATION_RATE);
	gtk_timeout_add(period, visualization_callback, this);

	period = int(1.0e3/conf->CALCULATION_RATE);
	gtk_timeout_add(period, calculation_callback, this);

	period = int(1.0e3/GAUGE_RATE);
	gtk_timeout_add(period, frequency_callback, this);
}


GUI::~GUI()
{
  delete gauge;
  delete core;
  delete freq_filter;
  delete conf;
  if (dialog_config) delete dialog_config;
}

void GUI::run() {

	core->start();	
	gtk_main();
}
// ---------------------------------------------------------------------------

void allocColor(GdkColor *col, int r, int g, int b)
{
  static GdkColormap* cmap = gdk_colormap_get_system();
  col->red   = r;
  col->green = g;
  col->blue  = b;    	    
  gdk_color_alloc(cmap, col);    
}

void fgColor(GdkGC *gc, int r, int g, int b)
{
  GdkColor col;
  allocColor(&col, r, g, b);
  gdk_gc_set_foreground(gc, &col);
}

void bgColor(GdkGC *gc, int r, int g, int b)
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
  GdkGC* gc = gauge_area->style->fg_gc[gauge_area->state];
  GdkWindow* w = gauge_area->window;
  GdkGCValues gv;

  gdk_gc_get_values(gc, &gv);
  
  FLT max = 1.0;

  // draws background
  if(!pix_stick) {
    pix_stick = gdk_pixmap_create_from_xpm_d(gauge_area->window, NULL, NULL,
					  background2_xpm);
  }
  gdk_draw_pixmap(gauge_area->window, gc, pix_stick, 0, 0, 0, 0, 160, 100);
  
  // and draws gauge
  fgColor(gc, 0xC000, 0x0000, 0x2000);

  gdk_draw_line(w, gc, 80, 99,
 		80 + (int)rint(90.0*sin(gauge->getPosition()*M_PI/(1.5*max))),
 		99 - (int)rint(90.0*cos(gauge->getPosition()*M_PI/(1.5*max))));

  // black edge.  
  fgColor(gc, 0x0000, 0x0000, 0x0000);
  gdk_draw_rectangle(w, gc, FALSE, 0, 0, 159, 99);

  gdk_draw_pixmap(gauge_area->window, gc, w, 0, 0, 0, 0, 160, 100);
  gdk_flush();
  //   Flush();
}


void GUI::drawSpectrum()
{
  GdkGC* gc = spectrum_area->style->fg_gc[spectrum_area->state];
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
  for (register unsigned int i = 0; (i < conf->FFT_SIZE) && (i < 256); i++) {    
    int j = (core->X[i] > 1.0) ? (int) (64 - PLOT_GAIN*log10(core->X[i])) : 64;   // dB.
    gdk_draw_line(w, gc, i, 63, i, j);
  }

  if (core->freq != 0.0) {

    // fundamental frequency marking with a red point. 
    fgColor(gc, 0xFFFF, 0x2222, 0x2222);
    // index of closest sample to fundamental frequency.
    int i = (int) rint(core->freq*conf->FFT_SIZE*conf->OVERSAMPLING/conf->SAMPLE_RATE);
    int j = (core->X[i] > 1.0) ? (int) (64 - PLOT_GAIN*log10(core->X[i])) : 64;   // dB.
    gdk_draw_rectangle(w, gc, TRUE, i-1, j-1, 3, 3);
  }

# undef  PLOT_GAIN

  fgColor(gc, 0x0000, 0x0000, 0x0000);

  //   Flush();
  //   Flip();
  gdk_draw_pixmap(spectrum_area->window, gc, w, 0, 0, 0, 0, 256, 64);
  gdk_flush();
}


void GUI::putFrequency()
{
  // note indexation by semitone.
  static char* note_string[] = 
    { "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#" };
  const FLT Log2 = log(2.0);

  FLT   fret_f;
  int   fret;
  char* current_note;
 
  static char error_string[30], freq_string[30];

  if (core->freq < 10.0) {
  
    current_note = "---";
    strcpy(error_string, "e = ---");
    strcpy(freq_string, "f = ---");
    gauge->compute(conf->VRP);
    //error = conf->VRP;

  } else {
  
    // bring up some octaves to avoid negative frets.
    fret_f = log(core->freq/conf->ROOT_FREQUENCY)/Log2*12.0 + 12e2;

    gauge->compute(fret_f - rint(fret_f));
    //error = fret_f - rint(fret_f);
  
    fret = ((int) rint(fret_f)) % 12;
  
    current_note = note_string[fret];
    sprintf(error_string, "e = %+4.2f%%", gauge->getPosition()*100.0);
    sprintf(freq_string, "f = %6.2f Hz", freq_filter->filter(core->freq));
  }

  gtk_label_set_text(GTK_LABEL(freq_label), freq_string);
  gtk_label_set_text(GTK_LABEL(error_label), error_string);
  char labeltext_current_note[100];
  sprintf(labeltext_current_note, "<big><b>%s</b></big>", current_note);
  gtk_label_set_markup(GTK_LABEL(note_label), labeltext_current_note);
}


void GUI::changeConfig(Config* conf) {
	
	core->stop();
	delete core;
	
	// dup.
	*GUI::conf = *conf;
	
	core = new Core(GUI::conf);
	core->start();
}
