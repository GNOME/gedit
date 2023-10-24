/*
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
 * Copyright (C) 2013, 2019 - Sébastien Wilmet
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
 * along with gedit. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gedit-view-frame.h"
#include <glib/gi18n.h>
#include "libgd/gd.h"

#define FLUSH_TIMEOUT_DURATION 30 /* in seconds */

#define SEARCH_POPUP_MARGIN 12

typedef enum
{
	SEARCH_MODE_GOTO_LINE,
	/* "Simple search", in contrast to "search and replace". */
	SEARCH_MODE_SIMPLE_SEARCH,
} SearchMode;

typedef enum
{
	SEARCH_STATE_NORMAL,
	SEARCH_STATE_NOT_FOUND
} SearchState;

struct _GeditViewFrame
{
	GtkOverlay parent_instance;

	GeditView *view;

	SearchMode search_mode;

	/* Where the search has started. When the user presses escape in the
	 * search entry (to cancel the search), we return to the start_mark.
	 */
	GtkTextMark *start_mark;

	GtkRevealer *revealer;
	GdTaggedEntry *search_entry;
	GdTaggedEntryTag *entry_tag;
	GtkWidget *go_up_button;
	GtkWidget *go_down_button;

	guint flush_timeout_id;
	guint idle_update_entry_tag_id;
	guint remove_entry_tag_timeout_id;
	gulong view_scroll_event_id;
	gulong search_entry_focus_out_id;
	gulong search_entry_changed_id;

	GtkSourceSearchSettings *search_settings;

	/* Used to restore the search state if an incremental search is
	 * cancelled.
	 */
	GtkSourceSearchSettings *old_search_settings;

	/* The original search texts. In search_settings and
	 * old_search_settings, the search text is unescaped. Since the escape
	 * function is not reciprocal, we need to store the original search
	 * texts.
	 */
	gchar *search_text;
	gchar *old_search_text;
};

G_DEFINE_TYPE (GeditViewFrame, gedit_view_frame, GTK_TYPE_OVERLAY)

static GeditDocument *
get_document (GeditViewFrame *frame)
{
	return GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->view)));
}

static void
get_iter_at_start_mark (GeditViewFrame *frame,
			GtkTextIter    *iter)
{
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->view));

	if (frame->start_mark != NULL)
	{
		gtk_text_buffer_get_iter_at_mark (buffer, iter, frame->start_mark);
	}
	else
	{
		g_warn_if_reached ();
		gtk_text_buffer_get_start_iter (buffer, iter);
	}
}

static void
gedit_view_frame_dispose (GObject *object)
{
	GeditViewFrame *frame = GEDIT_VIEW_FRAME (object);
	GtkTextBuffer *buffer = NULL;

	if (frame->view != NULL)
	{
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->view));
	}

	if (frame->start_mark != NULL && buffer != NULL)
	{
		gtk_text_buffer_delete_mark (buffer, frame->start_mark);
		frame->start_mark = NULL;
	}

	if (frame->flush_timeout_id != 0)
	{
		g_source_remove (frame->flush_timeout_id);
		frame->flush_timeout_id = 0;
	}

	if (frame->idle_update_entry_tag_id != 0)
	{
		g_source_remove (frame->idle_update_entry_tag_id);
		frame->idle_update_entry_tag_id = 0;
	}

	if (frame->remove_entry_tag_timeout_id != 0)
	{
		g_source_remove (frame->remove_entry_tag_timeout_id);
		frame->remove_entry_tag_timeout_id = 0;
	}

	if (buffer != NULL)
	{
		GtkSourceFile *file = gedit_document_get_file (GEDIT_DOCUMENT (buffer));
		gtk_source_file_set_mount_operation_factory (file, NULL, NULL, NULL);
	}

	g_clear_object (&frame->entry_tag);
	g_clear_object (&frame->search_settings);
	g_clear_object (&frame->old_search_settings);

	G_OBJECT_CLASS (gedit_view_frame_parent_class)->dispose (object);
}

static void
gedit_view_frame_finalize (GObject *object)
{
	GeditViewFrame *frame = GEDIT_VIEW_FRAME (object);

	g_free (frame->search_text);
	g_free (frame->old_search_text);

	G_OBJECT_CLASS (gedit_view_frame_parent_class)->finalize (object);
}

static void
hide_search_widget (GeditViewFrame *frame,
                    gboolean        cancel)
{
	GtkTextBuffer *buffer;

	if (!gtk_revealer_get_reveal_child (frame->revealer))
	{
		return;
	}

	if (frame->view_scroll_event_id != 0)
	{
		g_signal_handler_disconnect (frame->view,
		                             frame->view_scroll_event_id);
		frame->view_scroll_event_id = 0;
	}

	if (frame->flush_timeout_id != 0)
	{
		g_source_remove (frame->flush_timeout_id);
		frame->flush_timeout_id = 0;
	}

	gtk_revealer_set_reveal_child (frame->revealer, FALSE);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->view));

	if (cancel && frame->start_mark != NULL)
	{
		GtkTextIter iter;

		gtk_text_buffer_get_iter_at_mark (buffer, &iter,
		                                  frame->start_mark);
		gtk_text_buffer_place_cursor (buffer, &iter);

		tepl_view_scroll_to_cursor (TEPL_VIEW (frame->view));
	}

	if (frame->start_mark != NULL)
	{
		gtk_text_buffer_delete_mark (buffer, frame->start_mark);
		frame->start_mark = NULL;
	}
}

