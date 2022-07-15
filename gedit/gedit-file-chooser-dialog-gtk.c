/*
 * gedit-file-chooser-dialog-gtk.c
 * This file is part of gedit
 *
 * Copyright (C) 2005-2007 - Paolo Maggi
 * Copyright (C) 2014 - Jesse van den Kieboom
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

/* TODO: Override set_extra_widget */
/* TODO: add encoding property */

#include "config.h"
#include "gedit-file-chooser-dialog-gtk.h"
#include <string.h>
#include <glib/gi18n.h>
#include "gedit-encodings-combo-box.h"
#include "gedit-debug.h"
#include "gedit-enum-types.h"
#include "gedit-settings.h"
#include "gedit-utils.h"
#include "gedit-file-chooser.h"

struct _GeditFileChooserDialogGtk
{
	GtkFileChooserDialog parent_instance;

	GeditFileChooser *gedit_file_chooser;

	GtkWidget *option_menu;
	GtkWidget *extra_widget;

	GtkWidget *newline_label;
	GtkWidget *newline_combo;
	GtkListStore *newline_store;
};

static void gedit_file_chooser_dialog_gtk_chooser_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (GeditFileChooserDialogGtk,
                        gedit_file_chooser_dialog_gtk,
                        GTK_TYPE_FILE_CHOOSER_DIALOG,
                        0,
                        G_IMPLEMENT_INTERFACE (GEDIT_TYPE_FILE_CHOOSER_DIALOG,
                                               gedit_file_chooser_dialog_gtk_chooser_init))


static void
chooser_set_encoding (GeditFileChooserDialog  *dialog,
                      const GtkSourceEncoding *encoding)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	g_return_if_fail (GEDIT_IS_ENCODINGS_COMBO_BOX (dialog_gtk->option_menu));

	gedit_encodings_combo_box_set_selected_encoding (GEDIT_ENCODINGS_COMBO_BOX (dialog_gtk->option_menu),
	                                                 encoding);
}

static const GtkSourceEncoding *
chooser_get_encoding (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	g_return_val_if_fail (GEDIT_IS_ENCODINGS_COMBO_BOX (dialog_gtk->option_menu), NULL);
	g_return_val_if_fail ((gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog)) == GTK_FILE_CHOOSER_ACTION_OPEN ||
			       gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog)) == GTK_FILE_CHOOSER_ACTION_SAVE), NULL);

	return gedit_encodings_combo_box_get_selected_encoding (
				GEDIT_ENCODINGS_COMBO_BOX (dialog_gtk->option_menu));
}

static void
set_enum_combo (GtkComboBox *combo,
                gint         value)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	model = gtk_combo_box_get_model (combo);

	if (!gtk_tree_model_get_iter_first (model, &iter))
	{
		return;
	}

	do
	{
		gint nt;

		gtk_tree_model_get (model, &iter, 1, &nt, -1);

		if (value == nt)
		{
			gtk_combo_box_set_active_iter (combo, &iter);
			break;
		}
	} while (gtk_tree_model_iter_next (model, &iter));
}

static void
chooser_set_newline_type (GeditFileChooserDialog *dialog,
                          GtkSourceNewlineType    newline_type)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	g_return_if_fail (gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog)) == GTK_FILE_CHOOSER_ACTION_SAVE);

	set_enum_combo (GTK_COMBO_BOX (dialog_gtk->newline_combo), newline_type);
}

static GtkSourceNewlineType
chooser_get_newline_type (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);
	GtkTreeIter iter;
	GtkSourceNewlineType newline_type;

	g_return_val_if_fail (gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog)) == GTK_FILE_CHOOSER_ACTION_SAVE,
	                      GTK_SOURCE_NEWLINE_TYPE_DEFAULT);

	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (dialog_gtk->newline_combo),
	                               &iter);

	gtk_tree_model_get (GTK_TREE_MODEL (dialog_gtk->newline_store),
	                    &iter,
	                    1,
	                    &newline_type,
	                    -1);

	return newline_type;
}

static void
chooser_set_current_folder (GeditFileChooserDialog *dialog,
                            GFile                  *folder)
{
	gchar *uri = NULL;

	if (folder != NULL)
	{
		uri = g_file_get_uri (folder);
	}

	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dialog), uri);
	g_free (uri);
}

static void
chooser_set_current_name (GeditFileChooserDialog *dialog,
                          const gchar            *name)
{
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), name);
}

