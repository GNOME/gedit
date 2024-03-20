/* SPDX-FileCopyrightText: 2024 - Sébastien Wilmet
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "gedit-bottom-panel.h"
#include <tepl/tepl.h>
#include <glib/gi18n.h>
#include "gedit-notebook-stack-switcher.h"
#include "gedit-settings.h"

struct _GeditBottomPanelPrivate
{
	GtkStack *stack;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditBottomPanel, _gedit_bottom_panel, GTK_TYPE_GRID)

static void
_gedit_bottom_panel_dispose (GObject *object)
{
	GeditBottomPanel *panel = GEDIT_BOTTOM_PANEL (object);

	g_clear_object (&panel->priv->stack);

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
	GtkWidget *close_button;
	GtkWidget *switcher;

	panel->priv = _gedit_bottom_panel_get_instance_private (panel);

	panel->priv->stack = GTK_STACK (gtk_stack_new ());
	g_object_ref_sink (panel->priv->stack);
	gtk_widget_set_hexpand (GTK_WIDGET (panel->priv->stack), TRUE);
	gtk_widget_set_vexpand (GTK_WIDGET (panel->priv->stack), TRUE);
	gtk_widget_show (GTK_WIDGET (panel->priv->stack));
	gtk_grid_attach (GTK_GRID (panel),
			 GTK_WIDGET (panel->priv->stack),
			 0, 0, 1, 1);

	switcher = gedit_notebook_stack_switcher_new ();
	gtk_widget_set_vexpand (switcher, TRUE);
	gedit_notebook_stack_switcher_set_stack (GEDIT_NOTEBOOK_STACK_SWITCHER (switcher),
						 panel->priv->stack);
	gtk_widget_show (switcher);
	gtk_grid_attach (GTK_GRID (panel),
			 switcher,
			 0, 1, 1, 1);

	close_button = tepl_utils_create_close_button ();
	gtk_widget_set_tooltip_text (close_button, _("Hide panel"));
	gtk_actionable_set_action_name (GTK_ACTIONABLE (close_button), "win.bottom-panel");
	gtk_widget_set_valign (close_button, GTK_ALIGN_START);
	gtk_widget_show (close_button);
	gtk_grid_attach (GTK_GRID (panel),
			 close_button,
			 1, 0, 1, 2);
}

GeditBottomPanel *
_gedit_bottom_panel_new (void)
{
	return g_object_new (GEDIT_TYPE_BOTTOM_PANEL, NULL);
}

GtkStack *
_gedit_bottom_panel_get_stack (GeditBottomPanel *panel)
{
	g_return_val_if_fail (GEDIT_IS_BOTTOM_PANEL (panel), NULL);
	return panel->priv->stack;
}

void
_gedit_bottom_panel_save_state (GeditBottomPanel *panel)
{
	GeditSettings *settings;
	GSettings *window_state_settings;
	const gchar *panel_page;

	g_return_if_fail (GEDIT_IS_BOTTOM_PANEL (panel));

	settings = _gedit_settings_get_singleton ();
	window_state_settings = _gedit_settings_peek_window_state_settings (settings);

	panel_page = gtk_stack_get_visible_child_name (panel->priv->stack);
	if (panel_page != NULL)
	{
		g_settings_set_string (window_state_settings,
				       GEDIT_SETTINGS_BOTTOM_PANEL_ACTIVE_PAGE,
				       panel_page);
	}
}
