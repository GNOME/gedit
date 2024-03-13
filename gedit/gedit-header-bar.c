/* SPDX-FileCopyrightText: 2023 - Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "gedit-header-bar.h"
#include <glib/gi18n.h>
#include "gedit-app-private.h"
#include "gedit-commands.h"
#include "gedit-window-private.h"

struct _GeditHeaderBarPrivate
{
	/* Weak ref */
	GeditWindow *window;

	/* Part of the GtkHeaderBar */
	GtkMenuButton *open_recent_menu_button;
	GtkMenuButton *hamburger_menu_button;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditHeaderBar, _gedit_header_bar, GTK_TYPE_HEADER_BAR)

static void
_gedit_header_bar_dispose (GObject *object)
{
	GeditHeaderBar *bar = GEDIT_HEADER_BAR (object);

	g_clear_weak_pointer (&bar->priv->window);
	bar->priv->open_recent_menu_button = NULL;
	bar->priv->hamburger_menu_button = NULL;

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

static void
create_open_recent_menu_button (GeditHeaderBar *bar)
{
	GtkRecentChooserMenu *recent_menu;
	AmtkApplicationWindow *amtk_window;

	g_return_if_fail (bar->priv->open_recent_menu_button == NULL);

	bar->priv->open_recent_menu_button = GTK_MENU_BUTTON (gtk_menu_button_new ());
	gtk_widget_set_tooltip_text (GTK_WIDGET (bar->priv->open_recent_menu_button),
				     _("Open a recently used file"));

	recent_menu = amtk_application_window_create_open_recent_menu_base ();

	amtk_window = amtk_application_window_get_from_gtk_application_window (GTK_APPLICATION_WINDOW (bar->priv->window));
	amtk_application_window_connect_recent_chooser_menu_to_statusbar (amtk_window, recent_menu);

	g_signal_connect_object (recent_menu,
				 "item-activated",
				 G_CALLBACK (open_recent_menu_item_activated_cb),
				 bar,
				 G_CONNECT_DEFAULT);

	gtk_menu_button_set_popup (bar->priv->open_recent_menu_button,
				   GTK_WIDGET (recent_menu));
}

static void
add_open_buttons (GeditHeaderBar *bar)
{
	GtkWidget *hbox;
	GtkStyleContext *style_context;

	create_open_recent_menu_button (bar);

	/* It currently needs to be a GtkBox, not a GtkGrid, because GtkGrid and
	 * GTK_STYLE_CLASS_LINKED doesn't work as expected in a RTL locale.
	 * Probably a GtkGrid bug.
	 */
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	style_context = gtk_widget_get_style_context (hbox);
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_LINKED);

	gtk_container_add (GTK_CONTAINER (hbox), create_open_dialog_button ());
	gtk_container_add (GTK_CONTAINER (hbox), GTK_WIDGET (bar->priv->open_recent_menu_button));
	gtk_widget_show_all (hbox);

	gtk_header_bar_pack_start (GTK_HEADER_BAR (bar), hbox);
}

static void
add_new_tab_button (GeditHeaderBar *bar)
{
	GtkWidget *button;

	button = gtk_button_new_from_icon_name ("tab-new-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_tooltip_text (button, _("Create a new document"));
	gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.new-tab");
	gtk_widget_show (button);

	gtk_header_bar_pack_start (GTK_HEADER_BAR (bar), button);
}

static void
add_leave_fullscreen_button (GeditHeaderBar *bar)
{
	GtkWidget *button;

	button = gtk_button_new_from_icon_name ("view-restore-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_tooltip_text (button, _("Leave Fullscreen"));
	gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.leave-fullscreen");
	gtk_widget_show (button);

	gtk_header_bar_pack_end (GTK_HEADER_BAR (bar), button);
}

static void
add_hamburger_menu_button (GeditHeaderBar *bar)
{
	GMenuModel *hamburger_menu;

	g_return_if_fail (bar->priv->hamburger_menu_button == NULL);

	hamburger_menu = _gedit_app_get_hamburger_menu (GEDIT_APP (g_application_get_default ()));

	if (hamburger_menu == NULL)
	{
		return;
	}

	bar->priv->hamburger_menu_button = GTK_MENU_BUTTON (gtk_menu_button_new ());
	gtk_menu_button_set_direction (bar->priv->hamburger_menu_button, GTK_ARROW_NONE);
	gtk_menu_button_set_menu_model (bar->priv->hamburger_menu_button, hamburger_menu);
	gtk_widget_show (GTK_WIDGET (bar->priv->hamburger_menu_button));

	gtk_header_bar_pack_end (GTK_HEADER_BAR (bar),
				 GTK_WIDGET (bar->priv->hamburger_menu_button));
}

static void
add_save_button (GeditHeaderBar *bar)
{
	GtkWidget *button;

	button = gtk_button_new_with_mnemonic (_("_Save"));
	gtk_widget_set_tooltip_text (button, _("Save the current file"));
	gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.save");
	gtk_widget_show (button);

	gtk_header_bar_pack_end (GTK_HEADER_BAR (bar), button);
}

static void
setup_titles (GeditHeaderBar *bar)
{
	GeditWindowTitles *window_titles;

	if (bar->priv->window == NULL)
	{
		return;
	}

	window_titles = _gedit_window_get_window_titles (bar->priv->window);

	g_object_bind_property (window_titles, "title",
				bar, "title",
				G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

	g_object_bind_property (window_titles, "subtitle",
				bar, "subtitle",
				G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
}

GeditHeaderBar *
_gedit_header_bar_new (GeditWindow *window,
		       gboolean     fullscreen)
{
	GeditHeaderBar *bar;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	bar = g_object_new (GEDIT_TYPE_HEADER_BAR, NULL);

	g_set_weak_pointer (&bar->priv->window, window);

	add_open_buttons (bar);
	add_new_tab_button (bar);

	if (fullscreen)
	{
		add_leave_fullscreen_button (bar);
	}

	add_hamburger_menu_button (bar);
	add_save_button (bar);

	setup_titles (bar);

	return bar;
}

GtkMenuButton *
_gedit_header_bar_get_open_recent_menu_button (GeditHeaderBar *bar)
{
	g_return_val_if_fail (GEDIT_IS_HEADER_BAR (bar), NULL);
	return bar->priv->open_recent_menu_button;
}

GtkMenuButton *
_gedit_header_bar_get_hamburger_menu_button (GeditHeaderBar *bar)
{
	g_return_val_if_fail (GEDIT_IS_HEADER_BAR (bar), NULL);
	return bar->priv->hamburger_menu_button;
}
