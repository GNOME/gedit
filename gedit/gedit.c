/*
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
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
#include "gedit-app.h"

#if OS_MACOS
#include "gedit-app-osx.h"
#endif

#ifdef G_OS_WIN32
#include "gedit-app-win32.h"
#endif

#include <locale.h>
#include <libintl.h>
#include <tepl/tepl.h>

#include "gedit-dirs.h"
#include "gedit-debug.h"
#include "gedit-factory.h"
#include "gedit-settings.h"

static void
setup_i18n (void)
{
	const gchar *dir;

	setlocale (LC_ALL, "");

	dir = gedit_dirs_get_gedit_locale_dir ();
	bindtextdomain (GETTEXT_PACKAGE, dir);

	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
}

static void
setup_pango (void)
{
#ifdef G_OS_WIN32
	PangoFontMap *font_map;

	/* Prefer the fontconfig backend of pangocairo. The win32 backend gives
	 * ugly fonts. The fontconfig backend is what is used on Linux.
	 * Another way would be to set the environment variable:
	 * PANGOCAIRO_BACKEND=fontconfig
	 * See also the documentation of pango_cairo_font_map_new().
	 */
	font_map = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);

	if (font_map != NULL)
	{
		pango_cairo_font_map_set_default (PANGO_CAIRO_FONT_MAP (font_map));
		g_object_unref (font_map);
	}
#endif
}

int
main (int argc, char *argv[])
{
	GType type;
	GeditFactory *factory;
	GeditApp *app;
	gint status;

#if OS_MACOS
	type = GEDIT_TYPE_APP_OSX;
#elif defined G_OS_WIN32
	type = GEDIT_TYPE_APP_WIN32;
#else
	type = GEDIT_TYPE_APP;
#endif

	gedit_dirs_init ();
	setup_i18n ();
	setup_pango ();
	tepl_init ();

	factory = gedit_factory_new ();
	tepl_abstract_factory_set_singleton (TEPL_ABSTRACT_FACTORY (factory));

	app = g_object_new (type,
	                    "application-id", "org.gnome.gedit",
	                    "flags", G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_HANDLES_OPEN,
	                    NULL);

	status = g_application_run (G_APPLICATION (app), argc, argv);

	gedit_settings_unref_singleton ();

	/* Break reference cycles caused by the PeasExtensionSet
	 * for GeditAppActivatable which holds a ref on the GeditApp
	 */
	g_object_run_dispose (G_OBJECT (app));

	g_object_add_weak_pointer (G_OBJECT (app), (gpointer *) &app);
	g_object_unref (app);

	if (app != NULL)
	{
		gedit_debug_message (DEBUG_APP, "Leaking with %i refs",
		                     G_OBJECT (app)->ref_count);
	}

	tepl_finalize ();
	gedit_dirs_shutdown ();

	return status;
}

/* ex:set ts=8 noet: */
