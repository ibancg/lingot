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
 * along with lingot; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <json-c/json.h>

#include "lingot-defs-internal.h"
#include "lingot-io-ui-settings.h"

char LINGOT_UI_SETTINGS_FILE_NAME[];
lingot_ui_settings_t ui_settings;

int lingot_io_ui_settings_init()
{
    // default values
    ui_settings.app_version = NULL;
    ui_settings.spectrum_visible = TRUE;
    ui_settings.gauge_visible = TRUE;
    ui_settings.win_pos_x = -1;
    ui_settings.win_pos_y = -1;
    ui_settings.win_width = 480;
    ui_settings.win_height = 410;
    ui_settings.config_dialog_pos_x = -1;
    ui_settings.config_dialog_pos_y = -1;
    ui_settings.config_dialog_width = -1;
    ui_settings.config_dialog_height = -1;

    ui_settings.gauge_adaptation_constant = 150.0;
    ui_settings.gauge_damping_constant = 15.0;
    ui_settings.gauge_sampling_rate = 60.0;

    ui_settings.visualization_rate = 30.0; // fps
    ui_settings.error_dispatch_rate = 5.0;

    json_object* doc = json_object_from_file(LINGOT_UI_SETTINGS_FILE_NAME);
    if (!doc) {
        fprintf(stderr, "cannot read UI settings file");
        return 0;
    }

    json_object* obj;
    json_bool ok = TRUE;

    ok = json_object_object_get_ex(doc, "appVersion", &obj);
    if (ok) {
        ui_settings.app_version = _strdup(json_object_get_string(obj));
    }
    ok = json_object_object_get_ex(doc, "spectrumVisible", &obj);
    if (ok) {
        ui_settings.spectrum_visible = json_object_get_boolean(obj);
    }
    ok = json_object_object_get_ex(doc, "gaugeVisible", &obj);
    if (ok) {
        ui_settings.gauge_visible = json_object_get_boolean(obj);
    }
    ok = json_object_object_get_ex(doc, "winPosX", &obj);
    if (ok) {
        ui_settings.win_pos_x = json_object_get_int(obj);
    }
    ok = json_object_object_get_ex(doc, "winPosY", &obj);
    if (ok) {
        ui_settings.win_pos_y = json_object_get_int(obj);
    }
    ok = json_object_object_get_ex(doc, "winWidth", &obj);
    if (ok) {
        ui_settings.win_width = json_object_get_int(obj);
    }
    ok = json_object_object_get_ex(doc, "winHeight", &obj);
    if (ok) {
        ui_settings.win_height = json_object_get_int(obj);
    }
    ok = json_object_object_get_ex(doc, "configDialogPosX", &obj);
    if (ok) {
        ui_settings.config_dialog_pos_x = json_object_get_int(obj);
    }
    ok = json_object_object_get_ex(doc, "configDialogPosY", &obj);
    if (ok) {
        ui_settings.config_dialog_pos_y = json_object_get_int(obj);
    }
    ok = json_object_object_get_ex(doc, "configDialogWidth", &obj);
    if (ok) {
        ui_settings.config_dialog_width = json_object_get_int(obj);
    }
    ok = json_object_object_get_ex(doc, "configDialogHeight", &obj);
    if (ok) {
        ui_settings.config_dialog_height = json_object_get_int(obj);
    }
    ok = json_object_object_get_ex(doc, "gaugeAdaptationConstant", &obj);
    if (ok) {
        ui_settings.gauge_adaptation_constant = json_object_get_double(obj);
    }
    ok = json_object_object_get_ex(doc, "gaugeDampingConstant", &obj);
    if (ok) {
        ui_settings.gauge_damping_constant = json_object_get_double(obj);
    }
    ok = json_object_object_get_ex(doc, "gaugeSamplingRate", &obj);
    if (ok) {
        ui_settings.gauge_sampling_rate = json_object_get_double(obj);
    }

    ok = json_object_object_get_ex(doc, "errorDispatchRate", &obj);
    if (ok) {
        ui_settings.error_dispatch_rate = json_object_get_double(obj);
    }
    ok = json_object_object_get_ex(doc, "visualizationRate", &obj);
    if (ok) {
        ui_settings.visualization_rate = json_object_get_double(obj);
    }

    json_object_put(doc);
    return 1;
}

void lingot_io_ui_settings_save()
{
    json_object* doc = json_object_new_object();

    json_object_object_add(doc, "appVersion",
                           json_object_new_string(VERSION)); // we save the current version
    json_object_object_add(doc, "spectrumVisible",
                           json_object_new_boolean(ui_settings.spectrum_visible));
    json_object_object_add(doc, "gaugeVisible",
                           json_object_new_boolean(ui_settings.gauge_visible));
    json_object_object_add(doc, "winPosX",
                           json_object_new_int(ui_settings.win_pos_x));
    json_object_object_add(doc, "winPosY",
                           json_object_new_int(ui_settings.win_pos_y));
    json_object_object_add(doc, "winWidth",
                           json_object_new_int(ui_settings.win_width));
    json_object_object_add(doc, "winHeight",
                           json_object_new_int(ui_settings.win_height));
    json_object_object_add(doc, "configDialogPosX",
                           json_object_new_int(ui_settings.config_dialog_pos_x));
    json_object_object_add(doc, "configDialogPosY",
                           json_object_new_int(ui_settings.config_dialog_pos_y));
    json_object_object_add(doc, "configDialogWidth",
                           json_object_new_int(ui_settings.config_dialog_width));
    json_object_object_add(doc, "configDialogHeight",
                           json_object_new_int(ui_settings.config_dialog_height));

    json_object_object_add(doc, "gaugeAdaptationConstant",
                           json_object_new_double(ui_settings.gauge_adaptation_constant));
    json_object_object_add(doc, "gaugeDampingConstant",
                           json_object_new_double(ui_settings.gauge_damping_constant));
    json_object_object_add(doc, "gaugeSamplingRate",
                           json_object_new_double(ui_settings.gauge_sampling_rate));

    json_object_object_add(doc, "errorDispatchRate",
                           json_object_new_double(ui_settings.error_dispatch_rate));
    json_object_object_add(doc, "visualizationRate",
                           json_object_new_double(ui_settings.visualization_rate));

    // TODO: free ui_settings.version?

    int result = json_object_to_file_ext(LINGOT_UI_SETTINGS_FILE_NAME, doc, JSON_C_TO_STRING_PRETTY);
    if (result == -1) {
        fprintf(stderr, "error saving config file\n");
    }
    json_object_put(doc);

}
