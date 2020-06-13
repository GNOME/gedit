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

#include "gedit-file-chooser-open-dialog.h"
#include <glib/gi18n.h>

struct _GeditFileChooserOpenDialogPrivate
{
	gint something;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditFileChooserOpenDialog, _gedit_file_chooser_open_dialog, GEDIT_TYPE_FILE_CHOOSER_OPEN)

static void
_gedit_file_chooser_open_dialog_dispose (GObject *object)
{

	G_OBJECT_CLASS (_gedit_file_chooser_open_dialog_parent_class)->dispose (object);
}

static GtkFileChooser *
chooser_create_gtk_file_chooser (GeditFileChooser *chooser)
{
	GtkWidget *file_chooser;

	/* Translators: "Open Files" is the title of the file chooser window. */
	file_chooser = gtk_file_chooser_dialog_new (C_("window title", "Open Files"),
						    NULL,
						    GTK_FILE_CHOOSER_ACTION_OPEN,
						    _("_Cancel"), GTK_RESPONSE_CANCEL,
						    _("_Open"), GTK_RESPONSE_ACCEPT,
						    NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (file_chooser), GTK_RESPONSE_ACCEPT);

	if (g_object_is_floating (file_chooser))
	{
		g_object_ref_sink (file_chooser);
	}

	return GTK_FILE_CHOOSER (file_chooser);
}

static void
_gedit_file_chooser_open_dialog_class_init (GeditFileChooserOpenDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditFileChooserClass *file_chooser_class = GEDIT_FILE_CHOOSER_CLASS (klass);

	object_class->dispose = _gedit_file_chooser_open_dialog_dispose;

	file_chooser_class->create_gtk_file_chooser = chooser_create_gtk_file_chooser;
}

static void
_gedit_file_chooser_open_dialog_init (GeditFileChooserOpenDialog *chooser)
{
	chooser->priv = _gedit_file_chooser_open_dialog_get_instance_private (chooser);
}

GeditFileChooserOpen *
_gedit_file_chooser_open_dialog_new (void)
{
	return g_object_new (GEDIT_TYPE_FILE_CHOOSER_OPEN_DIALOG, NULL);
}
