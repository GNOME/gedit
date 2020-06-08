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

#include "gedit-file-chooser.h"
#include <glib/gi18n.h>
#include <gtksourceview/gtksource.h>
#include "gedit-settings.h"

/* Common code between the different GeditFileChooser's. */

#define ALL_FILES		_("All Files")
#define ALL_TEXT_FILES		_("All Text Files")

/* Returns: (transfer none) (element-type utf8): a list containing "text/plain"
 * first and then the list of mime-types unrelated to "text/plain" that
 * GtkSourceView supports for the syntax highlighting.
 */
static GSList *
get_supported_mime_types (void)
{
	static GSList *supported_mime_types = NULL;
	static gboolean initialized = FALSE;

	GtkSourceLanguageManager *language_manager;
	const gchar * const *language_ids;
	gint language_num;

	if (initialized)
	{
		return supported_mime_types;
	}

	language_manager = gtk_source_language_manager_get_default ();
	language_ids = gtk_source_language_manager_get_language_ids (language_manager);
	for (language_num = 0; language_ids != NULL && language_ids[language_num] != NULL; language_num++)
	{
		const gchar *cur_language_id = language_ids[language_num];
		GtkSourceLanguage *language;
		gchar **mime_types;
		gint mime_type_num;

		language = gtk_source_language_manager_get_language (language_manager, cur_language_id);
		mime_types = gtk_source_language_get_mime_types (language);

		if (mime_types == NULL)
		{
			continue;
		}

		for (mime_type_num = 0; mime_types[mime_type_num] != NULL; mime_type_num++)
		{
			const gchar *cur_mime_type = mime_types[mime_type_num];

			if (!g_content_type_is_a (cur_mime_type, "text/plain"))
			{
				//g_message ("Mime-type '%s' is not related to 'text/plain'", cur_mime_type);
				supported_mime_types = g_slist_prepend (supported_mime_types,
									g_strdup (cur_mime_type));
			}
		}

		g_strfreev (mime_types);
	}

	supported_mime_types = g_slist_prepend (supported_mime_types, g_strdup ("text/plain"));

	initialized = TRUE;
	return supported_mime_types;
}

/* FIXME: use globs too - Paolo (Aug. 27, 2007) */
static gboolean
all_text_files_filter (const GtkFileFilterInfo *filter_info,
		       gpointer                 data)
{
	GSList *supported_mime_types;
	GSList *l;

	if (filter_info->mime_type == NULL)
	{
		return FALSE;
	}

	/* The filter is matching:
	 * - the mime-types beginning with "text/".
	 * - the mime-types inheriting from a supported mime-type (note that
	 *   "text/plain" is the first supported mime-type).
	 */

	if (g_str_has_prefix (filter_info->mime_type, "text/"))
	{
		return TRUE;
	}

	supported_mime_types = get_supported_mime_types ();
	for (l = supported_mime_types; l != NULL; l = l->next)
	{
		const gchar *cur_supported_mime_type = l->data;

		if (g_content_type_is_a (filter_info->mime_type, cur_supported_mime_type))
		{
			return TRUE;
		}
	}

	return FALSE;
}

static void
notify_filter_cb (GtkFileChooser *chooser,
		  GParamSpec     *pspec,
		  gpointer        user_data)
{
	GtkFileFilter *filter;
	const gchar *name;
	gint id = 0;
	GeditSettings *settings;
	GSettings *file_chooser_state_settings;

	/* Remember the selected filter. */

	filter = gtk_file_chooser_get_filter (chooser);
	if (filter == NULL)
	{
		return;
	}

	name = gtk_file_filter_get_name (filter);
	if (g_strcmp0 (name, ALL_FILES) == 0)
	{
		id = 1;
	}

	settings = _gedit_settings_get_singleton ();
	file_chooser_state_settings = _gedit_settings_peek_file_chooser_state_settings (settings);
	g_settings_set_int (file_chooser_state_settings, GEDIT_SETTINGS_ACTIVE_FILE_FILTER, id);
}

void
_gedit_file_chooser_setup_filters (GtkFileChooser *chooser)
{
	GeditSettings *settings;
	GSettings *file_chooser_state_settings;
	gint active_filter;
	GtkFileFilter *filter;

	g_return_if_fail (GTK_IS_FILE_CHOOSER (chooser));

	settings = _gedit_settings_get_singleton ();
	file_chooser_state_settings = _gedit_settings_peek_file_chooser_state_settings (settings);
	active_filter = g_settings_get_int (file_chooser_state_settings, GEDIT_SETTINGS_ACTIVE_FILE_FILTER);

	/* "All Text Files" filter */
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, ALL_TEXT_FILES);
	gtk_file_filter_add_custom (filter,
				    GTK_FILE_FILTER_MIME_TYPE,
				    all_text_files_filter,
				    NULL,
				    NULL);

	g_object_ref_sink (filter);
	gtk_file_chooser_add_filter (chooser, filter);
	if (active_filter != 1)
	{
		/* Use this filter if set by user and as default. */
		gtk_file_chooser_set_filter (chooser, filter);
	}
	g_object_unref (filter);

	/* "All Files" filter */
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, ALL_FILES);
	gtk_file_filter_add_pattern (filter, "*");

	g_object_ref_sink (filter);
	gtk_file_chooser_add_filter (chooser, filter);
	if (active_filter == 1)
	{
		/* Use this filter if set by user. */
		gtk_file_chooser_set_filter (chooser, filter);
	}
	g_object_unref (filter);

	g_signal_connect (chooser,
			  "notify::filter",
			  G_CALLBACK (notify_filter_cb),
			  NULL);
}
