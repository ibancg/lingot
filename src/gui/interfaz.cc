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
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "defs.h"

/* Nuestros includes */
#include "filtro.h"
#include "interfaz.h"
#include "dialog_config.h"
#include "analizador.h"
#include "eventos.h"

/* así de fácil es incluir un pixmap */
#include "../background.xpm"

void callbackRedibujar(GtkWidget *w, GdkEventExpose *e, void *data)
{
  ((Interfaz*)data)->redibujar();
}

void callbackDestroy(GtkWidget *w, void *data)
{
  gtk_timeout_remove(((Interfaz*)data)->tout_handle);
  ((Interfaz*)data)->quit = true;
}

void callbackAbout(GtkWidget *w, void *data)
{
  quick_message(gettext("about lingot"), 
		gettext("\nlingot " VERSION ", (c) 2004\n"
			"\n"
			"Ibán Cereijo Graña <comominimo@hotpop.com>\n"
			"Jairo Chapela Martínez <jairochapela@terra.es>\n\n"));
}

Interfaz::Interfaz() : Analizador()
{

  dc = NULL;
  quit = false;
  pegata = NULL;

//   cogerConfig(&conf);

  valor_aguja = conf.VRP; //aguja en reposo

  //
  // ----- CONFIGURACIÓN DEL FILTRO DE AGUJA/ERROR -----
  //
  // modelo dinámico de la aguja, ecuación en diferencias basada en la
  // ecuación diferencial:
  //               2
  //              d                              d      
  //              --- v(t) = k (e(t) - v(t)) - q -- v(t)
  //                2                            dt     
  //              dt
  //
  // la aceleración de la posición de la aguja v(t) depende linealmente
  // de la diferencia respecto al estímulo e(t), introduciendo un coefi-
  // ciente de rozamiento, la aceleración disminuye proporcionalmente a
  // la velocidad.
  //
  // aproximación ecuación en diferencias (válida para elevada frecuencia
  // de muestreo fs):
  //
  //                 d
  //                 -- v(t) ~= (v[n] - v[n - 1])*fs
  //                 dt
  //
  //            2
  //           d                                            2
  //           --- v(t) ~= (v[n] - 2*v[n - 1] + v[n - 2])*fs
  //             2
  //           dt
  //

  FLT k = 60;   // velocidad de respuesta de la aguja.
  FLT q = 6;  // coeficiente de amortiguación.

  FLT filtro_aguja_a[] = {
    k + NEEDLE_RATE*(q + NEEDLE_RATE),
    -NEEDLE_RATE*(q + 2.0*NEEDLE_RATE),  
      NEEDLE_RATE*NEEDLE_RATE };
  FLT filtro_aguja_b[] = { k };

  filtro_aguja = new Filtro( 2, 0, filtro_aguja_a, filtro_aguja_b );

  // ----- CONFIGURACIÓN DEL FILTRO DE FRECUENCIA ------

  // filtro FIR, promediador de 8 muestras.
  //FLT filtro_frecuencia_a[] = { 1.0 };
  //FLT filtro_frecuencia_b[] = { 1.0/8.0, 1.0/8.0, 1.0/8.0, 1.0/8.0, 1.0/8.0, 1.0/8.0, 1.0/8.0, 1.0/8.0 };

  // filtro IIR paso bajo.
  FLT filtro_frecuencia_a[] = { 1.0, -0.5 };
  FLT filtro_frecuencia_b[] = { 0.5 };
  
  filtro_frecuencia = new Filtro( 1, 0, filtro_frecuencia_a, filtro_frecuencia_b );
    
  // ---------------------------------------------------

  gtk_rc_parse_string(
 		      "style \"title\""
 		      "{"
		      "font = \"-*-helvetica-bold-r-normal--32-*-*-*-*-*-*-*\""
 		      "}"
		      "widget \"*label_nota\" style \"title\""
 		      );


  // creamos la ventana
  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (win), gettext("lingot"));

  gtk_container_set_border_width(GTK_CONTAINER(win), 6);

  // no nos interesa que se pueda redimensionar la ventana
  gtk_window_set_policy(GTK_WINDOW(win), FALSE, TRUE, FALSE);  

  // organización tabular mediante el siguiente contenedor
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
      // lo primero que metemos en la caja vertical, es la barra de menú
      gtk_box_pack_start_defaults(GTK_BOX(vb), menu);*/

  /* Nuevo menú creado manualmente, para pasar parámetros.*/

  GtkWidget* tuner_menu = gtk_menu_new();    /* No hay que mostrar menús */
  GtkWidget* help_menu = gtk_menu_new();    /* No hay que mostrar menús */

  
  /* Crear los elementos del menú */
  GtkWidget* options_item = gtk_menu_item_new_with_label(gettext("Options"));
  GtkWidget* quit_item = gtk_menu_item_new_with_label(gettext("Quit"));

  GtkWidget* about_item = gtk_menu_item_new_with_label(gettext("About"));


  /* Añadirlos al menú */
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
  
  // un contenedor de tipo fixed nos servirá para poner en posiciones
  // fijas los dos cuadros superiores
  hb = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(vb), hb);
  
  // un cuadro para mostrar la aguja
  frame1 = gtk_frame_new(gettext("Deviation"));
  //  gtk_fixed_put(GTK_FIXED(fix), frame1, 0, 0);
  gtk_box_pack_start_defaults(GTK_BOX(hb), frame1);

  
  // otro cuadro para mostrar la nota
  frame3 = gtk_frame_new(gettext("Note"));
  //   gtk_fixed_put(GTK_FIXED(fix), frame3, 164, 0);
  gtk_box_pack_start_defaults(GTK_BOX(hb), frame3);

  // finalmente, otro cuadro para mostrar el espectro, situado en la
  // parte inferior
  frame4 = gtk_frame_new(gettext("Spectrum"));
  gtk_box_pack_end_defaults(GTK_BOX(vb), frame4);
  
  // para dibujar la aguja
  aguja = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(aguja), 160, 100);
  gtk_container_add(GTK_CONTAINER(frame1), aguja);
  
  // para dibujar el espectro
  espectro = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(espectro), 256, 64);
  gtk_container_add(GTK_CONTAINER(frame4), espectro);
  
  // para mostrar la nota y la frecuencia
  vbinfo = gtk_vbox_new(FALSE, 0);
  //   gtk_widget_set_usize(GTK_WIDGET(vbinfo), 80, 100);
  gtk_container_add(GTK_CONTAINER(frame3), vbinfo);

  info_freq = gtk_label_new(gettext("freq"));
  gtk_box_pack_start_defaults(GTK_BOX(vbinfo), info_freq);

  info_err = gtk_label_new(gettext("err"));
  gtk_box_pack_end_defaults(GTK_BOX(vbinfo), info_err);

  info_nota = gtk_label_new(gettext("nota"));
  gtk_widget_set_name(info_nota, "label_nota");
  gtk_box_pack_end_defaults(GTK_BOX(vbinfo), info_nota);

  // mostramos todo
  gtk_widget_show_all(win);

  // un par de pixmaps para implementar el doble buffer (pantalla
  // virtual) de los gráficos de la aguja y el espectro.
  pixagj = gdk_pixmap_new(aguja->window, 160, 100, -1);
  pixesp = gdk_pixmap_new(aguja->window, 256, 64, -1);

  // señales GTK
  gtk_signal_connect(GTK_OBJECT(aguja), "expose_event",
 		     (GtkSignalFunc)callbackRedibujar, this);
  gtk_signal_connect(GTK_OBJECT(espectro), "expose_event",
		     (GtkSignalFunc)callbackRedibujar, this);
  gtk_signal_connect(GTK_OBJECT(win), "destroy",
 		     (GtkSignalFunc)callbackDestroy, this);

  // función de temporización para representación periódica
  //  tout_handle = gtk_timeout_add(50, (GtkFunction)callbackTout, this);
}


