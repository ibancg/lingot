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

#ifndef __INTERFAZ_H__
#define __INTERFAZ_H__

#include "defs.h"
#include "core.h"
#include "events.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>

class DialogConfig;

//  La clase Interfaz es una Ventana que contiene todos los controles,
//  gráficos, etc, del afinador.
class Interfaz : public Analizador {

private:

  GestorEventos  GE;

  // widgets gtk
  GtkWidget      *win, *vb, *frame1, *frame3, *frame4, *hb, *vbinfo;
  GtkWidget      *aguja, *espectro, *info_nota, *info_freq, *info_err, *menu;
  GdkPixmap      *pixagj, *pixesp;
  GdkPixmap      *pegata;
  int            tout_handle;
  bool           quit;
  Filtro*        filtro_frecuencia;
  char           cad_error[30], cad_freq[30];
  char           *notaactual;

  Filtro*        filtro_aguja;
  FLT            valor_aguja; // valor de la aguja [-0.5, 0.5]

  void configuraFiltroAguja();
  void dibujarAguja();
  void dibujarEspectro();
  void allocColor(GdkColor *col, int r, int g, int b);
  void fgColor(GdkGC *gc, int r, int g, int b);
  void bgColor(GdkGC *gc, int r, int g, int b);

public:

  DialogConfig  *dc;

  Interfaz();
  ~Interfaz();

  void redibujar();
  void mainLoop();
  void ponerFrecuencia();
  void redimensionar(Config);

  friend void callbackDestroy(GtkWidget *w, void *data);
  friend void callbackRedibujar(GtkWidget *w, GdkEventExpose *e, void *data);
};


void quick_message(gchar *title, gchar *message);

#endif //__INTERFAZ_H__
