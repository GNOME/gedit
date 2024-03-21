/*
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
#include "gedit-commands-private.h"
#include <glib/gi18n.h>
#include "gedit-app.h"

void
_gedit_cmd_help_contents (GeditWindow *window)
{
	gedit_app_show_help (GEDIT_APP (g_application_get_default ()),
			     GTK_WINDOW (window),
			     NULL,
			     NULL);
}

void
_gedit_cmd_help_about (GeditWindow *window)
{
	const gchar * const authors[] = {
		/* Main authors: the top 5 (to not have a too long list), by
		 * relative contribution (number of commits at the time of
		 * writing).
		 */
		_("Main authors:"),
		"   Sébastien Wilmet",
		"   Paolo Borelli",
		"   Ignacio Casal Quinteiro",
		"   Jesse van den Kieboom",
		"   Paolo Maggi",
		"",
		_("Many thanks also to:"),
		"   Alex Roberts",
		"   Chema Celorio",
		"   Evan Lawrence",
		"   Federico Mena Quintero",
		"   Garrett Regier",
		"   James Willcox",
		"   Sébastien Lafargue",
		"   Steve Frécinaux",
		"",
		_("and many other contributors."),
		"",
		NULL
	};

	static const gchar * const documenters[] = {
		"Daniel Neel",
		"Eric Baudais",
		"Jim Campbell",
		"Sun GNOME Documentation Team",
		NULL
	};

	gtk_show_about_dialog (GTK_WINDOW (window),
			       "authors", authors,
			       "comments", _("gedit is an easy-to-use and general-purpose text editor"),
			       "copyright", "Copyright 1998-2024 – the gedit team",
			       "license-type", GTK_LICENSE_GPL_2_0,
			       "logo-icon-name", "gedit-logo",
			       "documenters", documenters,
			       "translator-credits", _("translator-credits"),
			       "version", VERSION,
			       "website", "https://gedit-technology.github.io/apps/gedit/",
			       NULL);
}

/* ex:set ts=8 noet: */