Interfaz::~Interfaz()
{
  delete filtro_aguja;
  delete filtro_frecuencia;
  if (dc) delete dc;
}


void Interfaz::allocColor(GdkColor *col, int r, int g, int b)
{
  static GdkColormap* cmap = gdk_colormap_get_system();
  col->red   = r;
  col->green = g;
  col->blue  = b;    	    
  gdk_color_alloc(cmap, col);    
}

void Interfaz::fgColor(GdkGC *gc, int r, int g, int b)
{
  GdkColor col;
  allocColor(&col, r, g, b);
  gdk_gc_set_foreground(gc, &col);
}

void Interfaz::bgColor(GdkGC *gc, int r, int g, int b)
{
  GdkColor col;
  allocColor(&col, r, g, b);
  gdk_gc_set_background(gc, &col);
}

void Interfaz::redibujar()
{
  dibujarAguja();
  dibujarEspectro();
}

void Interfaz::dibujarAguja()
{ 
  GdkGC* gc = aguja->style->fg_gc[aguja->state];
  GdkWindow* w = aguja->window;
  GdkGCValues gv;

  gdk_gc_get_values(gc, &gv);
  
  /* No se donde poner esto aún */
  //  FLT tol = 0.1;
  FLT max = 1.0;

  // primero pintamos el fondo
  if(!pegata) {
    pegata = gdk_pixmap_create_from_xpm_d(aguja->window, NULL, NULL,
					  background2_xpm);
  }
  gdk_draw_pixmap(aguja->window, gc, pegata, 0, 0, 0, 0, 160, 100);
  
  // finalmente dibujamos la aguja
  fgColor(gc, 0xC000, 0x0000, 0x2000);

  gdk_draw_line(w, gc, 80, 99,
 		80 + (int)rint(90.0*sin(valor_aguja*M_PI/(1.5*max))),
 		99 - (int)rint(90.0*cos(valor_aguja*M_PI/(1.5*max))));

  // y ponemos un bordillo negro al gráfico, y unas letritas, que
  // queda muy estético.  
  fgColor(gc, 0x0000, 0x0000, 0x0000);
  /*  gdk_draw_string(w, gv.font, gc, 5, 80, "½b");
      gdk_draw_string(w, gv.font, gc, 135, 80, "½#");*/ // ESTO DA WARNINGS!!!!
  gdk_draw_rectangle(w, gc, FALSE, 0, 0, 159, 99);

  gdk_draw_pixmap(aguja->window, gc, w, 0, 0, 0, 0, 160, 100);
  gdk_flush();
  //   Flush();
}


