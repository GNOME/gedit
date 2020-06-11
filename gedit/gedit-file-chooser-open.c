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

struct _GeditFileChooserOpenPrivate
{
	gint something;
};

enum
{
	SIGNAL_DONE,
	N_SIGNALS
};

static guint signals[N_SIGNALS];

G_DEFINE_TYPE_WITH_PRIVATE (GeditFileChooserOpen, _gedit_file_chooser_open, GEDIT_TYPE_FILE_CHOOSER)

static GtkFileChooser *
get_gtk_file_chooser (GeditFileChooserOpen *chooser)
{
	return _gedit_file_chooser_get_gtk_file_chooser (GEDIT_FILE_CHOOSER (chooser));
}

static void
_gedit_file_chooser_open_class_init (GeditFileChooserOpenClass *klass)
{
	/*
	 * GeditFileChooserOpen::done:
	 * @chooser: the #GeditFileChooserOpen emitting the signal.
	 * @accept: whether the response code is %GTK_RESPONSE_ACCEPT.
	 */
	signals[SIGNAL_DONE] =
		g_signal_new ("done",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_FIRST,
			      0, NULL, NULL, NULL,
			      G_TYPE_NONE,
			      1, G_TYPE_BOOLEAN);
}

static void
response_cb (GtkFileChooser       *gtk_chooser,
	     gint                  response_id,
	     GeditFileChooserOpen *chooser)
{
	gboolean accept;

	accept = response_id == GTK_RESPONSE_ACCEPT;
	g_signal_emit (chooser, signals[SIGNAL_DONE], 0, accept);
}

static void
_gedit_file_chooser_open_init (GeditFileChooserOpen *chooser)
{
	GtkFileChooser *gtk_chooser;

	chooser->priv = _gedit_file_chooser_open_get_instance_private (chooser);

	gtk_chooser = get_gtk_file_chooser (chooser);
	gtk_file_chooser_set_select_multiple (gtk_chooser, TRUE);

	g_signal_connect (gtk_chooser,
			  "response",
			  G_CALLBACK (response_cb),
			  chooser);
}

GeditFileChooserOpen *
_gedit_file_chooser_open_new (void)
{
	return g_object_new (GEDIT_TYPE_FILE_CHOOSER_OPEN, NULL);
}

GSList *
_gedit_file_chooser_open_get_files (GeditFileChooserOpen *chooser)
{
	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser), NULL);

	return gtk_file_chooser_get_files (get_gtk_file_chooser (chooser));
}

gchar *
_gedit_file_chooser_open_get_current_folder_uri (GeditFileChooserOpen *chooser)
{
	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser), NULL);

	return gtk_file_chooser_get_current_folder_uri (get_gtk_file_chooser (chooser));
}

void
_gedit_file_chooser_open_set_current_folder_uri (GeditFileChooserOpen *chooser,
						 const gchar          *uri)
{
	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser));

	gtk_file_chooser_set_current_folder_uri (get_gtk_file_chooser (chooser), uri);
}

const GtkSourceEncoding *
_gedit_file_chooser_open_get_encoding (GeditFileChooserOpen *chooser)
{
	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser), NULL);

	/* Stub */
	return NULL;
}
