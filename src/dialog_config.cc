//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004-2007  Ibán Cereijo Graña, Jairo Chapela Martínez.

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
#include "config.h"
#include "gui.h"
#include "dialog_config.h"

#define _(x) gettext(x)

/* button press event attention routine. */
void clicked_cb(GtkButton *b, DialogConfig *d)
{
  if((GtkWidget*)b == d->button_ok) {
    d->apply();
    d->conf->saveConfigFile(CONFIG_FILE_NAME);
    d->close();
  }else if((GtkWidget*)b == d->button_cancel) {
    d->gui->changeConfig(d->conf_old); // restore old configuration.
    d->close();
  }else if((GtkWidget*)b == d->button_apply) {
    d->apply();
    d->rewrite();
  }else if((GtkWidget*)b == d->button_default) {
    d->conf->reset();
    d->rewrite();
  }
}

void close_cb(GtkWidget *widget, DialogConfig *d) {
  delete d;
}

void change_label_sample_rate( GtkWidget *widget, DialogConfig *d)
{
  char   buff[20];
  float  sample_rate = 
    atof(gtk_combo_box_get_active_text(GTK_COMBO_BOX(d->combo_sample_rate)));
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

void combo_select_value(GtkWidget* combo, int value) {
	
  GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
	GtkTreeIter iter;
	
  gboolean valid = gtk_tree_model_get_iter_first (model, &iter);

  while (valid) {
		gchar *str_data;
    gtk_tree_model_get (model, &iter, 0, &str_data, -1);
    if (atoi(str_data) == value)
    	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), &iter);
    g_free (str_data);
    valid = gtk_tree_model_iter_next (model, &iter);
  }
}

void DialogConfig::rewrite()
{
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_calculation_rate), conf->CALCULATION_RATE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_visualization_rate), conf->VISUALIZATION_RATE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_oversampling), conf->OVERSAMPLING);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_root_frequency_error), conf->ROOT_FREQUENCY_ERROR);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_temporal_window), conf->TEMPORAL_WINDOW);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_noise_threshold), conf->NOISE_THRESHOLD);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_dft_number), conf->DFT_NUMBER);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_dft_size), conf->DFT_SIZE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_peak_number), conf->PEAK_NUMBER);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_peak_order), conf->PEAK_ORDER);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_peak_rejection_relation), conf->PEAK_REJECTION_RELATION);
	combo_select_value(combo_fft_size, (int) conf->FFT_SIZE);
	combo_select_value(combo_sample_rate, (int) conf->SAMPLE_RATE);
  change_label_sample_rate(NULL, this);
  change_label_a_frequence(NULL, this);
}