void Interfaz::dibujarEspectro()
{
  GdkGC* gc = espectro->style->fg_gc[espectro->state];
  GdkWindow* w = pixesp; //espectro->window;
  GdkGCValues gv;
  gdk_gc_get_values(gc, &gv);
  
  // primero borramos todo
  fgColor(gc, 0x1111, 0x3333, 0x1111);
  //   gdk_gc_set_foreground(gc, &espectro->style->bg[espectro->state]);
  gdk_draw_rectangle(w, gc, TRUE, 0, 0, 256, 64);
  
  //   CambiaColor(0x7000, 0x7000, 0x7000);

  //   // cuadrícula para dar un toque profesional.
  int Nlx = 9;
  int Nly = 3;

  fgColor(gc, 0x7000, 0x7000, 0x7000);
  //   gdk_gc_set_foreground(gc, &espectro->style->dark[espectro->state]);
  for (int i = 0; i <= Nly; i++)
    gdk_draw_line(w, gc, 0, i*63/Nly, 255, i*63/Nly);
  for (int i = 0; i <= Nlx; i++)
    gdk_draw_line(w, gc, i*255/Nlx, 0, i*255/Nlx, 63);

  //   CambiaColor(0xAAAA, 0xEEEE, 0x0000);
  fgColor(gc, 0x2222, 0xFFFF, 0x2222);
  //   fgColor(gc, 0x4000, 0x4000, 0x4000);
  //   gdk_gc_set_foreground(gc, &espectro->style->fg[espectro->state]);
   
# define PLOT_GAIN  8
   
  // dibujamos el espectro y calculamos la frecuencia visual fundamental.
  for (register unsigned int i = 0; (i < conf.FFT_SIZE) && (i < 256); i++) {    
    int j = (X[i] > 1.0) ? (int) (64 - PLOT_GAIN*log10(X[i])) : 64;   // en dB.
    gdk_draw_line(w, gc, i, 63, i, j);
  }

  if (frecuencia != 0.0) {

    // marcamos la frecuencia fundamental con un puntorro de otro color. 
    fgColor(gc, 0xFFFF, 0x2222, 0x2222);
    // índice de la muestra más próxima a la frecuencia fundamental.
    int i = (int) rint(frecuencia*conf.FFT_SIZE*conf.OVERSAMPLING/conf.SAMPLE_RATE);
    int j = (X[i] > 1.0) ? (int) (64 - PLOT_GAIN*log10(X[i])) : 64;   // en dB.
    gdk_draw_rectangle(w, gc, TRUE, i-1, j-1, 3, 3);
  }

# undef  PLOT_GAIN

  fgColor(gc, 0x0000, 0x0000, 0x0000);

  //   Flush();
  //   Flip();
  gdk_draw_pixmap(espectro->window, gc, w, 0, 0, 0, 0, 256, 64);
  gdk_flush();
}


