/*
 * gedit-view-centering.h
 * This file is part of gedit
 *
 * Copyright (C) 2014 - Sébastien Lafargue
 * Copyright (C) 2015 - Sébastien Wilmet
 *
 * Gedit is free software; you can redistribute this file and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef GEDIT_VIEW_CENTERING_H
#define GEDIT_VIEW_CENTERING_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_VIEW_CENTERING		(gedit_view_centering_get_type())
#define GEDIT_VIEW_CENTERING(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_VIEW_CENTERING, GeditViewCentering))
#define GEDIT_VIEW_CENTERING_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_VIEW_CENTERING, GeditViewCentering const))
#define GEDIT_VIEW_CENTERING_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_VIEW_CENTERING, GeditViewCenteringClass))
#define GEDIT_IS_VIEW_CENTERING(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_VIEW_CENTERING))
#define GEDIT_IS_VIEW_CENTERING_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_VIEW_CENTERING))
#define GEDIT_VIEW_CENTERING_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_VIEW_CENTERING, GeditViewCenteringClass))

typedef struct _GeditViewCentering		GeditViewCentering;
typedef struct _GeditViewCenteringClass		GeditViewCenteringClass;
typedef struct _GeditViewCenteringPrivate	GeditViewCenteringPrivate;

struct _GeditViewCentering
{
	GtkBin parent;

	GeditViewCenteringPrivate *priv;
};

struct _GeditViewCenteringClass
{
	GtkBinClass parent_class;
};

GEDIT_API
GType			gedit_view_centering_get_type			(void) G_GNUC_CONST;

GEDIT_API
GeditViewCentering *	gedit_view_centering_new			(void);

GEDIT_API
void			gedit_view_centering_set_centered		(GeditViewCentering *container,
									 gboolean            centered);

GEDIT_API
gboolean		gedit_view_centering_get_centered		(GeditViewCentering *container);

G_END_DECLS

#endif /* GEDIT_VIEW_CENTERING_H */

/* ex:set ts=8 noet: */
