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
	GtkRecentData *recent_data;
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

	/* FIXME: redundant with app_name. Settings 'groups' to NULL would
	 * permit to use the much easier gtk_recent_manager_add_item() as the
	 * other fields would be the same (except possibly for the mime_type but
	 * it's not really important for the recent list).
	 *
	 * See commit 3b874fbb2466b62d828f9e7629ba8d8f28ba6fd6 (2002) with as
	 * message:
	 * Add the "gedit" group to recent items. filter by group instead of
	 * mime.
	 *
	 * So before, the filtering was based on mime types (with the "text/"
	 * prefix), nowadays we can filter with the app_name (and/or mime types
	 * too if wanted).
	 *
	 * Some plugins may rely on the 'groups' field with "gedit" in it, so it
	 * needs some adaptation.
	 */
	groups[0] = (gchar *) g_get_application_name ();
	groups[1] = NULL;

	/* Ensures to initialize the whole struct to 0's. Useful if the struct
	 * is extended.
	 */
	recent_data = g_new0 (GtkRecentData, 1);

	recent_data->mime_type = gedit_document_get_mime_type (document);
	recent_data->app_name = (gchar *) g_get_application_name ();
	recent_data->app_exec = g_strjoin (" ", g_get_prgname (), "%u", NULL);
	recent_data->groups = groups;

	uri = g_file_get_uri (location);

	if (!gtk_recent_manager_add_full (recent_manager, uri, recent_data))
	{
		g_warning ("Failed to add uri '%s' to the recent manager.", uri);
	}

	g_free (uri);
	g_free (recent_data->mime_type);
	g_free (recent_data->app_exec);
	g_free (recent_data);
}

/* If a file is local, chances are that if load/save fails the file has been
 * removed and the failure is permanent so we remove it from the list of recent
 * files. For remote files the failure may be just transitory and we keep the
 * file in the list.
 *
 * FIXME: for files coming from external disks, USB keys etc, if the external
 * storage device is not mounted, do not remove the file from the list. On the
 * other hand, if the external storage device is mounted and the file isn't
 * there, remove it from the list.
 *
 * FIXME: for remote files, perhaps do the same as for external storage devices,
 * IF the connection to the server succeeds (the directory is there).
 *
 * FIXME: for the above FIXMEs, we should probably rely (in part) on the GError
 * we receive.
 * And if it was a file loading, a useful information to have is whether the
 * file was opened from the list of recent files. If it was *not*, the user can
 * also just navigate again through the file hierarchy to re-open it later if
 * the file comes back.
 *
 * IDEA: or just never remove files from the recent list, and call it the
 * Recent History or something like that.
 */
void
gedit_recent_remove_if_local (GFile *location)
{
	g_return_if_fail (G_IS_FILE (location));

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
