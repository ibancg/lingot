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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "defs.h"
#include "interfaz.h"

char* CONFIG_FILE;

int main(int argc, char *argv[])
{
//   setlocale(LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  gtk_init(&argc, &argv);
  gtk_set_locale();

  if (argc > 1) {

    if (strcmp(argv[1], "-c") || (argc < 3)) {
      printf("invalid argument\n");
      return -1;
    }
    
    CONFIG_FILE = new char[100];
    memset(CONFIG_FILE, 0, sizeof(CONFIG_FILE));
    sprintf(CONFIG_FILE, "%s/.lingot/%s.conf", getenv("HOME"), argv[2]);
  } else {

    // Obtengo el directorio home a partir de una variable de entorno.
    CONFIG_FILE = new char[100];
    memset(CONFIG_FILE, 0, sizeof(CONFIG_FILE));
    sprintf(CONFIG_FILE, "%s/%s", getenv("HOME"), CONFIG_FILE_HOME);

  }

  // veo si existe el archivo de configuración, si no existe lo creo.
  FILE* fp;
  if ((fp = fopen(CONFIG_FILE, "r")) == NULL) {
    
    char config_dir[100];
    sprintf(config_dir, "%s/.lingot/", getenv("HOME"));
    printf("creating directory %s ...\n", config_dir);
    mkdir(config_dir, 0777); // creo el directorio.
    printf("creating file %s ...\n", CONFIG_FILE);
    
    Config nueva_conf; // creo una nueva configuracion con valores por defecto.
    nueva_conf.guardaArchivoConf(CONFIG_FILE); // y la guardo.
    
    printf("ok\n");
    
  } else fclose(fp);

  Interfaz* V = new Interfaz();
  V->mainLoop();

  delete V;
  delete [] CONFIG_FILE;
  
  return 0;
}
