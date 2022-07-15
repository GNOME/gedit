/*
 * gedit-open-document-selector-helper.c
 * This file is part of gedit
 *
 * Copyright (C) 2015 - Sébastien Lafargue
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gedit-open-document-selector-helper.h"

void
gedit_open_document_selector_debug_print_list (const gchar *title,
                                               GList       *fileitem_list)
{
	FileItem *item;
	GList *l;
	char *accessed = NULL;

	g_print ("%s\n", title);

	for (l = fileitem_list; l != NULL; l = l->next)
	{
		item = (FileItem *)l->data;

		accessed = g_date_time_format_iso8601 (item->accessed);

		g_print ("%s uri:%s (%s %s)\n",
		         accessed,
		         item->uri,
		         item->name,
		         item->path);
		
		g_clear_pointer (&accessed, g_free);
	}
}

FileItem *
gedit_open_document_selector_create_fileitem_item (void)
{
	FileItem *item;

	item = g_slice_new0 (FileItem);

	return item;
}

void
gedit_open_document_selector_free_fileitem_item (FileItem *item)
{
	g_free (item->uri);
	g_free (item->name);
	g_free (item->path);
	g_clear_pointer (&item->accessed, g_date_time_unref);

	g_slice_free (FileItem, item);
}

FileItem *
gedit_open_document_selector_copy_fileitem_item (FileItem *item)
{
	FileItem *new_item;

	new_item = gedit_open_document_selector_create_fileitem_item ();

	new_item->uri = g_strdup (item->uri);
	new_item->name = g_strdup (item->name);
	new_item->path = g_strdup (item->path);
	new_item->accessed = g_date_time_ref (item->accessed);

	return new_item;
}

inline GList *
gedit_open_document_selector_copy_file_items_list (const GList *file_items_list)
{
	GList *new_file_items_list;

	new_file_items_list = g_list_copy_deep ((GList *)file_items_list,
	                                        (GCopyFunc)gedit_open_document_selector_copy_fileitem_item,
	                                        NULL);

	return new_file_items_list;
}

inline void
gedit_open_document_selector_free_file_items_list (GList *file_items_list)
{
	g_list_free_full (file_items_list,
	                  (GDestroyNotify)gedit_open_document_selector_free_fileitem_item);
}

/* ex:set ts=8 noet: */
