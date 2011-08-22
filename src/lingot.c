/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2011  Ibán Cereijo Graña, Jairo Chapela Martínez.
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
#include <getopt.h>

#include "lingot-defs.h"
#include "lingot-config.h"
#include "lingot-gui-mainframe.h"
#include "lingot-i18n.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

char CONFIG_FILE_NAME[100];

//void print_card_list(void) {
//   int status;
//   int card = -1;  // use -1 to prime the pump of iterating through card list
//   char* longname  = NULL;
//   char* shortname = NULL;
//
//   if ((status = snd_card_next(&card)) < 0) {
//      error("cannot determine card number: %s", snd_strerror(status));
//      return;
//   }
//   if (card < 0) {
//      error("no sound cards found");
//      return;
//   }
//   while (card >= 0) {
//      printf("Card %d:", card);
//      if ((status = snd_card_get_name(card, &shortname)) < 0) {
//         error("cannot determine card shortname: %s", snd_strerror(status));
//         break;
//      }
//      if ((status = snd_card_get_longname(card, &longname)) < 0) {
//         error("cannot determine card longname: %s", snd_strerror(status));
//         break;
//      }
//      printf("\tLONG NAME:  %s\n", longname);
//      printf("\tSHORT NAME: %s\n", shortname);
//      if ((status = snd_card_next(&card)) < 0) {
//         error("cannot determine card number: %s", snd_strerror(status));
//         break;
//      }
//   }
//}

int main(int argc, char *argv[]) {

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LINGOT_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	// default config file.
	sprintf(CONFIG_FILE_NAME, "%s/" CONFIG_DIR_NAME DEFAULT_CONFIG_FILE_NAME,
			getenv("HOME"));

	// TODO: indicate complete config file path
	if ((argc > 3) || (argc == 2)) {
		printf("\nusage: lingot [-c config]\n\n");
		return -1;
	} else if (argc > 1) {
		int c;
		//		int digit_optind = 0;

		while (1) {
			//		int this_option_optind = optind ? optind : 1;
			int option_index = 0;
			static struct option long_options[] = { { "config", 1, 0, 'c' }, {
					0, 0, 0, 0 } };

			c = getopt_long(argc, argv, "c:", long_options, &option_index);
			if (c == -1)
				break;

			switch (c) {
			case 'c':
				sprintf(CONFIG_FILE_NAME, "%s/%s%s.conf", getenv("HOME"),
						CONFIG_DIR_NAME, optarg);
				printf("using config file %s\n", CONFIG_FILE_NAME);
				break;

			case '?':
				break;

			default:
				printf("?? getopt returned character code 0%o ??\n", c);
				break;
			}
		}
	}

	// if config file doesn't exists, i will create it.
	FILE* fp;
	if ((fp = fopen(CONFIG_FILE_NAME, "r")) == NULL) {

		char config_dir[100];
		sprintf(config_dir, "%s/.lingot/", getenv("HOME"));
		printf("creating directory %s ...\n", config_dir);
		mkdir(config_dir, 0777); // creo el directorio.
		printf("creating file %s ...\n", CONFIG_FILE_NAME);

		// new configuration with default values.
		LingotConfig* new_conf = lingot_config_new();
		lingot_config_restore_default_values(new_conf);
		lingot_config_save(new_conf, CONFIG_FILE_NAME);
		lingot_config_destroy(new_conf);

		printf("ok\n");

	} else
		fclose(fp);

//	print_card_list();

	lingot_gui_mainframe_create(argc, argv);

	return 0;
}
