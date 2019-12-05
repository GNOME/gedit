/*
 * gedit-sort-plugin.c
 *
 * Original author: Carlo Borreo <borreo@softhome.net>
 * Ported to Gedit2 by Lee Mallabone <gnome@fonicmonkey.net>
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
 */

#include "config.h"

#include "gedit-sort-plugin.h"

#include <string.h>
#include <glib/gi18n.h>

#include <gedit/gedit-debug.h>
#include <gedit/gedit-utils.h>
#include <gedit/gedit-app.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-app-activatable.h>
#include <gedit/gedit-window-activatable.h>

static void gedit_app_activatable_iface_init (GeditAppActivatableInterface *iface);
static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);

struct _GeditSortPluginPrivate
{
	GeditWindow *window;

	GSimpleAction *action;
	GtkWidget *dialog;
	GtkWidget *col_num_spinbutton;
	GtkWidget *reverse_order_checkbutton;
	GtkWidget *case_checkbutton;
	GtkWidget *remove_dups_checkbutton;

	GeditApp *app;
	GeditMenuExtension *menu_ext;

	GtkTextIter start, end; /* selection */
};

enum
{
	PROP_0,
	PROP_WINDOW,
	PROP_APP
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditSortPlugin,
				gedit_sort_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_APP_ACTIVATABLE,
							       gedit_app_activatable_iface_init)
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init)
				G_ADD_PRIVATE_DYNAMIC (GeditSortPlugin))

static void
do_sort (GeditSortPlugin *plugin)
{
	GeditSortPluginPrivate *priv;
	GeditDocument *doc;
	GtkSourceSortFlags sort_flags = 0;
	gint starting_column;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	doc = gedit_window_get_active_document (priv->window);
	g_return_if_fail (doc != NULL);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->case_checkbutton)))
	{
		sort_flags |= GTK_SOURCE_SORT_FLAGS_CASE_SENSITIVE;
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->reverse_order_checkbutton)))
	{
		sort_flags |= GTK_SOURCE_SORT_FLAGS_REVERSE_ORDER;
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->remove_dups_checkbutton)))
	{
		sort_flags |= GTK_SOURCE_SORT_FLAGS_REMOVE_DUPLICATES;
	}

	starting_column = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->col_num_spinbutton)) - 1;

	gtk_source_buffer_sort_lines (GTK_SOURCE_BUFFER (doc),
	                              &priv->start,
	                              &priv->end,
	                              sort_flags,
	                              starting_column);

	gedit_debug_message (DEBUG_PLUGINS, "Done.");
}

static void
sort_dialog_response_handler (GtkDialog       *dlg,
			      gint             response,
			      GeditSortPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS);

	if (response == GTK_RESPONSE_OK)
	{
		do_sort (plugin);
	}

	gtk_widget_destroy (GTK_WIDGET (dlg));
}

/* NOTE: we store the current selection in the dialog since focusing
 * the text field (like the combo box) looses the documnent selection.
 * Storing the selection ONLY works because the dialog is modal */
static void
get_current_selection (GeditSortPlugin *plugin)
{
	GeditSortPluginPrivate *priv;
	GeditDocument *doc;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	doc = gedit_window_get_active_document (priv->window);

	if (!gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
						   &priv->start,
						   &priv->end))
	{
		/* No selection, get the whole document. */
		gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc),
					    &priv->start,
					    &priv->end);
	}
}

static void
create_sort_dialog (GeditSortPlugin *plugin)
{
	GeditSortPluginPrivate *priv;
	GtkBuilder *builder;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	builder = gtk_builder_new ();
	gtk_builder_add_from_resource (builder, "/org/gnome/gedit/plugins/sort/ui/gedit-sort-plugin.ui", NULL);
	priv->dialog = GTK_WIDGET (gtk_builder_get_object (builder, "sort_dialog"));
	priv->reverse_order_checkbutton = GTK_WIDGET (gtk_builder_get_object (builder, "reverse_order_checkbutton"));
	priv->col_num_spinbutton = GTK_WIDGET (gtk_builder_get_object (builder, "col_num_spinbutton"));
	priv->case_checkbutton = GTK_WIDGET (gtk_builder_get_object (builder, "case_checkbutton"));
	priv->remove_dups_checkbutton = GTK_WIDGET (gtk_builder_get_object (builder, "remove_dups_checkbutton"));
	g_object_unref (builder);

	gtk_dialog_set_default_response (GTK_DIALOG (priv->dialog),
					 GTK_RESPONSE_OK);

	g_signal_connect (priv->dialog,
			  "destroy",
			  G_CALLBACK (gtk_widget_destroyed),
			  &priv->dialog);

	g_signal_connect (priv->dialog,
			  "response",
			  G_CALLBACK (sort_dialog_response_handler),
			  plugin);

	get_current_selection (plugin);
}

