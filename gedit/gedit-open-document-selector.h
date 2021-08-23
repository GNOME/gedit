/*
 * gedit-open-document-selector.h
 * This file is part of gedit
 *
 * Copyright (C) 2014 - Sébastien Lafargue
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

#ifndef GEDIT_OPEN_DOCUMENT_SELECTOR_H
#define GEDIT_OPEN_DOCUMENT_SELECTOR_H

#include <glib-object.h>
#include "gedit-window.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_OPEN_DOCUMENT_SELECTOR (gedit_open_document_selector_get_type ())

GEDIT_API
G_DECLARE_FINAL_TYPE (GeditOpenDocumentSelector, gedit_open_document_selector, GEDIT, OPEN_DOCUMENT_SELECTOR, GtkBox)

GEDIT_API
GeditOpenDocumentSelector	*gedit_open_document_selector_new		(GeditWindow               *window);

GEDIT_API
GeditWindow			*gedit_open_document_selector_get_window	(GeditOpenDocumentSelector *selector);

GEDIT_API
GtkWidget			*gedit_open_document_selector_get_search_entry	(GeditOpenDocumentSelector *selector);

G_END_DECLS

#endif /* GEDIT_OPEN_DOCUMENT_SELECTOR_H */
/* ex:set ts=8 noet: */
