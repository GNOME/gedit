/*
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Jesse van den Kieboom
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "gedit-app-win32.h"

#define SAVE_DATADIR DATADIR
#undef DATADIR

#include <io.h>
#include <conio.h>

#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0501
#endif

#include <windows.h>

#define DATADIR SAVE_DATADIR
#undef SAVE_DATADIR

struct _GeditAppWin32
{
	GeditApp parent_instance;
};

G_DEFINE_TYPE (GeditAppWin32, gedit_app_win32, GEDIT_TYPE_APP)

static gchar *
gedit_app_win32_get_help_uri_impl (GeditApp    *app,
                                   const gchar *name_of_user_manual,
                                   const gchar *link_id_within_user_manual)
{
	/* FIXME: name_of_user_manual is expected to be always "gedit" here. */

	if (link_id_within_user_manual != NULL)
	{
		return g_strdup_printf ("https://gedit-technology.github.io/user-manuals/gedit/%s",
		                        link_id_within_user_manual);
	}

	return g_strdup ("https://gedit-technology.github.io/user-manuals/gedit/");
}

static void
setup_path (void)
{
	gchar *path;
	gchar *installdir;
	gchar *bin;

	installdir = g_win32_get_package_installation_directory_of_module (NULL);

	bin = g_build_filename (installdir, "bin", NULL);
	g_free (installdir);

	/* Set PATH to include the gedit executable's folder */
	path = g_build_path (";", bin, g_getenv ("PATH"), NULL);
	g_free (bin);

	if (!g_setenv ("PATH", path, TRUE))
	{
		g_warning ("Could not set PATH for gedit");
	}

	g_free (path);
}

static void
prep_console (void)
{
	/* If we open gedit from a console get the stdout printing */
	if (fileno (stdout) != -1 &&
		_get_osfhandle (fileno (stdout)) != -1)
	{
		/* stdout is fine, presumably redirected to a file or pipe */
	}
	else
	{
		typedef BOOL (* WINAPI AttachConsole_t) (DWORD);

		AttachConsole_t p_AttachConsole =
			(AttachConsole_t) GetProcAddress (GetModuleHandle ("kernel32.dll"),
							  "AttachConsole");

		if (p_AttachConsole != NULL && p_AttachConsole (ATTACH_PARENT_PROCESS))
		{
			freopen ("CONOUT$", "w", stdout);
			dup2 (fileno (stdout), 1);
			freopen ("CONOUT$", "w", stderr);
			dup2 (fileno (stderr), 2);
		}
	}
}

static void
gedit_app_win32_startup (GApplication *application)
{
	G_APPLICATION_CLASS (gedit_app_win32_parent_class)->startup (application);

	setup_path ();
	prep_console ();
}

static void
gedit_app_win32_class_init (GeditAppWin32Class *klass)
{
	GApplicationClass *gapp_class = G_APPLICATION_CLASS (klass);
	GeditAppClass *app_class = GEDIT_APP_CLASS (klass);

	gapp_class->startup = gedit_app_win32_startup;

	app_class->get_help_uri = gedit_app_win32_get_help_uri_impl;
}

static void
gedit_app_win32_init (GeditAppWin32 *self)
{
}

/* ex:set ts=8 noet: */
