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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-file-chooser-dialog-gtk.h"

#include <string.h>

#include <glib/gi18n.h>

#include "gedit-encoding-items.h"
#include "gedit-debug.h"
#include "gedit-enum-types.h"
#include "gedit-settings.h"
#include "gedit-utils.h"

/* Choice IDs */
#define ENCODING_CHOICE "encoding"
#define NEWLINE_CHOICE  "newline"

#define ALL_FILES		_("All Files")
#define ALL_TEXT_FILES		_("All Text Files")

struct _GeditFileChooserDialogGtk
{
	GObject parent_instance;

	GSettings *filter_settings;

	GtkFileChooserNative *dialog;
	GtkResponseType accept_response;
	GtkResponseType cancel_response;
};

static void gedit_file_chooser_dialog_gtk_chooser_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (GeditFileChooserDialogGtk,
                        gedit_file_chooser_dialog_gtk,
                        G_TYPE_OBJECT,
                        0,
                        G_IMPLEMENT_INTERFACE (GEDIT_TYPE_FILE_CHOOSER_DIALOG,
                                               gedit_file_chooser_dialog_gtk_chooser_init))


static void
chooser_set_encoding (GeditFileChooserDialog  *dialog,
                      const GtkSourceEncoding *encoding)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	gtk_file_chooser_set_choice (GTK_FILE_CHOOSER (dialog_gtk->dialog),
				     ENCODING_CHOICE,
				     gtk_source_encoding_get_charset (encoding));
}

static const GtkSourceEncoding *
chooser_get_encoding (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	const char *charset = gtk_file_chooser_get_choice (GTK_FILE_CHOOSER (dialog_gtk->dialog), ENCODING_CHOICE);

	g_return_val_if_fail (charset != NULL, NULL);
	return gtk_source_encoding_get_from_charset (charset);
}

static void
chooser_set_newline_type (GeditFileChooserDialog *dialog,
                          GtkSourceNewlineType    newline_type)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);
	GEnumClass *enum_class;
	GEnumValue *enum_value;

	g_return_if_fail (dialog_gtk->dialog != NULL);
	g_return_if_fail (gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog_gtk->dialog)) == GTK_FILE_CHOOSER_ACTION_SAVE);

	enum_class = g_type_class_ref (GTK_SOURCE_TYPE_NEWLINE_TYPE);
	enum_value = g_enum_get_value (enum_class, newline_type);
	g_assert (enum_value != NULL);

	gtk_file_chooser_set_choice (GTK_FILE_CHOOSER (dialog_gtk->dialog),
				     NEWLINE_CHOICE, enum_value->value_nick);

	g_type_class_unref (enum_class);
}

static GtkSourceNewlineType
chooser_get_newline_type (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);
	const char *option_id;
	GEnumClass *enum_class;
	GEnumValue *enum_value;
	GtkSourceNewlineType newline_type;

	g_return_val_if_fail (dialog_gtk->dialog != NULL, GTK_SOURCE_NEWLINE_TYPE_DEFAULT);
	g_return_val_if_fail (gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog_gtk->dialog)) == GTK_FILE_CHOOSER_ACTION_SAVE,
	                      GTK_SOURCE_NEWLINE_TYPE_DEFAULT);

	option_id = gtk_file_chooser_get_choice (GTK_FILE_CHOOSER (dialog_gtk->dialog), NEWLINE_CHOICE);
	g_assert (option_id != NULL);

	enum_class = g_type_class_ref (GTK_SOURCE_TYPE_NEWLINE_TYPE);
	enum_value = g_enum_get_value_by_nick (enum_class, option_id);
	g_assert (enum_value != NULL);

	newline_type = enum_value->value;

	g_type_class_unref (enum_class);
	return newline_type;
}

static void
chooser_set_current_folder (GeditFileChooserDialog *dialog,
                            GFile                  *folder)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);
	gchar *uri = NULL;

	if (folder != NULL)
	{
		uri = g_file_get_uri (folder);
	}

	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dialog_gtk), uri);
	g_free (uri);
}

static void
chooser_set_current_name (GeditFileChooserDialog *dialog,
                          const gchar            *name)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog_gtk->dialog), name);
}

static void
chooser_set_file (GeditFileChooserDialog *dialog,
                  GFile                  *file)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	gtk_file_chooser_set_file (GTK_FILE_CHOOSER (dialog_gtk->dialog), file, NULL);
}

