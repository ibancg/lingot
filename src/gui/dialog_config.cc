//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004   Ibán Cereijo Graña, Jairo Chapela Martínez.

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
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include "defs.h"

#include "core.h"
#include "gui.h"
#include "dialog_config.h"

#define _(x) gettext(x)

void dialog_config_cb(GtkWidget *w, GUI *G)
{
  if (G->dc) {
    G->dc->close();
    delete G->dc;
  }
  G->dc = new DialogConfig(G);
}

/* button press event attention routine. */
void clicked_cb(GtkButton *b, DialogConfig *d)
{
  //printf("%p (%p) %p %p %p\n", b, d, d->button_ok, d->button_cancel, d->button_apply);
  if((GtkWidget*)b == d->button_ok) {
    d->apply();
    d->close();
    d->conf.saveConfigFile(CONFIG_FILE);
  }else if((GtkWidget*)b == d->button_cancel) {
    d->close();
    d->G->changeConfig(d->conf_old); // restore old configuration.
  }else if((GtkWidget*)b == d->button_apply) {
    d->apply();
    d->rewrite();
  }else if((GtkWidget*)b == d->button_default) {
    d->conf.reset();
    d->rewrite();
  }
}

void change_label_sample_rate( GtkWidget *widget, DialogConfig *d)
{
  char   buff[20];
  float  sample_rate = 
    atof(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(d->combo_sample_rate)->entry)));
  float  oversampling =
    gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(d->spin_oversampling));
  sprintf(buff, " = %0.1f", sample_rate/oversampling);
  gtk_label_set_text ( GTK_LABEL(d->label_sample_rate), buff);
}

void change_label_a_frequence( GtkWidget *widget, DialogConfig *d)
{
  char   buff[20];

  float  root_freq = 440.0*pow(2.0, gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(d->spin_root_frequency_error))/1200.0);
  sprintf(buff, "%%    = %0.2f", root_freq);
  gtk_label_set_text ( GTK_LABEL(d->label_root_frequency), buff);
}

void DialogConfig::rewrite()
{
  char   buff[80];

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_calculation_rate), conf.CALCULATION_RATE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_oversampling), conf.OVERSAMPLING);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_root_frequency_error), conf.ROOT_FREQUENCY_ERROR);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_temporal_window), conf.TEMPORAL_WINDOW);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_noise_threshold), conf.NOISE_THRESHOLD);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_dft_number), conf.DFT_NUMBER);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_dft_size), conf.DFT_SIZE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_peak_number), conf.PEAK_NUMBER);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_peak_order), conf.PEAK_ORDER);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_peak_rejection_relation), conf.PEAK_REJECTION_RELATION);

  sprintf(buff, "%d", conf.FFT_SIZE);
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo_fft_size)->entry), (gchar *) buff);

  sprintf(buff, "%d", conf.SAMPLE_RATE);
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo_sample_rate)->entry), (gchar *) buff);

  change_label_sample_rate(NULL, this);
  change_label_a_frequence(NULL, this);
}

