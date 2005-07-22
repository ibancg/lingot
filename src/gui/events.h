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

#ifndef __EVENTOS_H__
#define __EVENTOS_H__

#include <sys/time.h>
#include <time.h>

#include "defs.h"
// gestión de eventos de temporización.

typedef struct timeval t_evento;

struct t_nodo_lista_eventos {
  t_evento*                    X;
  struct t_nodo_lista_eventos* sig;
};


// gestión de eventos mediante una lista simplemente enlazada.
class GestorEventos {
private:
  struct t_nodo_lista_eventos* lista; // lista de eventos.
public:
  GestorEventos();
  ~GestorEventos();
  int  anhadir(t_evento*);
  int  eliminar(t_evento*);
  //  void touch(struct timeval);
  struct timeval* proximo() { return lista->X; }
};

//double         periodo_tasa(t_evento periodo);
//struct timeval tasa_periodo(FLT tasa);

// devuelve cuándo ocurrirá el siguiente evento a partir de una hora actual y una tasa.
struct timeval siguiente_evento(struct timeval tactual, FLT tasa);

#endif
