/*
 * gedit-open-links-plugin.c
 *
 * Copyright (C) 2020 James Seibel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#include <math.h>

#include <glib/gi18n.h>
#include <gmodule.h>

#include <gedit/gedit-debug.h>
#include <gedit/gedit-tab.h>
#include <gedit/gedit-view.h>
#include <gedit/gedit-window-activatable.h>
#include <gedit/gedit-window.h>

#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "gedit-open-links-plugin.h"

struct _GeditOpenLinksPluginPrivate
{
	GeditWindow	*window;
	GList		*view_handles;
	gulong		tab_added_handle;
	gulong		tab_removed_handle;
	GRegex		*uri_char_regex;
	GString		*uri;
	gdouble		x;
	gdouble		y;
};

typedef struct _GeditViewHandleTuple GeditViewHandleTuple;

struct _GeditViewHandleTuple
{
	GeditView	*view;
	gulong		popup_handle;
	gulong		button_handle;
};

enum
{
	PROP_0,
	PROP_WINDOW
};

static void 	gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);
static gboolean gedit_open_links_plugin_on_button_pressed_cb (GtkWidget			*btn,
							      GdkEventButton		*event,
							      GeditOpenLinksPlugin	*plugin);
static gboolean gedit_open_links_plugin_open_link_cb (GtkWidget			*menu_item,
						      GeditOpenLinksPlugin	*plugin);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditOpenLinksPlugin,
				gedit_open_links_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init)
				G_ADD_PRIVATE_DYNAMIC (GeditOpenLinksPlugin))

/* Returns TRUE if found and new GString allocated in **uri, which must be freed by caller */
static gboolean
gedit_open_links_plugin_get_uri (GtkTextIter *start,
				 GRegex      *uri_char_regex,
				 GString     **uri)
{
	GtkTextIter *end;
	gunichar uri_test[2];
	uri_test[1] = '\0';

	end = g_malloc (sizeof (GtkTextIter));
	memcpy (end, start, sizeof (GtkTextIter));

	/* Move backwards one char to ensure we get the full string */
	gtk_text_iter_backward_char (end);
	while (gtk_text_iter_forward_char (end))
	{
		uri_test[0] = gtk_text_iter_get_char (end);
		if (!g_regex_match (uri_char_regex, (const gchar *) &uri_test, 0, NULL))
		{
			break;
		}
	}
	
	while (gtk_text_iter_backward_char (start))
	{
		uri_test[0] = gtk_text_iter_get_char (start);
		if (!g_regex_match (uri_char_regex, (const gchar *) &uri_test, 0, NULL))
		{
			gtk_text_iter_forward_char (start);
			break;
		}
	}

	*uri = g_string_new (gtk_text_iter_get_text (start, end));

	g_free (end);

	if (!g_str_has_prefix ((*uri)->str, "http:")	&&
	    !g_str_has_prefix ((*uri)->str, "https:")	&&
	    !g_str_has_prefix ((*uri)->str, "www."))
	{
		g_string_free (*uri, TRUE);
		*uri = NULL;

		return FALSE;
	}

	/* We must prepend the scheme or the gtk URL opening function fails */
	if (g_str_has_prefix ((*uri)->str, "www."))
	{
		g_string_prepend (*uri, "https://");
	}
	return TRUE;
}

static void
gedit_open_links_plugin_on_populate_popup_cb (GtkTextView		*view,
					      GtkMenu			*popup,
					      GeditOpenLinksPlugin	*plugin)
{
	GtkMenuShell *menu;
	GtkWidget *menu_item;
	GtkTextIter start;
	gint buffer_x;
	gint buffer_y;
	gboolean uri_success;

	if (!GTK_IS_MENU_SHELL (popup))
	{
		return;
	}

	menu = GTK_MENU_SHELL (popup);

	g_return_if_fail (GEDIT_IS_OPEN_LINKS_PLUGIN (plugin));

	if (plugin->priv->uri != NULL)
	{
		g_string_free (plugin->priv->uri, TRUE);
		plugin->priv->uri = NULL;
	}

	gtk_text_view_window_to_buffer_coords (view,
					       GTK_TEXT_WINDOW_TEXT,
					       (gint) rint (plugin->priv->x),
					       (gint) rint (plugin->priv->y),
					       &buffer_x,
					       &buffer_y);

	gtk_text_view_get_iter_at_location (view, &start, buffer_x, buffer_y);

	uri_success = gedit_open_links_plugin_get_uri (&start,
						       plugin->priv->uri_char_regex,
						       &plugin->priv->uri);

	if (!uri_success)
	{
		return;
	}

	menu_item = gtk_menu_item_new_with_label (_("Open Link"));
	g_signal_connect (menu_item, "activate", G_CALLBACK (gedit_open_links_plugin_open_link_cb), plugin);

	gtk_widget_show (menu_item);
	gtk_menu_shell_prepend (menu, menu_item);
}