static void
chooser_set_file (GeditFileChooserDialog *dialog,
                  GFile                  *file)
{
	gtk_file_chooser_set_file (GTK_FILE_CHOOSER (dialog), file, NULL);
}

static GFile *
chooser_get_file (GeditFileChooserDialog *dialog)
{
	return gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
}

static void
chooser_set_do_overwrite_confirmation (GeditFileChooserDialog *dialog,
                                       gboolean                overwrite_confirmation)
{
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), overwrite_confirmation);
}

static void
chooser_show (GeditFileChooserDialog *dialog)
{
	gtk_window_present (GTK_WINDOW (dialog));
	gtk_widget_grab_focus (GTK_WIDGET (dialog));
}

static void
chooser_destroy (GeditFileChooserDialog *dialog)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
chooser_set_modal (GeditFileChooserDialog *dialog,
                   gboolean is_modal)
{
	gtk_window_set_modal (GTK_WINDOW (dialog), is_modal);
}

static GtkWindow *
chooser_get_window (GeditFileChooserDialog *dialog)
{
	return GTK_WINDOW (dialog);
}

static void
gedit_file_chooser_dialog_gtk_chooser_init (gpointer g_iface,
                                            gpointer iface_data)
{
	GeditFileChooserDialogInterface *iface = g_iface;

	iface->set_encoding = chooser_set_encoding;
	iface->get_encoding = chooser_get_encoding;

	iface->set_newline_type = chooser_set_newline_type;
	iface->get_newline_type = chooser_get_newline_type;

	iface->set_current_folder = chooser_set_current_folder;
	iface->set_current_name = chooser_set_current_name;
	iface->set_file = chooser_set_file;
	iface->get_file = chooser_get_file;
	iface->set_do_overwrite_confirmation = chooser_set_do_overwrite_confirmation;
	iface->show = chooser_show;
	iface->destroy = chooser_destroy;
	iface->set_modal = chooser_set_modal;
	iface->get_window = chooser_get_window;
}

static void
gedit_file_chooser_dialog_gtk_dispose (GObject *object)
{
	GeditFileChooserDialogGtk *dialog = GEDIT_FILE_CHOOSER_DIALOG_GTK (object);

	g_clear_object (&dialog->gedit_file_chooser);

	G_OBJECT_CLASS (gedit_file_chooser_dialog_gtk_parent_class)->dispose (object);
}

static void
gedit_file_chooser_dialog_gtk_class_init (GeditFileChooserDialogGtkClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_file_chooser_dialog_gtk_dispose;
}

static void
create_option_menu (GeditFileChooserDialogGtk *dialog)
{
	GtkWidget *label;
	GtkWidget *menu;
	gboolean save_mode;

	label = gtk_label_new_with_mnemonic (_("C_haracter Encoding:"));
	gtk_widget_set_halign (label, GTK_ALIGN_START);

	save_mode = TRUE;
	menu = gedit_encodings_combo_box_new (save_mode);

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), menu);

	gtk_box_pack_start (GTK_BOX (dialog->extra_widget),
	                    label,
	                    FALSE,
	                    TRUE,
	                    0);

	gtk_box_pack_start (GTK_BOX (dialog->extra_widget),
	                    menu,
	                    TRUE,
	                    TRUE,
	                    0);

	gtk_widget_show (label);
	gtk_widget_show (menu);

	dialog->option_menu = menu;
}

static void
update_newline_visibility (GeditFileChooserDialogGtk *dialog)
{
	gboolean visible = gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog)) == GTK_FILE_CHOOSER_ACTION_SAVE;

	gtk_widget_set_visible (dialog->newline_label, visible);
	gtk_widget_set_visible (dialog->newline_combo, visible);
}

static void
newline_combo_append (GtkComboBox          *combo,
		      GtkListStore         *store,
		      GtkTreeIter          *iter,
		      const gchar          *label,
		      GtkSourceNewlineType  newline_type)
{
	gtk_list_store_append (store, iter);
	gtk_list_store_set (store, iter, 0, label, 1, newline_type, -1);

	if (newline_type == GTK_SOURCE_NEWLINE_TYPE_DEFAULT)
	{
		gtk_combo_box_set_active_iter (combo, iter);
	}
}