static gboolean
search_entry_flush_timeout (GeditViewFrame *frame)
{
	frame->flush_timeout_id = 0;
	hide_search_widget (frame, FALSE);

	return G_SOURCE_REMOVE;
}

static void
renew_flush_timeout (GeditViewFrame *frame)
{
	if (frame->flush_timeout_id != 0)
	{
		g_source_remove (frame->flush_timeout_id);
	}

	frame->flush_timeout_id =
		g_timeout_add_seconds (FLUSH_TIMEOUT_DURATION,
				       (GSourceFunc)search_entry_flush_timeout,
				       frame);
}

static GtkSourceSearchContext *
get_search_context (GeditViewFrame *frame)
{
	GeditDocument *doc;
	GtkSourceSearchContext *search_context;
	GtkSourceSearchSettings *search_settings;

	doc = get_document (frame);
	search_context = gedit_document_get_search_context (doc);

	if (search_context == NULL)
	{
		return NULL;
	}

	search_settings = gtk_source_search_context_get_settings (search_context);

	if (search_settings == frame->search_settings)
	{
		return search_context;
	}

	return NULL;
}

static void
set_search_state (GeditViewFrame *frame,
		  SearchState     state)
{
	GtkStyleContext *context;

	context = gtk_widget_get_style_context (GTK_WIDGET (frame->search_entry));

	if (state == SEARCH_STATE_NOT_FOUND)
	{
		gtk_style_context_add_class (context, GTK_STYLE_CLASS_ERROR);
	}
	else
	{
		gtk_style_context_remove_class (context, GTK_STYLE_CLASS_ERROR);
	}
}

static void
finish_search (GeditViewFrame    *frame,
	       gboolean           found)
{
	const gchar *entry_text = gtk_entry_get_text (GTK_ENTRY (frame->search_entry));

	if (found || (entry_text[0] == '\0'))
	{
		tepl_view_scroll_to_cursor (TEPL_VIEW (frame->view));

		set_search_state (frame, SEARCH_STATE_NORMAL);
	}
	else
	{
		set_search_state (frame, SEARCH_STATE_NOT_FOUND);
	}
}

static void
start_search_finished (GtkSourceSearchContext *search_context,
		       GAsyncResult           *result,
		       GeditViewFrame         *frame)
{
	GtkTextIter match_start;
	GtkTextIter match_end;
	gboolean found;
	GtkSourceBuffer *buffer;

	found = gtk_source_search_context_forward_finish (search_context,
							  result,
							  &match_start,
							  &match_end,
							  NULL,
							  NULL);

	buffer = gtk_source_search_context_get_buffer (search_context);

	if (found)
	{
		gtk_text_buffer_select_range (GTK_TEXT_BUFFER (buffer),
					      &match_start,
					      &match_end);
	}
	else if (frame->start_mark != NULL)
	{
		GtkTextIter start_at;

		gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (buffer),
						  &start_at,
						  frame->start_mark);

		gtk_text_buffer_select_range (GTK_TEXT_BUFFER (buffer),
					      &start_at,
					      &start_at);
	}

	finish_search (frame, found);
}

static void
start_search (GeditViewFrame *frame)
{
	GtkSourceSearchContext *search_context;
	GtkTextIter start_at;

	g_return_if_fail (frame->search_mode == SEARCH_MODE_SIMPLE_SEARCH);

	search_context = get_search_context (frame);

	if (search_context == NULL)
	{
		return;
	}

	get_iter_at_start_mark (frame, &start_at);

	gtk_source_search_context_forward_async (search_context,
						 &start_at,
						 NULL,
						 (GAsyncReadyCallback)start_search_finished,
						 frame);
}

static void
forward_search_finished (GtkSourceSearchContext *search_context,
			 GAsyncResult           *result,
			 GeditViewFrame         *frame)
{
	GtkTextIter match_start;
	GtkTextIter match_end;
	gboolean found;

	found = gtk_source_search_context_forward_finish (search_context,
							  result,
							  &match_start,
							  &match_end,
							  NULL,
							  NULL);

	if (found)
	{
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->view));

		gtk_text_buffer_select_range (buffer,
					      &match_start,
					      &match_end);
	}

	finish_search (frame, found);
}

static void
forward_search (GeditViewFrame *frame)
{
	GtkTextIter start_at;
	GtkTextBuffer *buffer;
	GtkSourceSearchContext *search_context;

	g_return_if_fail (frame->search_mode == SEARCH_MODE_SIMPLE_SEARCH);

	search_context = get_search_context (frame);

	if (search_context == NULL)
	{
		return;
	}

	renew_flush_timeout (frame);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->view));

	gtk_text_buffer_get_selection_bounds (buffer, NULL, &start_at);

	gtk_source_search_context_forward_async (search_context,
						 &start_at,
						 NULL,
						 (GAsyncReadyCallback)forward_search_finished,
						 frame);
}

