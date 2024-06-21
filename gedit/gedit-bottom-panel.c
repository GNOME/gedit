/* SPDX-FileCopyrightText: 2024 - Sébastien Wilmet
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "gedit-bottom-panel.h"
#include <glib/gi18n.h>
#include "gedit-settings.h"

struct _GeditBottomPanelPrivate
{
	TeplPanelSimple *panel_simple;
	TeplPanelNotebook *panel_notebook;

	gint height;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditBottomPanel, _gedit_bottom_panel, GTK_TYPE_GRID)

static void
_gedit_bottom_panel_dispose (GObject *object)
{
	GeditBottomPanel *panel = GEDIT_BOTTOM_PANEL (object);

	g_clear_object (&panel->priv->panel_simple);
	g_clear_object (&panel->priv->panel_notebook);

	G_OBJECT_CLASS (_gedit_bottom_panel_parent_class)->dispose (object);
}

static void
_gedit_bottom_panel_class_init (GeditBottomPanelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = _gedit_bottom_panel_dispose;
}

static void
init_height (GeditBottomPanel *panel)
{
	GeditSettings *settings;
	GSettings *window_state_settings;

	settings = _gedit_settings_get_singleton ();
	window_state_settings = _gedit_settings_peek_window_state_settings (settings);

	panel->priv->height = g_settings_get_int (window_state_settings,
						  GEDIT_SETTINGS_BOTTOM_PANEL_SIZE);
}

static GtkNotebook *
create_notebook (void)
{
	GtkNotebook *notebook;

	notebook = GTK_NOTEBOOK (gtk_notebook_new ());
	gtk_notebook_set_tab_pos (notebook, GTK_POS_BOTTOM);
	gtk_notebook_set_scrollable (notebook, TRUE);
	gtk_notebook_set_show_border (notebook, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (notebook), 0);
	gtk_widget_set_hexpand (GTK_WIDGET (notebook), TRUE);
	gtk_widget_set_vexpand (GTK_WIDGET (notebook), TRUE);
	gtk_widget_show (GTK_WIDGET (notebook));

	return notebook;
}

static GtkWidget *
create_close_button (void)
{
	GtkWidget *close_button;

	close_button = tepl_utils_create_close_button ();
	gtk_widget_set_tooltip_text (close_button, _("Hide panel"));
	gtk_actionable_set_action_name (GTK_ACTIONABLE (close_button), "win.bottom-panel");
	gtk_widget_set_valign (close_button, GTK_ALIGN_START);
	gtk_widget_show (close_button);

	return close_button;
}

static void
_gedit_bottom_panel_init (GeditBottomPanel *panel)
{
	GtkNotebook *notebook;

	panel->priv = _gedit_bottom_panel_get_instance_private (panel);

	init_height (panel);

	notebook = create_notebook ();
	gtk_container_add (GTK_CONTAINER (panel),
			   GTK_WIDGET (notebook));
	gtk_container_add (GTK_CONTAINER (panel),
			   create_close_button ());

	panel->priv->panel_simple = tepl_panel_simple_new ();
	panel->priv->panel_notebook = tepl_panel_notebook_new (panel->priv->panel_simple,
							       notebook);
}

GeditBottomPanel *
_gedit_bottom_panel_new (void)
{
	return g_object_new (GEDIT_TYPE_BOTTOM_PANEL, NULL);
}

TeplPanelSimple *
_gedit_bottom_panel_get_panel_simple (GeditBottomPanel *panel)
{
	g_return_val_if_fail (GEDIT_IS_BOTTOM_PANEL (panel), NULL);
	return panel->priv->panel_simple;
}

gint
_gedit_bottom_panel_get_height (GeditBottomPanel *panel)
{
	g_return_val_if_fail (GEDIT_IS_BOTTOM_PANEL (panel), 0);
	return panel->priv->height;
}

void
_gedit_bottom_panel_set_height (GeditBottomPanel *panel,
				gint              height)
{
	g_return_if_fail (GEDIT_IS_BOTTOM_PANEL (panel));
	panel->priv->height = height;
}

void
_gedit_bottom_panel_save_state (GeditBottomPanel *panel)
{
	GeditSettings *settings;
	GSettings *window_state_settings;
	const gchar *active_item_name;

	g_return_if_fail (GEDIT_IS_BOTTOM_PANEL (panel));

	settings = _gedit_settings_get_singleton ();
	window_state_settings = _gedit_settings_peek_window_state_settings (settings);

	active_item_name = tepl_panel_simple_get_active_item_name (panel->priv->panel_simple);
	if (active_item_name != NULL)
	{
		g_settings_set_string (window_state_settings,
				       GEDIT_SETTINGS_BOTTOM_PANEL_ACTIVE_PAGE,
				       active_item_name);
	}

	if (panel->priv->height > 0)
	{
		g_settings_set_int (window_state_settings,
				    GEDIT_SETTINGS_BOTTOM_PANEL_SIZE,
				    panel->priv->height);
	}
}

void
_gedit_bottom_panel_copy_settings (GeditBottomPanel *origin,
				   GeditBottomPanel *target)
{
	const gchar *active_item_name;

	g_return_if_fail (GEDIT_IS_BOTTOM_PANEL (origin));
	g_return_if_fail (GEDIT_IS_BOTTOM_PANEL (target));

	target->priv->height = origin->priv->height;

	active_item_name = tepl_panel_simple_get_active_item_name (origin->priv->panel_simple);
	if (active_item_name != NULL)
	{
		tepl_panel_simple_set_active_item_name (target->priv->panel_simple,
							active_item_name);
	}

	gtk_widget_set_visible (GTK_WIDGET (target),
				gtk_widget_get_visible (GTK_WIDGET (origin)));
}