DialogConfig::DialogConfig(GUI* G)
{  
  DialogConfig::G = G;
  
  conf = G->conf;
  conf_old = G->conf;
  
  // cretes the window
  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (win), gettext("lingot configuration"));

  gtk_container_set_border_width(GTK_CONTAINER(win), 6);

  gtk_window_set_policy(GTK_WINDOW(win), FALSE, TRUE, FALSE);  

  // items vertically displayed
  vb = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(win), vb);

  frame1 = gtk_frame_new(gettext("General parameters"));
  gtk_box_pack_start_defaults(GTK_BOX(vb), frame1);

  tab1 = gtk_table_new(5, 3, FALSE);
  int file = 0;

  gtk_table_set_col_spacings(GTK_TABLE(tab1), 20);
  gtk_table_set_row_spacings(GTK_TABLE(tab1), 3);
  gtk_container_set_border_width(GTK_CONTAINER(tab1), 5);

  gtk_container_add(GTK_CONTAINER(frame1), tab1);

  // --------------------------------------------------------------------------

  spin_calculation_rate = gtk_spin_button_new(
				 (GtkAdjustment*)
				 gtk_adjustment_new(conf.CALCULATION_RATE, 1.0, 60.0,
						    0.5, 10.0, 15.0), 
				 0.5, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_calculation_rate), TRUE);
  gtk_table_attach_defaults(GTK_TABLE(tab1), 
			    gtk_label_new(gettext("Calculation rate")),
			    0, 1, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), spin_calculation_rate, 
			    1, 2, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(gettext("Hz")),
			    2, 3, file, file + 1);
  file++;

  // --------------------------------------------------------------------------

  spin_visualization_rate = 
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf.VISUALIZATION_RATE, 1.0, 60.0,
					   0.5, 10.0, 15.0), 
			0.5, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_visualization_rate), TRUE);
  gtk_table_attach_defaults(GTK_TABLE(tab1), 
			    gtk_label_new(gettext("Visualization rate")),
			    0, 1, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), spin_visualization_rate,
			    1, 2, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(gettext("Hz")),
			    2, 3, file, file + 1);
  file++;

  // --------------------------------------------------------------------------

  char buff[80];

  combo_fft_size = gtk_combo_new();

  GList *glist = NULL;
  glist = g_list_append(glist, (void*) "256");
  glist = g_list_append(glist, (void*) "512");
  glist = g_list_append(glist, (void*) "1024");
  glist = g_list_append(glist, (void*) "2048");
  glist = g_list_append(glist, (void*) "4096");

  gtk_combo_set_popdown_strings( GTK_COMBO(combo_fft_size), glist);
  
  sprintf(buff, "%d", conf.FFT_SIZE);
  //gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo_fft_size)->entry), (gchar *) buff);

  gtk_combo_set_use_arrows_always( GTK_COMBO(combo_fft_size), TRUE );

  gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(gettext("FFT size")),
			    0, 1, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), combo_fft_size,
			    1, 2, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(gettext("samples")),
			    2, 3, file, file + 1);
  file++;

  // --------------------------------------------------------------------------

  spin_noise_threshold = 
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf.NOISE_THRESHOLD,
					   -30.0, 40.0, 0.1, 0.1, 0.1), 
			0.1, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_noise_threshold), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_noise_threshold), 1) ;

  gtk_table_attach_defaults(GTK_TABLE(tab1),
			    gtk_label_new(gettext("Noise threshold")),
			    0, 1, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(gettext("dB")),
			    2, 3, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), spin_noise_threshold,
			    1, 2, file, file + 1);
  file++;

  // --------------------------------------------------------------------------

  GtkWidget *box = gtk_hbox_new (FALSE, FALSE);
  
  combo_sample_rate = gtk_combo_new();
  
  glist = NULL;
  glist = g_list_append(glist, (void*) "8000");
  glist = g_list_append(glist, (void*) "11025");
  glist = g_list_append(glist, (void*) "22050");
  glist = g_list_append(glist, (void*) "44100");

  gtk_combo_set_popdown_strings( GTK_COMBO(combo_sample_rate), glist);

  sprintf(buff, "%d", conf.SAMPLE_RATE);
  //gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo_sample_rate)->entry), (gchar *) buff);
  
  gtk_combo_set_use_arrows_always( GTK_COMBO(combo_sample_rate), TRUE );

  GtkAdjustment* adj = 
    (GtkAdjustment*) gtk_adjustment_new(conf.OVERSAMPLING, 1.0, 20.0,
					  1.0, 10.0, 15.0);
  spin_oversampling = gtk_spin_button_new(adj, 1.0, 1);

  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_oversampling), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_oversampling), 0) ;

  sprintf(buff, " = %0.1f", atof(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_sample_rate)->entry)))/gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_oversampling)));
  label_sample_rate = gtk_label_new(_(buff));

  gtk_box_pack_start (GTK_BOX(box), combo_sample_rate, FALSE, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX(box), gtk_label_new(_(" / ")), FALSE, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX(box), spin_oversampling, FALSE, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX(box), label_sample_rate, FALSE, FALSE, FALSE);

  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo_sample_rate)->entry), "changed",
                       GTK_SIGNAL_FUNC (change_label_sample_rate), this);

  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (change_label_sample_rate), this);


  gtk_table_attach_defaults(GTK_TABLE(tab1), 
			    gtk_label_new(gettext("Sample rate")),
			    0, 1, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), box, 1, 2, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(gettext("Hz")),
			    2, 3, file, file + 1);
  file++;

  // --------------------------------------------------------------------------

  frame2 = gtk_frame_new(gettext("Advanced parameters"));
  gtk_box_pack_start_defaults(GTK_BOX(vb), frame2);

  tab2 = gtk_table_new(7, 3, FALSE);
  file = 0;

  gtk_table_set_col_spacings(GTK_TABLE(tab2), 20);
  gtk_table_set_row_spacings(GTK_TABLE(tab2), 3);
  gtk_container_set_border_width(GTK_CONTAINER(tab2), 5);

  gtk_container_add(GTK_CONTAINER(frame2), tab2);

  // --------------------------------------------------------------------------

  box = gtk_hbox_new (TRUE, FALSE);

  GtkAdjustment* adj2 = 
    (GtkAdjustment*) gtk_adjustment_new(conf.ROOT_FREQUENCY_ERROR,
					-500.0, +500.0, 1.0, 10.0, 15.0);
  spin_root_frequency_error = gtk_spin_button_new(adj2, 1.0, 1);

  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_root_frequency_error), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_root_frequency_error), 0) ;

  gtk_box_pack_start (GTK_BOX(box), gtk_label_new(gettext(" A (440 Hz) + ")),
		      FALSE, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX(box), spin_root_frequency_error,
		      FALSE, TRUE, FALSE);

  sprintf(buff, "%%    = %0.2f", conf.ROOT_FREQUENCY);
  label_root_frequency = gtk_label_new(_(buff));

  gtk_box_pack_start (GTK_BOX(box), label_root_frequency, FALSE, FALSE, FALSE);

  gtk_signal_connect (GTK_OBJECT (adj2), "value_changed",
                      GTK_SIGNAL_FUNC (change_label_a_frequence), this);

  gtk_table_attach_defaults(GTK_TABLE(tab2), 
			    gtk_label_new(gettext("Root frequency")),
			    0, 1, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), box, 1, 2, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(gettext("Hz")),
			    2, 3, file, file + 1);
  file++;

  // --------------------------------------------------------------------------

  spin_temporal_window = 
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf.TEMPORAL_WINDOW, 0.0, 2.0,
					   0.005, 0.1, 0.1), 
			0.005, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_temporal_window), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_temporal_window), 3) ;

  gtk_table_attach_defaults(GTK_TABLE(tab2),
			    gtk_label_new(gettext("Temporal window")),
			    0, 1, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(gettext("seconds")),
			    2, 3, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), spin_temporal_window,
			    1, 2, file, file + 1);
  file++;

  // --------------------------------------------------------------------------

  spin_dft_number =
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf.DFT_NUMBER, 0.0, 10.0,
					   1.0, 1.0, 1.0), 
			1.0, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_dft_number), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_dft_number), 0) ;
  
  gtk_table_attach_defaults(GTK_TABLE(tab2),
			    gtk_label_new(gettext("DFT number")),
			    0, 1, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), spin_dft_number,
			    1, 2, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(gettext("DFTs")),
			    2, 3, file, file + 1);
  file++;

  // --------------------------------------------------------------------------

  spin_dft_size =
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf.DFT_SIZE, 4.0, 100.0,
					   1.0, 1.0, 1.0), 
			1.0, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_dft_size), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_dft_size), 0) ;

  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(gettext("DFT size")),
			    0, 1, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(gettext("samples")),
			    2, 3, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), spin_dft_size,
			    1, 2, file, file + 1);
  file++;

  // --------------------------------------------------------------------------

  spin_peak_number =
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf.PEAK_NUMBER, 1.0, 10.0,
					   1.0, 1.0, 1.0), 
			1.0, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_peak_number), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_peak_number), 0) ;
  
  gtk_table_attach_defaults(GTK_TABLE(tab2),
			    gtk_label_new(gettext("Peak number")),
			    0, 1, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), spin_peak_number,
			    1, 2, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(gettext("peaks")),
			    2, 3, file, file + 1);
  file++;

  // --------------------------------------------------------------------------

  spin_peak_order =
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf.PEAK_ORDER, 1.0, 5.0,
					   1.0, 1.0, 1.0), 
			1.0, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_peak_order), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_peak_order), 0) ;

  gtk_table_attach_defaults(GTK_TABLE(tab2),
			    gtk_label_new(gettext("Peak order")),
			    0, 1, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), spin_peak_order,
			    1, 2, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2),
			    gtk_label_new(gettext("samples")),
			    2, 3, file, file + 1);
  file++;

  // --------------------------------------------------------------------------

  spin_peak_rejection_relation =
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf.PEAK_REJECTION_RELATION,
					   2.0, 40.0, 0.1, 1.0, 1.0), 
			0.1, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_peak_rejection_relation),
			      TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_peak_rejection_relation), 1);
  
  gtk_table_attach_defaults(GTK_TABLE(tab2),
			    gtk_label_new(gettext("Rejection peak relation")),
			    0, 1, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), spin_peak_rejection_relation,
			    1, 2, file, file + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(gettext("dB")),
			    2, 3, file, file + 1);
  file++;

  // --------------------------------------------------------------------------

  /* four horizontally aligned buttons at bottom. */
  hb = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start_defaults(GTK_BOX(vb), hb);  

  button_ok = gtk_button_new_with_label(gettext("Ok"));
  button_cancel = gtk_button_new_with_label(gettext("Cancel"));
  button_apply = gtk_button_new_with_label(gettext("Apply"));
  button_default = gtk_button_new_with_label(gettext("Default config"));

  gtk_box_pack_start_defaults(GTK_BOX(hb), button_ok);
  gtk_box_pack_start_defaults(GTK_BOX(hb), button_cancel);
  gtk_box_pack_start_defaults(GTK_BOX(hb), button_apply);
  gtk_box_pack_start_defaults(GTK_BOX(hb), button_default);

  gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		     GTK_SIGNAL_FUNC(clicked_cb), this);
  gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		     GTK_SIGNAL_FUNC(clicked_cb), this);
  gtk_signal_connect(GTK_OBJECT(button_apply), "clicked",
		     GTK_SIGNAL_FUNC(clicked_cb), this);
  gtk_signal_connect(GTK_OBJECT(button_default), "clicked",
		     GTK_SIGNAL_FUNC(clicked_cb), this);

  rewrite();
  gtk_widget_show_all(win);
  //printf("(%p) %p %p %p\n", this, button_ok, button_cancel, button_apply);
}

void DialogConfig::apply()
{
  conf.ROOT_FREQUENCY_ERROR = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(spin_root_frequency_error));
  conf.CALCULATION_RATE = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(spin_calculation_rate));
  conf.VISUALIZATION_RATE = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(spin_visualization_rate));
  conf.TEMPORAL_WINDOW = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(spin_temporal_window));
  conf.NOISE_THRESHOLD = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(spin_noise_threshold));
  conf.OVERSAMPLING = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_oversampling));
  conf.DFT_NUMBER = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_dft_number));
  conf.DFT_SIZE = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_dft_size));
  conf.PEAK_NUMBER = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_peak_number));
  conf.PEAK_ORDER = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_peak_order));
  conf.PEAK_REJECTION_RELATION = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_peak_rejection_relation));
  conf.FFT_SIZE = atoi(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_fft_size)->entry)));
  conf.SAMPLE_RATE = atoi(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_sample_rate)->entry)));

  conf.updateInternalParameters();

  G->changeConfig(conf);
}

void DialogConfig::close()
{
  gtk_widget_destroy(win);
}
