/*
 * gedit-file-chooser-dialog-gtk.h
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
 * Copyright (C) 2014 - Jesse van den Kieboom
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

#ifndef GEDIT_FILE_CHOOSER_DIALOG_GTK_H
#define GEDIT_FILE_CHOOSER_DIALOG_GTK_H

#include <gtk/gtk.h>
#include "gedit-file-chooser-dialog.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_CHOOSER_DIALOG_GTK (gedit_file_chooser_dialog_gtk_get_type ())

G_DECLARE_FINAL_TYPE (GeditFileChooserDialogGtk, gedit_file_chooser_dialog_gtk,
		      GEDIT, FILE_CHOOSER_DIALOG_GTK,
		      GtkFileChooserDialog)

GeditFileChooserDialog *	gedit_file_chooser_dialog_gtk_create	(const gchar *title,
									 GtkWindow   *parent,
									 const gchar *accept_label,
									 const gchar *cancel_label);

G_END_DECLS

#endif /* GEDIT_FILE_CHOOSER_DIALOG_GTK_H */

/* ex:set ts=8 noet: */
