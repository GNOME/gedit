/*
 * gedit-uri-context-menu-plugin.c
 *
 * Copyright (C) 2020 James Seibel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <glib/gi18n.h>

#include <gedit/gedit-debug.h>
#include <gedit/gedit-tab.h>
#include <gedit/gedit-view.h>
#include <gedit/gedit-window-activatable.h>
#include <gedit/gedit-window.h>

#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "gedit-uri-context-menu-plugin.h"

struct _GeditUriContextMenuPluginPrivate
{
	GeditWindow		*window;
	GList			*view_handles;
};

enum
{
	PROP_0,
	PROP_WINDOW
};

static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditUriContextMenuPlugin,
                                gedit_uri_context_menu_plugin,
                                PEAS_TYPE_EXTENSION_BASE,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
                                                               gedit_window_activatable_iface_init)
                                G_ADD_PRIVATE_DYNAMIC (GeditUriContextMenuPlugin))

static void
gedit_uri_context_menu_plugin_on_populate_popup_cb (GtkTextView *view,
		      GtkMenu *popup,
		      GeditUriContextMenuPlugin *plugin)
{
	g_warning ("PLUGIN POPUP OPENED!!!");
	GtkMenuShell *menu;
	GtkWidget *menu_item;

	if (!GTK_IS_MENU_SHELL (popup))
	{
		return;
	}

	menu = GTK_MENU_SHELL (popup);

	//GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	//GtkWidget *icon = gtk_image_new_from_stock (GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_MENU);
	//GtkWidget *label = gtk_label_new ("Open link");
	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_prepend (menu, menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_menu_item_new_with_label ("Open Link");

	//gtk_container_add (GTK_CONTAINER (box), icon);
	//gtk_container_add (GTK_CONTAINER (box), label);

	//gtk_container_add (GTK_CONTAINER (menu_item), box);

	gtk_widget_show (menu_item);

	gtk_menu_shell_prepend (menu, menu_item);

}

static void
gedit_uri_context_menu_plugin_connect_view (GeditUriContextMenuPlugin	*plugin,
					    GeditView			*view)
{
	GList *list;
	gulong handle_id;
	g_return_if_fail (GEDIT_IS_URI_CONTEXT_MENU_PLUGIN (plugin));
	g_return_if_fail (GEDIT_IS_VIEW (view));

	handle_id = g_signal_connect_after (view,
					    "populate-popup",
					    G_CALLBACK (gedit_uri_context_menu_plugin_on_populate_popup_cb),
					    plugin);

	list = plugin->priv->view_handles;
	plugin->priv->view_handles = g_list_prepend (list, handle_id);
}

static void
gedit_uri_context_menu_plugin_on_window_tab_added_cb (GeditWindow		*window,
						      GeditTab			*tab,
						      GeditUriContextMenuPlugin	*plugin)
{
	GeditView *view;
	view = gedit_tab_get_view (tab);
	g_return_if_fail (GEDIT_IS_URI_CONTEXT_MENU_PLUGIN (plugin));
	g_return_if_fail (GEDIT_IS_VIEW (view));
	g_warning ("TAB ADDED");

	gedit_uri_context_menu_plugin_connect_view (plugin, view);
}

static void
gedit_uri_context_menu_plugin_on_window_tab_removed_cb (GeditWindow			*window,
						        GeditTab			*tab,
						        GeditUriContextMenuPlugin	*plugin)
{
	GeditView *view;
	view = gedit_tab_get_view (tab);
	g_return_if_fail (GEDIT_IS_URI_CONTEXT_MENU_PLUGIN (plugin));
	g_return_if_fail (GEDIT_IS_VIEW (view));
	g_warning ("TAB REMOVED");

	gedit_uri_context_menu_plugin_connect_view (plugin, view);
}

static void
gedit_uri_context_menu_plugin_dispose (GObject *object)
{
	GeditUriContextMenuPlugin *plugin = GEDIT_URI_CONTEXT_MENU_PLUGIN (object);

	g_clear_object (&plugin->priv->window);

	if (plugin->priv->view_handles != NULL)
	{
		g_list_free (plugin->priv->view_handles);
	}

	G_OBJECT_CLASS (gedit_uri_context_menu_plugin_parent_class)->dispose (object);
}

static void
gedit_uri_context_menu_plugin_finalize (GObject *object)
{
	GeditUriContextMenuPlugin *plugin = GEDIT_URI_CONTEXT_MENU_PLUGIN (object);

	G_OBJECT_CLASS (gedit_uri_context_menu_plugin_parent_class)->finalize (object);
}

static void
gedit_uri_context_menu_plugin_get_property (GObject    *object,
                                           guint       prop_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
	GeditUriContextMenuPlugin *plugin = GEDIT_URI_CONTEXT_MENU_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_value_set_object (value, plugin->priv->window);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_uri_context_menu_plugin_set_property (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
	GeditUriContextMenuPlugin *plugin = GEDIT_URI_CONTEXT_MENU_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			plugin->priv->window = GEDIT_WINDOW (g_value_dup_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_uri_context_menu_plugin_class_init (GeditUriContextMenuPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_uri_context_menu_plugin_dispose;
	object_class->finalize = gedit_uri_context_menu_plugin_finalize;
	object_class->set_property = gedit_uri_context_menu_plugin_set_property;
	object_class->get_property = gedit_uri_context_menu_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
gedit_uri_context_menu_plugin_class_finalize (GeditUriContextMenuPluginClass *klass)
{
}

static void
gedit_uri_context_menu_plugin_init (GeditUriContextMenuPlugin *plugin)
{
	plugin->priv = gedit_uri_context_menu_plugin_get_instance_private (plugin);
}

static void
gedit_uri_context_menu_plugin_activate (GeditWindowActivatable *activatable)
{
	GeditUriContextMenuPlugin *plugin;
	GList *views;
	GList *view_handles;
	gulong handle_id;

	gedit_debug (DEBUG_PLUGINS);

	g_warning ("URI CONTEXT MENU ACTIVATED XXX");

	plugin = GEDIT_URI_CONTEXT_MENU_PLUGIN (activatable);

	g_return_if_fail (GEDIT_IS_WINDOW (plugin->priv->window));

	view_handles = plugin->priv->view_handles;
	handle_id = g_signal_connect (plugin->priv->window,
				      "tab-added",
				      G_CALLBACK (gedit_uri_context_menu_plugin_on_window_tab_added_cb),
				      plugin);
	plugin->priv->view_handles = g_list_prepend(view_handles, handle_id);

	view_handles = plugin->priv->view_handles;
	handle_id = g_signal_connect (plugin->priv->window,
				      "tab-removed",
				      G_CALLBACK (gedit_uri_context_menu_plugin_on_window_tab_removed_cb),
				      plugin);
	plugin->priv->view_handles = g_list_prepend(view_handles, handle_id);


	views = gedit_window_get_views(plugin->priv->window);
	GList *l;
	for (l = views; l != NULL; l = l->next)
 	{
 		gedit_uri_context_menu_plugin_connect_view(plugin, l->data);
	}
}

static void
gedit_uri_context_menu_plugin_deactivate (GeditWindowActivatable *activatable)
{
	GeditUriContextMenuPlugin *plugin;

	gedit_debug (DEBUG_PLUGINS);

	plugin = GEDIT_URI_CONTEXT_MENU_PLUGIN (activatable);

	if (plugin->priv->view_handles != NULL)
	{
		g_list_free (plugin->priv->view_handles);
	}

	if (plugin->priv->window != NULL)
	{
		g_clear_object (&plugin->priv->window);
	}
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_uri_context_menu_plugin_activate;
	iface->deactivate = gedit_uri_context_menu_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_uri_context_menu_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
	                                            GEDIT_TYPE_WINDOW_ACTIVATABLE,
	                                            GEDIT_TYPE_URI_CONTEXT_MENU_PLUGIN);
}

/* ex:set ts=8 noet: */
