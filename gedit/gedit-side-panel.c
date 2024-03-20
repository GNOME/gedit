/* SPDX-FileCopyrightText: 2023-2024 - Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "config.h"
#include "gedit-side-panel.h"
#include "gedit-settings.h"

struct _GeditSidePanelPrivate
{
	TeplPanelContainer *panel;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditSidePanel, _gedit_side_panel, GTK_TYPE_BIN)

static void
_gedit_side_panel_dispose (GObject *object)
{
	GeditSidePanel *panel = GEDIT_SIDE_PANEL (object);

	panel->priv->panel = NULL;

	G_OBJECT_CLASS (_gedit_side_panel_parent_class)->dispose (object);
}

static void
_gedit_side_panel_class_init (GeditSidePanelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = _gedit_side_panel_dispose;
}

static void
add_inline_switcher_if_needed (GeditSidePanel *panel,
			       GtkGrid        *vgrid)
{
#if !GEDIT_HAS_HEADERBAR
	TeplPanelSwitcherMenu *switcher;

	switcher = tepl_panel_switcher_menu_new (panel->priv->panel);

	gtk_widget_set_halign (GTK_WIDGET (switcher), GTK_ALIGN_CENTER);
	g_object_set (switcher,
		      "margin", 6,
		      NULL);

	gtk_container_add (GTK_CONTAINER (vgrid),
			   GTK_WIDGET (switcher));
#endif
}

static void
_gedit_side_panel_init (GeditSidePanel *panel)
{
	GtkGrid *vgrid;

	panel->priv = _gedit_side_panel_get_instance_private (panel);

	panel->priv->panel = tepl_panel_container_new ();

	vgrid = GTK_GRID (gtk_grid_new ());
	gtk_orientable_set_orientation (GTK_ORIENTABLE (vgrid), GTK_ORIENTATION_VERTICAL);

	add_inline_switcher_if_needed (panel, vgrid);

	gtk_container_add (GTK_CONTAINER (vgrid),
			   GTK_WIDGET (panel->priv->panel));

	gtk_widget_show_all (GTK_WIDGET (vgrid));
	gtk_container_add (GTK_CONTAINER (panel), GTK_WIDGET (vgrid));
}

GeditSidePanel *
_gedit_side_panel_new (void)
{
	return g_object_new (GEDIT_TYPE_SIDE_PANEL, NULL);
}

TeplPanelContainer *
_gedit_side_panel_get_panel_container (GeditSidePanel *panel)
{
	g_return_val_if_fail (GEDIT_IS_SIDE_PANEL (panel), NULL);
	return panel->priv->panel;
}

void
_gedit_side_panel_save_state (GeditSidePanel *panel,
			      gint            width)
{
	GeditSettings *settings;
	GSettings *window_state_settings;
	const gchar *item_name;

	g_return_if_fail (GEDIT_IS_SIDE_PANEL (panel));

	settings = _gedit_settings_get_singleton ();
	window_state_settings = _gedit_settings_peek_window_state_settings (settings);

	item_name = tepl_panel_container_get_active_item_name (panel->priv->panel);
	if (item_name != NULL)
	{
		g_settings_set_string (window_state_settings,
				       GEDIT_SETTINGS_SIDE_PANEL_ACTIVE_PAGE,
				       item_name);
	}

	if (width > 0)
	{
		g_settings_set_int (window_state_settings,
				    GEDIT_SETTINGS_SIDE_PANEL_SIZE,
				    width);
	}
}
