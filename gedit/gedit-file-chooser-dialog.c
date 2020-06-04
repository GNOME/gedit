/*
 * gedit-app-file-chooser-dialog.h
 * This file is part of gedit
 *
 * Copyright (C) 2014 Jesse van den Kieboom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gedit-file-chooser-dialog.h"

#ifdef OS_OSX
#include "gedit-file-chooser-dialog-osx.h"
#else
#include "gedit-file-chooser-dialog-gtk.h"
#endif

G_DEFINE_INTERFACE (GeditFileChooserDialog, gedit_file_chooser_dialog, G_TYPE_OBJECT)

static gboolean
confirm_overwrite_accumulator (GSignalInvocationHint *ihint,
                               GValue                *return_accu,
                               const GValue          *handler_return,
                               gpointer               dummy)
{
	gboolean continue_emission;
	GtkFileChooserConfirmation conf;

	conf = g_value_get_enum (handler_return);
	g_value_set_enum (return_accu, conf);
	continue_emission = (conf == GTK_FILE_CHOOSER_CONFIRMATION_CONFIRM);

	return continue_emission;
}

static void
gedit_file_chooser_dialog_default_init (GeditFileChooserDialogInterface *iface)
{
	g_signal_new ("response",
	              G_TYPE_FROM_INTERFACE (iface),
	              G_SIGNAL_RUN_LAST,
	              0,
	              NULL, NULL, NULL,
	              G_TYPE_NONE,
	              1,
	              G_TYPE_INT);

	g_signal_new ("confirm-overwrite",
	              G_TYPE_FROM_INTERFACE (iface),
	              G_SIGNAL_RUN_LAST,
	              0,
	              confirm_overwrite_accumulator, NULL, NULL,
	              GTK_TYPE_FILE_CHOOSER_CONFIRMATION,
	              0);
}

GeditFileChooserDialog *
gedit_file_chooser_dialog_create (const gchar           *title,
				  GtkWindow             *parent,
				  GeditFileChooserFlags  flags,
				  const gchar           *cancel_label,
				  const gchar           *accept_label)
{
#ifdef OS_OSX
	return gedit_file_chooser_dialog_osx_create (title,
	                                             parent,
	                                             flags,
	                                             cancel_label,
	                                             accept_label);
#else
	return gedit_file_chooser_dialog_gtk_create (title,
	                                             parent,
	                                             flags,
	                                             cancel_label,
	                                             accept_label);
#endif
}

void
gedit_file_chooser_dialog_set_encoding (GeditFileChooserDialog  *dialog,
                                        const GtkSourceEncoding *encoding)
{
	GeditFileChooserDialogInterface *iface;

	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog));

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_if_fail (iface->set_encoding != NULL);

	iface->set_encoding (dialog, encoding);
}

const GtkSourceEncoding *
gedit_file_chooser_dialog_get_encoding (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogInterface *iface;

	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog), NULL);

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_val_if_fail (iface->get_encoding != NULL, NULL);

	return iface->get_encoding (dialog);
}

void
gedit_file_chooser_dialog_set_newline_type (GeditFileChooserDialog *dialog,
                                            GtkSourceNewlineType    newline_type)
{
	GeditFileChooserDialogInterface *iface;

	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog));

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_if_fail (iface->set_newline_type != NULL);

	iface->set_newline_type (dialog, newline_type);
}

GtkSourceNewlineType
gedit_file_chooser_dialog_get_newline_type (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogInterface *iface;

	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog), GTK_SOURCE_NEWLINE_TYPE_DEFAULT);

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_val_if_fail (iface->get_newline_type != NULL, GTK_SOURCE_NEWLINE_TYPE_DEFAULT);

	return iface->get_newline_type (dialog);
}


void
gedit_file_chooser_dialog_set_current_folder (GeditFileChooserDialog *dialog,
                                              GFile                  *folder)
{
	GeditFileChooserDialogInterface *iface;

	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog));

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_if_fail (iface->set_current_folder != NULL);

	iface->set_current_folder (dialog, folder);
}

void
gedit_file_chooser_dialog_set_current_name (GeditFileChooserDialog *dialog,
                                            const gchar            *name)
{
	GeditFileChooserDialogInterface *iface;

	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog));

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_if_fail (iface->set_current_name != NULL);

	iface->set_current_name (dialog, name);
}

void
gedit_file_chooser_dialog_set_file (GeditFileChooserDialog *dialog,
                                    GFile                  *file)
{
	GeditFileChooserDialogInterface *iface;

	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog));
	g_return_if_fail (file == NULL || G_IS_FILE (file));

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_if_fail (iface->set_file != NULL);

	iface->set_file (dialog, file);
}

GSList *
gedit_file_chooser_dialog_get_files (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogInterface *iface;

	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog), NULL);

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_val_if_fail (iface->get_files != NULL, NULL);

	return iface->get_files (dialog);
}

GFile *
gedit_file_chooser_dialog_get_file (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogInterface *iface;

	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog), NULL);

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_val_if_fail (iface->get_file != NULL, NULL);

	return iface->get_file (dialog);
}

void
gedit_file_chooser_dialog_set_do_overwrite_confirmation (GeditFileChooserDialog *dialog,
                                                         gboolean                overwrite_confirmation)
{
	GeditFileChooserDialogInterface *iface;

	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog));

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_if_fail (iface->set_do_overwrite_confirmation != NULL);

	iface->set_do_overwrite_confirmation (dialog, overwrite_confirmation);
}

void
gedit_file_chooser_dialog_show (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogInterface *iface;

	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog));

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_if_fail (iface->show != NULL);

	iface->show (dialog);
}

void
gedit_file_chooser_dialog_hide (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogInterface *iface;

	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog));

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_if_fail (iface->hide != NULL);

	iface->hide (dialog);
}

void
gedit_file_chooser_dialog_destroy (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogInterface *iface;

	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog));

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_if_fail (iface->destroy != NULL);

	iface->destroy (dialog);
}

void
gedit_file_chooser_dialog_set_modal (GeditFileChooserDialog *dialog,
                                     gboolean                is_modal)
{
	GeditFileChooserDialogInterface *iface;

	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog));

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);
	g_return_if_fail (iface->set_modal != NULL);

	iface->set_modal (dialog, is_modal);
}

GtkWindow *
gedit_file_chooser_dialog_get_window (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogInterface *iface;

	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog), NULL);

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);

	if (iface->get_window)
	{
		return iface->get_window (dialog);
	}

	return NULL;
}

void
gedit_file_chooser_dialog_add_pattern_filter (GeditFileChooserDialog   *dialog,
                                              const gchar              *name,
                                              const gchar              *pattern)
{
	GeditFileChooserDialogInterface *iface;

	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog));

	iface = GEDIT_FILE_CHOOSER_DIALOG_GET_IFACE (dialog);

	if (iface->add_pattern_filter)
	{
		iface->add_pattern_filter (dialog, name, pattern);
	}
}

/* ex:set ts=8 noet: */
