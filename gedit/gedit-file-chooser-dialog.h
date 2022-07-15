/*
 * gedit-file-chooser-dialog.h
 * This file is part of gedit
 *
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

#ifndef GEDIT_FILE_CHOOSER_DIALOG_H
#define GEDIT_FILE_CHOOSER_DIALOG_H

#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_CHOOSER_DIALOG (gedit_file_chooser_dialog_get_type ())

G_DECLARE_INTERFACE (GeditFileChooserDialog, gedit_file_chooser_dialog,
		     GEDIT, FILE_CHOOSER_DIALOG,
		     GObject)

struct _GeditFileChooserDialogInterface
{
	GTypeInterface g_iface;

	/* Virtual public methods */
	void	(*set_encoding)		(GeditFileChooserDialog  *dialog,
					 const GtkSourceEncoding *encoding);

	const GtkSourceEncoding *
		(*get_encoding)		(GeditFileChooserDialog *dialog);

	void	(*set_newline_type)	(GeditFileChooserDialog  *dialog,
					 GtkSourceNewlineType     newline_type);

	GtkSourceNewlineType
		(*get_newline_type)	(GeditFileChooserDialog *dialog);

	void	(*set_current_folder)	(GeditFileChooserDialog *dialog,
					 GFile                  *folder);

	void	(*set_current_name)	(GeditFileChooserDialog *dialog,
					 const gchar            *name);

	void	(*set_file)		(GeditFileChooserDialog *dialog,
					 GFile                  *file);

	GFile *	(*get_file)		(GeditFileChooserDialog *dialog);

	void	(*set_do_overwrite_confirmation)
					(GeditFileChooserDialog *dialog,
					 gboolean                overwrite_confirmation);

	void	(*show)			(GeditFileChooserDialog *dialog);

	void    (*destroy)		(GeditFileChooserDialog *dialog);

	void	(*set_modal)		(GeditFileChooserDialog *dialog,
					 gboolean                is_modal);

	GtkWindow *
		(*get_window)		(GeditFileChooserDialog *dialog);
};

GeditFileChooserDialog *
		gedit_file_chooser_dialog_create		(const gchar              *title,
								 GtkWindow                *parent,
								 const gchar              *accept_label,
								 const gchar              *cancel_label);

void		 gedit_file_chooser_dialog_destroy		(GeditFileChooserDialog   *dialog);

void		 gedit_file_chooser_dialog_set_encoding		(GeditFileChooserDialog   *dialog,
								 const GtkSourceEncoding  *encoding);

const GtkSourceEncoding *
		 gedit_file_chooser_dialog_get_encoding		(GeditFileChooserDialog   *dialog);

void		 gedit_file_chooser_dialog_set_newline_type	(GeditFileChooserDialog   *dialog,
								 GtkSourceNewlineType      newline_type);

GtkSourceNewlineType
		 gedit_file_chooser_dialog_get_newline_type	(GeditFileChooserDialog   *dialog);

void		 gedit_file_chooser_dialog_set_current_folder	(GeditFileChooserDialog   *dialog,
								 GFile                    *folder);

void		 gedit_file_chooser_dialog_set_current_name	(GeditFileChooserDialog   *dialog,
								 const gchar              *name);

void		 gedit_file_chooser_dialog_set_file		(GeditFileChooserDialog   *dialog,
								 GFile                    *file);

GFile		*gedit_file_chooser_dialog_get_file		(GeditFileChooserDialog   *dialog);

void		 gedit_file_chooser_dialog_set_do_overwrite_confirmation (
								 GeditFileChooserDialog   *dialog,
								 gboolean                  overwrite_confirmation);

void		 gedit_file_chooser_dialog_show			(GeditFileChooserDialog   *dialog);

void		 gedit_file_chooser_dialog_set_modal		(GeditFileChooserDialog   *dialog,
								 gboolean                  is_modal);

GtkWindow	*gedit_file_chooser_dialog_get_window		(GeditFileChooserDialog   *dialog);

G_END_DECLS

#endif /* GEDIT_FILE_CHOOSER_DIALOG_H */

/* ex:set ts=8 noet: */