static void
create_newline_combo (GeditFileChooserDialogGtk *dialog)
{
	GtkWidget *label, *combo;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeIter iter;

	label = gtk_label_new_with_mnemonic (_("L_ine Ending:"));
	gtk_widget_set_halign (label, GTK_ALIGN_START);

	store = gtk_list_store_new (2, G_TYPE_STRING, GTK_SOURCE_TYPE_NEWLINE_TYPE);
	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
	renderer = gtk_cell_renderer_text_new ();

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
	                            renderer,
	                            TRUE);

	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo),
	                               renderer,
	                               "text",
	                               0);

	newline_combo_append (GTK_COMBO_BOX (combo),
	                      store,
	                      &iter,
	                      gedit_utils_newline_type_to_string (GTK_SOURCE_NEWLINE_TYPE_LF),
	                      GTK_SOURCE_NEWLINE_TYPE_LF);

	newline_combo_append (GTK_COMBO_BOX (combo),
	                      store,
	                      &iter,
	                      gedit_utils_newline_type_to_string (GTK_SOURCE_NEWLINE_TYPE_CR),
	                      GTK_SOURCE_NEWLINE_TYPE_CR);

	newline_combo_append (GTK_COMBO_BOX (combo),
	                      store,
	                      &iter,
	                      gedit_utils_newline_type_to_string (GTK_SOURCE_NEWLINE_TYPE_CR_LF),
	                      GTK_SOURCE_NEWLINE_TYPE_CR_LF);

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

	gtk_box_pack_start (GTK_BOX (dialog->extra_widget),
	                    label,
	                    FALSE,
	                    TRUE,
	                    0);

	gtk_box_pack_start (GTK_BOX (dialog->extra_widget),
	                    combo,
	                    TRUE,
	                    TRUE,
	                    0);

	dialog->newline_combo = combo;
	dialog->newline_label = label;
	dialog->newline_store = store;

	update_newline_visibility (dialog);
}

static void
create_extra_widget (GeditFileChooserDialogGtk *dialog)
{
	dialog->extra_widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (dialog->extra_widget);

	create_option_menu (dialog);
	create_newline_combo (dialog);

	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), dialog->extra_widget);
}

static void
action_changed (GeditFileChooserDialogGtk *dialog,
		GParamSpec                *pspec,
		gpointer                   data)
{
	GtkFileChooserAction action;

	action = gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog));

	switch (action)
	{
		case GTK_FILE_CHOOSER_ACTION_OPEN:
			g_object_set (dialog->option_menu,
			              "save_mode", FALSE,
			              NULL);
			gtk_widget_show (dialog->option_menu);
			break;
		case GTK_FILE_CHOOSER_ACTION_SAVE:
			g_object_set (dialog->option_menu,
			              "save_mode", TRUE,
			              NULL);
			gtk_widget_show (dialog->option_menu);
			break;
		default:
			gtk_widget_hide (dialog->option_menu);
	}

	update_newline_visibility (dialog);
}

static void
gedit_file_chooser_dialog_gtk_init (GeditFileChooserDialogGtk *dialog)
{
}

GeditFileChooserDialog *
gedit_file_chooser_dialog_gtk_create (const gchar *title,
				      GtkWindow   *parent,
				      const gchar *accept_label,
				      const gchar *cancel_label)
{
	GeditFileChooserDialogGtk *result;

	result = g_object_new (GEDIT_TYPE_FILE_CHOOSER_DIALOG_GTK,
			       "title", title,
			       "local-only", FALSE,
			       "action", GTK_FILE_CHOOSER_ACTION_SAVE,
			       "select-multiple", FALSE,
			       NULL);

	create_extra_widget (result);

	g_signal_connect (result,
			  "notify::action",
			  G_CALLBACK (action_changed),
			  NULL);

	/* FIXME: attention, there is a ref cycle here. This will be fixed when
	 * GeditFileChooserSave will be created (and this class removed).
	 */
	result->gedit_file_chooser = _gedit_file_chooser_new ();
	_gedit_file_chooser_set_gtk_file_chooser (result->gedit_file_chooser,
						  GTK_FILE_CHOOSER (result));

	if (parent != NULL)
	{
		gtk_window_set_transient_for (GTK_WINDOW (result), parent);
		gtk_window_set_destroy_with_parent (GTK_WINDOW (result), TRUE);
	}

	gtk_dialog_add_button (GTK_DIALOG (result), cancel_label, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (result), accept_label, GTK_RESPONSE_ACCEPT);
	gtk_dialog_set_default_response (GTK_DIALOG (result), GTK_RESPONSE_ACCEPT);

	return GEDIT_FILE_CHOOSER_DIALOG (result);
}

/* ex:set ts=8 noet: */
