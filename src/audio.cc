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

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <stdlib.h>

#include "defs.h"
#include "audio.h"

audio::audio(int channels, int rate, int format, char* fdsp) {
	//  int dma  = 4;

	dsp = open(fdsp, O_RDONLY);
	if (dsp < 0) {
		char buff[80];
		sprintf(buff, "Unable to open audio device %s", fdsp);
		perror(buff);
		exit(-1);
	}

	//if (ioctl(dsp, SOUND_PCM_READ_CHANNELS, &channels) < 0)
	if (ioctl(dsp, SNDCTL_DSP_CHANNELS, &channels) < 0) {
		perror("Error setting number of channels");
		exit(-1);
	}

	/*  if (ioctl(dsp, SOUND_PCM_WRITE_CHANNELS, &channels) < 0)
	 {
	 perror("Error setting number of channels");
	 exit(-1);
	 }*/

	// sample size
	//if (ioctl(dsp, SOUND_PCM_SETFMT, &format) < 0)
	if (ioctl(dsp, SNDCTL_DSP_SETFMT, &format) < 0) {
		perror("Error setting bits per sample");
		exit(-1);
	}

	int fragment_size = 1;
	int DMA_buffer_size = 512;
	int param = 0;

	for (param = 0; fragment_size < DMA_buffer_size; param++)
		fragment_size <<= 1;

	param |= 0x00ff0000;

	if (ioctl(dsp, SNDCTL_DSP_SETFRAGMENT, &param) < 0) {
		perror("Error setting DMA buffer size");
		exit(-1);
	}

	// DMA divisor
	/*  if (ioctl(dsp, SNDCTL_DSP_SUBDIVIDE, &dma) < 0)
	 {
	 perror("Error setting DMA divisor");
	 exit(-1);
	 }
	 
	 // Rate de muestreo / reproduccion
	 if (ioctl(dsp, SOUND_PCM_WRITE_RATE, &rate) < 0)
	 {
	 perror("Error setting write rate");
	 exit(-1);
	 }*/

	//  if (ioctl(dsp, SOUND_PCM_READ_RATE, &rate) < 0)
	if (ioctl(dsp, SNDCTL_DSP_SPEED, &rate) < 0) {
		perror("Error setting sample rate");
		exit(-1);
	}

	/*
	 // non-blocking reads.
	 if (fcntl(dsp, F_SETFL, O_NONBLOCK) < 0)
	 {
	 perror("Error setting non-blocking reads");
	 exit(-1);
	 }
	 */
}

audio::~audio() {
	close(dsp);
}

int audio::read(void *buffer, int size) {
	return:: read(dsp, buffer, size);
}
