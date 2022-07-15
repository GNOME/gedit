/*
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
 * Copyright (C) 2014 - Paolo Borelli
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

#include "gedit-recent.h"

void
gedit_recent_add_document (GeditDocument *document)
{
	TeplFile *file;
	GFile *location;
	GtkRecentManager *recent_manager;
	GtkRecentData recent_data;
	gchar *uri;
	static gchar *groups[2];

	g_return_if_fail (GEDIT_IS_DOCUMENT (document));

	file = tepl_buffer_get_file (TEPL_BUFFER (document));
	location = tepl_file_get_location (file);

	if (location == NULL)
	{
		return;
	}

	recent_manager = gtk_recent_manager_get_default ();

	groups[0] = (gchar *) g_get_application_name ();
	groups[1] = NULL;

	recent_data.display_name = NULL;
	recent_data.description = NULL;
	recent_data.mime_type = gedit_document_get_mime_type (document);
	recent_data.app_name = (gchar *) g_get_application_name ();
	recent_data.app_exec = g_strjoin (" ", g_get_prgname (), "%u", NULL);
	recent_data.groups = groups;
	recent_data.is_private = FALSE;

	uri = g_file_get_uri (location);

	if (!gtk_recent_manager_add_full (recent_manager, uri, &recent_data))
	{
		g_warning ("Failed to add uri '%s' to the recent manager.", uri);
	}

	g_free (uri);
	g_free (recent_data.app_exec);
	g_free (recent_data.mime_type);
}

void
gedit_recent_remove_if_local (GFile *location)
{
	g_return_if_fail (G_IS_FILE (location));

	/* If a file is local chances are that if load/save fails the file has
	 * beed removed and the failure is permanent so we remove it from the
	 * list of recent files. For remote files the failure may be just
	 * transitory and we keep the file in the list.
	 */
	if (g_file_has_uri_scheme (location, "file"))
	{
		GtkRecentManager *recent_manager;
		gchar *uri;

		recent_manager = gtk_recent_manager_get_default ();

		uri = g_file_get_uri (location);
		gtk_recent_manager_remove_item (recent_manager, uri, NULL);
		g_free (uri);
	}
}

/* ex:set ts=8 noet: */
