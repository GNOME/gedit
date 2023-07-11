/* SPDX-FileCopyrightText: 2023 - Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "config.h"
#include "gedit-side-panel.h"

struct _GeditSidePanelPrivate
{
	TeplStack *stack;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditSidePanel, gedit_side_panel, GTK_TYPE_BIN)

static void
gedit_side_panel_dispose (GObject *object)
{
	GeditSidePanel *panel = GEDIT_SIDE_PANEL (object);

	panel->priv->stack = NULL;

	G_OBJECT_CLASS (gedit_side_panel_parent_class)->dispose (object);
}

static void
gedit_side_panel_class_init (GeditSidePanelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_side_panel_dispose;
}

static void
add_inline_switcher_if_needed (GeditSidePanel *panel,
			       GtkGrid        *vgrid)
{
#if INLINE_SIDE_PANEL_SWITCHER
	TeplStackSwitcherMenu *switcher;

	switcher = tepl_stack_switcher_menu_new (panel->priv->stack);

	gtk_container_add (GTK_CONTAINER (vgrid),
			   GTK_WIDGET (switcher));
#endif
}

static void
gedit_side_panel_init (GeditSidePanel *panel)
{
	GtkGrid *vgrid;

	panel->priv = gedit_side_panel_get_instance_private (panel);

	panel->priv->stack = tepl_stack_new ();

	vgrid = GTK_GRID (gtk_grid_new ());
	gtk_orientable_set_orientation (GTK_ORIENTABLE (vgrid), GTK_ORIENTATION_VERTICAL);

	add_inline_switcher_if_needed (panel, vgrid);

	gtk_container_add (GTK_CONTAINER (vgrid),
			   GTK_WIDGET (panel->priv->stack));

	gtk_widget_show_all (GTK_WIDGET (vgrid));
	gtk_container_add (GTK_CONTAINER (panel), GTK_WIDGET (vgrid));
}

GeditSidePanel *
gedit_side_panel_new (void)
{
	return g_object_new (GEDIT_TYPE_SIDE_PANEL, NULL);
}

TeplStack *
gedit_side_panel_get_stack (GeditSidePanel *panel)
{
	g_return_val_if_fail (GEDIT_IS_SIDE_PANEL (panel), NULL);
	return panel->priv->stack;
}