static gboolean
gedit_open_links_plugin_open_link_cb (GtkWidget			*menu_item,
				      GeditOpenLinksPlugin	*plugin)
{
	GError *err;
	gboolean success;
	g_return_val_if_fail (GEDIT_IS_OPEN_LINKS_PLUGIN (plugin), TRUE);

	success = gtk_show_uri_on_window (GTK_WINDOW (plugin->priv->window),
					  plugin->priv->uri->str,
					  GDK_CURRENT_TIME,
					  &err);
	if (!success)
	{
		g_warning ("Unable to open URI '%s': %s", plugin->priv->uri->str, err->message);
		g_error_free (err);
	}

	g_string_free (plugin->priv->uri, TRUE);
	plugin->priv->uri = NULL;
	return TRUE;
}

static void
gedit_open_links_plugin_connect_view (GeditOpenLinksPlugin	*plugin,
				      GeditView			*view)
{
	GList *list;
	GeditViewHandleTuple *view_handle_tuple;
	gulong handle_id;
	g_return_if_fail (GEDIT_IS_OPEN_LINKS_PLUGIN (plugin));
	g_return_if_fail (GEDIT_IS_VIEW (view));

	view_handle_tuple = g_new (GeditViewHandleTuple, 1);
	view_handle_tuple->view = view;

	handle_id = g_signal_connect_after (view,
					    "populate-popup",
					    G_CALLBACK (gedit_open_links_plugin_on_populate_popup_cb),
					    plugin);
	view_handle_tuple->popup_handle = handle_id;

	handle_id = g_signal_connect (view,
				      "button-press-event",
				      G_CALLBACK (gedit_open_links_plugin_on_button_pressed_cb),
				      plugin);
	view_handle_tuple->button_handle = handle_id;

	list = plugin->priv->view_handles;
	plugin->priv->view_handles = g_list_prepend (list, view_handle_tuple);
}

static void
gedit_open_links_plugin_on_window_tab_added_cb (GeditWindow		*window,
						GeditTab		*tab,
						GeditOpenLinksPlugin	*plugin)
{
	GeditView *view;
	view = gedit_tab_get_view (tab);
	g_return_if_fail (GEDIT_IS_OPEN_LINKS_PLUGIN (plugin));
	g_return_if_fail (GEDIT_IS_VIEW (view));

	gedit_open_links_plugin_connect_view (plugin, view);
}

static void
gedit_open_links_plugin_on_window_tab_removed_cb (GeditWindow		*window,
						  GeditTab		*tab,
						  GeditOpenLinksPlugin	*plugin)
{
	GeditView *view;
	GeditViewHandleTuple *view_handle_elem;
	GList *list;

	view = gedit_tab_get_view (tab);

	g_return_if_fail (GEDIT_IS_OPEN_LINKS_PLUGIN (plugin));
	g_return_if_fail (GEDIT_IS_VIEW (view));

	/* Disconnect signal and remove from the list */
	GList *l;
	for (l = plugin->priv->view_handles; l != NULL; l = l->next)
	{
		view_handle_elem = (GeditViewHandleTuple*) l->data;
		if (view_handle_elem->view == view)
		{
			g_signal_handler_disconnect (view_handle_elem->view, view_handle_elem->popup_handle);
			g_signal_handler_disconnect (view_handle_elem->view, view_handle_elem->button_handle);
			list = g_list_remove (plugin->priv->view_handles, view_handle_elem);
			plugin->priv->view_handles = list;
			g_free (view_handle_elem);
			break;
		}
	}
}

static gboolean
gedit_open_links_plugin_on_button_pressed_cb (GtkWidget			*btn,
					      GdkEventButton		*event,
					      GeditOpenLinksPlugin	*plugin)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 3)
	{
		if (!GEDIT_IS_OPEN_LINKS_PLUGIN (plugin))
		{
			return FALSE;
		}
		plugin->priv->x = event->x;
		plugin->priv->y = event->y;
	}
	return FALSE;
}

static void
gedit_open_links_plugin_dispose (GObject *object)
{
	G_OBJECT_CLASS (gedit_open_links_plugin_parent_class)->dispose (object);
}

static void
gedit_open_links_plugin_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_open_links_plugin_parent_class)->finalize (object);
}

