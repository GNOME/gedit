/*
 * This file is part of gedit
 *
 * Copyright (C) 2020 Sébastien Wilmet <swilmet@gnome.org>
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

#ifndef GEDIT_FILE_CHOOSER_H
#define GEDIT_FILE_CHOOSER_H

#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_CHOOSER             (_gedit_file_chooser_get_type ())
#define GEDIT_FILE_CHOOSER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_FILE_CHOOSER, GeditFileChooser))
#define GEDIT_FILE_CHOOSER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_FILE_CHOOSER, GeditFileChooserClass))
#define GEDIT_IS_FILE_CHOOSER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_FILE_CHOOSER))
#define GEDIT_IS_FILE_CHOOSER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_FILE_CHOOSER))
#define GEDIT_FILE_CHOOSER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_FILE_CHOOSER, GeditFileChooserClass))

typedef struct _GeditFileChooser         GeditFileChooser;
typedef struct _GeditFileChooserClass    GeditFileChooserClass;
typedef struct _GeditFileChooserPrivate  GeditFileChooserPrivate;

struct _GeditFileChooser
{
	GObject parent;

	GeditFileChooserPrivate *priv;
};

struct _GeditFileChooserClass
{
	GObjectClass parent_class;

	/* Returns: (transfer full). */
	GtkFileChooser *		(* create_gtk_file_chooser)	(GeditFileChooser *chooser);

	const GtkSourceEncoding *	(* get_encoding)		(GeditFileChooser *chooser);
};

G_GNUC_INTERNAL
gboolean		_gedit_file_chooser_is_native			(void);

G_GNUC_INTERNAL
GType			_gedit_file_chooser_get_type			(void);

G_GNUC_INTERNAL
GeditFileChooser *	_gedit_file_chooser_new				(void);

G_GNUC_INTERNAL
void			_gedit_file_chooser_set_gtk_file_chooser	(GeditFileChooser *chooser,
									 GtkFileChooser   *gtk_chooser);

G_GNUC_INTERNAL
GtkFileChooser *	_gedit_file_chooser_get_gtk_file_chooser	(GeditFileChooser *chooser);

G_GNUC_INTERNAL
void			_gedit_file_chooser_set_transient_for		(GeditFileChooser *chooser,
									 GtkWindow        *parent);

G_GNUC_INTERNAL
void			_gedit_file_chooser_show			(GeditFileChooser *chooser);

G_GNUC_INTERNAL
gchar *			_gedit_file_chooser_get_current_folder_uri	(GeditFileChooser *chooser);

G_GNUC_INTERNAL
void			_gedit_file_chooser_set_current_folder_uri	(GeditFileChooser *chooser,
									 const gchar      *uri);

G_GNUC_INTERNAL
const GtkSourceEncoding *
			_gedit_file_chooser_get_encoding		(GeditFileChooser *chooser);

G_END_DECLS

#endif /* GEDIT_FILE_CHOOSER_H */
