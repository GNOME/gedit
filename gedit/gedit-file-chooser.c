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
					//g_message ("Mime-type '%s' is not related to 'text/plain'", mime_types[i]);

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