DialogConfig::DialogConfig(GUI* gui)
{  
  DialogConfig::gui = gui;
  
  conf = new Config();
  conf_old = new Config();
  
  // config copy
  *conf = *gui->conf;
  *conf_old = *gui->conf;
  
  win = gtk_dialog_new_with_buttons (_("lingot configuration"),
			GTK_WINDOW(gui->win),
      GTK_DIALOG_MODAL,
      NULL);
                                                  
  gtk_container_set_border_width(GTK_CONTAINER(win), 6);

  // items vertically displayed  
  GtkWidget* vb = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER (GTK_DIALOG(win)->vbox), vb);

  GtkWidget* frame1 = gtk_frame_new(_("General parameters"));
  gtk_box_pack_start_defaults(GTK_BOX(vb), frame1);

  GtkWidget* tab1 = gtk_table_new(5, 3, FALSE);
  int row = 0;

  gtk_table_set_col_spacings(GTK_TABLE(tab1), 20);
  gtk_table_set_row_spacings(GTK_TABLE(tab1), 3);
  gtk_container_set_border_width(GTK_CONTAINER(tab1), 5);

  gtk_container_add(GTK_CONTAINER(frame1), tab1);

  // --------------------------------------------------------------------------

  spin_calculation_rate = gtk_spin_button_new(
				 (GtkAdjustment*)
				 gtk_adjustment_new(conf->CALCULATION_RATE, 1.0, 60.0,
						    0.5, 10.0, 15.0), 
				 0.5, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_calculation_rate), TRUE);
  gtk_table_attach_defaults(GTK_TABLE(tab1), 
			    gtk_label_new(_("Calculation rate")),
			    0, 1, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), spin_calculation_rate, 
			    1, 2, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(_("Hz")),
			    2, 3, row, row + 1);
  row++;

  // --------------------------------------------------------------------------

  spin_visualization_rate = 
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf->VISUALIZATION_RATE, 1.0, 60.0,
					   0.5, 10.0, 15.0), 
			0.5, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_visualization_rate), TRUE);
  gtk_table_attach_defaults(GTK_TABLE(tab1), 
			    gtk_label_new(_("Visualization rate")),
			    0, 1, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), spin_visualization_rate,
			    1, 2, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(_("Hz")),
			    2, 3, row, row + 1);
  row++;

  // --------------------------------------------------------------------------

  char buff[80];

  combo_fft_size = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo_fft_size), "256");
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo_fft_size), "512");
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo_fft_size), "1024");
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo_fft_size), "2048");
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo_fft_size), "4096");
	combo_select_value(combo_fft_size, (int) conf->FFT_SIZE);

  gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(_("FFT size")),
			    0, 1, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), combo_fft_size,
			    1, 2, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(_("samples")),
			    2, 3, row, row + 1);
  row++;

  // --------------------------------------------------------------------------

  spin_noise_threshold = 
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf->NOISE_THRESHOLD,
					   -30.0, 40.0, 0.1, 0.1, 0.1), 
			0.1, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_noise_threshold), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_noise_threshold), 1) ;

  gtk_table_attach_defaults(GTK_TABLE(tab1),
			    gtk_label_new(_("Noise threshold")),
			    0, 1, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(_("dB")),
			    2, 3, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), spin_noise_threshold,
			    1, 2, row, row + 1);
  row++;

  // --------------------------------------------------------------------------

  GtkWidget *box = gtk_hbox_new (FALSE, FALSE);
  
  combo_sample_rate = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo_sample_rate), "8000");
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo_sample_rate), "11025");
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo_sample_rate), "22050");
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo_sample_rate), "44100");

	combo_select_value(combo_sample_rate, (int) conf->SAMPLE_RATE);

  spin_oversampling = gtk_spin_button_new(
  				(GtkAdjustment*) gtk_adjustment_new(conf->OVERSAMPLING, 1.0, 20.0,
					  1.0, 10.0, 15.0), 1.0, 1);

  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_oversampling), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_oversampling), 0) ;

  sprintf(buff, " = %0.1f", atof(gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo_sample_rate)))/gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_oversampling)));
  label_sample_rate = gtk_label_new(buff);

  gtk_box_pack_start (GTK_BOX(box), combo_sample_rate, FALSE, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX(box), gtk_label_new(" / "), FALSE, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX(box), spin_oversampling, FALSE, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX(box), label_sample_rate, FALSE, FALSE, FALSE);

  gtk_signal_connect(GTK_OBJECT(combo_sample_rate), "changed",
                       GTK_SIGNAL_FUNC (change_label_sample_rate), this);

  gtk_signal_connect (GTK_OBJECT (spin_oversampling), "value_changed",
                      GTK_SIGNAL_FUNC (change_label_sample_rate), this);


  gtk_table_attach_defaults(GTK_TABLE(tab1), 
			    gtk_label_new(_("Sample rate")),
			    0, 1, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), box, 1, 2, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(_("Hz")),
			    2, 3, row, row + 1);
  row++;

  // --------------------------------------------------------------------------

  GtkWidget* frame2 = gtk_frame_new(_("Advanced parameters"));
  gtk_box_pack_start_defaults(GTK_BOX(vb), frame2);

  GtkWidget* tab2 = gtk_table_new(7, 3, FALSE);
  row = 0;

  gtk_table_set_col_spacings(GTK_TABLE(tab2), 20);
  gtk_table_set_row_spacings(GTK_TABLE(tab2), 3);
  gtk_container_set_border_width(GTK_CONTAINER(tab2), 5);

  gtk_container_add(GTK_CONTAINER(frame2), tab2);

  // --------------------------------------------------------------------------

  box = gtk_hbox_new (TRUE, FALSE);

  spin_root_frequency_error = gtk_spin_button_new((GtkAdjustment*) 
  			gtk_adjustment_new(conf->ROOT_FREQUENCY_ERROR,
					-500.0, +500.0, 1.0, 10.0, 15.0), 1.0, 1);

  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_root_frequency_error), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_root_frequency_error), 0) ;

  gtk_box_pack_start (GTK_BOX(box), gtk_label_new(_(" A (440 Hz) + ")),
		      FALSE, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX(box), spin_root_frequency_error,
		      FALSE, TRUE, FALSE);

  sprintf(buff, "%%    = %0.2f", conf->ROOT_FREQUENCY);
  label_root_frequency = gtk_label_new(_(buff));

  gtk_box_pack_start (GTK_BOX(box), label_root_frequency, FALSE, FALSE, FALSE);

  gtk_signal_connect (GTK_OBJECT (spin_root_frequency_error), "value_changed",
                      GTK_SIGNAL_FUNC (change_label_a_frequence), this);

  gtk_table_attach_defaults(GTK_TABLE(tab2), 
			    gtk_label_new(_("Root frequency")),
			    0, 1, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), box, 1, 2, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("Hz")),
			    2, 3, row, row + 1);
  row++;

  // --------------------------------------------------------------------------

  spin_temporal_window = 
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf->TEMPORAL_WINDOW, 0.0, 2.0,
					   0.005, 0.1, 0.1), 
			0.005, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_temporal_window), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_temporal_window), 3) ;

  gtk_table_attach_defaults(GTK_TABLE(tab2),
			    gtk_label_new(_("Temporal window")),
			    0, 1, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("seconds")),
			    2, 3, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), spin_temporal_window,
			    1, 2, row, row + 1);
  row++;

  // --------------------------------------------------------------------------

  spin_dft_number =
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf->DFT_NUMBER, 0.0, 10.0,
					   1.0, 1.0, 1.0), 
			1.0, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_dft_number), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_dft_number), 0) ;
  
  gtk_table_attach_defaults(GTK_TABLE(tab2),
			    gtk_label_new(_("DFT number")),
			    0, 1, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), spin_dft_number,
			    1, 2, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("DFTs")),
			    2, 3, row, row + 1);
  row++;

  // --------------------------------------------------------------------------

  spin_dft_size =
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf->DFT_SIZE, 4.0, 100.0,
					   1.0, 1.0, 1.0), 
			1.0, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_dft_size), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_dft_size), 0) ;

  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("DFT size")),
			    0, 1, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("samples")),
			    2, 3, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), spin_dft_size,
			    1, 2, row, row + 1);
  row++;

  // --------------------------------------------------------------------------

  spin_peak_number =
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf->PEAK_NUMBER, 1.0, 10.0,
					   1.0, 1.0, 1.0), 
			1.0, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_peak_number), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_peak_number), 0) ;
  
  gtk_table_attach_defaults(GTK_TABLE(tab2),
			    gtk_label_new(_("Peak number")),
			    0, 1, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), spin_peak_number,
			    1, 2, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("peaks")),
			    2, 3, row, row + 1);
  row++;

  // --------------------------------------------------------------------------

  spin_peak_order =
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf->PEAK_ORDER, 1.0, 5.0,
					   1.0, 1.0, 1.0), 
			1.0, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_peak_order), TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_peak_order), 0) ;

  gtk_table_attach_defaults(GTK_TABLE(tab2),
			    gtk_label_new(_("Peak order")),
			    0, 1, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), spin_peak_order,
			    1, 2, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2),
			    gtk_label_new(_("samples")),
			    2, 3, row, row + 1);
  row++;

  // --------------------------------------------------------------------------

  spin_peak_rejection_relation =
    gtk_spin_button_new(
			(GtkAdjustment*)
			gtk_adjustment_new(conf->PEAK_REJECTION_RELATION,
					   2.0, 40.0, 0.1, 1.0, 1.0), 
			0.1, 1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_peak_rejection_relation),
			      TRUE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_peak_rejection_relation), 1);
  
  gtk_table_attach_defaults(GTK_TABLE(tab2),
			    gtk_label_new(_("Rejection peak relation")),
			    0, 1, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), spin_peak_rejection_relation,
			    1, 2, row, row + 1);
  gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("dB")),
			    2, 3, row, row + 1);
  row++;

  // --------------------------------------------------------------------------

  button_ok = gtk_button_new_from_stock(GTK_STOCK_OK);
  button_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  button_apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
  button_default = gtk_button_new_with_label(_("Default config"));
