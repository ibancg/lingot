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

#ifndef __DIALOG_CONFIG_H__
#define __DIALOG_CONFIG_H__

class GUI;

/*--------- Dialog box with multiple options --------------------*/

class DialogConfig {

private:


	// widgets that contains configuration information.
  GtkWidget*	spin_calculation_rate;
  GtkWidget*	spin_visualization_rate;
  GtkWidget*	spin_oversampling;
  GtkWidget*	spin_root_frequency_error;  
  GtkWidget*	spin_temporal_window;
  GtkWidget*	spin_noise_threshold;
  GtkWidget*	spin_dft_number;
  GtkWidget*	spin_dft_size;
  GtkWidget*	spin_peak_number;
  GtkWidget*	spin_peak_order;
  GtkWidget*	spin_peak_rejection_relation;
  GtkWidget*	combo_fft_size;
  GtkWidget*	combo_sample_rate;

  GtkWidget*	button_ok;
  GtkWidget*	button_cancel;
  GtkWidget*	button_apply;
  GtkWidget*	button_default;
  
  GtkWidget*	label_sample_rate;
  GtkWidget*	label_root_frequency;

  Config*			conf;     // provisional configuration.
  Config*			conf_old; // restoration point for cancel.

  GtkWidget*	win; // window

  GUI*				gui;

public:

	DialogConfig(GUI*);
	virtual ~DialogConfig();
	
  void rewrite();
  void apply();
  void close();
  
  friend void clicked_cb(GtkButton *b, DialogConfig *d);
  
  friend void change_label_sample_rate( GtkWidget *widget, DialogConfig *d);
  friend void change_label_a_frequence( GtkWidget *widget, DialogConfig *d);
};


#endif
