/*
 * gedit-app-activatable.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Steve Frécinaux
 * Copyright (C) 2010 - Jesse van den Kieboom
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GEDIT_APP_ACTIVATABLE_H
#define GEDIT_APP_ACTIVATABLE_H

#include <glib-object.h>
#include <gedit/gedit-menu-extension.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_APP_ACTIVATABLE (gedit_app_activatable_get_type ())

GEDIT_API
G_DECLARE_INTERFACE (GeditAppActivatable, gedit_app_activatable, GEDIT, APP_ACTIVATABLE, GObject)

struct _GeditAppActivatableInterface
{
	GTypeInterface g_iface;

	/* Virtual public methods */
	void	(*activate)		(GeditAppActivatable *activatable);
	void	(*deactivate)		(GeditAppActivatable *activatable);
};

GEDIT_API
void	 gedit_app_activatable_activate			(GeditAppActivatable *activatable);
GEDIT_API
void	 gedit_app_activatable_deactivate		(GeditAppActivatable *activatable);

/**
 * gedit_app_activatable_extend_menu:
 * @activatable: A #GeditAppActivatable.
 * @extension_point: the extension point section of the menu to get.
 *
 * Gets the #GeditMenuExtension for the menu @extension_point. Note that
 * the extension point could be in different menus (gear menu, app menu, etc)
 * depending on the platform.
 *
 * Returns: (transfer full): a #GeditMenuExtension for the specific section
 * or %NULL if not found.
 */
GEDIT_API
GeditMenuExtension	*gedit_app_activatable_extend_menu	(GeditAppActivatable *activatable,
								 const gchar *extension_point);

G_END_DECLS

#endif /* GEDIT_APP_ACTIVATABLE_H */
/* ex:set ts=8 noet: */
