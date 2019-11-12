/*
 * gedit-window-activatable.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 Steve Frécinaux
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

#include "config.h"

#include "gedit-window-activatable.h"

#include <string.h>

#include "gedit-window.h"

/**
 * SECTION:gedit-window-activatable
 * @short_description: Interface for activatable extensions on windows
 * @see_also: #PeasExtensionSet
 *
 * #GeditWindowActivatable is an interface which should be implemented by
 * extensions that should be activated on a gedit main window.
 **/

G_DEFINE_INTERFACE(GeditWindowActivatable, gedit_window_activatable, G_TYPE_OBJECT)

static void
gedit_window_activatable_default_init (GeditWindowActivatableInterface *iface)
{
	/**
	 * GeditWindowActivatable:window:
	 *
	 * The window property contains the gedit window for this
	 * #GeditWindowActivatable instance.
	 */
	g_object_interface_install_property (iface,
	                                     g_param_spec_object ("window",
	                                                          "Window",
	                                                          "The gedit window",
	                                                          GEDIT_TYPE_WINDOW,
	                                                          G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
}

/**
 * gedit_window_activatable_activate:
 * @activatable: A #GeditWindowActivatable.
 *
 * Activates the extension on the window property.
 */
void
gedit_window_activatable_activate (GeditWindowActivatable *activatable)
{
	GeditWindowActivatableInterface *iface;

	g_return_if_fail (GEDIT_IS_WINDOW_ACTIVATABLE (activatable));

	iface = GEDIT_WINDOW_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->activate != NULL)
	{
		iface->activate (activatable);
	}
}

/**
 * gedit_window_activatable_deactivate:
 * @activatable: A #GeditWindowActivatable.
 *
 * Deactivates the extension on the window property.
 */
void
gedit_window_activatable_deactivate (GeditWindowActivatable *activatable)
{
	GeditWindowActivatableInterface *iface;

	g_return_if_fail (GEDIT_IS_WINDOW_ACTIVATABLE (activatable));

	iface = GEDIT_WINDOW_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->deactivate != NULL)
	{
		iface->deactivate (activatable);
	}
}

/**
 * gedit_window_activatable_update_state:
 * @activatable: A #GeditWindowActivatable.
 *
 * Triggers an update of the extension internal state to take into account
 * state changes in the window, due to some event or user action.
 */
void
gedit_window_activatable_update_state (GeditWindowActivatable *activatable)
{
	GeditWindowActivatableInterface *iface;

	g_return_if_fail (GEDIT_IS_WINDOW_ACTIVATABLE (activatable));

	iface = GEDIT_WINDOW_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->update_state != NULL)
	{
		iface->update_state (activatable);
	}
}

/* ex:set ts=8 noet: */
