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
#include "gedit-file-chooser-dialog.h"

struct _GeditFileChooserOpenPrivate
{
	GeditFileChooserDialog *dialog;
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

	if (chooser->priv->dialog != NULL)
	{
		gedit_file_chooser_dialog_destroy (chooser->priv->dialog);
		chooser->priv->dialog = NULL;
	}

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
dialog_response_cb (GeditFileChooserDialog *dialog,
		    gint                    response_id,
		    GeditFileChooserOpen   *chooser)
{
	gboolean accept;

	accept = response_id == GTK_RESPONSE_ACCEPT;
	g_signal_emit (chooser, signals[SIGNAL_DONE], 0, accept);
}

/* Defer the call to gedit_file_chooser_dialog_create() when we know the parent,
 * or when we know that there is no parent.
 */
static void
create_dialog (GeditFileChooserOpen *chooser,
	       GtkWindow            *parent)
{
	if (chooser->priv->dialog != NULL)
	{
		return;
	}

	/* Translators: "Open Files" is the title of the file chooser window. */
	chooser->priv->dialog = gedit_file_chooser_dialog_create (C_("window title", "Open Files"),
								  parent,
								  GEDIT_FILE_CHOOSER_FLAG_OPEN,
								  _("_Open"),
								  _("_Cancel"));

	g_signal_connect (chooser->priv->dialog,
			  "response",
			  G_CALLBACK (dialog_response_cb),
			  chooser);
}

static void
_gedit_file_chooser_open_init (GeditFileChooserOpen *chooser)
{
	chooser->priv = _gedit_file_chooser_open_get_instance_private (chooser);
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

	create_dialog (chooser, parent);
}

void
_gedit_file_chooser_open_show (GeditFileChooserOpen *chooser)
{
	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser));

	create_dialog (chooser, NULL);
	gedit_file_chooser_dialog_show (chooser->priv->dialog);
}

GSList *
_gedit_file_chooser_open_get_files (GeditFileChooserOpen *chooser)
{
	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser), NULL);

	return gedit_file_chooser_dialog_get_files (chooser->priv->dialog);
}

gchar *
_gedit_file_chooser_open_get_current_folder_uri (GeditFileChooserOpen *chooser)
{
	GSList *files;
	GFile *parent;
	gchar *folder_uri = NULL;

	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser), NULL);

	files = _gedit_file_chooser_open_get_files (chooser);
	if (files == NULL)
	{
		return NULL;
	}

	parent = g_file_get_parent (files->data);
	if (parent != NULL)
	{
		folder_uri = g_file_get_uri (parent);
		g_object_unref (parent);
	}

	g_slist_free_full (files, g_object_unref);
	return folder_uri;
}

void
_gedit_file_chooser_open_set_current_folder_uri (GeditFileChooserOpen *chooser,
						 const gchar          *uri)
{
	GFile *folder;

	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser));

	folder = g_file_new_for_uri (uri);
	gedit_file_chooser_dialog_set_current_folder (chooser->priv->dialog, folder);
	g_object_unref (folder);
}

const GtkSourceEncoding *
_gedit_file_chooser_open_get_encoding (GeditFileChooserOpen *chooser)
{
	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_OPEN (chooser), NULL);

	return gedit_file_chooser_dialog_get_encoding (chooser->priv->dialog);
}
