/*
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002 - 2005 Paolo Maggi
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

#ifndef GEDIT_UTILS_H
#define GEDIT_UTILS_H

#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

gboolean	gedit_utils_menu_position_under_tree_view		(GtkTreeView  *tree_view,
									 GdkRectangle *rect);

gboolean	gedit_utils_is_valid_location				(GFile *location);

gchar *		gedit_utils_basename_for_display			(GFile *location);

/* Turns data from a drop into a list of well formatted uris */
gchar **	gedit_utils_drop_get_uris				(GtkSelectionData *selection_data);

gchar *		gedit_utils_set_direct_save_filename			(GdkDragContext *context);

GtkSourceCompressionType
		gedit_utils_get_compression_type_from_content_type	(const gchar *content_type);

const gchar *	gedit_utils_newline_type_to_string			(GtkSourceNewlineType newline_type);

G_GNUC_INTERNAL
gchar *		_gedit_utils_location_get_dirname_for_display		(GFile *location);

G_END_DECLS

#endif /* GEDIT_UTILS_H */

/* ex:set ts=8 noet: */
