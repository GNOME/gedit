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
#include "gedit-encodings-combo-box.h"

/* A GtkFileChooserDialog to *open* files. */

struct _GeditFileChooserOpenDialogPrivate
{
	GeditEncodingsComboBox *encodings_combo_box;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditFileChooserOpenDialog, _gedit_file_chooser_open_dialog, GEDIT_TYPE_FILE_CHOOSER_OPEN)

static void
setup_encoding_extra_widget (GeditFileChooserOpenDialog *chooser,
			     GtkFileChooser             *gtk_chooser)
{
	GtkWidget *label;
	GtkWidget *encodings_combo_box;
	GtkWidget *hgrid;

	g_assert (chooser->priv->encodings_combo_box == NULL);

	label = gtk_label_new_with_mnemonic (_("C_haracter Encoding:"));
	encodings_combo_box = gedit_encodings_combo_box_new (FALSE);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), encodings_combo_box);

	hgrid = gtk_grid_new ();
	gtk_grid_set_column_spacing (GTK_GRID (hgrid), 6);
	gtk_container_add (GTK_CONTAINER (hgrid), label);
	gtk_container_add (GTK_CONTAINER (hgrid), encodings_combo_box);

	chooser->priv->encodings_combo_box = GEDIT_ENCODINGS_COMBO_BOX (encodings_combo_box);
	g_object_ref_sink (chooser->priv->encodings_combo_box);

	gtk_widget_show_all (hgrid);
	gtk_file_chooser_set_extra_widget (gtk_chooser, hgrid);
}

static void
_gedit_file_chooser_open_dialog_dispose (GObject *object)
{
	GeditFileChooserOpenDialog *chooser = GEDIT_FILE_CHOOSER_OPEN_DIALOG (object);

	g_clear_object (&chooser->priv->encodings_combo_box);

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

	setup_encoding_extra_widget (GEDIT_FILE_CHOOSER_OPEN_DIALOG (chooser),
				     GTK_FILE_CHOOSER (file_chooser));

	if (g_object_is_floating (file_chooser))
	{
		g_object_ref_sink (file_chooser);
	}

	return GTK_FILE_CHOOSER (file_chooser);
}

static const GtkSourceEncoding *
chooser_get_encoding (GeditFileChooser *_chooser)
{
	GeditFileChooserOpenDialog *chooser = GEDIT_FILE_CHOOSER_OPEN_DIALOG (_chooser);

	return gedit_encodings_combo_box_get_selected_encoding (chooser->priv->encodings_combo_box);
}

static void
_gedit_file_chooser_open_dialog_class_init (GeditFileChooserOpenDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditFileChooserClass *file_chooser_class = GEDIT_FILE_CHOOSER_CLASS (klass);

	object_class->dispose = _gedit_file_chooser_open_dialog_dispose;

	file_chooser_class->create_gtk_file_chooser = chooser_create_gtk_file_chooser;
	file_chooser_class->get_encoding = chooser_get_encoding;
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
