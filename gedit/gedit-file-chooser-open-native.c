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

#include "gedit-file-chooser-open-native.h"
#include <glib/gi18n.h>

/* A GtkFileChooserNative to *open* files. */
/* TODO: finish the implementation. */

struct _GeditFileChooserOpenNativePrivate
{
	gint something;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditFileChooserOpenNative, _gedit_file_chooser_open_native, GEDIT_TYPE_FILE_CHOOSER_OPEN)

#if 0
static GtkNativeDialog *
get_native_dialog (GeditFileChooserOpenNative *chooser)
{
	return GTK_NATIVE_DIALOG (_gedit_file_chooser_get_gtk_file_chooser (GEDIT_FILE_CHOOSER (chooser)));
}
#endif

static void
_gedit_file_chooser_open_native_dispose (GObject *object)
{

	G_OBJECT_CLASS (_gedit_file_chooser_open_native_parent_class)->dispose (object);
}

static GtkFileChooser *
chooser_create_gtk_file_chooser (GeditFileChooser *chooser)
{
	GtkFileChooserNative *native_chooser;

	/* Translators: "Open Files" is the title of the file chooser window. */
	native_chooser = gtk_file_chooser_native_new (C_("window title", "Open Files"),
						      NULL,
						      GTK_FILE_CHOOSER_ACTION_OPEN,
						      NULL,
						      NULL);

	return GTK_FILE_CHOOSER (native_chooser);
}

static void
_gedit_file_chooser_open_native_class_init (GeditFileChooserOpenNativeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditFileChooserClass *chooser_class = GEDIT_FILE_CHOOSER_CLASS (klass);

	object_class->dispose = _gedit_file_chooser_open_native_dispose;

	chooser_class->create_gtk_file_chooser = chooser_create_gtk_file_chooser;
}

static void
_gedit_file_chooser_open_native_init (GeditFileChooserOpenNative *chooser)
{
	chooser->priv = _gedit_file_chooser_open_native_get_instance_private (chooser);
}

GeditFileChooserOpen *
_gedit_file_chooser_open_native_new (void)
{
	return g_object_new (GEDIT_TYPE_FILE_CHOOSER_OPEN_NATIVE, NULL);
}
