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
#include <glib/gi18n.h>

struct _GeditFileChooserOpenPrivate
{
	GtkFileChooserNative *chooser_native;
};

enum
{
	SIGNAL_DONE,
	N_SIGNALS
};

static guint signals[N_SIGNALS];

G_DEFINE_TYPE_WITH_PRIVATE (GeditFileChooserOpen, _gedit_file_chooser_open, G_TYPE_OBJECT)

static void
_gedit_file_chooser_open_dispose (GObject *object)
{
	GeditFileChooserOpen *chooser = GEDIT_FILE_CHOOSER_OPEN (object);

	g_clear_object (&chooser->priv->chooser_native);

	G_OBJECT_CLASS (_gedit_file_chooser_open_parent_class)->dispose (object);
}

static void
_gedit_file_chooser_open_class_init (GeditFileChooserOpenClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = _gedit_file_chooser_open_dispose;

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
chooser_native_response_cb (GtkFileChooserNative *chooser_native,
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
	chooser->priv = _gedit_file_chooser_open_get_instance_private (chooser);

	/* Translators: "Open Files" is the title of the file chooser window. */
	chooser->priv->chooser_native = gtk_file_chooser_native_new (C_("window title", "Open Files"),
								     NULL,
								     GTK_FILE_CHOOSER_ACTION_OPEN,
								     NULL,
								     NULL);

	/* Set the dialog as modal. It's a workaround for this bug:
	 * https://gitlab.gnome.org/GNOME/gtk/issues/2824
	 * "GtkNativeDialog: non-modal and gtk_native_dialog_show(), doesn't
	 * present the window"
	 *
	 * - Drag-and-drop files from the file chooser to the GeditWindow: OK,
	 *   it still works.
	 * - Other main windows not being "blocked"/insensitive
	 *   (GtkWindowGroup): OK, calling gtk_native_dialog_set_transient_for()
	 *   does the right thing.
	 *
	 * Even if the above GTK bug is fixed, the file chooser can be kept
	 * modal, except if there was a good reason for not being modal (what
	 * reason(s)?).
	 */
	gtk_native_dialog_set_modal (GTK_NATIVE_DIALOG (chooser->priv->chooser_native), TRUE);

	g_signal_connect (chooser->priv->chooser_native,
			  "response",
			  G_CALLBACK (chooser_native_response_cb),
			  chooser);
}

GeditFileChooserOpen *
_gedit_file_chooser_open_new (void)
{
	return g_object_new (GEDIT_TYPE_FILE_CHOOSER_OPEN, NULL);
}

void
_gedit_file_chooser_open_set_transient_for (GeditFileChooserOpen *chooser,
					    GtkWindow            *parent)
{
	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser));
	g_return_if_fail (parent == NULL || GTK_IS_WINDOW (parent));

	gtk_native_dialog_set_transient_for (GTK_NATIVE_DIALOG (chooser->priv->chooser_native), parent);
}

void
_gedit_file_chooser_open_show (GeditFileChooserOpen *chooser)
{
	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser));

	gtk_native_dialog_show (GTK_NATIVE_DIALOG (chooser->priv->chooser_native));
}

GSList *
_gedit_file_chooser_open_get_files (GeditFileChooserOpen *chooser)
{
	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser), NULL);

	return gtk_file_chooser_get_files (GTK_FILE_CHOOSER (chooser->priv->chooser_native));
}

gchar *
_gedit_file_chooser_open_get_current_folder_uri (GeditFileChooserOpen *chooser)
{
	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser), NULL);

	return gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (chooser->priv->chooser_native));
}

void
_gedit_file_chooser_open_set_current_folder_uri (GeditFileChooserOpen *chooser,
						 const gchar          *uri)
{
	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser));

	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (chooser->priv->chooser_native), uri);
}
