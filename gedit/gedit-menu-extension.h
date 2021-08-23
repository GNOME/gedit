/*
 * gedit-menu-extension.h
 * This file is part of gedit
 *
 * Copyright (C) 2014 - Ignacio Casal Quinteiro
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

#ifndef GEDIT_MENU_EXTENSION_H
#define GEDIT_MENU_EXTENSION_H

#include <glib-object.h>
#include <gio/gio.h>
#include <gedit/gedit-macros.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_MENU_EXTENSION (gedit_menu_extension_get_type ())

GEDIT_API
G_DECLARE_FINAL_TYPE (GeditMenuExtension, gedit_menu_extension, GEDIT, MENU_EXTENSION, GObject)

GEDIT_API
GeditMenuExtension       *gedit_menu_extension_new                 (GMenu                *menu);

GEDIT_API
void                      gedit_menu_extension_append_menu_item    (GeditMenuExtension   *menu,
                                                                    GMenuItem            *item);

GEDIT_API
void                      gedit_menu_extension_prepend_menu_item   (GeditMenuExtension   *menu,
                                                                    GMenuItem            *item);

GEDIT_API
void                      gedit_menu_extension_remove_items        (GeditMenuExtension   *menu);

G_END_DECLS

#endif /* GEDIT_MENU_EXTENSION_H */

/* ex:set ts=8 noet: */
