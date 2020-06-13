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

#include "gedit-file-chooser-open.h"
#include "gedit-file-chooser-open-dialog.h"
#include "gedit-file-chooser-open-native.h"

/* Common code for file choosers that *open* files. */

G_DEFINE_TYPE (GeditFileChooserOpen, _gedit_file_chooser_open, GEDIT_TYPE_FILE_CHOOSER)

static GtkFileChooser *
get_gtk_file_chooser (GeditFileChooserOpen *chooser)
{
	return _gedit_file_chooser_get_gtk_file_chooser (GEDIT_FILE_CHOOSER (chooser));
}

static void
_gedit_file_chooser_open_constructed (GObject *object)
{
	GeditFileChooserOpen *chooser = GEDIT_FILE_CHOOSER_OPEN (object);

	if (G_OBJECT_CLASS (_gedit_file_chooser_open_parent_class)->constructed != NULL)
	{
		G_OBJECT_CLASS (_gedit_file_chooser_open_parent_class)->constructed (object);
	}

	gtk_file_chooser_set_select_multiple (get_gtk_file_chooser (chooser), TRUE);
}

static void
_gedit_file_chooser_open_class_init (GeditFileChooserOpenClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->constructed = _gedit_file_chooser_open_constructed;
}

static void
_gedit_file_chooser_open_init (GeditFileChooserOpen *chooser)
{
}

GeditFileChooserOpen *
_gedit_file_chooser_open_new (void)
{
	if (_gedit_file_chooser_is_native ())
	{
		return _gedit_file_chooser_open_native_new ();
	}

	return _gedit_file_chooser_open_dialog_new ();
}

GSList *
_gedit_file_chooser_open_get_files (GeditFileChooserOpen *chooser)
{
	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser), NULL);

	return gtk_file_chooser_get_files (get_gtk_file_chooser (chooser));
}
