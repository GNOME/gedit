/*
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002 - 2005 Paolo Maggi
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

#ifndef GEDIT_DEBUG_H
#define GEDIT_DEBUG_H

#include <glib.h>

/**
 * GeditDebugSection:
 *
 * Enumeration of debug sections.
 *
 * Debugging output for a section is enabled by setting an environment variable
 * of the same name. For example, setting the <code>GEDIT_DEBUG_PLUGINS</code>
 * environment variable enables all debugging output for the %GEDIT_DEBUG_PLUGINS
 * section. Setting the special environment variable <code>GEDIT_DEBUG</code>
 * enables output for all sections.
 */
typedef enum {
	GEDIT_NO_DEBUG       = 0,
	GEDIT_DEBUG_PREFS    = 1 << 0,
	GEDIT_DEBUG_WINDOW   = 1 << 1,
	GEDIT_DEBUG_PANEL    = 1 << 2,
	GEDIT_DEBUG_PLUGINS  = 1 << 3,
	GEDIT_DEBUG_TAB      = 1 << 4,
	GEDIT_DEBUG_DOCUMENT = 1 << 5,
	GEDIT_DEBUG_COMMANDS = 1 << 6,
	GEDIT_DEBUG_APP      = 1 << 7,
	GEDIT_DEBUG_UTILS    = 1 << 8,
} GeditDebugSection;

#define	DEBUG_PREFS	GEDIT_DEBUG_PREFS,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_WINDOW	GEDIT_DEBUG_WINDOW,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PANEL	GEDIT_DEBUG_PANEL,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PLUGINS	GEDIT_DEBUG_PLUGINS, __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_TAB	GEDIT_DEBUG_TAB,     __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_DOCUMENT	GEDIT_DEBUG_DOCUMENT,__FILE__, __LINE__, G_STRFUNC
#define	DEBUG_COMMANDS	GEDIT_DEBUG_COMMANDS,__FILE__, __LINE__, G_STRFUNC
#define	DEBUG_APP	GEDIT_DEBUG_APP,     __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_UTILS	GEDIT_DEBUG_UTILS,   __FILE__, __LINE__, G_STRFUNC

void gedit_debug_init (void);

void gedit_debug (GeditDebugSection  section,
		  const gchar       *file,
		  gint               line,
		  const gchar       *function);

void gedit_debug_message (GeditDebugSection  section,
			  const gchar       *file,
			  gint               line,
			  const gchar       *function,
			  const gchar       *format, ...) G_GNUC_PRINTF(5, 6);

void gedit_debug_plugin_message (const gchar       *file,
				 gint               line,
				 const gchar       *function,
				 const gchar       *message);

#endif /* GEDIT_DEBUG_H */
/* ex:set ts=8 noet: */