static void
backward_search_finished (GtkSourceSearchContext *search_context,
			  GAsyncResult           *result,
			  GeditViewFrame         *frame)
{
	GtkTextIter match_start;
	GtkTextIter match_end;
	gboolean found;
	GtkSourceBuffer *buffer;

	found = gtk_source_search_context_backward_finish (search_context,
							   result,
							   &match_start,
							   &match_end,
							   NULL,
							   NULL);

	buffer = gtk_source_search_context_get_buffer (search_context);

	if (found)
	{
		gtk_text_buffer_select_range (GTK_TEXT_BUFFER (buffer),
					      &match_start,
					      &match_end);
	}

	finish_search (frame, found);
}

static void
backward_search (GeditViewFrame *frame)
{
	GtkTextIter start_at;
	GtkTextBuffer *buffer;
	GtkSourceSearchContext *search_context;

	g_return_if_fail (frame->search_mode == SEARCH_MODE_SIMPLE_SEARCH);

	search_context = get_search_context (frame);

	if (search_context == NULL)
	{
		return;
	}

	renew_flush_timeout (frame);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->view));

	gtk_text_buffer_get_selection_bounds (buffer, &start_at, NULL);

	gtk_source_search_context_backward_async (search_context,
						  &start_at,
						  NULL,
						  (GAsyncReadyCallback)backward_search_finished,
						  frame);
}

static gboolean
search_widget_scroll_event (GtkWidget      *widget,
                            GdkEventScroll *event,
                            GeditViewFrame *frame)
{
	if (frame->search_mode == SEARCH_MODE_GOTO_LINE)
	{
		return GDK_EVENT_PROPAGATE;
	}

	if (event->state & GDK_CONTROL_MASK)
	{
		if (event->direction == GDK_SCROLL_UP)
		{
			backward_search (frame);
			return GDK_EVENT_STOP;
		}
		else if (event->direction == GDK_SCROLL_DOWN)
		{
			forward_search (frame);
			return GDK_EVENT_STOP;
		}
	}

	return GDK_EVENT_PROPAGATE;
}

static GtkSourceSearchSettings *
copy_search_settings (GtkSourceSearchSettings *settings)
{
	GtkSourceSearchSettings *new_settings = gtk_source_search_settings_new ();
	gboolean val;
	const gchar *text;

	if (settings == NULL)
	{
		return new_settings;
	}

	val = gtk_source_search_settings_get_case_sensitive (settings);
	gtk_source_search_settings_set_case_sensitive (new_settings, val);

	val = gtk_source_search_settings_get_wrap_around (settings);
	gtk_source_search_settings_set_wrap_around (new_settings, val);

	val = gtk_source_search_settings_get_at_word_boundaries (settings);
	gtk_source_search_settings_set_at_word_boundaries (new_settings, val);

	val = gtk_source_search_settings_get_regex_enabled (settings);
	gtk_source_search_settings_set_regex_enabled (new_settings, val);

	text = gtk_source_search_settings_get_search_text (settings);
	gtk_source_search_settings_set_search_text (new_settings, text);

	return new_settings;
}

static gboolean
search_widget_key_press_event (GtkWidget      *widget,
                               GdkEventKey    *event,
                               GeditViewFrame *frame)
{
	/* Close window */
	if (event->keyval == GDK_KEY_Tab)
	{
		hide_search_widget (frame, FALSE);
		gtk_widget_grab_focus (GTK_WIDGET (frame->view));

		return GDK_EVENT_STOP;
	}

	if (frame->search_mode == SEARCH_MODE_GOTO_LINE)
	{
		return GDK_EVENT_PROPAGATE;
	}

	/* SEARCH_MODE_SIMPLE_SEARCH */

	/* select previous matching iter */
	if (event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up)
	{
		backward_search (frame);
		return GDK_EVENT_STOP;
	}

	/* select next matching iter */
	if (event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down)
	{
		forward_search (frame);
		return GDK_EVENT_STOP;
	}

	return GDK_EVENT_PROPAGATE;
}

static gboolean
remove_entry_tag_timeout_cb (GeditViewFrame *frame)
{
	frame->remove_entry_tag_timeout_id = 0;

	gd_tagged_entry_remove_tag (frame->search_entry,
				    frame->entry_tag);

	return G_SOURCE_REMOVE;
}