static GFile *
chooser_get_file (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	return gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog_gtk->dialog));
}


static GSList *
chooser_get_files (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	return gtk_file_chooser_get_files (GTK_FILE_CHOOSER (dialog_gtk->dialog));
}

static void
chooser_set_do_overwrite_confirmation (GeditFileChooserDialog *dialog,
                                       gboolean                overwrite_confirmation)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog_gtk->dialog), overwrite_confirmation);
}

static void
chooser_show (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	gtk_native_dialog_show (GTK_NATIVE_DIALOG (dialog_gtk->dialog));
}

static void
chooser_hide (GeditFileChooserDialog *dialog)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	gtk_native_dialog_hide (GTK_NATIVE_DIALOG (dialog_gtk->dialog));
}

static void
chooser_destroy (GeditFileChooserDialog *dialog)
{
	//gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
chooser_set_modal (GeditFileChooserDialog *dialog,
                   gboolean is_modal)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);

	gtk_native_dialog_set_modal (GTK_NATIVE_DIALOG (dialog_gtk->dialog), is_modal);
}

static void
chooser_add_pattern_filter (GeditFileChooserDialog *dialog,
                            const gchar            *name,
                            const gchar            *pattern)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (dialog);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog_gtk->dialog);
	GtkFileFilter *filter;

	filter = gtk_file_filter_new ();

	gtk_file_filter_set_name (filter, name);
	gtk_file_filter_add_pattern (filter, pattern);

	gtk_file_chooser_add_filter (chooser, filter);

	if (gtk_file_chooser_get_filter (chooser) == NULL)
	{
		gtk_file_chooser_set_filter (chooser, filter);
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
	iface->add_pattern_filter = chooser_add_pattern_filter;
}

static void
gedit_file_chooser_dialog_gtk_dispose (GObject *object)
{
	GeditFileChooserDialogGtk *dialog_gtk = GEDIT_FILE_CHOOSER_DIALOG_GTK (object);

	g_clear_object (&dialog_gtk->dialog);
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
create_encoding_choice (GeditFileChooserDialogGtk *dialog)
{
	GPtrArray *options, *option_labels;
	GSList *encodings;
	const GSList *l;

	options = g_ptr_array_new ();
	option_labels = g_ptr_array_new ();

	encodings = gedit_encoding_items_get ();
	for (l = encodings; l != NULL; l = l->next)
	{
		GeditEncodingItem *item = l->data;

		g_ptr_array_add (options, (gpointer)gtk_source_encoding_get_charset (gedit_encoding_item_get_encoding (item)));
		g_ptr_array_add (option_labels, (gpointer)gedit_encoding_item_get_name (item));
	}
	g_ptr_array_add (options, NULL);
	g_ptr_array_add (option_labels, NULL);

	gtk_file_chooser_add_choice (GTK_FILE_CHOOSER (dialog->dialog),
				     ENCODING_CHOICE, _("Character Encoding:"),
				     (const char **)options->pdata,
				     (const char **)option_labels->pdata);
	gtk_file_chooser_set_choice (GTK_FILE_CHOOSER (dialog->dialog),
				     ENCODING_CHOICE, g_ptr_array_index(options, 0));

	g_ptr_array_free (options, TRUE);
	g_ptr_array_free (option_labels, TRUE);
	g_slist_free_full (encodings, (GDestroyNotify)gedit_encoding_item_free);
}

static void
create_newline_choice (GeditFileChooserDialogGtk *dialog)
{
	GEnumClass *enum_class;
	GPtrArray *options, *option_labels;
	int i;

	enum_class = g_type_class_ref (GTK_SOURCE_TYPE_NEWLINE_TYPE);

	options = g_ptr_array_new ();
	option_labels = g_ptr_array_new ();

	for (i = 0; i < enum_class->n_values; i++)
	{
		const GEnumValue *v = &enum_class->values[i];
		g_ptr_array_add (options, (gpointer)v->value_nick);
		g_ptr_array_add (option_labels, (gpointer)gedit_utils_newline_type_to_string(v->value));
	}
	g_ptr_array_add (options, NULL);
	g_ptr_array_add (option_labels, NULL);

	gtk_file_chooser_add_choice (GTK_FILE_CHOOSER (dialog->dialog),
				     NEWLINE_CHOICE, _("Line Ending:"),
				     (const char **)options->pdata,
				     (const char **)option_labels->pdata);

	g_ptr_array_free (options, TRUE);
	g_ptr_array_free (option_labels, TRUE);
	g_type_class_unref (enum_class);

	chooser_set_newline_type (GEDIT_FILE_CHOOSER_DIALOG (dialog), GTK_SOURCE_NEWLINE_TYPE_DEFAULT);
}

static void
create_choices (GeditFileChooserDialogGtk *dialog,
		GeditFileChooserFlags      flags)
{
	gboolean needs_encoding;
	gboolean needs_line_ending;

	needs_encoding = (flags & GEDIT_FILE_CHOOSER_ENABLE_ENCODING) != 0;
	needs_line_ending = (flags & GEDIT_FILE_CHOOSER_ENABLE_LINE_ENDING) != 0;

	if (needs_encoding)
	{
		create_encoding_choice (dialog);
	}

	if (needs_line_ending)
	{
		create_newline_choice (dialog);
	}
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
		const gchar *name;
		gint id = 0;

		name = gtk_file_filter_get_name (filter);
		g_return_if_fail (name != NULL);

		if (strcmp (name, ALL_TEXT_FILES) == 0)
			id = 1;

		gedit_debug_message (DEBUG_COMMANDS, "Active filter: %s (%d)", name, id);

		g_settings_set_int (dialog->filter_settings,
				    GEDIT_SETTINGS_ACTIVE_FILE_FILTER, id);
	}
}

