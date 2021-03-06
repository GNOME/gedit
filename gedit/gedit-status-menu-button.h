/*
 * This file is part of gedit
 *
 * Copyright (C) 2008 - Jesse van den Kieboom
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

#ifndef GEDIT_STATUS_MENU_BUTTON_H
#define GEDIT_STATUS_MENU_BUTTON_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_STATUS_MENU_BUTTON (gedit_status_menu_button_get_type ())

G_DECLARE_FINAL_TYPE (GeditStatusMenuButton, gedit_status_menu_button,
		      GEDIT, STATUS_MENU_BUTTON,
		      GtkMenuButton)

GtkWidget *	gedit_status_menu_button_new		(void);

void		gedit_status_menu_button_set_label	(GeditStatusMenuButton *button,
							 const gchar           *label);

const gchar *	gedit_status_menu_button_get_label	(GeditStatusMenuButton *button);

G_END_DECLS

#endif /* GEDIT_STATUS_MENU_BUTTON_H */

/* ex:set ts=8 noet: */
