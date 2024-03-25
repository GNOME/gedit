/*
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2014-2020 Sébastien Wilmet
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

#ifndef GEDIT_DOCUMENT_H
#define GEDIT_DOCUMENT_H

#include <tepl/tepl.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_DOCUMENT (gedit_document_get_type())

G_DECLARE_DERIVABLE_TYPE (GeditDocument, gedit_document,
			  GEDIT, DOCUMENT,
			  TeplBuffer)

struct _GeditDocumentClass
{
	TeplBufferClass parent_class;

	/* Signals */

	void (* load)		(GeditDocument *document);

	void (* loaded)		(GeditDocument *document);

	void (* save)		(GeditDocument *document);

	void (* saved)		(GeditDocument *document);
};

GeditDocument *	gedit_document_new			(void);

GtkSourceFile *	gedit_document_get_file			(GeditDocument *doc);

gchar *		gedit_document_get_content_type		(GeditDocument *doc);

gchar *		gedit_document_get_mime_type		(GeditDocument *doc);

void 		gedit_document_set_language		(GeditDocument     *doc,
							 GtkSourceLanguage *lang);

gchar *		gedit_document_get_metadata		(GeditDocument *doc,
							 const gchar   *key);

void		gedit_document_set_metadata		(GeditDocument *doc,
							 const gchar   *first_key,
							 ...);

void		gedit_document_set_search_context	(GeditDocument          *doc,
							 GtkSourceSearchContext *search_context);

GtkSourceSearchContext *
		gedit_document_get_search_context	(GeditDocument *doc);

G_END_DECLS

#endif /* GEDIT_DOCUMENT_H */

/* ex:set ts=8 noet: */
