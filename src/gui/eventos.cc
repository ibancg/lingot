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

#include "eventos.h"

// constructor, lista inicialmente vacía.
GestorEventos::GestorEventos() {
  lista = NULL;
}


// destructor, borra la lista.
GestorEventos::~GestorEventos() {
  t_nodo_lista_eventos* aux = lista;
  t_nodo_lista_eventos* aux2;
# ifdef DEBUG_EVENTOS
  if (aux != NULL) printf(WARNING " Borrando lista de eventos no vacía\n");
# endif
  while (aux != NULL) {
    aux2 = aux->sig;
    delete aux;
    aux = aux2;
  }
}

// hay que insertar el nuevo evento ordenado cronológicamente.
int GestorEventos::anhadir(t_evento* X) {
  
  t_nodo_lista_eventos* anterior = NULL;
  t_nodo_lista_eventos* actual = lista;

  // recorro la lista para ver dónde inserto el nuevo evento.
  while (actual != NULL) {
    if (X == actual->X) return -1; // el evento ya estaba en la lista.
    if (timercmp(X, actual->X, <)) break;
    anterior = actual;
    actual = actual->sig;
  }
  
  t_nodo_lista_eventos* nuevo;
  
  // creo el nuevo elemento.
  nuevo= new t_nodo_lista_eventos;
  nuevo->X = X;
  nuevo->sig = actual;

  // y lo inserto.
  if (anterior == NULL) lista = nuevo; // al principio.
  else anterior->sig = nuevo;          // o en general.

  return 0;
}

// elimina un elemento.
int GestorEventos::eliminar(t_evento* X) {

  t_nodo_lista_eventos* anterior = NULL;
  t_nodo_lista_eventos* actual = lista;

  // recorro la lista para ver dónde está el elemento víctima.
  while (actual != NULL) {
    if (X == actual->X) break;
    anterior = actual;
    actual = actual->sig;
  }

  if (actual == NULL) return -1; // elemento no encontrado.

  // debo eliminar el nodo actual, referenciado por el anterior.
  if (anterior == NULL) lista = actual->sig; // borro el primer nodo.
  else anterior->sig = actual->sig; // u otro cualquiera
  
  delete actual;

  return 0;
}

/*
// retoca eventos de timeout (resta un determinado valor a todos, no modifica orden).
void GestorEventos::touch(struct timeval diff) {

  t_nodo_lista_eventos* actual = lista;

  // recorro la lista.
  while (actual != NULL) {

    if (timercmp(actual->X, &diff, >))
      timersub(actual->X, &diff, actual->X);
    else actual->X->tv_usec = actual->X->tv_sec = 0;
    
    actual = actual->sig;
  }
  
}
*/
//-----------------------------------------------------------------

/*double periodo_tasa(t_evento periodo)
{
  unsigned long int usec = periodo.tv_usec + 1000000*periodo.tv_sec;
  return 1e6/usec;
}

struct timeval tasa_periodo(double tasa)
{
  struct timeval periodo;

  double periodo_d = 1.0/tasa;
  periodo.tv_sec  = (long int) periodo_d;
  periodo.tv_usec = (long int) (1e6*(periodo_d - periodo.tv_sec));
  return periodo;
}*/

struct timeval siguiente_evento(struct timeval tactual, FLT tasa)
{
  struct timeval periodo, resul;

  double periodo_d = 1.0/tasa;
  periodo.tv_sec  = (long int) periodo_d;
  periodo.tv_usec = (long int) (1e6*(periodo_d - periodo.tv_sec));

  timeradd(&tactual, &periodo, &resul); 
  return resul;
}