static void
update_entry_tag (GeditViewFrame *frame)
{
	GtkSourceSearchContext *search_context;
	GtkTextBuffer *buffer;
	GtkTextIter select_start;
	GtkTextIter select_end;
	gint count;
	gint pos;
	gchar *label;

	if (frame->search_mode == SEARCH_MODE_GOTO_LINE)
	{
		gd_tagged_entry_remove_tag (frame->search_entry,
					    frame->entry_tag);
		return;
	}

	search_context = get_search_context (frame);

	if (search_context == NULL)
	{
		return;
	}

	count = gtk_source_search_context_get_occurrences_count (search_context);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->view));
	gtk_text_buffer_get_selection_bounds (buffer, &select_start, &select_end);

	pos = gtk_source_search_context_get_occurrence_position (search_context,
								 &select_start,
								 &select_end);

	if (count == -1 || pos == -1)
	{
		/* The buffer is not fully scanned. Remove the tag after a short
		 * delay. If we don't remove the tag at all, the information can
		 * be outdated during a too long time (for big buffers). And if
		 * the tag is removed directly, there is some flashing for small
		 * buffers: the tag disappears and reappear after a really short
		 * time.
		 */

		if (frame->remove_entry_tag_timeout_id == 0)
		{
			frame->remove_entry_tag_timeout_id =
				g_timeout_add (500,
					       (GSourceFunc)remove_entry_tag_timeout_cb,
					       frame);
		}

		return;
	}

	if (count == 0 || pos == 0)
	{
		gd_tagged_entry_remove_tag (frame->search_entry,
					    frame->entry_tag);
		return;
	}

	if (frame->remove_entry_tag_timeout_id != 0)
	{
		g_source_remove (frame->remove_entry_tag_timeout_id);
		frame->remove_entry_tag_timeout_id = 0;
	}

	/* Translators: the first %d is the position of the current search
	 * occurrence, and the second %d is the total number of search
	 * occurrences.
	 */
	label = g_strdup_printf (_("%d of %d"), pos, count);

	gd_tagged_entry_tag_set_label (frame->entry_tag, label);

	gd_tagged_entry_add_tag (frame->search_entry,
				 frame->entry_tag);

	g_free (label);
}

static gboolean
update_entry_tag_idle_cb (GeditViewFrame *frame)
{
	frame->idle_update_entry_tag_id = 0;

	update_entry_tag (frame);

	return G_SOURCE_REMOVE;
}

static void
install_update_entry_tag_idle (GeditViewFrame *frame)
{
	if (frame->idle_update_entry_tag_id == 0)
	{
		frame->idle_update_entry_tag_id = g_idle_add ((GSourceFunc)update_entry_tag_idle_cb,
		                                              frame);
	}
}

static void
update_search_text (GeditViewFrame *frame)
{
	const gchar *entry_text = gtk_entry_get_text (GTK_ENTRY (frame->search_entry));

	g_free (frame->search_text);
	frame->search_text = g_strdup (entry_text);

	if (gtk_source_search_settings_get_regex_enabled (frame->search_settings))
	{
		gtk_source_search_settings_set_search_text (frame->search_settings,
							    entry_text);
	}
	else
	{
		gchar *unescaped_entry_text = gtk_source_utils_unescape_search_text (entry_text);

		gtk_source_search_settings_set_search_text (frame->search_settings,
							    unescaped_entry_text);

		g_free (unescaped_entry_text);
	}
}

static void
regex_toggled_cb (GtkCheckMenuItem *menu_item,
		  GeditViewFrame   *frame)
{
	gtk_source_search_settings_set_regex_enabled (frame->search_settings,
						      gtk_check_menu_item_get_active (menu_item));

	start_search (frame);
}

static void
at_word_boundaries_toggled_cb (GtkCheckMenuItem *menu_item,
			       GeditViewFrame   *frame)
{
	gtk_source_search_settings_set_at_word_boundaries (frame->search_settings,
							   gtk_check_menu_item_get_active (menu_item));

	start_search (frame);
}

static void
case_sensitive_toggled_cb (GtkCheckMenuItem *menu_item,
			   GeditViewFrame   *frame)
{
	gtk_source_search_settings_set_case_sensitive (frame->search_settings,
						       gtk_check_menu_item_get_active (menu_item));

	start_search (frame);
}

static void
add_popup_menu_items (GeditViewFrame *frame,
		      GtkWidget      *menu)
{
	GtkWidget *menu_item;
	gboolean val;

	/* create "Wrap Around" menu item. */
	menu_item = gtk_check_menu_item_new_with_mnemonic (_("_Wrap Around"));

	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	g_object_bind_property (frame->search_settings, "wrap-around",
				menu_item, "active",
				G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	/* create "Match as Regular Expression" menu item. */
	menu_item = gtk_check_menu_item_new_with_mnemonic (_("Match as _Regular Expression"));

	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	val = gtk_source_search_settings_get_regex_enabled (frame->search_settings);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), val);

	g_signal_connect (menu_item,
			  "toggled",
			  G_CALLBACK (regex_toggled_cb),
			  frame);

	/* create "Match Entire Word Only" menu item. */
	menu_item = gtk_check_menu_item_new_with_mnemonic (_("Match _Entire Word Only"));

	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	val = gtk_source_search_settings_get_at_word_boundaries (frame->search_settings);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), val);

	g_signal_connect (menu_item,
			  "toggled",
			  G_CALLBACK (at_word_boundaries_toggled_cb),
			  frame);

	/* create "Match Case" menu item. */
	menu_item = gtk_check_menu_item_new_with_mnemonic (_("_Match Case"));

	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	val = gtk_source_search_settings_get_case_sensitive (frame->search_settings);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), val);

	g_signal_connect (menu_item,
			  "toggled",
			  G_CALLBACK (case_sensitive_toggled_cb),
			  frame);
}

static void
popup_menu_hide_cb (GeditViewFrame *frame)
{
	renew_flush_timeout (frame);

	g_signal_handler_unblock (frame->search_entry,
				  frame->search_entry_focus_out_id);
}

