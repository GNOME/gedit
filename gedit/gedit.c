/*
 * gedit.c
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

#ifdef G_OS_WIN32
#include <gmodule.h>
static GModule *libgedit_dll = NULL;

/* This code must live in gedit.exe, not in libgedit.dll, since the whole
 * point is to find and load libgedit.dll.
 */
static gboolean
gedit_w32_load_private_dll (void)
{
	gchar *dllpath;
	gchar *prefix;

	prefix = g_win32_get_package_installation_directory_of_module (NULL);

	if (prefix != NULL)
	{
		/* Instead of g_module_open () it may be possible to do any of the
		 * following:
		 * A) Change PATH to "${dllpath}/lib/gedit;$PATH"
		 * B) Call SetDllDirectory ("${dllpath}/lib/gedit")
		 * C) Call AddDllDirectory ("${dllpath}/lib/gedit")
		 * But since we only have one library, and its name is known, may as well
		 * use gmodule.
		 */
		dllpath = g_build_filename (prefix, "lib", "gedit", "lib" PACKAGE_STRING ".dll", NULL);
		g_free (prefix);

		libgedit_dll = g_module_open (dllpath, 0);
		if (libgedit_dll == NULL)
		{
			g_printerr ("Failed to load '%s': %s\n",
			            dllpath, g_module_error ());
		}

		g_free (dllpath);
	}

	if (libgedit_dll == NULL)
	{
		libgedit_dll = g_module_open ("lib" PACKAGE_STRING ".dll", 0);
		if (libgedit_dll == NULL)
		{
			g_printerr ("Failed to load 'lib" PACKAGE_STRING ".dll': %s\n",
			            g_module_error ());
		}
	}

	return (libgedit_dll != NULL);
}

static void
gedit_w32_unload_private_dll (void)
{
	if (libgedit_dll)
	{
		g_module_close (libgedit_dll);
		libgedit_dll = NULL;
	}
}
#endif /* G_OS_WIN32 */

static void
setup_i18n_first_part (void)
{
	/* Disable translations because some underlying modules are not on
	 * l10n.gnome.org or equivalent.
	 * See the docs of g_setenv(), needs to be called very early in main().
	 */
	g_setenv ("LC_ALL", "C.UTF-8", TRUE);
}

static void
setup_i18n_second_part (void)
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

	setup_i18n_first_part ();

#if OS_MACOS
	type = GEDIT_TYPE_APP_OSX;
#elif defined G_OS_WIN32
	if (!gedit_w32_load_private_dll ())
	{
		return 1;
	}

	type = GEDIT_TYPE_APP_WIN32;
#else
	type = GEDIT_TYPE_APP;
#endif

	/* NOTE: we should not make any calls to the gedit API before the
	 * private library is loaded.
	 */
	gedit_dirs_init ();

	setup_i18n_second_part ();
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

#ifdef G_OS_WIN32
	gedit_w32_unload_private_dll ();
#endif

	return status;
}

/* ex:set ts=8 noet: */
