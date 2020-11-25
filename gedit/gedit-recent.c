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
	GtkSourceFile *file;
	GFile *location;
	GtkRecentManager *recent_manager;
	GtkRecentData recent_data;
	gchar *uri;
	static gchar *groups[2];

	g_return_if_fail (GEDIT_IS_DOCUMENT (document));

	file = gedit_document_get_file (document);
	location = gtk_source_file_get_location (file);

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

static gint
sort_recent_items_mru (GtkRecentInfo *a,
                       GtkRecentInfo *b,
                       gpointer       unused)
{
	g_assert (a != NULL && b != NULL);
	return gtk_recent_info_get_modified (b) - gtk_recent_info_get_modified (a);
}

static void
populate_filter_info (GtkRecentInfo        *info,
                      GtkRecentFilterInfo  *filter_info,
                      GtkRecentFilterFlags  needed)
{
	filter_info->uri = gtk_recent_info_get_uri (info);
	filter_info->mime_type = gtk_recent_info_get_mime_type (info);

	filter_info->contains = GTK_RECENT_FILTER_URI | GTK_RECENT_FILTER_MIME_TYPE;

	if (needed & GTK_RECENT_FILTER_DISPLAY_NAME)
	{
		filter_info->display_name = gtk_recent_info_get_display_name (info);
		filter_info->contains |= GTK_RECENT_FILTER_DISPLAY_NAME;
	}
	else
	{
		filter_info->uri = NULL;
	}

	if (needed & GTK_RECENT_FILTER_APPLICATION)
	{
		filter_info->applications = (const gchar **) gtk_recent_info_get_applications (info, NULL);
		filter_info->contains |= GTK_RECENT_FILTER_APPLICATION;
	}
	else
	{
		filter_info->applications = NULL;
	}

	if (needed & GTK_RECENT_FILTER_GROUP)
	{
		filter_info->groups = (const gchar **) gtk_recent_info_get_groups (info, NULL);
		filter_info->contains |= GTK_RECENT_FILTER_GROUP;
	}
	else
	{
		filter_info->groups = NULL;
	}

	if (needed & GTK_RECENT_FILTER_AGE)
	{
		filter_info->age = gtk_recent_info_get_age (info);
		filter_info->contains |= GTK_RECENT_FILTER_AGE;
	}
	else
	{
		filter_info->age = -1;
	}
}

/* The GeditRecentConfiguration struct is allocated and owned by the caller */
void
gedit_recent_configuration_init_default (GeditRecentConfiguration *config)
{
	config->manager = gtk_recent_manager_get_default ();

	if (config->filter != NULL)
	{
		g_object_unref (config->filter);
	}

	config->filter = gtk_recent_filter_new ();
	gtk_recent_filter_add_application (config->filter, g_get_application_name ());
	gtk_recent_filter_add_mime_type (config->filter, "text/plain");
	g_object_ref_sink (config->filter);

	config->limit = 5;
	config->show_not_found = TRUE;
	config->show_private = FALSE;
	config->local_only = FALSE;

	config->substring_filter = NULL;
}

/* The GeditRecentConfiguration struct is owned and destroyed by the caller */
void
gedit_recent_configuration_destroy (GeditRecentConfiguration *config)
{
	g_clear_object (&config->filter);
	config->manager = NULL;

	g_clear_pointer (&config->substring_filter, (GDestroyNotify)g_free);
}

GList *
gedit_recent_get_items (GeditRecentConfiguration *config)
{
	GtkRecentFilterFlags needed;
	GList *items;
	GList *retitems = NULL;
	gint length;
	char *substring_filter = NULL;

	if (config->limit == 0)
	{
		return NULL;
	}

	items = gtk_recent_manager_get_items (config->manager);

	if (!items)
	{
		return NULL;
	}

	needed = gtk_recent_filter_get_needed (config->filter);
	if (config->substring_filter && *config->substring_filter != '\0')
	{
		gchar *filter_normalized;

		filter_normalized = g_utf8_normalize (config->substring_filter, -1, G_NORMALIZE_ALL);
		substring_filter = g_utf8_casefold (filter_normalized, -1);
		g_free (filter_normalized);
	}

	while (items)
	{
		GtkRecentInfo *info;
		GtkRecentFilterInfo filter_info;
		gboolean is_filtered;

		info = items->data;
		is_filtered = FALSE;

		if (config->local_only && !gtk_recent_info_is_local (info))
		{
			is_filtered = TRUE;
		}
		else if (!config->show_private && gtk_recent_info_get_private_hint (info))
		{
			is_filtered = TRUE;
		}
		else if (!config->show_not_found && !gtk_recent_info_exists (info))
		{
			is_filtered = TRUE;
		}
		else
		{
			if (substring_filter)
			{
				gchar *uri_normalized;
				gchar *uri_casefolded;

				uri_normalized = g_utf8_normalize (gtk_recent_info_get_uri_display (info), -1, G_NORMALIZE_ALL);
				uri_casefolded = g_utf8_casefold (uri_normalized, -1);
				g_free (uri_normalized);

				if (strstr (uri_casefolded, substring_filter) == NULL)
				{
					is_filtered = TRUE;
				}

				g_free (uri_casefolded);
			}

			if (!is_filtered)
			{
				populate_filter_info (info, &filter_info, needed);
				is_filtered = !gtk_recent_filter_filter (config->filter, &filter_info);

				/* these we own */
				if (filter_info.applications)
				{
					g_strfreev ((gchar **) filter_info.applications);
				}

				if (filter_info.groups)
				{
					g_strfreev ((gchar **) filter_info.groups);
				}
			}
		}

		if (!is_filtered)
		{
			retitems = g_list_prepend (retitems, info);
		}
		else
		{
			gtk_recent_info_unref (info);
		}

		items = g_list_delete_link (items, items);
	}

	g_free (substring_filter);

	if (!retitems)
	{
		return NULL;
	}

	retitems = g_list_sort_with_data (retitems, (GCompareDataFunc) sort_recent_items_mru, NULL);
	length = g_list_length (retitems);

	if ((config->limit != -1) && (length > config->limit))
	{
		GList *clamp, *l;

		clamp = g_list_nth (retitems, config->limit - 1);

		if (!clamp)
		{
			return retitems;
		}

		l = clamp->next;
		clamp->next = NULL;

		g_list_free_full (l, (GDestroyNotify) gtk_recent_info_unref);
	}

	return retitems;
}

/* ex:set ts=8 noet: */
