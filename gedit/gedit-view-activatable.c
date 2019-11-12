/*
 * gedit-view-activatable.h
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

#include "gedit-view-activatable.h"

#include "gedit-view.h"

/**
 * SECTION:gedit-view-activatable
 * @short_description: Interface for activatable extensions on views
 * @see_also: #PeasExtensionSet
 *
 * #GeditViewActivatable is an interface which should be implemented by
 * extensions that should be activated on a gedit view.
 **/

G_DEFINE_INTERFACE(GeditViewActivatable, gedit_view_activatable, G_TYPE_OBJECT)

static void
gedit_view_activatable_default_init (GeditViewActivatableInterface *iface)
{
	/**
	 * GeditViewActivatable:view:
	 *
	 * The window property contains the gedit window for this
	 * #GeditViewActivatable instance.
	 */
	g_object_interface_install_property (iface,
	                                     g_param_spec_object ("view",
	                                                          "view",
	                                                          "A gedit view",
	                                                          GEDIT_TYPE_VIEW,
	                                                          G_PARAM_READWRITE |
	                                                          G_PARAM_CONSTRUCT_ONLY |
	                                                          G_PARAM_STATIC_STRINGS));
}

/**
 * gedit_view_activatable_activate:
 * @activatable: A #GeditViewActivatable.
 *
 * Activates the extension on the window property.
 */
void
gedit_view_activatable_activate (GeditViewActivatable *activatable)
{
	GeditViewActivatableInterface *iface;

	g_return_if_fail (GEDIT_IS_VIEW_ACTIVATABLE (activatable));

	iface = GEDIT_VIEW_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->activate != NULL)
	{
		iface->activate (activatable);
	}
}

/**
 * gedit_view_activatable_deactivate:
 * @activatable: A #GeditViewActivatable.
 *
 * Deactivates the extension on the window property.
 */
void
gedit_view_activatable_deactivate (GeditViewActivatable *activatable)
{
	GeditViewActivatableInterface *iface;

	g_return_if_fail (GEDIT_IS_VIEW_ACTIVATABLE (activatable));

	iface = GEDIT_VIEW_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->deactivate != NULL)
	{
		iface->deactivate (activatable);
	}
}