static void
gedit_open_links_plugin_get_property (GObject		*object,
				      guint		prop_id,
				      GValue		*value,
				      GParamSpec	*pspec)
{
	GeditOpenLinksPlugin *plugin = GEDIT_OPEN_LINKS_PLUGIN (object);

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
gedit_open_links_plugin_set_property (GObject		*object,
				      guint		prop_id,
				      const GValue	*value,
				      GParamSpec	*pspec)
{
	GeditOpenLinksPlugin *plugin = GEDIT_OPEN_LINKS_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_set_object (&plugin->priv->window, g_value_get_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_open_links_plugin_class_init (GeditOpenLinksPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_open_links_plugin_dispose;
	object_class->finalize = gedit_open_links_plugin_finalize;
	object_class->set_property = gedit_open_links_plugin_set_property;
	object_class->get_property = gedit_open_links_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
gedit_open_links_plugin_class_finalize (GeditOpenLinksPluginClass *klass)
{
}

static void
gedit_open_links_plugin_init (GeditOpenLinksPlugin *plugin)
{
	plugin->priv = gedit_open_links_plugin_get_instance_private (plugin);
}

static GRegex*
gedit_open_links_plugin_get_uri_regex ()
{
	GRegex *uri_char_regex;
	GError *err = NULL;

	/* Unescaped: [\w#/\?:%@&\=\+\.\\~-]+ */
	uri_char_regex = g_regex_new ("[\\w#/\\?:%@&\\=\\+\\.\\\\~-]+",
				      G_REGEX_MULTILINE,
				      0,
				      &err);
	if (err != NULL)
	{
		fprintf (stderr, "Regex compilation error: %s\n", err->message);
		g_error_free (err);
	}
	return uri_char_regex;
}

static void
gedit_open_links_plugin_activate (GeditWindowActivatable *activatable)
{
	GeditOpenLinksPlugin *plugin;
	GList *views;
	gulong handle_id;

	gedit_debug (DEBUG_PLUGINS);

	plugin = GEDIT_OPEN_LINKS_PLUGIN (activatable);

	g_return_if_fail (GEDIT_IS_WINDOW (plugin->priv->window));

	plugin->priv->uri_char_regex = gedit_open_links_plugin_get_uri_regex ();
	plugin->priv->uri = NULL;

	handle_id = g_signal_connect (plugin->priv->window,
				      "tab-added",
				      G_CALLBACK (gedit_open_links_plugin_on_window_tab_added_cb),
				      plugin);
	plugin->priv->tab_added_handle = handle_id;

	handle_id = g_signal_connect (plugin->priv->window,
				      "tab-removed",
				      G_CALLBACK (gedit_open_links_plugin_on_window_tab_removed_cb),
				      plugin);
	plugin->priv->tab_removed_handle = handle_id;

	views = gedit_window_get_views (plugin->priv->window);
	GList *l;
	for (l = views; l != NULL; l = l->next)
	{
		gedit_open_links_plugin_connect_view (plugin, l->data);
	}
}

static void
gedit_open_links_plugin_deactivate (GeditWindowActivatable *activatable)
{
	GeditOpenLinksPlugin *plugin;
	GList *view_handles;
	GeditViewHandleTuple *view_handle;

	plugin = GEDIT_OPEN_LINKS_PLUGIN (activatable);

	g_regex_unref (plugin->priv->uri_char_regex);

	if (plugin->priv->uri != NULL)
	{
		g_string_free (plugin->priv->uri, TRUE);
		plugin->priv->uri = NULL;
	}

	if (plugin->priv->window != NULL)
	{
		g_signal_handler_disconnect (plugin->priv->window, plugin->priv->tab_added_handle);
		g_signal_handler_disconnect (plugin->priv->window, plugin->priv->tab_removed_handle);
		g_clear_object (&plugin->priv->window);
	}

	if (plugin->priv->view_handles != NULL)
	{
		/* Disconnect all handles and then free structs */
		view_handles = plugin->priv->view_handles;
		GList *l;
		for (l = view_handles; l != NULL; l = l->next)
		{
			view_handle = (GeditViewHandleTuple*) l->data;
			g_signal_handler_disconnect (view_handle->view, view_handle->popup_handle);
			g_signal_handler_disconnect (view_handle->view, view_handle->button_handle);
		}
		g_list_free_full (plugin->priv->view_handles, g_free);
		plugin->priv->view_handles = NULL;
	}

}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_open_links_plugin_activate;
	iface->deactivate = gedit_open_links_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_open_links_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_OPEN_LINKS_PLUGIN);
}

/* ex:set ts=8 noet: */
