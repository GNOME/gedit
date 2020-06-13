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

#ifndef GEDIT_FILE_CHOOSER_OPEN_H
#define GEDIT_FILE_CHOOSER_OPEN_H

#include "gedit-file-chooser.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_CHOOSER_OPEN             (_gedit_file_chooser_open_get_type ())
#define GEDIT_FILE_CHOOSER_OPEN(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_FILE_CHOOSER_OPEN, GeditFileChooserOpen))
#define GEDIT_FILE_CHOOSER_OPEN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_FILE_CHOOSER_OPEN, GeditFileChooserOpenClass))
#define GEDIT_IS_FILE_CHOOSER_OPEN(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_FILE_CHOOSER_OPEN))
#define GEDIT_IS_FILE_CHOOSER_OPEN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_FILE_CHOOSER_OPEN))
#define GEDIT_FILE_CHOOSER_OPEN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_FILE_CHOOSER_OPEN, GeditFileChooserOpenClass))

typedef struct _GeditFileChooserOpen         GeditFileChooserOpen;
typedef struct _GeditFileChooserOpenClass    GeditFileChooserOpenClass;

struct _GeditFileChooserOpen
{
	GeditFileChooser parent;
};

struct _GeditFileChooserOpenClass
{
	GeditFileChooserClass parent_class;
};

G_GNUC_INTERNAL
GType			_gedit_file_chooser_open_get_type		(void);

G_GNUC_INTERNAL
GeditFileChooserOpen *	_gedit_file_chooser_open_new			(void);

G_GNUC_INTERNAL
GSList *		_gedit_file_chooser_open_get_files		(GeditFileChooserOpen *chooser);

G_END_DECLS

#endif /* GEDIT_FILE_CHOOSER_OPEN_H */
