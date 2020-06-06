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

#define ALL_FILES		_("All Files")
#define ALL_TEXT_FILES		_("All Text Files")

struct _GeditFileChooserDialogGtk
{
	GtkFileChooserDialog parent_instance;

	GSettings *filter_settings;

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


static GSList *
chooser_get_files (GeditFileChooserDialog *dialog)
{
	return gtk_file_chooser_get_files (GTK_FILE_CHOOSER (dialog));
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
chooser_hide (GeditFileChooserDialog *dialog)
{
	gtk_widget_hide (GTK_WIDGET (dialog));
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
chooser_add_pattern_filter (GeditFileChooserDialog *dialog,
                            const gchar            *name,
                            const gchar            *pattern)
{
	GtkFileFilter *filter;

	filter = gtk_file_filter_new ();

	gtk_file_filter_set_name (filter, name);
	gtk_file_filter_add_pattern (filter, pattern);

	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	if (gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (dialog)) == NULL)
	{
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);
	}
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
	iface->get_files = chooser_get_files;
	iface->set_do_overwrite_confirmation = chooser_set_do_overwrite_confirmation;
	iface->show = chooser_show;
	iface->hide = chooser_hide;
	iface->destroy = chooser_destroy;
	iface->set_modal = chooser_set_modal;
	iface->get_window = chooser_get_window;
	iface->add_pattern_filter = chooser_add_pattern_filter;
}

static void
gedit_file_chooser_dialog_gtk_dispose (GObject *object)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (object);

	g_clear_object (&dialog_gtk->filter_settings);

	G_OBJECT_CLASS (gedit_file_chooser_dialog_gtk_parent_class)->dispose (object);
}

static void
gedit_file_chooser_dialog_gtk_class_init (GeditFileChooserDialogGtkClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_file_chooser_dialog_gtk_dispose;
}