static void
setup_popup_menu (GeditViewFrame *frame,
		  GtkWidget      *menu)
{
	if (frame->flush_timeout_id != 0)
	{
		g_source_remove (frame->flush_timeout_id);
		frame->flush_timeout_id = 0;
	}

	g_signal_handler_block (frame->search_entry,
				frame->search_entry_focus_out_id);

	g_signal_connect_swapped (menu,
				  "hide",
				  G_CALLBACK (popup_menu_hide_cb),
				  frame);
}

static void
search_entry_escaped (GtkSearchEntry *entry,
                      GeditViewFrame *frame)
{
	GtkSourceSearchContext *search_context = get_search_context (frame);

	if (frame->search_mode == SEARCH_MODE_SIMPLE_SEARCH &&
	    search_context != NULL)
	{
		GtkSourceSearchContext *new_search_context;
		GtkTextBuffer *buffer;

		g_clear_object (&frame->search_settings);
		frame->search_settings = copy_search_settings (frame->old_search_settings);

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->view));
		new_search_context = gtk_source_search_context_new (GTK_SOURCE_BUFFER (buffer),
								    frame->search_settings);
		gedit_document_set_search_context (GEDIT_DOCUMENT (buffer), new_search_context);
		g_object_unref (new_search_context);

		g_free (frame->search_text);
		frame->search_text = NULL;

		if (frame->old_search_text != NULL)
		{
			frame->search_text = g_strdup (frame->old_search_text);
		}
	}

	hide_search_widget (frame, TRUE);
	gtk_widget_grab_focus (GTK_WIDGET (frame->view));
}

static void
search_entry_previous_match (GtkSearchEntry *entry,
                             GeditViewFrame *frame)
{
	backward_search (frame);
}

static void
search_entry_next_match (GtkSearchEntry *entry,
                         GeditViewFrame *frame)
{
	forward_search (frame);
}

static void
search_entry_populate_popup (GtkEntry       *entry,
                             GtkMenu        *menu,
                             GeditViewFrame *frame)
{
	GtkWidget *menu_item;

	if (frame->search_mode == SEARCH_MODE_GOTO_LINE)
	{
		return;
	}

	setup_popup_menu (frame, GTK_WIDGET (menu));

	/* separator */
	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	add_popup_menu_items (frame, GTK_WIDGET (menu));
}

