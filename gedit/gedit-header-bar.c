/* SPDX-FileCopyrightText: 2023 - Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "gedit-header-bar.h"
#include <glib/gi18n.h>
#include "gedit-commands.h"

struct _GeditHeaderBarPrivate
{
	/* Weak ref */
	GeditWindow *window;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditHeaderBar, _gedit_header_bar, G_TYPE_OBJECT)

static void
_gedit_header_bar_dispose (GObject *object)
{
	GeditHeaderBar *bar = GEDIT_HEADER_BAR (object);

	g_clear_weak_pointer (&bar->priv->window);

	G_OBJECT_CLASS (_gedit_header_bar_parent_class)->dispose (object);
}

static void
_gedit_header_bar_class_init (GeditHeaderBarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = _gedit_header_bar_dispose;
}

static void
_gedit_header_bar_init (GeditHeaderBar *bar)
{
	bar->priv = _gedit_header_bar_get_instance_private (bar);
}

GeditHeaderBar *
_gedit_header_bar_new (GeditWindow *window)
{
	GeditHeaderBar *bar;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	bar = g_object_new (GEDIT_TYPE_HEADER_BAR, NULL);

	g_set_weak_pointer (&bar->priv->window, window);

	return bar;
}

static GtkWidget *
create_open_dialog_button (void)
{
	GtkWidget *button;

	button = gtk_button_new_with_mnemonic (_("_Open"));
	gtk_widget_set_tooltip_text (button, _("Open a file"));
	gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.open");

	return button;
}

static void
open_recent_menu_item_activated_cb (GtkRecentChooser *recent_chooser,
				    gpointer          user_data)
{
	GeditHeaderBar *bar = GEDIT_HEADER_BAR (user_data);
	gchar *uri;
	GFile *location;

	if (bar->priv->window == NULL)
	{
		return;
	}

	uri = gtk_recent_chooser_get_current_uri (recent_chooser);
	location = g_file_new_for_uri (uri);

	gedit_commands_load_location (bar->priv->window, location, NULL, 0, 0);

	g_free (uri);
	g_object_unref (location);
}

static GtkMenuButton *
create_open_recent_menu_button (GeditHeaderBar *bar)
{
	GtkWidget *button;
	GtkRecentChooserMenu *recent_menu;
	AmtkApplicationWindow *amtk_window;

	button = gtk_menu_button_new ();
	gtk_widget_set_tooltip_text (button, _("Open a recently used file"));

	recent_menu = amtk_application_window_create_open_recent_menu_base ();

	amtk_window = amtk_application_window_get_from_gtk_application_window (GTK_APPLICATION_WINDOW (bar->priv->window));
	amtk_application_window_connect_recent_chooser_menu_to_statusbar (amtk_window, recent_menu);

	g_signal_connect_object (recent_menu,
				 "item-activated",
				 G_CALLBACK (open_recent_menu_item_activated_cb),
				 bar,
				 G_CONNECT_DEFAULT);

	gtk_menu_button_set_popup (GTK_MENU_BUTTON (button),
				   GTK_WIDGET (recent_menu));

	return GTK_MENU_BUTTON (button);
}

GtkWidget *
_gedit_header_bar_create_open_buttons (GeditHeaderBar  *bar,
				       GtkMenuButton  **open_recent_button)
{
	GtkWidget *hbox;
	GtkStyleContext *style_context;
	GtkMenuButton *my_open_recent_button;

	g_return_val_if_fail (GEDIT_IS_HEADER_BAR (bar), NULL);
	g_return_val_if_fail (bar->priv->window != NULL, NULL);

	/* It currently needs to be a GtkBox, not a GtkGrid, because GtkGrid and
	 * GTK_STYLE_CLASS_LINKED doesn't work as expected in a RTL locale.
	 * Probably a GtkGrid bug.
	 */
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	style_context = gtk_widget_get_style_context (hbox);
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_LINKED);

	my_open_recent_button = create_open_recent_menu_button (bar);

	gtk_container_add (GTK_CONTAINER (hbox), create_open_dialog_button ());
	gtk_container_add (GTK_CONTAINER (hbox), GTK_WIDGET (my_open_recent_button));
	gtk_widget_show_all (hbox);

	if (open_recent_button != NULL)
	{
		*open_recent_button = my_open_recent_button;
	}

	return hbox;
}
