/*
 * gedit-modeline-plugin.c
 * Emacs, Kate and Vim-style modelines support for gedit.
 *
 * Copyright (C) 2005-2010 - Steve Frécinaux <code@istique.net>
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

#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include "gedit-modeline-plugin.h"
#include "modeline-parser.h"

#include <gedit/gedit-debug.h>
#include <gedit/gedit-view-activatable.h>
#include <gedit/gedit-view.h>

struct _GeditModelinePluginPrivate
{
	GeditView *view;

	gulong document_loaded_handler_id;
	gulong document_saved_handler_id;
};

enum
{
	PROP_0,
	PROP_VIEW
};

static void	gedit_view_activatable_iface_init (GeditViewActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditModelinePlugin,
				gedit_modeline_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_VIEW_ACTIVATABLE,
							       gedit_view_activatable_iface_init)
				G_ADD_PRIVATE_DYNAMIC (GeditModelinePlugin))

static void
gedit_modeline_plugin_constructed (GObject *object)
{
	gchar *data_dir;

	data_dir = peas_extension_base_get_data_dir (PEAS_EXTENSION_BASE (object));

	modeline_parser_init (data_dir);

	g_free (data_dir);

	G_OBJECT_CLASS (gedit_modeline_plugin_parent_class)->constructed (object);
}

static void
gedit_modeline_plugin_init (GeditModelinePlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditModelinePlugin initializing");

	plugin->priv = gedit_modeline_plugin_get_instance_private (plugin);

}

static void
gedit_modeline_plugin_dispose (GObject *object)
{
	GeditModelinePlugin *plugin = GEDIT_MODELINE_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS, "GeditModelinePlugin disposing");

	g_clear_object (&plugin->priv->view);

	G_OBJECT_CLASS (gedit_modeline_plugin_parent_class)->dispose (object);
}

static void
gedit_modeline_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditModelinePlugin finalizing");

	modeline_parser_shutdown ();

	G_OBJECT_CLASS (gedit_modeline_plugin_parent_class)->finalize (object);
}

static void
gedit_modeline_plugin_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
	GeditModelinePlugin *plugin = GEDIT_MODELINE_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_VIEW:
			plugin->priv->view = GEDIT_VIEW (g_value_dup_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_modeline_plugin_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
	GeditModelinePlugin *plugin = GEDIT_MODELINE_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_VIEW:
			g_value_set_object (value, plugin->priv->view);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
on_document_loaded_or_saved (GeditDocument *document,
			     GtkSourceView *view)
{
	modeline_parser_apply_modeline (view);
}

static void
gedit_modeline_plugin_activate (GeditViewActivatable *activatable)
{
	GeditModelinePlugin *plugin;
	GtkTextBuffer *doc;

	gedit_debug (DEBUG_PLUGINS);

	plugin = GEDIT_MODELINE_PLUGIN (activatable);

	modeline_parser_apply_modeline (GTK_SOURCE_VIEW (plugin->priv->view));

	doc = gtk_text_view_get_buffer (GTK_TEXT_VIEW (plugin->priv->view));

	plugin->priv->document_loaded_handler_id =
		g_signal_connect (doc, "loaded",
				  G_CALLBACK (on_document_loaded_or_saved),
				  plugin->priv->view);
	plugin->priv->document_saved_handler_id =
		g_signal_connect (doc, "saved",
				  G_CALLBACK (on_document_loaded_or_saved),
				  plugin->priv->view);
}

static void
gedit_modeline_plugin_deactivate (GeditViewActivatable *activatable)
{
	GeditModelinePlugin *plugin;
	GtkTextBuffer *doc;

	gedit_debug (DEBUG_PLUGINS);

	plugin = GEDIT_MODELINE_PLUGIN (activatable);

	doc = gtk_text_view_get_buffer (GTK_TEXT_VIEW (plugin->priv->view));

	g_signal_handler_disconnect (doc, plugin->priv->document_loaded_handler_id);
	g_signal_handler_disconnect (doc, plugin->priv->document_saved_handler_id);
}

static void
gedit_modeline_plugin_class_init (GeditModelinePluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->constructed = gedit_modeline_plugin_constructed;
	object_class->dispose = gedit_modeline_plugin_dispose;
	object_class->finalize = gedit_modeline_plugin_finalize;
	object_class->set_property = gedit_modeline_plugin_set_property;
	object_class->get_property = gedit_modeline_plugin_get_property;

	g_object_class_override_property (object_class, PROP_VIEW, "view");
}

static void
gedit_view_activatable_iface_init (GeditViewActivatableInterface *iface)
{
	iface->activate = gedit_modeline_plugin_activate;
	iface->deactivate = gedit_modeline_plugin_deactivate;
}

static void
gedit_modeline_plugin_class_finalize (GeditModelinePluginClass *klass)
{
}


G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_modeline_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_VIEW_ACTIVATABLE,
						    GEDIT_TYPE_MODELINE_PLUGIN);
}

/* ex:set ts=8 noet: */