static void
search_entry_icon_release (GtkEntry             *entry,
                           GtkEntryIconPosition  icon_pos,
                           GdkEventButton       *event,
                           GeditViewFrame       *frame)
{
	GtkWidget *menu;

	if (frame->search_mode == SEARCH_MODE_GOTO_LINE ||
	    icon_pos != GTK_ENTRY_ICON_PRIMARY)
	{
		return;
	}

	menu = gtk_menu_new ();
	gtk_widget_show (menu);

	setup_popup_menu (frame, menu);
	add_popup_menu_items (frame, menu);

	g_signal_connect (menu,
			  "selection-done",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_menu_popup_at_widget (GTK_MENU (menu), GTK_WIDGET (entry), GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
}

static void
search_entry_activate (GtkEntry       *entry,
                       GeditViewFrame *frame)
{
	hide_search_widget (frame, FALSE);
	gtk_widget_grab_focus (GTK_WIDGET (frame->view));
}

static void
search_entry_insert_text (GtkEditable    *editable,
                          const gchar    *text,
                          gint            length,
                          gint           *position,
                          GeditViewFrame *frame)
{
	gunichar c;
	const gchar *p;
	const gchar *end;
	const gchar *next;

	if (frame->search_mode == SEARCH_MODE_SIMPLE_SEARCH)
	{
		return;
	}

	p = text;
	end = text + length;

	if (p == end)
	{
		return;
	}

	c = g_utf8_get_char (p);

	if (((c == '-' || c == '+') && *position == 0) ||
	    (c == ':' && *position != 0))
	{
		gchar *s = NULL;

		if (c == ':')
		{
			s = gtk_editable_get_chars (editable, 0, -1);
			s = g_utf8_strchr (s, -1, ':');
		}

		if (s == NULL || s == p)
		{
			next = g_utf8_next_char (p);
			p = next;
		}

		g_free (s);
	}

	while (p != end)
	{
		next = g_utf8_next_char (p);

		c = g_utf8_get_char (p);

		if (!g_unichar_isdigit (c))
		{
			g_signal_stop_emission_by_name (editable, "insert_text");
			gtk_widget_error_bell (GTK_WIDGET (frame->search_entry));
			break;
		}

		p = next;
	}
}

static void
customize_for_search_mode (GeditViewFrame *frame)
{
	GIcon *icon;
	gint width_request;

	if (frame->search_mode == SEARCH_MODE_SIMPLE_SEARCH)
	{
		icon = g_themed_icon_new_with_default_fallbacks ("edit-find-symbolic");

		width_request = 260;

		gtk_widget_set_tooltip_text (GTK_WIDGET (frame->search_entry),
		                             _("String you want to search for"));

		gtk_widget_show (frame->go_up_button);
		gtk_widget_show (frame->go_down_button);
	}
	else
	{
		icon = g_themed_icon_new_with_default_fallbacks ("go-jump-symbolic");

		width_request = 160;

		gtk_widget_set_tooltip_text (GTK_WIDGET (frame->search_entry),
		                             _("Line you want to move the cursor to"));

		gtk_widget_hide (frame->go_up_button);
		gtk_widget_hide (frame->go_down_button);
	}

	gtk_entry_set_icon_from_gicon (GTK_ENTRY (frame->search_entry),
	                               GTK_ENTRY_ICON_PRIMARY,
	                               icon);

	gtk_widget_set_size_request (GTK_WIDGET (frame->search_entry),
				     width_request,
				     -1);

	g_object_unref (icon);
}

static void
update_goto_line (GeditViewFrame *frame)
{
	const gchar *entry_text;
	gboolean moved;
	gboolean moved_offset;
	gint line;
	gint offset_line = 0;
	gint line_offset = 0;
	gchar **split_text = NULL;
	const gchar *text;
	GtkTextIter iter;

	entry_text = gtk_entry_get_text (GTK_ENTRY (frame->search_entry));

	if (entry_text[0] == '\0')
	{
		return;
	}

	get_iter_at_start_mark (frame, &iter);

	split_text = g_strsplit (entry_text, ":", -1);

	if (g_strv_length (split_text) > 1)
	{
		text = split_text[0];
	}
	else
	{
		text = entry_text;
	}

	if (text[0] == '-')
	{
		gint cur_line = gtk_text_iter_get_line (&iter);

		if (text[1] != '\0')
		{
			offset_line = MAX (atoi (text + 1), 0);
		}

		line = MAX (cur_line - offset_line, 0);
	}
	else if (entry_text[0] == '+')
	{
		gint cur_line = gtk_text_iter_get_line (&iter);

		if (text[1] != '\0')
		{
			offset_line = MAX (atoi (text + 1), 0);
		}

		line = cur_line + offset_line;
	}
	else
	{
		line = MAX (atoi (text) - 1, 0);
	}

	if (split_text[1] != NULL)
	{
		line_offset = atoi (split_text[1]);
	}

	g_strfreev (split_text);

	moved = tepl_view_goto_line (TEPL_VIEW (frame->view), line);
	moved_offset = tepl_view_goto_line_offset (TEPL_VIEW (frame->view), line, line_offset);

	if (!moved || !moved_offset)
	{
		set_search_state (frame, SEARCH_STATE_NOT_FOUND);
	}
	else
	{
		set_search_state (frame, SEARCH_STATE_NORMAL);
	}
}

static void
search_entry_changed_cb (GtkEntry       *entry,
			 GeditViewFrame *frame)
{
	renew_flush_timeout (frame);

	if (frame->search_mode == SEARCH_MODE_SIMPLE_SEARCH)
	{
		update_search_text (frame);
		start_search (frame);
	}
	else
	{
		update_goto_line (frame);
	}
}

static gboolean
search_entry_focus_out_event (GtkWidget      *widget,
                              GdkEventFocus  *event,
                              GeditViewFrame *frame)
{
	hide_search_widget (frame, FALSE);
	return GDK_EVENT_PROPAGATE;
}

static void
mark_set_cb (GtkTextBuffer  *buffer,
	     GtkTextIter    *location,
	     GtkTextMark    *mark,
	     GeditViewFrame *frame)
{
	GtkTextMark *insert;
	GtkTextMark *selection_bound;

	insert = gtk_text_buffer_get_insert (buffer);
	selection_bound = gtk_text_buffer_get_selection_bound (buffer);

	if (mark == insert || mark == selection_bound)
	{
		install_update_entry_tag_idle (frame);
	}
}

static gboolean
get_selected_text (GtkTextBuffer  *doc,
		   gchar         **selected_text,
		   gint           *len)
{
	GtkTextIter start;
	GtkTextIter end;

	g_return_val_if_fail (selected_text != NULL, FALSE);
	g_return_val_if_fail (*selected_text == NULL, FALSE);

	if (!gtk_text_buffer_get_selection_bounds (doc, &start, &end))
	{
		if (len != NULL)
		{
			*len = 0;
		}

		return FALSE;
	}

	*selected_text = gtk_text_buffer_get_slice (doc, &start, &end, TRUE);

	if (len != NULL)
	{
		*len = g_utf8_strlen (*selected_text, -1);
	}

	return TRUE;
}

static void
init_search_entry (GeditViewFrame *frame)
{
	if (frame->search_mode == SEARCH_MODE_GOTO_LINE)
	{
		gint line;
		gchar *line_str;
		GtkTextIter iter;

		get_iter_at_start_mark (frame, &iter);

		line = gtk_text_iter_get_line (&iter);

		line_str = g_strdup_printf ("%d", line + 1);

		gtk_entry_set_text (GTK_ENTRY (frame->search_entry), line_str);

		gtk_editable_select_region (GTK_EDITABLE (frame->search_entry),
		                            0, -1);

		g_free (line_str);
	}
	else
	{
		/* SEARCH_MODE_SIMPLE_SEARCH */
		GtkTextBuffer *buffer;
		gboolean selection_exists;
		gchar *search_text = NULL;
		gint selection_len = 0;
		GtkSourceSearchContext *search_context;

		if (frame->search_settings == NULL)
		{
			frame->search_settings = gtk_source_search_settings_new ();
			gtk_source_search_settings_set_wrap_around (frame->search_settings, TRUE);
		}

		g_clear_object (&frame->old_search_settings);
		frame->old_search_settings = copy_search_settings (frame->search_settings);

		g_free (frame->old_search_text);
		frame->old_search_text = NULL;

		if (frame->search_text != NULL)
		{
			frame->old_search_text = g_strdup (frame->search_text);
		}

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->view));

		search_context = get_search_context (frame);

		if (search_context == NULL)
		{
			search_context = gtk_source_search_context_new (GTK_SOURCE_BUFFER (buffer),
									frame->search_settings);

			gedit_document_set_search_context (GEDIT_DOCUMENT (buffer),
							   search_context);

			g_signal_connect_swapped (search_context,
						  "notify::occurrences-count",
						  G_CALLBACK (install_update_entry_tag_idle),
						  frame);

			g_object_unref (search_context);
		}

		selection_exists = get_selected_text (buffer,
		                                      &search_text,
		                                      &selection_len);

		if (selection_exists && (search_text != NULL) && (selection_len <= 160))
		{
			gchar *search_text_escaped;

			if (gtk_source_search_settings_get_regex_enabled (frame->search_settings))
			{
				search_text_escaped = g_regex_escape_string (search_text, -1);
			}
			else
			{
				search_text_escaped = gtk_source_utils_escape_search_text (search_text);
			}

			if (g_strcmp0 (search_text_escaped, frame->search_text) == 0)
			{
				/* The search text is the same, no need to
				 * trigger the search again. We prefer to select
				 * the text in the search entry, so the user can
				 * easily search something else.
				 */
				g_signal_handler_block (frame->search_entry,
							frame->search_entry_changed_id);

				gtk_entry_set_text (GTK_ENTRY (frame->search_entry),
						    search_text_escaped);

				gtk_editable_select_region (GTK_EDITABLE (frame->search_entry),
							    0, -1);

				g_signal_handler_unblock (frame->search_entry,
							  frame->search_entry_changed_id);
			}
			else
			{
				/* search_text_escaped is new, so we trigger the
				 * search (by not blocking the signal), and we
				 * don't select the text in the search entry
				 * because the user wants to search for
				 * search_text_escaped, not for something else.
				 */
				gtk_entry_set_text (GTK_ENTRY (frame->search_entry),
						    search_text_escaped);

				gtk_editable_set_position (GTK_EDITABLE (frame->search_entry), -1);
			}

			g_free (search_text_escaped);
		}
		else if (frame->search_text != NULL)
		{
			g_signal_handler_block (frame->search_entry,
			                        frame->search_entry_changed_id);

			gtk_entry_set_text (GTK_ENTRY (frame->search_entry),
					    frame->search_text);

			gtk_editable_select_region (GTK_EDITABLE (frame->search_entry),
			                            0, -1);

			g_signal_handler_unblock (frame->search_entry,
			                          frame->search_entry_changed_id);
		}

		g_free (search_text);
	}
}

