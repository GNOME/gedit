/*
 * gedit-open-document-selector-store.h
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

#ifndef GEDIT_OPEN_DOCUMENT_SELECTOR_STORE_H
#define GEDIT_OPEN_DOCUMENT_SELECTOR_STORE_H

#include "gedit-open-document-selector-helper.h"
#include "gedit-open-document-selector.h"

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_OPEN_DOCUMENT_SELECTOR_STORE (gedit_open_document_selector_store_get_type ())

GEDIT_API
G_DECLARE_FINAL_TYPE (GeditOpenDocumentSelectorStore, gedit_open_document_selector_store, GEDIT, OPEN_DOCUMENT_SELECTOR_STORE, GObject)

#define GEDIT_OPEN_DOCUMENT_SELECTOR_STORE_ERROR gedit_open_document_selector_store_error_quark ()

typedef enum
{
	TYPE_OUT_OF_RANGE
} GeditOpenDocumentSelectorStoreError;

GEDIT_API
GQuark				 gedit_open_document_selector_store_error_quark				(void);

GEDIT_API
gint				 gedit_open_document_selector_store_get_recent_limit			(GeditOpenDocumentSelectorStore *store);

GEDIT_API
void				 gedit_open_document_selector_store_set_filter				(GeditOpenDocumentSelectorStore *store,
                                                                                          
const gchar                    *filter);

GEDIT_API
gchar				*gedit_open_document_selector_store_get_filter				(GeditOpenDocumentSelectorStore *store);

GEDIT_API
GList				*gedit_open_document_selector_store_update_list_finish			(GeditOpenDocumentSelectorStore  *open_document_selector_store,
                                                                                                         GAsyncResult                    *res,
                                                                                                         GError                         **error);

GEDIT_API
void				gedit_open_document_selector_store_update_list_async			(GeditOpenDocumentSelectorStore *open_document_selector_store,
                                                                                                         GeditOpenDocumentSelector      *open_document_selector,
                                                                                                         GCancellable                   *cancellable,
                                                                                                         GAsyncReadyCallback             callback,
                                                                                                         ListType                        type,
                                                                                                         gpointer                        user_data);

GEDIT_API
GeditOpenDocumentSelectorStore	*gedit_open_document_selector_store_get_default				(void);

G_END_DECLS

#endif /* GEDIT_OPEN_DOCUMENT_SELECTOR_STORE_H */
/* ex:set ts=8 noet: */
