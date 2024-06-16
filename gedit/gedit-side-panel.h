/* SPDX-FileCopyrightText: 2023-2024 - Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef GEDIT_SIDE_PANEL_H
#define GEDIT_SIDE_PANEL_H

#include <tepl/tepl.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_SIDE_PANEL             (_gedit_side_panel_get_type ())
#define GEDIT_SIDE_PANEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_SIDE_PANEL, GeditSidePanel))
#define GEDIT_SIDE_PANEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_SIDE_PANEL, GeditSidePanelClass))
#define GEDIT_IS_SIDE_PANEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_SIDE_PANEL))
#define GEDIT_IS_SIDE_PANEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_SIDE_PANEL))
#define GEDIT_SIDE_PANEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_SIDE_PANEL, GeditSidePanelClass))

typedef struct _GeditSidePanel         GeditSidePanel;
typedef struct _GeditSidePanelClass    GeditSidePanelClass;
typedef struct _GeditSidePanelPrivate  GeditSidePanelPrivate;

struct _GeditSidePanel
{
	GtkBin parent;

	GeditSidePanelPrivate *priv;
};

struct _GeditSidePanelClass
{
	GtkBinClass parent_class;
};

G_GNUC_INTERNAL
GType			_gedit_side_panel_get_type		(void);

G_GNUC_INTERNAL
GeditSidePanel *	_gedit_side_panel_new			(void);

G_GNUC_INTERNAL
TeplPanelSimple *	_gedit_side_panel_get_panel_simple	(GeditSidePanel *panel);

G_GNUC_INTERNAL
gint			_gedit_side_panel_get_width		(GeditSidePanel *panel);

G_GNUC_INTERNAL
void			_gedit_side_panel_set_width		(GeditSidePanel *panel,
								 gint            width);

G_GNUC_INTERNAL
void			_gedit_side_panel_save_state		(GeditSidePanel *panel);

G_GNUC_INTERNAL
void			_gedit_side_panel_copy_settings		(GeditSidePanel *origin,
								 GeditSidePanel *target);

G_END_DECLS

#endif /* GEDIT_SIDE_PANEL_H */
