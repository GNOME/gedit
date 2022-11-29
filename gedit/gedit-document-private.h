/*
 * gedit-document.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2014, 2020 Sébastien Wilmet
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

#ifndef GEDIT_DOCUMENT_PRIVATE_H
#define GEDIT_DOCUMENT_PRIVATE_H

#include "gedit-document.h"

G_BEGIN_DECLS

#define GEDIT_METADATA_ATTRIBUTE_POSITION "gedit-position"
#define GEDIT_METADATA_ATTRIBUTE_ENCODING "gedit-encoding"
#define GEDIT_METADATA_ATTRIBUTE_LANGUAGE "gedit-language"

G_GNUC_INTERNAL
glong		_gedit_document_get_seconds_since_last_save_or_load	(GeditDocument *doc);

G_GNUC_INTERNAL
gboolean	_gedit_document_needs_saving				(GeditDocument *doc);

G_GNUC_INTERNAL
gboolean	_gedit_document_get_empty_search			(GeditDocument *doc);

G_GNUC_INTERNAL
void		_gedit_document_set_create				(GeditDocument *doc,
									 gboolean       create);

G_GNUC_INTERNAL
gboolean	_gedit_document_get_create				(GeditDocument *doc);

G_GNUC_INTERNAL
gboolean	_gedit_document_is_untitled				(GeditDocument       *doc);

G_END_DECLS

#endif /* GEDIT_DOCUMENT_PRIVATE_H */
/* ex:set ts=8 noet: */