static void
create_option_menu (GeditFileChooserDialogGtk *dialog,
                    GeditFileChooserFlags      flags)
{
	GtkWidget *label;
	GtkWidget *menu;
	gboolean save_mode;

	label = gtk_label_new_with_mnemonic (_("C_haracter Encoding:"));
	gtk_widget_set_halign (label, GTK_ALIGN_START);

	save_mode = (flags & GEDIT_FILE_CHOOSER_SAVE) != 0;
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
create_extra_widget (GeditFileChooserDialogGtk *dialog,
                     GeditFileChooserFlags      flags)
{
	dialog->extra_widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_show (dialog->extra_widget);

	create_option_menu (dialog, flags);

	if ((flags & GEDIT_FILE_CHOOSER_SAVE) != 0)
	{
		create_newline_combo (dialog);
	}

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
filter_changed (GeditFileChooserDialogGtk *dialog,
		GParamSpec	       *pspec,
		gpointer		data)
{
	GtkFileFilter *filter;

	filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (dialog));
	if (filter != NULL)
	{
		/* Store ID of selected filter */
		const gchar *name;
		gint id = 0;

		name = gtk_file_filter_get_name (filter);
		g_return_if_fail (name != NULL);

		if (strcmp (name, ALL_FILES) == 0)
			id = 1;

		gedit_debug_message (DEBUG_COMMANDS, "Active filter: %s (%d)", name, id);

		g_settings_set_int (dialog->filter_settings,
				    GEDIT_SETTINGS_ACTIVE_FILE_FILTER, id);
	}
}

/* FIXME: use globs too - Paolo (Aug. 27, 2007) */
static gboolean
all_text_files_filter (const GtkFileFilterInfo *filter_info,
		       gpointer                 data)
{
	static GSList *known_mime_types = NULL;
	GSList *mime_types;

	if (known_mime_types == NULL)
	{
		GtkSourceLanguageManager *lm;
		const gchar * const *languages;

		lm = gtk_source_language_manager_get_default ();
		languages = gtk_source_language_manager_get_language_ids (lm);

		while ((languages != NULL) && (*languages != NULL))
		{
			gchar **mime_types;
			gint i;
			GtkSourceLanguage *lang;

			lang = gtk_source_language_manager_get_language (lm, *languages);
			g_return_val_if_fail (GTK_SOURCE_IS_LANGUAGE (lang), FALSE);
			++languages;

			mime_types = gtk_source_language_get_mime_types (lang);
			if (mime_types == NULL)
				continue;

			for (i = 0; mime_types[i] != NULL; i++)
			{
				if (!g_content_type_is_a (mime_types[i], "text/plain"))
				{
					gedit_debug_message (DEBUG_COMMANDS,
							     "Mime-type %s is not related to text/plain",
							     mime_types[i]);

					known_mime_types = g_slist_prepend (known_mime_types,
									    g_strdup (mime_types[i]));
				}
			}

			g_strfreev (mime_types);
		}

		/* known_mime_types always has "text/plain" as first item" */
		known_mime_types = g_slist_prepend (known_mime_types, g_strdup ("text/plain"));
	}

	/* known mime_types contains "text/plain" and then the list of mime-types unrelated to "text/plain"
	 * that gedit recognizes */

	if (filter_info->mime_type == NULL)
		return FALSE;

	/*
	 * The filter is matching:
	 * - the mime-types beginning with "text/"
	 * - the mime-types inheriting from a known mime-type (note the text/plain is
	 *   the first known mime-type)
	 */

	if (strncmp (filter_info->mime_type, "text/", 5) == 0)
		return TRUE;

	mime_types = known_mime_types;
	while (mime_types != NULL)
	{
		if (g_content_type_is_a (filter_info->mime_type, (const gchar*)mime_types->data))
			return TRUE;

		mime_types = g_slist_next (mime_types);
	}

	return FALSE;
}

static void
gedit_file_chooser_dialog_gtk_init (GeditFileChooserDialogGtk *dialog)
{
	dialog->filter_settings = g_settings_new ("org.gnome.gedit.state.file-chooser");
}

GeditFileChooserDialog *
gedit_file_chooser_dialog_gtk_create (const gchar           *title,
				      GtkWindow             *parent,
				      GeditFileChooserFlags  flags,
				      const gchar           *accept_label,
				      const gchar           *cancel_label)
{
	GeditFileChooserDialogGtk *result;
	GtkFileFilter *filter;
	gint active_filter;
	GtkFileChooserAction action;
	gboolean select_multiple;

	if ((flags & GEDIT_FILE_CHOOSER_SAVE) != 0)
	{
		action = GTK_FILE_CHOOSER_ACTION_SAVE;
		select_multiple = FALSE;
	}
	else
	{
		action = GTK_FILE_CHOOSER_ACTION_OPEN;
		select_multiple = TRUE;
	}

	result = g_object_new (GEDIT_TYPE_FILE_CHOOSER_DIALOG_GTK,
			       "title", title,
			       "local-only", FALSE,
			       "action", action,
			       "select-multiple", select_multiple,
			       NULL);

	create_extra_widget (result, flags);

	g_signal_connect (result,
			  "notify::action",
			  G_CALLBACK (action_changed),
			  NULL);

	active_filter = g_settings_get_int (result->filter_settings, GEDIT_SETTINGS_ACTIVE_FILE_FILTER);
	gedit_debug_message (DEBUG_COMMANDS, "Active filter: %d", active_filter);

	/* "All Text Files" filter */
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, ALL_TEXT_FILES);
	gtk_file_filter_add_custom (filter,
				    GTK_FILE_FILTER_MIME_TYPE,
				    all_text_files_filter,
				    NULL,
				    NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (result), filter);

	if (active_filter != 1)
	{
		/* Use this filter if set by user and as default */
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (result), filter);
	}

	/* "All Files" filter */
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, ALL_FILES);
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (result), filter);

	if (active_filter == 1)
	{
		/* Use this filter if set by user */
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (result), filter);
	}

	g_signal_connect (result,
			  "notify::filter",
			  G_CALLBACK (filter_changed),
			  NULL);

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
