/*
 * gedit-commands-edit.c
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

#include <gtk/gtk.h>

#include "gedit-window.h"
#include "gedit-debug.h"
#include "gedit-view.h"

void
_gedit_cmd_edit_undo (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);
	GeditView *active_view;
	GtkSourceBuffer *active_document;

	gedit_debug (DEBUG_COMMANDS);

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view != NULL);

	active_document = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (active_view)));

	gtk_source_buffer_undo (active_document);

	tepl_view_scroll_to_cursor (TEPL_VIEW (active_view));

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void
_gedit_cmd_edit_redo (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);
	GeditView *active_view;
	GtkSourceBuffer *active_document;

	gedit_debug (DEBUG_COMMANDS);

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view != NULL);

	active_document = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (active_view)));

	gtk_source_buffer_redo (active_document);

	tepl_view_scroll_to_cursor (TEPL_VIEW (active_view));

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void
_gedit_cmd_edit_cut (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);
	GeditView *active_view;

	gedit_debug (DEBUG_COMMANDS);

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view != NULL);

	tepl_view_cut_clipboard (TEPL_VIEW (active_view));

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void
_gedit_cmd_edit_copy (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);
	GeditView *active_view;

	gedit_debug (DEBUG_COMMANDS);

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view != NULL);

	tepl_view_copy_clipboard (TEPL_VIEW (active_view));

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void
_gedit_cmd_edit_paste (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);
	GeditView *active_view;

	gedit_debug (DEBUG_COMMANDS);

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view != NULL);

	tepl_view_paste_clipboard (TEPL_VIEW (active_view));

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void
_gedit_cmd_edit_delete (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);
	GeditView *active_view;

	gedit_debug (DEBUG_COMMANDS);

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view != NULL);

	tepl_view_delete_selection (TEPL_VIEW (active_view));

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void
_gedit_cmd_edit_select_all (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);
	GeditView *active_view;

	gedit_debug (DEBUG_COMMANDS);

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view != NULL);

	tepl_view_select_all (TEPL_VIEW (active_view));

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void
_gedit_cmd_edit_overwrite_mode (GSimpleAction *action,
                                GVariant      *state,
                                gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);
	GeditView *active_view;
	gboolean overwrite;

	gedit_debug (DEBUG_COMMANDS);

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	overwrite = g_variant_get_boolean (state);
	g_simple_action_set_state (action, state);

	gtk_text_view_set_overwrite (GTK_TEXT_VIEW (active_view), overwrite);
	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}
