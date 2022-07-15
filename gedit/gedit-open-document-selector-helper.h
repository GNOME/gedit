/*
 * gedit-open-document-selector-helper.h
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

#ifndef GEDIT_OPEN_DOCUMENT_SELECTOR_HELPER_H
#define GEDIT_OPEN_DOCUMENT_SELECTOR_HELPER_H

#include "gedit-open-document-selector.h"

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct
{
	gchar *uri;
	gchar *name;
	gchar *path;
	GDateTime *accessed;
} FileItem;

typedef enum
{
	GEDIT_OPEN_DOCUMENT_SELECTOR_RECENT_FILES_LIST = 0,
	GEDIT_OPEN_DOCUMENT_SELECTOR_HOME_DIR_LIST,
	GEDIT_OPEN_DOCUMENT_SELECTOR_DESKTOP_DIR_LIST,
	GEDIT_OPEN_DOCUMENT_SELECTOR_LOCAL_BOOKMARKS_DIR_LIST,
	GEDIT_OPEN_DOCUMENT_SELECTOR_FILE_BROWSER_ROOT_DIR_LIST,
	GEDIT_OPEN_DOCUMENT_SELECTOR_ACTIVE_DOC_DIR_LIST,
	GEDIT_OPEN_DOCUMENT_SELECTOR_CURRENT_DOCS_LIST,
	GEDIT_OPEN_DOCUMENT_SELECTOR_LIST_TYPE_NUM_OF_LISTS
} ListType;

/* Use #if 1 and rebuild to activate selector debugging and timing */
#if 0
#define DEBUG_OPEN_DOCUMENT_SELECTOR
#endif

#ifdef DEBUG_OPEN_DOCUMENT_SELECTOR
G_GNUC_UNUSED static const gchar *list_type_string[] =
{
	"RECENT_FILES_LIST",
	"HOME_DIR_LIST",
	"DESKTOP_DIR_LIST",
	"LOCAL_BOOKMARKS_DIR_LIST",
	"FILE_BROWSER_ROOT_DIR_LIST",
	"ACTIVE_DOC_DIR_LIST",
	"CURRENT_DOCS_LIST"
};

#define DEBUG_SELECTOR(x) do { x; } while (0)
#define DEBUG_SELECTOR_TIMER_DECL G_GNUC_UNUSED GTimer *debug_timer;
#define DEBUG_SELECTOR_TIMER_NEW debug_timer = g_timer_new ();
#define DEBUG_SELECTOR_TIMER_DESTROY g_timer_destroy (debug_timer);
#define DEBUG_SELECTOR_TIMER_GET g_timer_elapsed (debug_timer, NULL)
#else
#define DEBUG_SELECTOR(x)
#define DEBUG_SELECTOR_TIMER_DECL
#define DEBUG_SELECTOR_TIMER_NEW
#define DEBUG_SELECTOR_TIMER_DESTROY
#define DEBUG_SELECTOR_TIMER_GET
#endif

typedef struct
{
	GeditOpenDocumentSelector *selector;
	ListType type;
} PushMessage;

void		 gedit_open_document_selector_debug_print_list		(const gchar *title,
                                                                         GList       *fileitem_list);

GList		*gedit_open_document_selector_copy_file_items_list	(const GList *file_items_list);

void		 gedit_open_document_selector_free_file_items_list	(GList *file_items_list);

FileItem	*gedit_open_document_selector_create_fileitem_item	(void);

void		 gedit_open_document_selector_free_fileitem_item	(FileItem *item);

FileItem	*gedit_open_document_selector_copy_fileitem_item	(FileItem *item);

G_END_DECLS

#endif /* GEDIT_OPEN_DOCUMENT_SELECTOR_HELPER_H */

/* ex:set ts=8 noet: */
