/* SPDX-FileCopyrightText: 2023 - Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef GEDIT_HEADER_BAR_H
#define GEDIT_HEADER_BAR_H

#include "gedit-window.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_HEADER_BAR             (_gedit_header_bar_get_type ())
#define GEDIT_HEADER_BAR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_HEADER_BAR, GeditHeaderBar))
#define GEDIT_HEADER_BAR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_HEADER_BAR, GeditHeaderBarClass))
#define GEDIT_IS_HEADER_BAR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_HEADER_BAR))
#define GEDIT_IS_HEADER_BAR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_HEADER_BAR))
#define GEDIT_HEADER_BAR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_HEADER_BAR, GeditHeaderBarClass))

typedef struct _GeditHeaderBar         GeditHeaderBar;
typedef struct _GeditHeaderBarClass    GeditHeaderBarClass;
typedef struct _GeditHeaderBarPrivate  GeditHeaderBarPrivate;

struct _GeditHeaderBar
{
	GtkHeaderBar parent;

	GeditHeaderBarPrivate *priv;
};

struct _GeditHeaderBarClass
{
	GtkHeaderBarClass parent_class;
};

GType			_gedit_header_bar_get_type			(void);

GeditHeaderBar *	_gedit_header_bar_new				(GeditWindow *window,
									 gboolean     fullscreen);

GtkMenuButton *		_gedit_header_bar_get_open_recent_menu_button	(GeditHeaderBar *bar);

GtkMenuButton *		_gedit_header_bar_get_hamburger_menu_button	(GeditHeaderBar *bar);

G_END_DECLS

#endif /* GEDIT_HEADER_BAR_H */