void Interfaz::ponerFrecuencia()
{
  // indexación de notas por semitonos.
  static char* cadena_nota[] = { "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#" };
  const FLT Log2 = log(2.0);

  FLT traste_f, error;
  int traste;
 
  if (frecuencia < 10.0) {
  
    notaactual = "---";
    strcpy(cad_error, "e = ---");
    strcpy(cad_freq, "f = ---");
    error = filtro_aguja->filtrarII(conf.VRP);
    //error = conf.VRP;

  } else {
  
    // subo unas cuantas octavas para evitar un traste negativo.
    traste_f = log(frecuencia/conf.ROOT_FREQUENCY)/Log2*12.0 + 12e2;

    error = filtro_aguja->filtrarII(traste_f - rint(traste_f));
    //error = traste_f - rint(traste_f);
  
    traste = ((int) rint(traste_f)) % 12;
  
    notaactual = cadena_nota[traste];
    sprintf(cad_error, "e = %+4.2f%%", error*100.0);
    sprintf(cad_freq, "f = %6.2f Hz", filtro_frecuencia->filtrarII(frecuencia));
  }

  valor_aguja = error;  

  gtk_label_set_text(GTK_LABEL(info_freq), cad_freq);
  gtk_label_set_text(GTK_LABEL(info_err), cad_error);
  gtk_label_set_text(GTK_LABEL(info_nota), notaactual);
}


void Interfaz::mainLoop()
{
  struct timeval  t_actual, t_diff;

  t_evento        e_aguja, e_gtk, e_vis, e_calculo;
  t_evento*       proximo_tout;

  gettimeofday(&t_actual, NULL);

  e_vis     = siguiente_evento(t_actual, conf.VISUALIZATION_RATE);
  e_aguja   = siguiente_evento(t_actual, NEEDLE_RATE);
  e_gtk     = siguiente_evento(t_actual, GTK_EVENTS_RATE);
  e_calculo = siguiente_evento(t_actual, conf.CALCULATION_RATE);

  GE.anhadir(&e_aguja);
  GE.anhadir(&e_gtk);
  GE.anhadir(&e_vis);
  GE.anhadir(&e_calculo);

  while(!quit) {

    proximo_tout = GE.proximo(); // consulto el próximo evento.
    gettimeofday(&t_actual, NULL);
    //    printf("hora actual: %6.6f\n", 1.0*t_actual.tv_sec + 1e-6*t_actual.tv_usec);
    //    printf("proximo evento: %6.6f\n", 1.0*proximo_tout->tv_sec + 1e-6*proximo_tout->tv_usec);

    if (timercmp(proximo_tout, &t_actual, >)) {
      timersub(proximo_tout, &t_actual, &t_diff);
      select(0, NULL, NULL, NULL, &t_diff);
      gettimeofday(&t_actual, NULL);
    }

    GE.eliminar(proximo_tout);

    if (proximo_tout == &e_vis) {
      e_vis   = siguiente_evento(t_actual, conf.VISUALIZATION_RATE);
      //      printf("Despierta para representar aguja\n");
      GE.anhadir(&e_vis);
      dibujarAguja();
    }

    if (proximo_tout == &e_aguja) {
      e_aguja   = siguiente_evento(t_actual, NEEDLE_RATE);
      GE.anhadir(&e_aguja);	
      //printf("Despierta para calcular posición aguja\n");
      ponerFrecuencia();
    } 

    if (proximo_tout == &e_gtk) {
      e_gtk = siguiente_evento(t_actual, GTK_EVENTS_RATE);
      GE.anhadir(&e_gtk);	
      //      printf("Despierta para atender eventos GTK\n");
      while(gtk_events_pending()) gtk_main_iteration_do(FALSE);
    }

    if (proximo_tout == &e_calculo) {
      e_calculo = siguiente_evento(t_actual, conf.CALCULATION_RATE);
      GE.anhadir(&e_calculo);	
      //      printf("Despierta para representar espectro\n");
      dibujarEspectro();
    }
  }
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
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT(dialog));
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area),
		     okay_button);

  /* Add the label, and show everything we've added to the dialog. */
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
		     label);
  gtk_widget_show_all (dialog);
}