/*  gtk_button_set_image(GTK_BUTTON(button_default), 
  	gtk_button_get_image(GTK_BUTTON(gtk_button_new_from_stock(GTK_STOCK_UNDO))));*/
	
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (win)->action_area),
                        button_default, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (win)->action_area),
                        button_apply, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (win)->action_area),
                        button_ok, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (win)->action_area),
                        button_cancel, TRUE, TRUE, 0);
                        
  g_signal_connect(GTK_OBJECT(button_default), "clicked",
		     GTK_SIGNAL_FUNC(clicked_cb), this);
  g_signal_connect(GTK_OBJECT(button_apply), "clicked",
		     GTK_SIGNAL_FUNC(clicked_cb), this);
  g_signal_connect(GTK_OBJECT(button_ok), "clicked",
		     GTK_SIGNAL_FUNC(clicked_cb), this);
  g_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		     GTK_SIGNAL_FUNC(clicked_cb), this);

	g_signal_connect(GTK_OBJECT(win), "destroy",
		     GTK_SIGNAL_FUNC(close_cb), this);
		     
  rewrite();
  gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
  gtk_widget_show_all(win);
}

DialogConfig::~DialogConfig() {
	gui->dialog_config = NULL;	
	delete conf;
	delete conf_old;
}

void DialogConfig::apply()
{
  conf->ROOT_FREQUENCY_ERROR = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(spin_root_frequency_error));
  conf->CALCULATION_RATE = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(spin_calculation_rate));
  conf->VISUALIZATION_RATE = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(spin_visualization_rate));
  conf->TEMPORAL_WINDOW = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(spin_temporal_window));
  conf->NOISE_THRESHOLD = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(spin_noise_threshold));
  conf->OVERSAMPLING = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_oversampling));
  conf->DFT_NUMBER = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_dft_number));
  conf->DFT_SIZE = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_dft_size));
  conf->PEAK_NUMBER = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_peak_number));
  conf->PEAK_ORDER = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_peak_order));
  conf->PEAK_REJECTION_RELATION = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_peak_rejection_relation));
  conf->FFT_SIZE = atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo_fft_size)));
  conf->SAMPLE_RATE = atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo_sample_rate)));

  bool result = conf->updateInternalParameters();
  
  gui->changeConfig(conf);
  
	if (!result) {
		GtkWidget* dialog = gtk_message_dialog_new(
			GTK_WINDOW(win),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_INFO,
			GTK_BUTTONS_CLOSE,
			_("Temporal buffer is smaller than FFT size. It has been increased to %0.3f seconds"), conf->TEMPORAL_WINDOW);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy (dialog);		
	}
}


void DialogConfig::close()
{
  gtk_widget_destroy(win);
}
