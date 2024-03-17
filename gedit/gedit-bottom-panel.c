/* SPDX-FileCopyrightText: 2024 - Sébastien Wilmet
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "gedit-bottom-panel.h"

struct _GeditBottomPanelPrivate
{
	gint something;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditBottomPanel, _gedit_bottom_panel, GTK_TYPE_GRID)

static void
_gedit_bottom_panel_dispose (GObject *object)
{

	G_OBJECT_CLASS (_gedit_bottom_panel_parent_class)->dispose (object);
}

static void
_gedit_bottom_panel_class_init (GeditBottomPanelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = _gedit_bottom_panel_dispose;
}

static void
_gedit_bottom_panel_init (GeditBottomPanel *panel)
{
	panel->priv = _gedit_bottom_panel_get_instance_private (panel);
}

GeditBottomPanel *
_gedit_bottom_panel_new (void)
{
	return g_object_new (GEDIT_TYPE_BOTTOM_PANEL, NULL);
}
