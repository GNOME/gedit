/*
 * gedit-statusbar.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Borelli
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

#include "gedit-statusbar.h"
#include <tepl/tepl.h>

struct _GeditStatusbar
{
	GtkStatusbar parent_instance;

	/* tmp flash timeout data */
	guint flash_timeout;
	guint flash_context_id;
	guint flash_message_id;

	guint generic_message_context_id;
};

G_DEFINE_TYPE (GeditStatusbar, gedit_statusbar, GTK_TYPE_STATUSBAR)

static void
gedit_statusbar_dispose (GObject *object)
{
	GeditStatusbar *statusbar = GEDIT_STATUSBAR (object);

	if (statusbar->flash_timeout > 0)
	{
		g_source_remove (statusbar->flash_timeout);
		statusbar->flash_timeout = 0;
	}

	G_OBJECT_CLASS (gedit_statusbar_parent_class)->dispose (object);
}

static void
gedit_statusbar_class_init (GeditStatusbarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_statusbar_dispose;
}

static void
gedit_statusbar_init (GeditStatusbar *statusbar)
{
	statusbar->generic_message_context_id =
		gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), "generic_message");

	tepl_utils_setup_statusbar (GTK_STATUSBAR (statusbar));
}

GeditStatusbar *
_gedit_statusbar_new (void)
{
	return g_object_new (GEDIT_TYPE_STATUSBAR, NULL);
}

static gboolean
remove_message_timeout (GeditStatusbar *statusbar)
{
	gtk_statusbar_remove (GTK_STATUSBAR (statusbar),
	                      statusbar->flash_context_id,
	                      statusbar->flash_message_id);

	/* Remove the timeout. */
	statusbar->flash_timeout = 0;
	return G_SOURCE_REMOVE;
}

static void
flash_text (GeditStatusbar *statusbar,
	    guint           context_id,
	    const gchar    *text)
{
	const guint32 flash_length = 3000; /* Three seconds. */

	/* Remove a currently ongoing flash message. */
	if (statusbar->flash_timeout > 0)
	{
		g_source_remove (statusbar->flash_timeout);
		statusbar->flash_timeout = 0;

		gtk_statusbar_remove (GTK_STATUSBAR (statusbar),
				      statusbar->flash_context_id,
				      statusbar->flash_message_id);
	}

	statusbar->flash_context_id = context_id;
	statusbar->flash_message_id = gtk_statusbar_push (GTK_STATUSBAR (statusbar),
							  context_id,
							  text);

	statusbar->flash_timeout = g_timeout_add (flash_length,
						  (GSourceFunc) remove_message_timeout,
						  statusbar);
}

/* FIXME this is an issue for introspection */
/**
 * gedit_statusbar_flash_message:
 * @statusbar: a #GeditStatusbar
 * @context_id: message context_id
 * @format: message to flash on the statusbar
 * @...: the arguments to insert in @format
 *
 * Flash a temporary message on the statusbar.
 */
void
gedit_statusbar_flash_message (GeditStatusbar *statusbar,
			       guint           context_id,
			       const gchar    *format,
			       ...)
{
	va_list args;
	gchar *text;

	g_return_if_fail (GEDIT_IS_STATUSBAR (statusbar));
	g_return_if_fail (format != NULL);

	va_start (args, format);
	text = g_strdup_vprintf (format, args);
	va_end (args);

	flash_text (statusbar, context_id, text);

	g_free (text);
}

void
_gedit_statusbar_flash_generic_message (GeditStatusbar *statusbar,
					const gchar    *format,
					...)
{
	va_list args;
	gchar *text;

	g_return_if_fail (GEDIT_IS_STATUSBAR (statusbar));
	g_return_if_fail (format != NULL);

	va_start (args, format);
	text = g_strdup_vprintf (format, args);
	va_end (args);

	flash_text (statusbar, statusbar->generic_message_context_id, text);

	g_free (text);
}

/* ex:set ts=8 noet: */
