/*
 * gedit-encodings-combo-box.h
 * This file is part of gedit
 *
 * Copyright (C) 2003-2005 - Paolo Maggi
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

#ifndef GEDIT_ENCODINGS_COMBO_BOX_H
#define GEDIT_ENCODINGS_COMBO_BOX_H

#include <gtksourceview/gtksource.h>
#include <gedit/gedit-macros.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_ENCODINGS_COMBO_BOX (gedit_encodings_combo_box_get_type ())

GEDIT_API
G_DECLARE_FINAL_TYPE (GeditEncodingsComboBox, gedit_encodings_combo_box, GEDIT, ENCODINGS_COMBO_BOX, GtkComboBox)

GEDIT_API
GtkWidget		*gedit_encodings_combo_box_new 				(gboolean save_mode);

GEDIT_API
const GtkSourceEncoding	*gedit_encodings_combo_box_get_selected_encoding	(GeditEncodingsComboBox *menu);

GEDIT_API
void			 gedit_encodings_combo_box_set_selected_encoding	(GeditEncodingsComboBox  *menu,
										 const GtkSourceEncoding *encoding);

G_END_DECLS

#endif /* GEDIT_ENCODINGS_COMBO_BOX_H */

/* ex:set ts=8 noet: */
