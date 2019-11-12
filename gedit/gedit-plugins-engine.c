/*
 * gedit-plugins-engine.c
 * This file is part of gedit
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2010 Steve Frécinaux
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include "gedit-plugins-engine.h"

#include <string.h>
#include <glib/gi18n.h>
#include <girepository.h>
#include "gedit-debug.h"
#include "gedit-dirs.h"
#include "gedit-settings.h"

struct _GeditPluginsEngine
{
	PeasEngine parent_instance;

	GSettings *plugin_settings;
};

G_DEFINE_TYPE (GeditPluginsEngine, gedit_plugins_engine, PEAS_TYPE_ENGINE)

static GeditPluginsEngine *default_engine = NULL;

static void
gedit_plugins_engine_init (GeditPluginsEngine *engine)
{
	gchar *typelib_dir;
	GError *error = NULL;

	gedit_debug (DEBUG_PLUGINS);

	peas_engine_enable_loader (PEAS_ENGINE (engine), "python3");

	engine->plugin_settings = g_settings_new ("org.gnome.gedit.plugins");

	/* Require gedit's typelib. */
	typelib_dir = g_build_filename (gedit_dirs_get_gedit_lib_dir (),
	                                "girepository-1.0",
	                                NULL);

	if (!g_irepository_require_private (g_irepository_get_default (),
	                                    typelib_dir, "Gedit", "3.0", 0, &error))
	{
		g_warning ("Could not load Gedit repository: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	g_free (typelib_dir);

	/* This should be moved to libpeas */
	if (!g_irepository_require (g_irepository_get_default (),
	                            "Peas", "1.0", 0, &error))
	{
		g_warning ("Could not load Peas repository: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	if (!g_irepository_require (g_irepository_get_default (),
	                            "PeasGtk", "1.0", 0, &error))
	{
		g_warning ("Could not load PeasGtk repository: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	peas_engine_add_search_path (PEAS_ENGINE (engine),
	                             gedit_dirs_get_user_plugins_dir (),
	                             gedit_dirs_get_user_plugins_dir ());

	peas_engine_add_search_path (PEAS_ENGINE (engine),
	                             gedit_dirs_get_gedit_plugins_dir (),
	                             gedit_dirs_get_gedit_plugins_data_dir ());

	g_settings_bind (engine->plugin_settings,
	                 GEDIT_SETTINGS_ACTIVE_PLUGINS,
	                 engine,
	                 "loaded-plugins",
	                 G_SETTINGS_BIND_DEFAULT);
}

static void
gedit_plugins_engine_dispose (GObject *object)
{
	GeditPluginsEngine *engine = GEDIT_PLUGINS_ENGINE (object);

	g_clear_object (&engine->plugin_settings);

	G_OBJECT_CLASS (gedit_plugins_engine_parent_class)->dispose (object);
}

static void
gedit_plugins_engine_class_init (GeditPluginsEngineClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_plugins_engine_dispose;
}

GeditPluginsEngine *
gedit_plugins_engine_get_default (void)
{
	if (default_engine == NULL)
	{
		default_engine = GEDIT_PLUGINS_ENGINE (g_object_new (GEDIT_TYPE_PLUGINS_ENGINE,
		                                                     NULL));

		g_object_add_weak_pointer (G_OBJECT (default_engine),
		                           (gpointer) &default_engine);
	}

	return default_engine;
}

/* ex:set ts=8 noet: */
