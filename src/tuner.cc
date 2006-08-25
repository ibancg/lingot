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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
//#include <gtk/gtk.h>
#include "defs.h"
#include "gui.h"

char CONFIG_FILE_NAME[100];

int main(int argc, char *argv[])
{
//   setlocale(LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  // default config file.
  sprintf(CONFIG_FILE_NAME, "%s/" CONFIG_DIR_NAME DEFAULT_CONFIG_FILE_NAME,
          getenv("HOME"));

  // TODO: indicar path completo del fichero de configuración.
  if ((argc > 3) || (argc == 2)) {
    printf("\nusage: lingot [-c config]\n\n");
    return -1;
  } else if (argc > 1) {

    if (strcmp(argv[1], "-c") || (argc < 3)) {
      printf("invalid argument\n");
      return -1;
    }
    
    sprintf(CONFIG_FILE_NAME, "%s/%s%s.conf",
          getenv("HOME"), CONFIG_DIR_NAME, argv[2]);
	printf("using config file %s\n", CONFIG_FILE_NAME);	
  }

  // if config file doesn't exists, i will create it.
  FILE* fp;
  if ((fp = fopen(CONFIG_FILE_NAME, "r")) == NULL) {
    
    char config_dir[100];
    sprintf(config_dir, "%s/.lingot/", getenv("HOME"));
    printf("creating directory %s ...\n", config_dir);
    mkdir(config_dir, 0777); // creo el directorio.
    printf("creating file %s ...\n", CONFIG_FILE_NAME);
    
    Config new_conf; // new configuration with default values.
    new_conf.saveConfigFile(CONFIG_FILE_NAME);
    
    printf("ok\n");
    
  } else fclose(fp);

  GUI gui(argc, argv);
  gui.run();

  return 0;
}
