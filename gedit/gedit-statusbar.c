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
#include "gedit-settings.h"

struct _GeditStatusbar
{
	GtkStatusbar parent_instance;

	/* Weak ref */
	GeditWindow *window;

	/* tmp flash timeout data */
	guint flash_timeout;
	guint flash_context_id;
	guint flash_message_id;

	guint generic_message_context_id;
};

G_DEFINE_TYPE (GeditStatusbar, gedit_statusbar, GTK_TYPE_STATUSBAR)

static void
update_visibility (GeditStatusbar *statusbar)
{
	GeditSettings *settings;
	GSettings *ui_settings;
	gboolean visible;

	if (statusbar->window == NULL)
	{
		return;
	}

	if (_gedit_window_is_fullscreen (statusbar->window))
	{
		gtk_widget_hide (GTK_WIDGET (statusbar));
		return;
	}

	settings = _gedit_settings_get_singleton ();
	ui_settings = _gedit_settings_peek_ui_settings (settings);

	visible = g_settings_get_boolean (ui_settings, GEDIT_SETTINGS_STATUSBAR_VISIBLE);
	gtk_widget_set_visible (GTK_WIDGET (statusbar), visible);
}

static void
statusbar_visible_setting_changed_cb (GSettings   *ui_settings,
				      const gchar *key,
				      gpointer     user_data)
{
	GeditStatusbar *statusbar = GEDIT_STATUSBAR (user_data);
	update_visibility (statusbar);
}

static gboolean
window_state_event_cb (GtkWidget *window,
		       GdkEvent  *event,
		       gpointer   user_data)
{
	GeditStatusbar *statusbar = GEDIT_STATUSBAR (user_data);
	GdkEventWindowState *event_window_state = (GdkEventWindowState *) event;

	if (event_window_state->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
	{
		update_visibility (statusbar);
	}

	return GDK_EVENT_PROPAGATE;
}

static void
gedit_statusbar_dispose (GObject *object)
{
	GeditStatusbar *statusbar = GEDIT_STATUSBAR (object);

	g_clear_weak_pointer (&statusbar->window);

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

/* TODO: remove this function, and take a GeditWindow parameter in new().
 * This requires to create the statusbar in code, not in a *.ui file.
 */
void
_gedit_statusbar_set_window (GeditStatusbar *statusbar,
			     GeditWindow    *window)
{
	GeditSettings *settings;
	GSettings *ui_settings;

	g_return_if_fail (GEDIT_IS_STATUSBAR (statusbar));
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	g_return_if_fail (statusbar->window == NULL);

	g_set_weak_pointer (&statusbar->window, window);

	settings = _gedit_settings_get_singleton ();
	ui_settings = _gedit_settings_peek_ui_settings (settings);

	g_signal_connect_object (ui_settings,
				 "changed::" GEDIT_SETTINGS_STATUSBAR_VISIBLE,
				 G_CALLBACK (statusbar_visible_setting_changed_cb),
				 statusbar,
				 G_CONNECT_DEFAULT);

	g_signal_connect_object (window,
				 "window-state-event",
				 G_CALLBACK (window_state_event_cb),
				 statusbar,
				 G_CONNECT_AFTER);

	update_visibility (statusbar);
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
