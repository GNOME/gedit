/*
 * gedit-help-commands.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi
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

#include "gedit-commands.h"
#include "gedit-commands-private.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gedit-debug.h"
#include "gedit-app.h"
#include "gedit-dirs.h"

void
_gedit_cmd_help_keyboard_shortcuts (GeditWindow *window)
{
	static GtkWidget *shortcuts_window;

	gedit_debug (DEBUG_COMMANDS);

	if (shortcuts_window == NULL)
	{
		GtkBuilder *builder;

		builder = gtk_builder_new_from_resource ("/org/gnome/gedit/ui/gedit-shortcuts.ui");
		shortcuts_window = GTK_WIDGET (gtk_builder_get_object (builder, "shortcuts-gedit"));

		g_signal_connect (shortcuts_window,
				  "destroy",
				  G_CALLBACK (gtk_widget_destroyed),
				  &shortcuts_window);

		g_object_unref (builder);
	}

	if (GTK_WINDOW (window) != gtk_window_get_transient_for (GTK_WINDOW (shortcuts_window)))
	{
		gtk_window_set_transient_for (GTK_WINDOW (shortcuts_window), GTK_WINDOW (window));
	}

	gtk_widget_show_all (shortcuts_window);
	gtk_window_present (GTK_WINDOW (shortcuts_window));
}

void
_gedit_cmd_help_contents (GeditWindow *window)
{
	gedit_debug (DEBUG_COMMANDS);

	gedit_app_show_help (GEDIT_APP (g_application_get_default ()),
	                     GTK_WINDOW (window),
	                     NULL,
	                     NULL);
}

void
_gedit_cmd_help_about (GeditWindow *window)
{
	static const gchar * const authors[] = {
		"Alex Roberts",
		"Chema Celorio",
		"Evan Lawrence",
		"Federico Mena Quintero <federico@novell.com>",
		"Garrett Regier <garrettregier@gmail.com>",
		"Ignacio Casal Quinteiro <icq@gnome.org>",
		"James Willcox <jwillcox@gnome.org>",
		"Jesse van den Kieboom <jessevdk@gnome.org>",
		"Paolo Borelli <pborelli@gnome.org>",
		"Paolo Maggi <paolo@gnome.org>",
		"Sébastien Lafargue <slafargue@gnome.org>",
		"Sébastien Wilmet <swilmet@gnome.org>",
		"Steve Frécinaux <steve@istique.net>",
		NULL
	};

	static const gchar * const documenters[] = {
		"Jim Campbell <jwcampbell@gmail.com>",
		"Daniel Neel <dneelyep@gmail.com>",
		"Sun GNOME Documentation Team <gdocteam@sun.com>",
		"Eric Baudais <baudais@okstate.edu>",
		NULL
	};

	gedit_debug (DEBUG_COMMANDS);

	gtk_show_about_dialog (GTK_WINDOW (window),
			       "program-name", "gedit",
			       "authors", authors,
			       "comments", _("gedit is a small and lightweight text editor for the GNOME desktop"),
			       "copyright", "Copyright 1998-2020 – the gedit team",
			       "license-type", GTK_LICENSE_GPL_2_0,
			       "logo-icon-name", "org.gnome.gedit",
			       "documenters", documenters,
			       "translator-credits", _("translator-credits"),
			       "version", VERSION,
			       "website", "http://www.gedit.org",
			       "website-label", "www.gedit.org",
			       NULL);
}

/* ex:set ts=8 noet: */
