//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004, 2005  Ibán Cereijo Graña, Jairo Chapela Martínez.

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

#ifndef __INTERFAZ_H__
#define __INTERFAZ_H__

#include "defs.h"
#include "core.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>

class DialogConfig;

// Window that contains all controls, graphics, etc. of the tuner.
class GUI : public Core {

private:

  // widgets gtk
  GtkWidget      *win, *vb, *frame1, *frame3, *frame4, *hb, *vbinfo;
  GtkWidget      *gauge, *spectrum, *note_info, *freq_info, *error_info, *menu;
  GdkPixmap      *pix_spectrum, *pix_stick;
  bool           quit;
  IIR*           freq_filter;

  IIR*           gauge_filter;
  FLT            gauge_value; // valor de la aguja [-0.5, 0.5]

  void setupGaugeFilter();
  void allocColor(GdkColor *col, int r, int g, int b);
  void fgColor(GdkGC *gc, int r, int g, int b);
  void bgColor(GdkGC *gc, int r, int g, int b);

public:

  DialogConfig  *dc;

  GUI();
  ~GUI();

  void redraw();
  void mainLoop();
  void putFrequency();
  void resize(Config);
  void drawGauge();
  void drawSpectrum();

  friend void callbackDestroy(GtkWidget *w, void *data);
  friend void callbackRedraw(GtkWidget *w, GdkEventExpose *e, void *data);
};


void quick_message(gchar *title, gchar *message);

#endif //__INTERFAZ_H__
