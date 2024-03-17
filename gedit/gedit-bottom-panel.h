/* SPDX-FileCopyrightText: 2024 - Sébastien Wilmet
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef GEDIT_BOTTOM_PANEL_H
#define GEDIT_BOTTOM_PANEL_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_BOTTOM_PANEL             (_gedit_bottom_panel_get_type ())
#define GEDIT_BOTTOM_PANEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_BOTTOM_PANEL, GeditBottomPanel))
#define GEDIT_BOTTOM_PANEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_BOTTOM_PANEL, GeditBottomPanelClass))
#define GEDIT_IS_BOTTOM_PANEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_BOTTOM_PANEL))
#define GEDIT_IS_BOTTOM_PANEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_BOTTOM_PANEL))
#define GEDIT_BOTTOM_PANEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_BOTTOM_PANEL, GeditBottomPanelClass))

typedef struct _GeditBottomPanel         GeditBottomPanel;
typedef struct _GeditBottomPanelClass    GeditBottomPanelClass;
typedef struct _GeditBottomPanelPrivate  GeditBottomPanelPrivate;

struct _GeditBottomPanel
{
	GtkGrid parent;

	GeditBottomPanelPrivate *priv;
};

struct _GeditBottomPanelClass
{
	GtkGridClass parent_class;
};

G_GNUC_INTERNAL
GType			_gedit_bottom_panel_get_type	(void);

G_GNUC_INTERNAL
GeditBottomPanel *	_gedit_bottom_panel_new		(void);

G_END_DECLS

#endif /* GEDIT_BOTTOM_PANEL_H */
