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

#include "events.h"

// constructor, empty list.
EventScheduler::EventScheduler() {
  list = NULL;
}


// destructor, deletes the list.
EventScheduler::~EventScheduler() {
  t_event_list_node* aux = list;
  t_event_list_node* aux2;
# ifdef DEBUG_EVENTS
  if (aux != NULL) printf(WARNING " Removing non empty event list\n");
# endif
  while (aux != NULL) {
    aux2 = aux->sig;
    delete aux;
    aux = aux2;
  }
}

// sorted insertion.
int EventScheduler::add(t_event* X) {
  
  t_event_list_node* prev = NULL;
  t_event_list_node* current = list;

  // search insertion position.
  while (current != NULL) {
    if (X == current->X) return -1; // the event already was in the list.
    if (timercmp(X, current->X, <)) break;
    prev = current;
    current = current->sig;
  }
  
  t_event_list_node* new_event;
  
  // creates new element.
  new_event = new t_event_list_node;
  new_event->X = X;
  new_event->sig = current;

  // inserts it.
  if (prev == NULL) list = new_event; // at the beginning
  else prev->sig = new_event;         // or at any position.

  return 0;
}

// removes an event from the list.
int EventScheduler::remove(t_event* X) {

  t_event_list_node* prev = NULL;
  t_event_list_node* current = list;

  // where is the event to remove.
  while (current != NULL) {
    if (X == current->X) break;
    prev = current;
    current = current->sig;
  }

  if (current == NULL) return -1; // not found.

  if (prev == NULL) list = current->sig; // removes first node.
  else prev->sig = current->sig;         // or any else.
  
  delete current;

  return 0;
}

//-----------------------------------------------------------------

struct timeval next_event(struct timeval current_time, FLT rate)
{
  struct timeval period, result;

  double period_d = 1.0/rate;
  period.tv_sec  = (long int) period_d;
  period.tv_usec = (long int) (1e6*(period_d - period.tv_sec));

  timeradd(&current_time, &period, &result); 
  return result;
}
