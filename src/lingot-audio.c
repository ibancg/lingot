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

#include "lingot-defs.h"
#include "lingot-audio.h"

LingotAudio *
lingot_audio_new (int channels, int rate, int format, char *fdsp)
{
  LingotAudio *audio;
#ifdef ALSA
  snd_pcm_hw_params_t *hw_params;
  int err;
#endif

  audio = malloc (sizeof (LingotAudio));
  //  int dma  = 4;

#ifdef ALSA

  if ((err =
       snd_pcm_open (&audio->capture_handle, "default", SND_PCM_STREAM_CAPTURE,
		     0)) < 0)
    {
      fprintf (stderr, "cannot open audio device %s (%s)\n",
	       fdsp, snd_strerror (err));
      exit (1);
    }

  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0)
    {
      fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
	       snd_strerror (err));
      exit (1);
    }

  if ((err = snd_pcm_hw_params_any (audio->capture_handle, hw_params)) < 0)
    {
      fprintf (stderr,
	       "cannot initialize hardware parameter structure (%s)\n",
	       snd_strerror (err));
      exit (1);
    }

  if ((err =
       snd_pcm_hw_params_set_access (audio->capture_handle, hw_params,
				     SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
      fprintf (stderr, "cannot set access type (%s)\n", snd_strerror (err));
      exit (1);
    }

  if ((err =
       snd_pcm_hw_params_set_format (audio->capture_handle, hw_params,
				     SND_PCM_FORMAT_S16_LE)) < 0)
    {
      fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
      exit (1);
    }

  if ((err =
       snd_pcm_hw_params_set_rate_near (audio->capture_handle, hw_params,
					&rate, 0)) < 0)
    {
      fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
      exit (1);
    }

  if ((err =
       snd_pcm_hw_params_set_channels (audio->capture_handle, hw_params,
				       channels)) < 0)
    {
      fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
      exit (1);
    }

  if ((err = snd_pcm_hw_params (audio->capture_handle, hw_params)) < 0)
    {
      fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
      exit (1);
    }

  snd_pcm_hw_params_free (hw_params);

  if ((err = snd_pcm_prepare (audio->capture_handle)) < 0)
    {
      fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
	       snd_strerror (err));
      exit (1);
    }

#else

  audio->dsp = open (fdsp, O_RDONLY);
  if (audio->dsp < 0)
    {
      char buff[80];
      sprintf (buff, "Unable to open audio device %s", fdsp);
      perror (buff);
      exit (-1);
    }

  //if (ioctl(audio->dsp, SOUND_PCM_READ_CHANNELS, &channels) < 0)
  if (ioctl (audio->dsp, SNDCTL_DSP_CHANNELS, &channels) < 0)
    {
      perror ("Error setting number of channels");
      exit (-1);
    }

  /*  if (ioctl(audio->dsp, SOUND_PCM_WRITE_CHANNELS, &channels) < 0)
     {
     perror("Error setting number of channels");
     exit(-1);
     } */

  // sample size
  //if (ioctl(audio->dsp, SOUND_PCM_SETFMT, &format) < 0)
  if (ioctl (audio->dsp, SNDCTL_DSP_SETFMT, &format) < 0)
    {
      perror ("Error setting bits per sample");
      exit (-1);
    }

  int fragment_size = 1;
  int DMA_buffer_size = 512;
  int param = 0;

  for (param = 0; fragment_size < DMA_buffer_size; param++)
    fragment_size <<= 1;

  param |= 0x00ff0000;

  if (ioctl (audio->dsp, SNDCTL_DSP_SETFRAGMENT, &param) < 0)
    {
      perror ("Error setting DMA buffer size");
      exit (-1);
    }

  // DMA divisor
  /*  if (ioctl(audio->dsp, SNDCTL_DSP_SUBDIVIDE, &dma) < 0)
     {
     perror("Error setting DMA divisor");
     exit(-1);
     }

     // Rate de muestreo / reproduccion
     if (ioctl(audio->dsp, SOUND_PCM_WRITE_RATE, &rate) < 0)
     {
     perror("Error setting write rate");
     exit(-1);
     } */

  //  if (ioctl(audio->dsp, SOUND_PCM_READ_RATE, &rate) < 0)
  if (ioctl (audio->dsp, SNDCTL_DSP_SPEED, &rate) < 0)
    {
      perror ("Error setting sample rate");
      exit (-1);
    }

  /*
     // non-blocking reads.
     if (fcntl(audio->dsp, F_SETFL, O_NONBLOCK) < 0)
     {
     perror("Error setting non-blocking reads");
     exit(-1);
     }
   */
#endif

  return audio;
}

void
lingot_audio_destroy (LingotAudio * audio)
{
#ifdef ALSA
  snd_pcm_close (audio->capture_handle);
#else
  close (audio->dsp);
  free (audio);
#endif
}

int
lingot_audio_read (LingotAudio * audio, void *buffer, int size)
{
#ifdef ALSA
  return snd_pcm_readi (audio->capture_handle, buffer, size);
#else
  return read (audio->dsp, buffer, size);
#endif
}

