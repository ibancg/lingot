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

#ifndef __INTERFAZ_H__
#define __INTERFAZ_H__

#include "defs.h"
#include "core.h"

#include <gtk/gtk.h>

class DialogConfig;
class Gauge;
class Config;

// Window that contains all controls, graphics, etc. of the tuner.
class GUI {

private:

	// widgets gtk
	GtkWidget* gauge_area;
	GtkWidget* spectrum_area;
	GtkWidget* note_label;

	GtkWidget* freq_label;
	GtkWidget* error_label;

	GdkPixmap* pix_spectrum;
	GdkPixmap* pix_stick;

	bool quit;
	IIR* freq_filter;

	Gauge* gauge;

	Core* core;

public:

	GtkWidget* win;

	DialogConfig* dialog_config;
	Config* conf;

	GUI(int argc, char *argv[]);
	~GUI();

	void run();

	void redraw();
	void putFrequency();
	void changeConfig(Config*);
	void drawGauge();
	void drawSpectrum();

	friend void callbackDestroy(GtkWidget* w, GUI* gui);
};

#endif //__INTERFAZ_H__
