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
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <stdlib.h>

#include "defs.h"
#include "audio.h"

audio::audio(int canales, int frecuencia, int formato, char* fdsp)
{
  //  int dma  = 4;

  audio::freq = frecuencia;

  // Abre el dsp
  dsp = open(fdsp, O_RDONLY);
  if (dsp < 0)
    {
      char buff[80];
      sprintf(buff, "No puedo abrir el dispositivo de audio %s", fdsp);
      perror(buff);
      exit(-1);
    }

  // Los canales...
  //if (ioctl(dsp, SOUND_PCM_READ_CHANNELS, &canales) < 0)
  if (ioctl(dsp, SNDCTL_DSP_CHANNELS, &canales) < 0)
      {
	perror("error al poner el número de canales");
	exit(-1);
      }
  
  /*  if (ioctl(dsp, SOUND_PCM_WRITE_CHANNELS, &canales) < 0)
      {
      perror("error al poner el número de canales");
      exit(-1);
      }*/
  
  // Tamaño de las muestras
  //if (ioctl(dsp, SOUND_PCM_SETFMT, &formato) < 0)
  if (ioctl(dsp, SNDCTL_DSP_SETFMT, &formato) < 0)
    {
      perror("error al poner los bits por muestra");
      exit(-1);
    }

  int tam_fragmento = 1;
  int tam_buffer_DMA = 512;
  int parametro = 0;

  for (parametro = 0; tam_fragmento < tam_buffer_DMA; parametro++)
    tam_fragmento <<= 1;

  parametro |= 0x00ff0000;

  if (ioctl(dsp, SNDCTL_DSP_SETFRAGMENT, &parametro) < 0)
    {
      perror("error al poner el tamaño de buffer de dma");
      exit(-1);
    }

  // Divisor de DMA
  /*  if (ioctl(dsp, SNDCTL_DSP_SUBDIVIDE, &dma) < 0)
    {
      perror("error al poner el divisor de dma");
      exit(-1);
      }*/

  /*  // Frecuencia de muestreo / reproduccion
  if (ioctl(dsp, SOUND_PCM_WRITE_RATE, &freq) < 0)
    {
      perror("error al poner la frecuencia de reproducción");
      exit(-1);
      }*/

  //  if (ioctl(dsp, SOUND_PCM_READ_RATE, &freq) < 0)
  if (ioctl(dsp, SNDCTL_DSP_SPEED, &freq) < 0)
    {
      perror("error al poner la frecuencia de muestreo");
      exit(-1);
    }

  /*
  // vamos a hacer las lecturas de manera NO BLOQUEANTE.
  if (fcntl(dsp, F_SETFL, O_NONBLOCK) < 0)
  {
  perror("error al poner lectura no bloqueante");
  exit(-1);
  }
  */
}

audio::~audio()
{
  close(dsp);
}

int audio::lee(void *buffer, int tamanho)
{
  return read(dsp, buffer, tamanho);
}

void audio::escribe(void *buffer, int tamanho)
{
  write(dsp, buffer, tamanho);
}
