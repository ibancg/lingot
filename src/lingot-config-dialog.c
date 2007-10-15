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
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include "lingot-defs.h"

#include "lingot-core.h"
#include "lingot-config.h"
#include "lingot-mainframe.h"
#include "lingot-config-dialog.h"
#include "lingot-i18n.h"

/* button press event attention routine. */
void lingot_config_dialog_callback_click(GtkButton *button,
    LingotConfigDialog *dialog)
  {
    if ((GtkWidget*)button == dialog->button_ok)
      {
        lingot_config_dialog_apply(dialog);
        lingot_config_save(dialog->conf, CONFIG_FILE_NAME);
        lingot_config_dialog_close(dialog);
      }
    else if ((GtkWidget*)button == dialog->button_cancel)
      {
        lingot_mainframe_change_config(dialog->mainframe, dialog->conf_old); // restore old configuration.
        lingot_config_dialog_close(dialog);
      }
    else if ((GtkWidget*)button == dialog->button_apply)
      {
        lingot_config_dialog_apply(dialog);
        lingot_config_dialog_rewrite(dialog);
      }
    else if ((GtkWidget*)button == dialog->button_default)
      {
        lingot_config_reset(dialog->conf);
        lingot_config_dialog_rewrite(dialog);
      }
  }

void lingot_config_dialog_callback_close(GtkWidget *widget,
    LingotConfigDialog *dialog)
  {
    gtk_widget_destroy(dialog->win);
    lingot_config_dialog_destroy(dialog);
  }

void lingot_config_dialog_callback_change_sample_rate(GtkWidget *widget,
    LingotConfigDialog *dialog)
  {
    char buff[20];
    float sample_rate =atof(gtk_combo_box_get_active_text(GTK_COMBO_BOX(dialog->combo_sample_rate)));
    float oversampling=
    gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(dialog->spin_oversampling));
    sprintf(buff, " = %0.1f", sample_rate/oversampling);
    gtk_label_set_text ( GTK_LABEL(dialog->label_sample_rate), buff);
  }

void lingot_config_dialog_callback_change_a_frequence(GtkWidget *widget,
    LingotConfigDialog *dialog)
  {
    char buff[20];

    float root_freq = 440.0*pow(2.0, gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(dialog->spin_root_frequency_error))/1200.0);
    sprintf(buff, "%%    = %0.2f", root_freq);
    gtk_label_set_text ( GTK_LABEL(dialog->label_root_frequency), buff);
  }

void lingot_config_dialog_combo_select_value(GtkWidget* combo, int value)
  {

    GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    GtkTreeIter iter;

    gboolean valid = gtk_tree_model_get_iter_first (model, &iter);

    while (valid)
      {
        gchar *str_data;
        gtk_tree_model_get (model, &iter, 0, &str_data, -1);
        if (atoi(str_data) == value)
          gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), &iter);
        g_free (str_data);
        valid = gtk_tree_model_iter_next (model, &iter);
      }
  }

void lingot_config_dialog_rewrite(LingotConfigDialog* dialog)
  {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->spin_calculation_rate), dialog->conf->calculation_rate);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->spin_visualization_rate), dialog->conf->visualization_rate);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->spin_oversampling), dialog->conf->oversampling);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->spin_root_frequency_error), dialog->conf->root_frequency_error);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->spin_temporal_window), dialog->conf->temporal_window);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->spin_noise_threshold), dialog->conf->noise_threshold_db);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->spin_dft_number), dialog->conf->dft_number);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->spin_dft_size), dialog->conf->dft_size);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->spin_peak_number), dialog->conf->peak_number);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->spin_peak_order), dialog->conf->peak_order);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->spin_peak_rejection_relation), dialog->conf->peak_rejection_relation_db);
    lingot_config_dialog_combo_select_value(dialog->combo_fft_size,
        (int) dialog->conf->fft_size);
    lingot_config_dialog_combo_select_value(dialog->combo_sample_rate,
        (int) dialog->conf->sample_rate);
    lingot_config_dialog_callback_change_sample_rate(NULL, dialog);
    lingot_config_dialog_callback_change_a_frequence(NULL, dialog);
  }

