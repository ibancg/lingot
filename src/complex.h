//-*- C++ -*-
/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2007  Ibán Cereijo Graña, Jairo Chapela Martínez.
 *
 * This file is part of lingot.
 *
 * lingot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * lingot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with lingot; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _COMPLEX_H_
#define _COMPLEX_H_

#include <math.h>
#include "defs.h"

// single complex arithmetic  :)

class CPX {

public:

	FLT r, i;

	inline friend void addCPX(CPX A, CPX B, CPX* R) {
		R->r = A.r + B.r;
		R->i = A.i + B.i;
	}

	inline friend void subCPX(CPX A, CPX B, CPX* R) {
		R->r = A.r - B.r;
		R->i = A.i - B.i;
	}

	inline friend void mulCPX(CPX A, CPX B, CPX* R) {
		R->r = A.r*B.r- A.i*B.i;
		R->i = A.i*B.r+ A.r*B.i;
	}
};

#endif