static void
dialog_response_cb (GtkNativeDialog           *dialog,
		    gint                       response_id,
		    GeditFileChooserDialogGtk *dialog_gtk)
{
	switch (response_id) {
	case GTK_RESPONSE_ACCEPT:
		response_id = dialog_gtk->accept_response;
		break;
	case GTK_RESPONSE_CANCEL:
		response_id = dialog_gtk->cancel_response;
		break;
	}
	g_signal_emit_by_name (dialog_gtk, "response", response_id);
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
	dialog->filter_settings = g_settings_new ("org.gnome.gedit.state.file-filter");
}

GeditFileChooserDialog *
gedit_file_chooser_dialog_gtk_create (const gchar             *title,
			              GtkWindow               *parent,
			              GeditFileChooserFlags    flags,
			              const GtkSourceEncoding *encoding,
				      const gchar             *cancel_label,
				      GtkResponseType          cancel_response,
				      const gchar             *accept_label,
				      GtkResponseType          accept_response)
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

	result = g_object_new (GEDIT_TYPE_FILE_CHOOSER_DIALOG_GTK, NULL);
	result->cancel_response = cancel_response;
	result->accept_response = accept_response;
	result->dialog = gtk_file_chooser_native_new (
		title, parent, action, accept_label, cancel_label);
	g_object_set (result->dialog,
		      "local-only", FALSE,
		      "select-multiple", select_multiple,
		      NULL);

	create_choices (result, flags);

	if (encoding != NULL)
	{
		chooser_set_encoding (GEDIT_FILE_CHOOSER_DIALOG (result), encoding);
	}

	active_filter = g_settings_get_int (result->filter_settings, GEDIT_SETTINGS_ACTIVE_FILE_FILTER);
	gedit_debug_message (DEBUG_COMMANDS, "Active filter: %d", active_filter);

	if ((flags & GEDIT_FILE_CHOOSER_ENABLE_DEFAULT_FILTERS) != 0)
	{
		/* Filters */
		filter = gtk_file_filter_new ();

		gtk_file_filter_set_name (filter, ALL_FILES);
		gtk_file_filter_add_pattern (filter, "*");
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (result->dialog), filter);

		if (active_filter != 1)
		{
			/* Make this filter the default */
			gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (result->dialog), filter);
		}

		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name (filter, ALL_TEXT_FILES);
		gtk_file_filter_add_custom (filter,
					    GTK_FILE_FILTER_MIME_TYPE,
					    all_text_files_filter,
					    NULL,
					    NULL);
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (result->dialog), filter);

		if (active_filter == 1)
		{
			/* Make this filter the default */
			gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (result->dialog), filter);
		}

		g_signal_connect (result->dialog,
				  "notify::filter",
				  G_CALLBACK (filter_changed),
				  NULL);
	}

	g_signal_connect (result->dialog, "response", G_CALLBACK (dialog_response_cb), result);

	return GEDIT_FILE_CHOOSER_DIALOG (result);
}

/* ex:set ts=8 noet: */