LingotConfigDialog* lingot_config_dialog_new(LingotMainFrame* frame)
  {
    GtkWidget* vertical_box;
    GtkWidget* frame1;
    GtkWidget* tab1;
    GtkWidget* frame2;
    GtkWidget* tab2;
    GtkWidget* box;
    char buff[80];
    LingotConfigDialog* dialog = malloc(sizeof(LingotConfigDialog));

    dialog->mainframe = frame;
    dialog->conf = lingot_config_new();
    dialog->conf_old = lingot_config_new();

    // config copy
    *dialog->conf = *frame->conf;
    *dialog->conf_old = *frame->conf;

    dialog->win = gtk_dialog_new_with_buttons (_("lingot configuration"),
    GTK_WINDOW(frame->win), 0, NULL);

    gtk_container_set_border_width(GTK_CONTAINER(dialog->win), 6);

    // items vertically displayed  
    vertical_box = gtk_vbox_new(FALSE, 6);
    gtk_container_add(GTK_CONTAINER (GTK_DIALOG(dialog->win)->vbox), vertical_box);

    frame1 = gtk_frame_new(_("General parameters"));
    gtk_box_pack_start_defaults(GTK_BOX(vertical_box), frame1);

    tab1 = gtk_table_new(5, 3, FALSE);
    int row = 0;

    gtk_table_set_col_spacings(GTK_TABLE(tab1), 20);
    gtk_table_set_row_spacings(GTK_TABLE(tab1), 3);
    gtk_container_set_border_width(GTK_CONTAINER(tab1), 5);

    gtk_container_add(GTK_CONTAINER(frame1), tab1);

    // --------------------------------------------------------------------------

    dialog->spin_calculation_rate = gtk_spin_button_new(
        (GtkAdjustment*)gtk_adjustment_new(dialog->conf->calculation_rate, 1.0,
            60.0, 0.5, 10.0, 15.0), 0.5, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->spin_calculation_rate), TRUE);
    gtk_table_attach_defaults(GTK_TABLE(tab1),
    gtk_label_new(_("Calculation rate")), 0, 1, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab1), dialog->spin_calculation_rate, 1, 2, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(_("Hz")), 2, 3, row, row + 1);
    row++;

    // --------------------------------------------------------------------------

    dialog->spin_visualization_rate =gtk_spin_button_new(
        (GtkAdjustment*)gtk_adjustment_new(dialog->conf->visualization_rate,
            1.0, 60.0, 0.5, 10.0, 15.0), 0.5, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->spin_visualization_rate), TRUE);
    gtk_table_attach_defaults(GTK_TABLE(tab1),
    gtk_label_new(_("Visualization rate")), 0, 1, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab1), dialog->spin_visualization_rate, 1, 2, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(_("Hz")), 2, 3, row, row + 1);
    row++;

    // --------------------------------------------------------------------------

    dialog->combo_fft_size = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->combo_fft_size), "256");
    gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->combo_fft_size), "512");
    gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->combo_fft_size), "1024");
    gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->combo_fft_size), "2048");
    gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->combo_fft_size), "4096");
    lingot_config_dialog_combo_select_value(dialog->combo_fft_size,
        (int) dialog->conf->fft_size);

    gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(_("FFT size")), 0, 1, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab1), dialog->combo_fft_size, 1, 2, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(_("samples")), 2, 3, row, row + 1);
    row++;

    // --------------------------------------------------------------------------

    dialog->spin_noise_threshold =gtk_spin_button_new(
        (GtkAdjustment*)gtk_adjustment_new(dialog->conf->noise_threshold_db,
            -30.0, 60.0, 0.1, 0.1, 0.1), 0.1, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->spin_noise_threshold), TRUE);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dialog->spin_noise_threshold), 1) ;

    gtk_table_attach_defaults(GTK_TABLE(tab1),
    gtk_label_new(_("Noise threshold")), 0, 1, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(_("dB")), 2, 3, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab1), dialog->spin_noise_threshold, 1, 2, row, row + 1);
    row++;

    // --------------------------------------------------------------------------

    box = gtk_hbox_new (FALSE, FALSE);

    dialog->combo_sample_rate = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->combo_sample_rate), "8000");
    gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->combo_sample_rate), "11025");
    gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->combo_sample_rate), "22050");
    gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->combo_sample_rate), "44100");

    lingot_config_dialog_combo_select_value(dialog->combo_sample_rate,
        (int) dialog->conf->sample_rate);

    dialog->spin_oversampling = gtk_spin_button_new(
        (GtkAdjustment*) gtk_adjustment_new(dialog->conf->oversampling, 1.0,
            30.0, 1.0, 10.0, 15.0), 1.0, 1);

    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->spin_oversampling), TRUE);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dialog->spin_oversampling), 0) ;

    sprintf(buff, " = %0.1f", atof(gtk_combo_box_get_active_text(GTK_COMBO_BOX(dialog->combo_sample_rate)))
        /gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->spin_oversampling)));
    dialog->label_sample_rate = gtk_label_new(buff);

    gtk_box_pack_start (GTK_BOX(box), dialog->combo_sample_rate, FALSE, FALSE, FALSE);
    gtk_box_pack_start (GTK_BOX(box), gtk_label_new(" / "), FALSE, FALSE, FALSE);
    gtk_box_pack_start (GTK_BOX(box), dialog->spin_oversampling, FALSE, FALSE, FALSE);
    gtk_box_pack_start (GTK_BOX(box), dialog->label_sample_rate, FALSE, FALSE, FALSE);

    gtk_signal_connect(GTK_OBJECT(dialog->combo_sample_rate), "changed",
        GTK_SIGNAL_FUNC (lingot_config_dialog_callback_change_sample_rate), dialog);

    gtk_signal_connect (GTK_OBJECT (dialog->spin_oversampling), "value_changed",
        GTK_SIGNAL_FUNC (lingot_config_dialog_callback_change_sample_rate), dialog);

    gtk_table_attach_defaults(GTK_TABLE(tab1),
    gtk_label_new(_("Sample rate")), 0, 1, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab1), box, 1, 2, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab1), gtk_label_new(_("Hz")), 2, 3, row, row + 1);
    row++;

    // --------------------------------------------------------------------------

    frame2 = gtk_frame_new(_("Advanced parameters"));
    gtk_box_pack_start_defaults(GTK_BOX(vertical_box), frame2);

    tab2 = gtk_table_new(7, 3, FALSE);
    row = 0;

    gtk_table_set_col_spacings(GTK_TABLE(tab2), 20);
    gtk_table_set_row_spacings(GTK_TABLE(tab2), 3);
    gtk_container_set_border_width(GTK_CONTAINER(tab2), 5);

    gtk_container_add(GTK_CONTAINER(frame2), tab2);

    // --------------------------------------------------------------------------

    box = gtk_hbox_new (TRUE, FALSE);

    dialog->spin_root_frequency_error = gtk_spin_button_new(
        (GtkAdjustment*)gtk_adjustment_new(dialog->conf->root_frequency_error,
            -500.0, +500.0, 1.0, 10.0, 15.0), 1.0, 1);

    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->spin_root_frequency_error), TRUE);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dialog->spin_root_frequency_error), 0) ;

    gtk_box_pack_start (GTK_BOX(box), gtk_label_new(_(" A (440 Hz) + ")), FALSE, FALSE, FALSE);
    gtk_box_pack_start (GTK_BOX(box), dialog->spin_root_frequency_error, FALSE, TRUE, FALSE);

    sprintf(buff, "%%    = %0.2f", dialog->conf->root_frequency);
    dialog->label_root_frequency = gtk_label_new(_(buff));

    gtk_box_pack_start (GTK_BOX(box), dialog->label_root_frequency, FALSE, FALSE, FALSE);

    gtk_signal_connect (GTK_OBJECT (dialog->spin_root_frequency_error), "value_changed",
        GTK_SIGNAL_FUNC (lingot_config_dialog_callback_change_a_frequence), dialog);

    gtk_table_attach_defaults(GTK_TABLE(tab2),
    gtk_label_new(_("Root frequency")), 0, 1, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), box, 1, 2, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("Hz")), 2, 3, row, row + 1);
    row++;

    // --------------------------------------------------------------------------

    dialog->spin_temporal_window =gtk_spin_button_new(
        (GtkAdjustment*)gtk_adjustment_new(dialog->conf->temporal_window, 0.0,
            15.0, 0.005, 0.1, 0.1), 0.005, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->spin_temporal_window), TRUE);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dialog->spin_temporal_window), 3) ;

    gtk_table_attach_defaults(GTK_TABLE(tab2),
    gtk_label_new(_("Temporal window")), 0, 1, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("seconds")), 2, 3, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), dialog->spin_temporal_window, 1, 2, row, row + 1);
    row++;

    // --------------------------------------------------------------------------

    dialog->spin_dft_number =gtk_spin_button_new(
        (GtkAdjustment*)gtk_adjustment_new(dialog->conf->dft_number, 0.0, 10.0,
            1.0, 1.0, 1.0), 1.0, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->spin_dft_number), TRUE);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dialog->spin_dft_number), 0) ;

    gtk_table_attach_defaults(GTK_TABLE(tab2),
    gtk_label_new(_("DFT number")), 0, 1, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), dialog->spin_dft_number, 1, 2, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("DFTs")), 2, 3, row, row + 1);
    row++;

    // --------------------------------------------------------------------------

    dialog->spin_dft_size =gtk_spin_button_new(
        (GtkAdjustment*)gtk_adjustment_new(dialog->conf->dft_size, 4.0, 100.0,
            1.0, 1.0, 1.0), 1.0, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->spin_dft_size), TRUE);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dialog->spin_dft_size), 0) ;

    gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("DFT size")), 0, 1, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("samples")), 2, 3, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), dialog->spin_dft_size, 1, 2, row, row + 1);
    row++;

    // --------------------------------------------------------------------------

    dialog->spin_peak_number =gtk_spin_button_new(
        (GtkAdjustment*)gtk_adjustment_new(dialog->conf->peak_number, 1.0,
            10.0, 1.0, 1.0, 1.0), 1.0, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->spin_peak_number), TRUE);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dialog->spin_peak_number), 0) ;

    gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("Peak number")), 0, 1, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), dialog->spin_peak_number, 1, 2, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("peaks")), 2, 3, row, row + 1);
    row++;

    // --------------------------------------------------------------------------

    dialog->spin_peak_order = gtk_spin_button_new(
        (GtkAdjustment*)gtk_adjustment_new(dialog->conf->peak_order, 1.0, 5.0,
            1.0, 1.0, 1.0), 1.0, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->spin_peak_order), TRUE);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dialog->spin_peak_order), 0) ;

    gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("Peak order")), 0, 1, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), dialog->spin_peak_order, 1, 2, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("samples")), 2, 3, row, row + 1);
    row++;

    // --------------------------------------------------------------------------

    dialog->spin_peak_rejection_relation
        = gtk_spin_button_new(
            (GtkAdjustment*)gtk_adjustment_new(
                dialog->conf->peak_rejection_relation_db, 2.0, 40.0, 0.1, 1.0,
                1.0), 0.1, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->spin_peak_rejection_relation), TRUE);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dialog->spin_peak_rejection_relation), 1);

    gtk_table_attach_defaults(GTK_TABLE(tab2),
    gtk_label_new(_("Rejection peak relation")), 0, 1, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), dialog->spin_peak_rejection_relation, 1, 2, row, row + 1);
    gtk_table_attach_defaults(GTK_TABLE(tab2), gtk_label_new(_("dB")), 2, 3, row, row + 1);
    row++;

    // --------------------------------------------------------------------------

    dialog->button_ok = gtk_button_new_from_stock(GTK_STOCK_OK);
    dialog->button_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    dialog->button_apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    dialog->button_default = gtk_button_new_with_label(_("Default config"));

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->win)->action_area), dialog->button_default, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->win)->action_area), dialog->button_apply, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->win)->action_area), dialog->button_ok, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->win)->action_area), dialog->button_cancel, TRUE, TRUE, 0);

    g_signal_connect(GTK_OBJECT(dialog->button_default), "clicked", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_click), dialog);
    g_signal_connect(GTK_OBJECT(dialog->button_apply), "clicked", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_click), dialog);
    g_signal_connect(GTK_OBJECT(dialog->button_ok), "clicked", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_click), dialog);
    g_signal_connect(GTK_OBJECT(dialog->button_cancel), "clicked", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_click), dialog);

    g_signal_connect(GTK_OBJECT(dialog->win), "destroy", GTK_SIGNAL_FUNC(lingot_config_dialog_callback_close), dialog);

    lingot_config_dialog_rewrite(dialog);
    gtk_window_set_resizable(GTK_WINDOW(dialog->win), FALSE);
    gtk_widget_show_all(dialog->win);

    return dialog;
  }

