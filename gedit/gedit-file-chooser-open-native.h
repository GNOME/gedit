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

#ifndef GEDIT_FILE_CHOOSER_OPEN_NATIVE_H
#define GEDIT_FILE_CHOOSER_OPEN_NATIVE_H

#include "gedit-file-chooser-open.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_FILE_CHOOSER_OPEN_NATIVE             (_gedit_file_chooser_open_native_get_type ())
#define GEDIT_FILE_CHOOSER_OPEN_NATIVE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_FILE_CHOOSER_OPEN_NATIVE, GeditFileChooserOpenNative))
#define GEDIT_FILE_CHOOSER_OPEN_NATIVE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_FILE_CHOOSER_OPEN_NATIVE, GeditFileChooserOpenNativeClass))
#define GEDIT_IS_FILE_CHOOSER_OPEN_NATIVE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_FILE_CHOOSER_OPEN_NATIVE))
#define GEDIT_IS_FILE_CHOOSER_OPEN_NATIVE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_FILE_CHOOSER_OPEN_NATIVE))
#define GEDIT_FILE_CHOOSER_OPEN_NATIVE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_FILE_CHOOSER_OPEN_NATIVE, GeditFileChooserOpenNativeClass))

typedef struct _GeditFileChooserOpenNative         GeditFileChooserOpenNative;
typedef struct _GeditFileChooserOpenNativeClass    GeditFileChooserOpenNativeClass;
typedef struct _GeditFileChooserOpenNativePrivate  GeditFileChooserOpenNativePrivate;

struct _GeditFileChooserOpenNative
{
	GeditFileChooserOpen parent;

	GeditFileChooserOpenNativePrivate *priv;
};

struct _GeditFileChooserOpenNativeClass
{
	GeditFileChooserOpenClass parent_class;
};

G_GNUC_INTERNAL
GType			_gedit_file_chooser_open_native_get_type	(void);

G_GNUC_INTERNAL
GeditFileChooserOpen *	_gedit_file_chooser_open_native_new		(void);

G_END_DECLS

#endif /* GEDIT_FILE_CHOOSER_OPEN_NATIVE_H */
