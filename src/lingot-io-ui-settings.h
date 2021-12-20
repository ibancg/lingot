/*
 * lingot, a musical instrument tuner.
 *
 * Copyright (C) 2004-2020  Iban Cereijo.
 * Copyright (C) 2004-2008  Jairo Chapela.

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
 * along with lingot; if not, write to the Free Software Foundation,
 * Inc. 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef LINGOT_IO_UI_SETTINGS_H
#define LINGOT_IO_UI_SETTINGS_H

// Persistent settings for the UI

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifndef VERSION
#  define VERSION "unknown"
#endif

extern char LINGOT_UI_SETTINGS_FILE_NAME[200];
#define LINGOT_DEFAULT_UI_SETTINGS_FILE_NAME "ui-settings.json"


// configuration parameter specification (id, type, minimum and maximum allowed values, ...)
typedef struct {

    // app version last launched
    const char* app_version;

    // visible / invisible widgets
    int spectrum_visible;
    int gauge_visible;
    int deviation_visible;

    // position and size of main window
    int win_width;
    int win_height;
    int win_pos_x;
    int win_pos_y;

    // position and size of config dialog
    int config_dialog_width;
    int config_dialog_height;
    int config_dialog_pos_x;
    int config_dialog_pos_y;

    // dynamic response of the gauge
    double gauge_adaptation_constant; // how quick the gauge adapts to the target error (the bigger the value, the quicker)
    double gauge_damping_constant;    // how much we damp the "bouncing" of the gauge (the bigger the value, the less the bouncing is)
    double gauge_sampling_rate;       // sampling rate for computing the next gauge position. keep high

    double visualization_rate;        // refresh of visual displays.
    double error_dispatch_rate;       // dispatch of error messages.

    int horizontal_paned_pos;
    int vertical_paned_pos;

} lingot_ui_settings_t;

extern lingot_ui_settings_t ui_settings;

int  lingot_io_ui_settings_init();
void lingot_io_ui_settings_save();

#ifdef __cplusplus
}
#endif

#endif // LINGOT_IO_UI_SETTINGS_H
