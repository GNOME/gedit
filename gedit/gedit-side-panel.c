/* SPDX-FileCopyrightText: 2023-2024 - Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "config.h"
#include "gedit-side-panel.h"
#include "gedit-settings.h"

struct _GeditSidePanelPrivate
{
	TeplPanelStack *panel_stack;
	gint width;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditSidePanel, _gedit_side_panel, GTK_TYPE_BIN)

static void
_gedit_side_panel_dispose (GObject *object)
{
	GeditSidePanel *panel = GEDIT_SIDE_PANEL (object);

	g_clear_object (&panel->priv->panel_stack);

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
	TeplPanelSimple *panel_simple;
	TeplPanelSwitcherMenu *switcher;

	panel_simple = tepl_panel_stack_get_panel_simple (panel->priv->panel_stack);
	switcher = tepl_panel_switcher_menu_new (panel_simple);

	gtk_widget_set_halign (GTK_WIDGET (switcher), GTK_ALIGN_CENTER);
	g_object_set (switcher,
		      "margin", 6,
		      NULL);

	gtk_container_add (GTK_CONTAINER (vgrid),
			   GTK_WIDGET (switcher));
#endif
}

static void
init_width (GeditSidePanel *panel)
{
	GeditSettings *settings;
	GSettings *window_state_settings;

	settings = _gedit_settings_get_singleton ();
	window_state_settings = _gedit_settings_peek_window_state_settings (settings);

	panel->priv->width = g_settings_get_int (window_state_settings,
						 GEDIT_SETTINGS_SIDE_PANEL_SIZE);
}

static void
_gedit_side_panel_init (GeditSidePanel *panel)
{
	GtkGrid *vgrid;
	GtkStack *stack;

	panel->priv = _gedit_side_panel_get_instance_private (panel);

	init_width (panel);

	panel->priv->panel_stack = tepl_panel_stack_new (NULL, NULL);

	vgrid = GTK_GRID (gtk_grid_new ());
	gtk_orientable_set_orientation (GTK_ORIENTABLE (vgrid), GTK_ORIENTATION_VERTICAL);

	add_inline_switcher_if_needed (panel, vgrid);

	stack = tepl_panel_stack_get_stack (panel->priv->panel_stack);
	gtk_container_add (GTK_CONTAINER (vgrid), GTK_WIDGET (stack));

	gtk_widget_show_all (GTK_WIDGET (vgrid));
	gtk_container_add (GTK_CONTAINER (panel), GTK_WIDGET (vgrid));
}

GeditSidePanel *
_gedit_side_panel_new (void)
{
	return g_object_new (GEDIT_TYPE_SIDE_PANEL, NULL);
}

TeplPanelSimple *
_gedit_side_panel_get_panel_simple (GeditSidePanel *panel)
{
	g_return_val_if_fail (GEDIT_IS_SIDE_PANEL (panel), NULL);
	return tepl_panel_stack_get_panel_simple (panel->priv->panel_stack);
}

gint
_gedit_side_panel_get_width (GeditSidePanel *panel)
{
	g_return_val_if_fail (GEDIT_IS_SIDE_PANEL (panel), 0);
	return panel->priv->width;
}

void
_gedit_side_panel_set_width (GeditSidePanel *panel,
			     gint            width)
{
	g_return_if_fail (GEDIT_IS_SIDE_PANEL (panel));
	panel->priv->width = width;
}

void
_gedit_side_panel_save_state (GeditSidePanel *panel)
{
	GeditSettings *settings;
	GSettings *window_state_settings;
	TeplPanelSimple *panel_simple;
	const gchar *item_name;

	g_return_if_fail (GEDIT_IS_SIDE_PANEL (panel));

	settings = _gedit_settings_get_singleton ();
	window_state_settings = _gedit_settings_peek_window_state_settings (settings);

	panel_simple = tepl_panel_stack_get_panel_simple (panel->priv->panel_stack);
	item_name = tepl_panel_simple_get_active_item_name (panel_simple);
	if (item_name != NULL)
	{
		g_settings_set_string (window_state_settings,
				       GEDIT_SETTINGS_SIDE_PANEL_ACTIVE_PAGE,
				       item_name);
	}

	if (panel->priv->width > 0)
	{
		g_settings_set_int (window_state_settings,
				    GEDIT_SETTINGS_SIDE_PANEL_SIZE,
				    panel->priv->width);
	}
}

void
_gedit_side_panel_copy_settings (GeditSidePanel *origin,
				 GeditSidePanel *target)
{
	TeplPanelSimple *origin_panel_simple;
	const gchar *active_item_name;

	g_return_if_fail (GEDIT_IS_SIDE_PANEL (origin));
	g_return_if_fail (GEDIT_IS_SIDE_PANEL (target));

	target->priv->width = origin->priv->width;

	origin_panel_simple = tepl_panel_stack_get_panel_simple (origin->priv->panel_stack);
	active_item_name = tepl_panel_simple_get_active_item_name (origin_panel_simple);
	if (active_item_name != NULL)
	{
		TeplPanelSimple *target_panel_simple;

		target_panel_simple = tepl_panel_stack_get_panel_simple (target->priv->panel_stack);
		tepl_panel_simple_set_active_item_name (target_panel_simple, active_item_name);
	}

	gtk_widget_set_visible (GTK_WIDGET (target),
				gtk_widget_get_visible (GTK_WIDGET (origin)));
}
