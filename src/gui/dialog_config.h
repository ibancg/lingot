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

#ifndef __DIALOG_CONFIG_H__
#define __DIALOG_CONFIG_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "config.h"

class Interfaz;

/*--------- Cuadro de diálogo con todas las opciones --------------------*/

void dialog_config_cb(GtkWidget *, Interfaz *);

class DialogConfig {

private:

  GtkWidget *win, *vb, *hb, *frame1, *frame2, *frame3, *frame4, *tab1, *tab2, *vbinfo;

  GtkWidget *spin_calculation_rate, *spin_visualization_rate;
  GtkWidget *spin_oversampling, *spin_root_frequency_error;
  
  GtkWidget *spin_temporal_window, *spin_noise_threshold;
  GtkWidget *spin_dft_number, *spin_dft_size, *spin_peak_number;
  GtkWidget *spin_peak_order, *spin_peak_rejection_relation;
  GtkWidget *combo_fft_size, *combo_sample_rate;

  GtkWidget *bot_ok, *bot_cancel, *bot_apply, *bot_default;
  GtkWidget *label_sample_rate, *label_root_frequency;

  Config    conf;     // configuración provisional
  Config    conf_old; // configuración anterior, por si pulsamos cancel.

  Interfaz* I;

public:

  DialogConfig(Interfaz*);
  void reescribir();
  void aplicar();
  void cerrar();
  
  friend void clicked_cb(GtkButton *b, DialogConfig *d);
//   friend void dialog_config_cb(GtkWidget *, Interfaz *);
  friend void change_label_sample_rate( GtkWidget *widget, DialogConfig *d);
  friend void change_label_a_frequence( GtkWidget *widget, DialogConfig *d);

};


#endif
