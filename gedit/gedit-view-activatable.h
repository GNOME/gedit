/*
 * gedit-view-activatable.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Steve Frécinaux
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

#ifndef GEDIT_VIEW_ACTIVATABLE_H
#define GEDIT_VIEW_ACTIVATABLE_H

#include <glib-object.h>
#include <gedit/gedit-macros.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_VIEW_ACTIVATABLE (gedit_view_activatable_get_type ())

GEDIT_API
G_DECLARE_INTERFACE (GeditViewActivatable, gedit_view_activatable, GEDIT, VIEW_ACTIVATABLE, GObject)

struct _GeditViewActivatableInterface
{
	GTypeInterface g_iface;

	/* Virtual public methods */
	void	(*activate)		(GeditViewActivatable *activatable);
	void	(*deactivate)		(GeditViewActivatable *activatable);
};

GEDIT_API
void	 gedit_view_activatable_activate	(GeditViewActivatable *activatable);
GEDIT_API
void	 gedit_view_activatable_deactivate	(GeditViewActivatable *activatable);

G_END_DECLS

#endif /* GEDIT_VIEW_ACTIVATABLE_H */
/* ex:set ts=8 noet: */