static void
start_interactive_search_real (GeditViewFrame *frame,
			       SearchMode      request_search_mode)
{
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	if (gtk_revealer_get_reveal_child (frame->revealer))
	{
		if (frame->search_mode != request_search_mode)
		{
			hide_search_widget (frame, TRUE);
		}
		else
		{
			gtk_editable_select_region (GTK_EDITABLE (frame->search_entry),
			                            0, -1);
			return;
		}
	}

	frame->search_mode = request_search_mode;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (frame->view));

	if (frame->search_mode == SEARCH_MODE_SIMPLE_SEARCH)
	{
		gtk_text_buffer_get_selection_bounds (buffer, &iter, NULL);
	}
	else
	{
		GtkTextMark *mark = gtk_text_buffer_get_insert (buffer);
		gtk_text_buffer_get_iter_at_mark (buffer, &iter, mark);
	}

	if (frame->start_mark != NULL)
	{
		gtk_text_buffer_delete_mark (buffer, frame->start_mark);
	}

	frame->start_mark = gtk_text_buffer_create_mark (buffer, NULL, &iter, FALSE);

	gtk_revealer_set_reveal_child (frame->revealer, TRUE);

	/* NOTE: we must be very careful here to not have any text before
	   focusing the entry because when the entry is focused the text is
	   selected, and gtk+ doesn't allow us to have more than one selection
	   active */
	g_signal_handler_block (frame->search_entry,
	                        frame->search_entry_changed_id);

	gtk_entry_set_text (GTK_ENTRY (frame->search_entry), "");

	g_signal_handler_unblock (frame->search_entry,
	                          frame->search_entry_changed_id);

	gtk_widget_grab_focus (GTK_WIDGET (frame->search_entry));

	customize_for_search_mode (frame);
	init_search_entry (frame);

	/* Manage the scroll also for the view */
	frame->view_scroll_event_id =
		g_signal_connect (frame->view, "scroll-event",
			          G_CALLBACK (search_widget_scroll_event),
			          frame);

	renew_flush_timeout (frame);

	install_update_entry_tag_idle (frame);
}

