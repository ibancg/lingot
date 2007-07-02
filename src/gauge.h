//-*- C++ -*-
/*
  lingot, a musical instrument tuner.

  Copyright (C) 2004-2007  Ib�n Cereijo Gra�a, Jairo Chapela Mart�nez.

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

#ifndef GAUGE_H_
#define GAUGE_H_

#include "defs.h"

class IIR;

/*
 * Implements the dynamic behaviour of the gauge with a digital filter.
 */
class Gauge {
	
	private:
		IIR*	filter;
		FLT		position;
	
	public:
		Gauge(FLT initial_position);
		virtual ~Gauge();
		
		
		void compute(FLT sample);
		FLT getPosition();
};

#endif /*GAUGE_H_*/
