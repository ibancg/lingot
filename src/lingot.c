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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "lingot-defs.h"
#include "lingot-config.h"
#include "lingot-mainframe.h"
#include "lingot-i18n.h"

char CONFIG_FILE_NAME[100];

int main(int argc, char *argv[])
  {

#ifdef ENABLE_NLS	
    bindtextdomain (GETTEXT_PACKAGE, LINGOT_LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    // default config file.
    sprintf(CONFIG_FILE_NAME, "%s/" CONFIG_DIR_NAME DEFAULT_CONFIG_FILE_NAME,
        getenv("HOME"));

    // TODO: indicate complete config file path
    if ((argc > 3) || (argc == 2))
      {
        printf("\nusage: lingot [-c config]\n\n");
        return -1;
      }
    else if (argc > 1)
      {

        if (strcmp(argv[1], "-c") || (argc < 3))
          {
            printf("invalid argument\n");
            return -1;
          }

        sprintf(CONFIG_FILE_NAME, "%s/%s%s.conf", getenv("HOME"),
            CONFIG_DIR_NAME, argv[2]);
        printf("using config file %s\n", CONFIG_FILE_NAME);
      }

    // if config file doesn't exists, i will create it.
    FILE* fp;
    if ((fp = fopen(CONFIG_FILE_NAME, "r")) == NULL)
      {

        char config_dir[100];
        sprintf(config_dir, "%s/.lingot/", getenv("HOME"));
        printf("creating directory %s ...\n", config_dir);
        mkdir(config_dir, 0777); // creo el directorio.
        printf("creating file %s ...\n", CONFIG_FILE_NAME);

        LingotConfig new_conf; // new configuration with default values.
        lingot_config_save(&new_conf, CONFIG_FILE_NAME);

        printf("ok\n");

      }
    else
    fclose(fp);

    LingotMainFrame* gui = lingot_mainframe_new(argc, argv);
    lingot_mainframe_run(gui);

    return 0;
  }