static void
sort_cb (GAction         *action,
         GVariant        *parameter,
	 GeditSortPlugin *plugin)
{
	GeditSortPluginPrivate *priv;
	GtkWindowGroup *wg;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	create_sort_dialog (plugin);

	wg = gedit_window_get_group (priv->window);
	gtk_window_group_add_window (wg,
				     GTK_WINDOW (priv->dialog));

	gtk_window_set_transient_for (GTK_WINDOW (priv->dialog),
				      GTK_WINDOW (priv->window));

	gtk_window_set_modal (GTK_WINDOW (priv->dialog),
			      TRUE);

	gtk_widget_show (GTK_WIDGET (priv->dialog));
}

static void
update_ui (GeditSortPlugin *plugin)
{
	GeditView *view;

	gedit_debug (DEBUG_PLUGINS);

	view = gedit_window_get_active_view (plugin->priv->window);

	g_simple_action_set_enabled (plugin->priv->action,
	                             (view != NULL) &&
	                             gtk_text_view_get_editable (GTK_TEXT_VIEW (view)));
}

static void
gedit_sort_plugin_app_activate (GeditAppActivatable *activatable)
{
	GeditSortPluginPrivate *priv;
	GMenuItem *item;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_SORT_PLUGIN (activatable)->priv;

	priv->menu_ext = gedit_app_activatable_extend_menu (activatable, "tools-section");
	item = g_menu_item_new (_("S_ort…"), "win.sort");
	gedit_menu_extension_append_menu_item (priv->menu_ext, item);
	g_object_unref (item);
}

static void
gedit_sort_plugin_app_deactivate (GeditAppActivatable *activatable)
{
	GeditSortPluginPrivate *priv;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_SORT_PLUGIN (activatable)->priv;

	g_clear_object (&priv->menu_ext);
}

static void
gedit_sort_plugin_window_activate (GeditWindowActivatable *activatable)
{
	GeditSortPluginPrivate *priv;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_SORT_PLUGIN (activatable)->priv;

	priv->action = g_simple_action_new ("sort", NULL);
	g_signal_connect (priv->action, "activate",
	                  G_CALLBACK (sort_cb), activatable);
	g_action_map_add_action (G_ACTION_MAP (priv->window),
	                         G_ACTION (priv->action));

	update_ui (GEDIT_SORT_PLUGIN (activatable));
}

static void
gedit_sort_plugin_window_deactivate (GeditWindowActivatable *activatable)
{
	GeditSortPluginPrivate *priv;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_SORT_PLUGIN (activatable)->priv;
	g_action_map_remove_action (G_ACTION_MAP (priv->window), "sort");
}

static void
gedit_sort_plugin_window_update_state (GeditWindowActivatable *activatable)
{
	gedit_debug (DEBUG_PLUGINS);

	update_ui (GEDIT_SORT_PLUGIN (activatable));
}

static void
gedit_sort_plugin_init (GeditSortPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditSortPlugin initializing");

	plugin->priv = gedit_sort_plugin_get_instance_private (plugin);
}

static void
gedit_sort_plugin_dispose (GObject *object)
{
	GeditSortPlugin *plugin = GEDIT_SORT_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS, "GeditSortPlugin disposing");

	g_clear_object (&plugin->priv->action);
	g_clear_object (&plugin->priv->window);
	g_clear_object (&plugin->priv->menu_ext);
	g_clear_object (&plugin->priv->app);

	G_OBJECT_CLASS (gedit_sort_plugin_parent_class)->dispose (object);
}


static void
gedit_sort_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditSortPlugin finalizing");

	G_OBJECT_CLASS (gedit_sort_plugin_parent_class)->finalize (object);
}

static void
gedit_sort_plugin_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
	GeditSortPlugin *plugin = GEDIT_SORT_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			plugin->priv->window = GEDIT_WINDOW (g_value_dup_object (value));
			break;
		case PROP_APP:
			plugin->priv->app = GEDIT_APP (g_value_dup_object (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_sort_plugin_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
	GeditSortPlugin *plugin = GEDIT_SORT_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_value_set_object (value, plugin->priv->window);
			break;
		case PROP_APP:
			g_value_set_object (value, plugin->priv->app);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_sort_plugin_class_init (GeditSortPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_sort_plugin_dispose;
	object_class->finalize = gedit_sort_plugin_finalize;
	object_class->set_property = gedit_sort_plugin_set_property;
	object_class->get_property = gedit_sort_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
	g_object_class_override_property (object_class, PROP_APP, "app");
}

static void
gedit_sort_plugin_class_finalize (GeditSortPluginClass *klass)
{
}

static void
gedit_app_activatable_iface_init (GeditAppActivatableInterface *iface)
{
	iface->activate = gedit_sort_plugin_app_activate;
	iface->deactivate = gedit_sort_plugin_app_deactivate;
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_sort_plugin_window_activate;
	iface->deactivate = gedit_sort_plugin_window_deactivate;
	iface->update_state = gedit_sort_plugin_window_update_state;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_sort_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_APP_ACTIVATABLE,
						    GEDIT_TYPE_SORT_PLUGIN);
	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_SORT_PLUGIN);
}

/* ex:set ts=8 noet: */
