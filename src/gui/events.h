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

#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <sys/time.h>
#include <time.h>

#include "defs.h"

// temporization events scheduling.

typedef struct timeval t_event;


// event scheduling by single list.
struct t_event_list_node {
  t_event*                 X;
  struct t_event_list_node* sig;
};


// event scheduler class.
class EventScheduler {
private:
  struct t_event_list_node* list; // event list.
public:
  EventScheduler();
  ~EventScheduler();
  int  add(t_event*);
  int  remove(t_event*);
  struct timeval* next() { return list->X; }
};

// returns the expiration time for the next event, given a current time and
// an ocurrence rate.
struct timeval next_event(struct timeval current_time, FLT time);

#endif