void lingot_config_dialog_destroy(LingotConfigDialog* dialog)
  {
    dialog->mainframe->config_dialog = NULL;
    lingot_config_destroy(dialog->conf);
    lingot_config_destroy(dialog->conf_old);

    //gtk_widget_destroy(dialog->win);
    free(dialog);
  }

void lingot_config_dialog_apply(LingotConfigDialog* dialog)
  {
    GtkWidget* message_dialog;

    dialog->conf->root_frequency_error = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(dialog->spin_root_frequency_error));
    dialog->conf->calculation_rate = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(dialog->spin_calculation_rate));
    dialog->conf->visualization_rate = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(dialog->spin_visualization_rate));
    dialog->conf->temporal_window = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(dialog->spin_temporal_window));
    dialog->conf->noise_threshold_db = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(dialog->spin_noise_threshold));
    dialog->conf->oversampling
        = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->spin_oversampling));
    dialog->conf->dft_number= gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->spin_dft_number));
    dialog->conf->dft_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->spin_dft_size));
    dialog->conf->peak_number
        = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->spin_peak_number));
    dialog->conf->peak_order= gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->spin_peak_order));
    dialog->conf->peak_rejection_relation_db
        = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->spin_peak_rejection_relation));
    dialog->conf->fft_size = atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(dialog->combo_fft_size)));
    dialog->conf->sample_rate
        = atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(dialog->combo_sample_rate)));

    int result = lingot_config_update_internal_params(dialog->conf);

    lingot_mainframe_change_config(dialog->mainframe, dialog->conf);

    if (!result)
      {
        message_dialog
            = gtk_message_dialog_new(
            GTK_WINDOW(dialog->win),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_CLOSE,
            _("Temporal buffer is smaller than FFT size. It has been increased to %0.3f seconds"), dialog->conf->temporal_window);
        gtk_dialog_run(GTK_DIALOG(message_dialog));
        gtk_widget_destroy (message_dialog);
      }
  }

void lingot_config_dialog_close(LingotConfigDialog* dialog)
  {
    gtk_widget_destroy(dialog->win);
  }
