/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-metadata-manager.h
 * This file is part of gedit
 *
 * Copyright (C) 2003  Paolo Maggi
 * Copyright (C) 2019  Canonical LTD
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

#ifndef GEDIT_METADATA_MANAGER_H
#define GEDIT_METADATA_MANAGER_H

#include <gio/gio.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_METADATA_MANAGER (gedit_metadata_manager_get_type())

G_DECLARE_FINAL_TYPE (GeditMetadataManager, gedit_metadata_manager, GEDIT, METADATA_MANAGER, GObject)

GeditMetadataManager *gedit_metadata_manager_new	(const gchar *metadata_filename);

gchar		     *gedit_metadata_manager_get	(GeditMetadataManager *self,
							 GFile                *location,
							 const gchar          *key);

void		      gedit_metadata_manager_set	(GeditMetadataManager *self,
							 GFile                *location,
							 const gchar          *key,
							 const gchar          *value);

G_END_DECLS

#endif /* GEDIT_METADATA_MANAGER_H */

/* ex:set ts=8 noet: */