static void
gedit_view_frame_class_init (GeditViewFrameClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->dispose = gedit_view_frame_dispose;
	object_class->finalize = gedit_view_frame_finalize;

	/* Bind class to template */
	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/gedit/ui/gedit-view-frame.ui");
	gtk_widget_class_bind_template_child (widget_class, GeditViewFrame, view);
	gtk_widget_class_bind_template_child (widget_class, GeditViewFrame, revealer);
	gtk_widget_class_bind_template_child (widget_class, GeditViewFrame, search_entry);
	gtk_widget_class_bind_template_child (widget_class, GeditViewFrame, go_up_button);
	gtk_widget_class_bind_template_child (widget_class, GeditViewFrame, go_down_button);
}

static GMountOperation *
view_frame_mount_operation_factory (GtkSourceFile *file,
				    gpointer       user_data)
{
	GtkWidget *view_frame = user_data;
	GtkWidget *window = gtk_widget_get_toplevel (view_frame);

	return gtk_mount_operation_new (GTK_WINDOW (window));
}

static void
gedit_view_frame_init (GeditViewFrame *frame)
{
	GeditDocument *doc;
	GtkSourceFile *file;

	gtk_widget_init_template (GTK_WIDGET (frame));

	doc = get_document (frame);
	file = gedit_document_get_file (doc);

	gtk_source_file_set_mount_operation_factory (file,
						     view_frame_mount_operation_factory,
						     frame,
						     NULL);

	frame->entry_tag = gd_tagged_entry_tag_new ("");

	gd_tagged_entry_tag_set_style (frame->entry_tag,
				       "gedit-search-entry-occurrences-tag");

	gd_tagged_entry_tag_set_has_close_button (frame->entry_tag, FALSE);

	gtk_widget_set_margin_end (GTK_WIDGET (frame->revealer),
				   SEARCH_POPUP_MARGIN);

	g_signal_connect (doc,
			  "mark-set",
			  G_CALLBACK (mark_set_cb),
			  frame);

	g_signal_connect (frame->revealer,
			  "key-press-event",
	                  G_CALLBACK (search_widget_key_press_event),
	                  frame);

	g_signal_connect (frame->revealer,
			  "scroll-event",
	                  G_CALLBACK (search_widget_scroll_event),
	                  frame);

	g_signal_connect (frame->search_entry,
			  "populate-popup",
	                  G_CALLBACK (search_entry_populate_popup),
	                  frame);

	g_signal_connect (frame->search_entry,
			  "icon-release",
	                  G_CALLBACK (search_entry_icon_release),
	                  frame);

	g_signal_connect (frame->search_entry,
			  "activate",
	                  G_CALLBACK (search_entry_activate),
	                  frame);

	g_signal_connect (frame->search_entry,
			  "insert-text",
	                  G_CALLBACK (search_entry_insert_text),
	                  frame);

	g_signal_connect (frame->search_entry,
	                  "stop-search",
	                  G_CALLBACK (search_entry_escaped),
	                  frame);

	g_signal_connect (frame->search_entry,
	                  "next-match",
	                  G_CALLBACK (search_entry_next_match),
	                  frame);

	g_signal_connect (frame->search_entry,
	                  "previous-match",
	                  G_CALLBACK (search_entry_previous_match),
	                  frame);

	frame->search_entry_changed_id =
		g_signal_connect (frame->search_entry,
		                  "changed",
		                  G_CALLBACK (search_entry_changed_cb),
		                  frame);

	frame->search_entry_focus_out_id =
		g_signal_connect (frame->search_entry,
				  "focus-out-event",
				  G_CALLBACK (search_entry_focus_out_event),
				  frame);

	g_signal_connect_swapped (frame->go_up_button,
				  "clicked",
				  G_CALLBACK (backward_search),
				  frame);

	g_signal_connect_swapped (frame->go_down_button,
				  "clicked",
				  G_CALLBACK (forward_search),
				  frame);
}

GeditViewFrame *
gedit_view_frame_new (void)
{
	return g_object_new (GEDIT_TYPE_VIEW_FRAME, NULL);
}

GeditView *
gedit_view_frame_get_view (GeditViewFrame *frame)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_FRAME (frame), NULL);

	return frame->view;
}

void
gedit_view_frame_popup_search (GeditViewFrame *frame)
{
	g_return_if_fail (GEDIT_IS_VIEW_FRAME (frame));

	start_interactive_search_real (frame, SEARCH_MODE_SIMPLE_SEARCH);
}

void
gedit_view_frame_popup_goto_line (GeditViewFrame *frame)
{
	g_return_if_fail (GEDIT_IS_VIEW_FRAME (frame));

	start_interactive_search_real (frame, SEARCH_MODE_GOTO_LINE);
}

void
gedit_view_frame_clear_search (GeditViewFrame *frame)
{
	g_return_if_fail (GEDIT_IS_VIEW_FRAME (frame));

	g_signal_handler_block (frame->search_entry,
	                        frame->search_entry_changed_id);

	gtk_entry_set_text (GTK_ENTRY (frame->search_entry), "");

	g_signal_handler_unblock (frame->search_entry,
	                          frame->search_entry_changed_id);

	gtk_widget_grab_focus (GTK_WIDGET (frame->view));
}
