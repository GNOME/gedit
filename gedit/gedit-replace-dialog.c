/*
 * gedit-replace-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 Paolo Maggi
 * Copyright (C) 2013 Sébastien Wilmet
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

#include "gedit-replace-dialog.h"

#include <string.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include "gedit-history-entry.h"
#include "gedit-document.h"

#define GEDIT_SEARCH_CONTEXT_KEY "gedit-search-context-key"

struct _GeditReplaceDialog
{
	GtkDialog parent_instance;

	GtkWidget *grid;
	GtkWidget *search_label;
	GtkWidget *search_entry;
	GtkWidget *search_text_entry;
	GtkWidget *replace_label;
	GtkWidget *replace_entry;
	GtkWidget *replace_text_entry;
	GtkWidget *match_case_checkbutton;
	GtkWidget *entire_word_checkbutton;
	GtkWidget *regex_checkbutton;
	GtkWidget *backwards_checkbutton;
	GtkWidget *wrap_around_checkbutton;
	GtkWidget *close_button;

	GeditDocument *active_document;

	guint idle_update_sensitivity_id;
};

G_DEFINE_TYPE (GeditReplaceDialog, gedit_replace_dialog, GTK_TYPE_DIALOG)

static GtkSourceSearchContext *
get_search_context (GeditReplaceDialog *dialog,
		    GeditDocument      *doc)
{
	GtkSourceSearchContext *search_context;

	if (doc == NULL)
	{
		return NULL;
	}

	search_context = gedit_document_get_search_context (doc);

	if (search_context != NULL &&
	    g_object_get_data (G_OBJECT (search_context), GEDIT_SEARCH_CONTEXT_KEY) == dialog)
	{
		return search_context;
	}

	return NULL;
}

/* The search settings between the dialog's widgets (checkbuttons and the text
 * entry) and the SearchSettings object are not bound. Instead, this function is
 * called to set the search settings from the widgets to the SearchSettings
 * object.
 *
 * The reason: the search and replace dialog is not an incremental search. You
 * have to press the buttons to have an effect. The SearchContext is created
 * only when a button is pressed, not before. If the SearchContext was created
 * directly when the dialog window is shown, or when the document tab is
 * switched, there would be a problem. When we switch betweeen tabs to find on
 * which tab(s) we want to do the search, we may have older searches (still
 * highlighted) that we don't want to lose, and we don't want the new search to
 * appear on each tab that we open. Only when we press a button. So when the
 * SearchContext is not already created, this is not an incremental search. Once
 * the SearchContext is created, it's better to be consistent, and therefore we
 * don't want the incremental search: we have to always press a button to
 * execute the search.
 *
 * Likewise, each created SearchContext (from the GeditReplaceDialog) contains a
 * different SearchSettings. When set_search_settings() is called for one
 * document tab (and thus one SearchSettings), it doesn't have an effect on the
 * other tabs. But the dialog widgets don't change.
 */
static void
set_search_settings (GeditReplaceDialog *dialog)
{
	GtkSourceSearchContext *search_context;
	GtkSourceSearchSettings *search_settings;
	gboolean case_sensitive;
	gboolean at_word_boundaries;
	gboolean regex_enabled;
	gboolean wrap_around;
	const gchar *search_text;

	search_context = get_search_context (dialog, dialog->active_document);

	if (search_context == NULL)
	{
		return;
	}

	search_settings = gtk_source_search_context_get_settings (search_context);

	case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->match_case_checkbutton));
	gtk_source_search_settings_set_case_sensitive (search_settings, case_sensitive);

	at_word_boundaries = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->entire_word_checkbutton));
	gtk_source_search_settings_set_at_word_boundaries (search_settings, at_word_boundaries);

	regex_enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->regex_checkbutton));
	gtk_source_search_settings_set_regex_enabled (search_settings, regex_enabled);

	wrap_around = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->wrap_around_checkbutton));
	gtk_source_search_settings_set_wrap_around (search_settings, wrap_around);

	search_text = gtk_entry_get_text (GTK_ENTRY (dialog->search_text_entry));

	if (regex_enabled)
	{
		gtk_source_search_settings_set_search_text (search_settings, search_text);
	}
	else
	{
		gchar *unescaped_search_text = gtk_source_utils_unescape_search_text (search_text);
		gtk_source_search_settings_set_search_text (search_settings, unescaped_search_text);
		g_free (unescaped_search_text);
	}
}

static GeditWindow *
get_gedit_window (GeditReplaceDialog *dialog)
{
	GtkWindow *transient_for = gtk_window_get_transient_for (GTK_WINDOW (dialog));

	return transient_for != NULL ? GEDIT_WINDOW (transient_for) : NULL;
}

static GeditDocument *
get_active_document (GeditReplaceDialog *dialog)
{
	GeditWindow *window = get_gedit_window (dialog);

	return window != NULL ? gedit_window_get_active_document (window) : NULL;
}

void
gedit_replace_dialog_present_with_time (GeditReplaceDialog *dialog,
					guint32             timestamp)
{
	g_return_if_fail (GEDIT_REPLACE_DIALOG (dialog));

	gtk_window_present_with_time (GTK_WINDOW (dialog), timestamp);

	gtk_widget_grab_focus (dialog->search_text_entry);
}

static gboolean
gedit_replace_dialog_delete_event (GtkWidget   *widget,
                                   GdkEventAny *event)
{
	/* prevent destruction */
	return TRUE;
}

static void
set_error (GtkEntry    *entry,
	   const gchar *error_msg)
{
	if (error_msg == NULL || error_msg[0] == '\0')
	{
		gtk_entry_set_icon_from_gicon (entry, GTK_ENTRY_ICON_SECONDARY, NULL);
		gtk_entry_set_icon_tooltip_text (entry, GTK_ENTRY_ICON_SECONDARY, NULL);
	}
	else
	{
		GIcon *icon = g_themed_icon_new_with_default_fallbacks ("dialog-error-symbolic");

		gtk_entry_set_icon_from_gicon (entry, GTK_ENTRY_ICON_SECONDARY, icon);
		gtk_entry_set_icon_tooltip_text (entry, GTK_ENTRY_ICON_SECONDARY, error_msg);

		g_object_unref (icon);
	}
}

static void
set_search_error (GeditReplaceDialog *dialog,
		  const gchar        *error_msg)
{
	set_error (GTK_ENTRY (dialog->search_text_entry), error_msg);
}

void
gedit_replace_dialog_set_replace_error (GeditReplaceDialog *dialog,
					const gchar        *error_msg)
{
	set_error (GTK_ENTRY (dialog->replace_text_entry), error_msg);
}

static gboolean
has_search_error (GeditReplaceDialog *dialog)
{
	GIcon *icon;

	icon = gtk_entry_get_icon_gicon (GTK_ENTRY (dialog->search_text_entry),
	                                 GTK_ENTRY_ICON_SECONDARY);

	return icon != NULL;
}

static gboolean
has_replace_error (GeditReplaceDialog *dialog)
{
	GIcon *icon;

	icon = gtk_entry_get_icon_gicon (GTK_ENTRY (dialog->replace_text_entry),
	                                 GTK_ENTRY_ICON_SECONDARY);

	return icon != NULL;
}

static void
update_regex_error (GeditReplaceDialog *dialog)
{
	GtkSourceSearchContext *search_context;
	GError *regex_error;

	set_search_error (dialog, NULL);

	search_context = get_search_context (dialog, dialog->active_document);

	if (search_context == NULL)
	{
		return;
	}

	regex_error = gtk_source_search_context_get_regex_error (search_context);

	if (regex_error != NULL)
	{
		set_search_error (dialog, regex_error->message);
		g_error_free (regex_error);
	}
}

static gboolean
update_replace_response_sensitivity_cb (GeditReplaceDialog *dialog)
{
	GtkSourceSearchContext *search_context;
	GtkTextIter start;
	GtkTextIter end;
	gint pos;

	if (has_replace_error (dialog))
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
						   GEDIT_REPLACE_DIALOG_REPLACE_RESPONSE,
						   FALSE);

		dialog->idle_update_sensitivity_id = 0;
		return G_SOURCE_REMOVE;
	}

	search_context = get_search_context (dialog, dialog->active_document);

	if (search_context == NULL)
	{
		dialog->idle_update_sensitivity_id = 0;
		return G_SOURCE_REMOVE;
	}

	gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (dialog->active_document),
					      &start,
					      &end);

	pos = gtk_source_search_context_get_occurrence_position (search_context,
								 &start,
								 &end);

	if (pos < 0)
	{
		return G_SOURCE_CONTINUE;
	}

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
					   GEDIT_REPLACE_DIALOG_REPLACE_RESPONSE,
					   pos > 0);

	dialog->idle_update_sensitivity_id = 0;
	return G_SOURCE_REMOVE;
}

static void
install_idle_update_sensitivity (GeditReplaceDialog *dialog)
{
	if (dialog->idle_update_sensitivity_id != 0)
	{
		return;
	}

	dialog->idle_update_sensitivity_id =
		g_idle_add ((GSourceFunc)update_replace_response_sensitivity_cb,
			    dialog);
}

static void
mark_set_cb (GtkTextBuffer      *buffer,
	     GtkTextIter        *location,
	     GtkTextMark        *mark,
	     GeditReplaceDialog *dialog)
{
	GtkTextMark *insert;
	GtkTextMark *selection_bound;

	insert = gtk_text_buffer_get_insert (buffer);
	selection_bound = gtk_text_buffer_get_selection_bound (buffer);

	if (mark == insert || mark == selection_bound)
	{
		install_idle_update_sensitivity (dialog);
	}
}

static void
update_responses_sensitivity (GeditReplaceDialog *dialog)
{
	const gchar *search_text;
	gboolean sensitive = TRUE;

	install_idle_update_sensitivity (dialog);

	search_text = gtk_entry_get_text (GTK_ENTRY (dialog->search_text_entry));

	if (search_text[0] == '\0')
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
						   GEDIT_REPLACE_DIALOG_FIND_RESPONSE,
						   FALSE);

		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
						   GEDIT_REPLACE_DIALOG_REPLACE_ALL_RESPONSE,
						   FALSE);

		return;
	}

	sensitive = !has_search_error (dialog);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
					   GEDIT_REPLACE_DIALOG_FIND_RESPONSE,
					   sensitive);

	if (has_replace_error (dialog))
	{
		sensitive = FALSE;
	}

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
					   GEDIT_REPLACE_DIALOG_REPLACE_ALL_RESPONSE,
					   sensitive);
}

static void
regex_error_notify_cb (GeditReplaceDialog *dialog)
{
	update_regex_error (dialog);
	update_responses_sensitivity (dialog);
}

static void
disconnect_document (GeditReplaceDialog *dialog)
{
	GtkSourceSearchContext *search_context;

	if (dialog->active_document == NULL)
	{
		return;
	}

	search_context = get_search_context (dialog, dialog->active_document);

	if (search_context != NULL)
	{
		g_signal_handlers_disconnect_by_func (search_context,
						      regex_error_notify_cb,
						      dialog);
	}

	g_signal_handlers_disconnect_by_func (dialog->active_document,
					      mark_set_cb,
					      dialog);

	g_clear_object (&dialog->active_document);
}

static void
connect_active_document (GeditReplaceDialog *dialog)
{
	GeditDocument *doc;
	GtkSourceSearchContext *search_context;

	disconnect_document (dialog);

	doc = get_active_document (dialog);

	if (doc == NULL)
	{
		return;
	}

	dialog->active_document = g_object_ref (doc);

	search_context = get_search_context (dialog, doc);

	if (search_context == NULL)
	{
		GtkSourceSearchSettings *settings = gtk_source_search_settings_new ();

		search_context = gtk_source_search_context_new (GTK_SOURCE_BUFFER (doc),
								settings);

		/* Mark the search context that it comes from the search and
		 * replace dialog. Search contexts can be created also from the
		 * GeditViewFrame.
		 */
		g_object_set_data (G_OBJECT (search_context),
				   GEDIT_SEARCH_CONTEXT_KEY,
				   dialog);

		gedit_document_set_search_context (doc, search_context);

		g_object_unref (settings);
		g_object_unref (search_context);
	}

	g_signal_connect_object (search_context,
				 "notify::regex-error",
				 G_CALLBACK (regex_error_notify_cb),
				 dialog,
				 G_CONNECT_SWAPPED);

	g_signal_connect_object (doc,
				 "mark-set",
				 G_CALLBACK (mark_set_cb),
				 dialog,
				 0);

	update_regex_error (dialog);
	update_responses_sensitivity (dialog);
}

static void
response_cb (GtkDialog *dialog,
	     gint       response_id)
{
	GeditReplaceDialog *dlg = GEDIT_REPLACE_DIALOG (dialog);
	const gchar *str;

	switch (response_id)
	{
		case GEDIT_REPLACE_DIALOG_REPLACE_RESPONSE:
		case GEDIT_REPLACE_DIALOG_REPLACE_ALL_RESPONSE:
			str = gtk_entry_get_text (GTK_ENTRY (dlg->replace_text_entry));
			if (*str != '\0')
			{
				gedit_history_entry_prepend_text
						(GEDIT_HISTORY_ENTRY (dlg->replace_entry),
						 str);
			}
			/* fall through, so that we also save the find entry */
			G_GNUC_FALLTHROUGH;

		case GEDIT_REPLACE_DIALOG_FIND_RESPONSE:
			str = gtk_entry_get_text (GTK_ENTRY (dlg->search_text_entry));
			if (*str != '\0')
			{
				gedit_history_entry_prepend_text
						(GEDIT_HISTORY_ENTRY (dlg->search_entry),
						 str);
			}
	}

	switch (response_id)
	{
		case GEDIT_REPLACE_DIALOG_REPLACE_RESPONSE:
		case GEDIT_REPLACE_DIALOG_REPLACE_ALL_RESPONSE:
		case GEDIT_REPLACE_DIALOG_FIND_RESPONSE:
			connect_active_document (GEDIT_REPLACE_DIALOG (dialog));
			set_search_settings (GEDIT_REPLACE_DIALOG (dialog));
	}
}

static void
gedit_replace_dialog_dispose (GObject *object)
{
	GeditReplaceDialog *dialog = GEDIT_REPLACE_DIALOG (object);

	g_clear_object (&dialog->active_document);

	if (dialog->idle_update_sensitivity_id != 0)
	{
		g_source_remove (dialog->idle_update_sensitivity_id);
		dialog->idle_update_sensitivity_id = 0;
	}

	G_OBJECT_CLASS (gedit_replace_dialog_parent_class)->dispose (object);
}

static void
gedit_replace_dialog_class_init (GeditReplaceDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	gobject_class->dispose = gedit_replace_dialog_dispose;
	widget_class->delete_event = gedit_replace_dialog_delete_event;

	/* Bind class to template */
	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/gedit/ui/gedit-replace-dialog.ui");
	gtk_widget_class_bind_template_child (widget_class, GeditReplaceDialog, grid);
	gtk_widget_class_bind_template_child (widget_class, GeditReplaceDialog, search_label);
	gtk_widget_class_bind_template_child (widget_class, GeditReplaceDialog, replace_label);
	gtk_widget_class_bind_template_child (widget_class, GeditReplaceDialog, match_case_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditReplaceDialog, entire_word_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditReplaceDialog, regex_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditReplaceDialog, backwards_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditReplaceDialog, wrap_around_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditReplaceDialog, close_button);
}

static void
search_text_entry_changed (GtkEditable        *editable,
			   GeditReplaceDialog *dialog)
{
	set_search_error (dialog, NULL);

	update_responses_sensitivity (dialog);
}

static void
replace_text_entry_changed (GtkEditable        *editable,
			    GeditReplaceDialog *dialog)
{
	gedit_replace_dialog_set_replace_error (dialog, NULL);

	update_responses_sensitivity (dialog);
}

static void
regex_checkbutton_toggled (GtkToggleButton    *checkbutton,
			   GeditReplaceDialog *dialog)
{
	if (!gtk_toggle_button_get_active (checkbutton))
	{
		/* Remove the regex error state so the user can search again */
		set_search_error (dialog, NULL);
		update_responses_sensitivity (dialog);
	}
}

/* TODO: move in gedit-document.c and share it with gedit-view-frame */
static gboolean
get_selected_text (GtkTextBuffer  *doc,
		   gchar         **selected_text,
		   gint           *len)
{
	GtkTextIter start, end;

	g_return_val_if_fail (selected_text != NULL, FALSE);
	g_return_val_if_fail (*selected_text == NULL, FALSE);

	if (!gtk_text_buffer_get_selection_bounds (doc, &start, &end))
	{
		if (len != NULL)
		{
			len = 0;
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
show_cb (GeditReplaceDialog *dialog)
{
	GeditWindow *window;
	GeditDocument *doc;
	gboolean selection_exists;
	gchar *selection = NULL;
	gint selection_length;

	window = get_gedit_window (dialog);

	if (window == NULL)
	{
		return;
	}

	doc = get_active_document (dialog);

	if (doc == NULL)
	{
		return;
	}

	selection_exists = get_selected_text (GTK_TEXT_BUFFER (doc),
					      &selection,
					      &selection_length);

	if (selection_exists && selection != NULL && selection_length < 80)
	{
		gboolean regex_enabled;
		gchar *escaped_selection;

		regex_enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->regex_checkbutton));

		if (regex_enabled)
		{
			escaped_selection = g_regex_escape_string (selection, -1);
		}
		else
		{
			escaped_selection = gtk_source_utils_escape_search_text (selection);
		}

		gtk_entry_set_text (GTK_ENTRY (dialog->search_text_entry),
				    escaped_selection);

		g_free (escaped_selection);
	}

	g_free (selection);
}

static void
hide_cb (GeditReplaceDialog *dialog)
{
	disconnect_document (dialog);
}

static void
gedit_replace_dialog_init (GeditReplaceDialog *dlg)
{
	gtk_widget_init_template (GTK_WIDGET (dlg));

	dlg->search_entry = gedit_history_entry_new ("search-for-entry", TRUE);
	gtk_widget_set_size_request (dlg->search_entry, 300, -1);
	gtk_widget_set_hexpand (GTK_WIDGET (dlg->search_entry), TRUE);
	dlg->search_text_entry = gedit_history_entry_get_entry (GEDIT_HISTORY_ENTRY (dlg->search_entry));
	gtk_entry_set_activates_default (GTK_ENTRY (dlg->search_text_entry), TRUE);
	gtk_grid_attach_next_to (GTK_GRID (dlg->grid),
				 dlg->search_entry,
				 dlg->search_label,
				 GTK_POS_RIGHT, 1, 1);
	gtk_widget_show_all (dlg->search_entry);

	dlg->replace_entry = gedit_history_entry_new ("replace-with-entry", TRUE);
	gtk_widget_set_hexpand (GTK_WIDGET (dlg->replace_entry), TRUE);
	dlg->replace_text_entry = gedit_history_entry_get_entry (GEDIT_HISTORY_ENTRY (dlg->replace_entry));
	gtk_entry_set_placeholder_text (GTK_ENTRY (dlg->replace_text_entry), _("Nothing"));
	gtk_entry_set_activates_default (GTK_ENTRY (dlg->replace_text_entry), TRUE);
	gtk_grid_attach_next_to (GTK_GRID (dlg->grid),
				 dlg->replace_entry,
				 dlg->replace_label,
				 GTK_POS_RIGHT, 1, 1);
	gtk_widget_show_all (dlg->replace_entry);

	gtk_label_set_mnemonic_widget (GTK_LABEL (dlg->search_label),
				       dlg->search_entry);
	gtk_label_set_mnemonic_widget (GTK_LABEL (dlg->replace_label),
				       dlg->replace_entry);

	gtk_dialog_set_default_response (GTK_DIALOG (dlg),
					 GEDIT_REPLACE_DIALOG_FIND_RESPONSE);

	/* insensitive by default */
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg),
					   GEDIT_REPLACE_DIALOG_FIND_RESPONSE,
					   FALSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg),
					   GEDIT_REPLACE_DIALOG_REPLACE_RESPONSE,
					   FALSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg),
					   GEDIT_REPLACE_DIALOG_REPLACE_ALL_RESPONSE,
					   FALSE);

	g_signal_connect (dlg->search_text_entry,
			  "changed",
			  G_CALLBACK (search_text_entry_changed),
			  dlg);

	g_signal_connect (dlg->replace_text_entry,
			  "changed",
			  G_CALLBACK (replace_text_entry_changed),
			  dlg);

	g_signal_connect (dlg->regex_checkbutton,
			  "toggled",
			  G_CALLBACK (regex_checkbutton_toggled),
			  dlg);

	g_signal_connect (dlg,
			  "show",
			  G_CALLBACK (show_cb),
			  NULL);

	g_signal_connect (dlg,
			  "hide",
			  G_CALLBACK (hide_cb),
			  NULL);

	/* We connect here to make sure this handler runs before the others so
	 * that the search context is created.
	 */
	g_signal_connect (dlg,
			  "response",
			  G_CALLBACK (response_cb),
			  NULL);
}

GtkWidget *
gedit_replace_dialog_new (GeditWindow *window)
{
	GeditReplaceDialog *dialog;
	gboolean use_header;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	dialog = g_object_new (GEDIT_TYPE_REPLACE_DIALOG,
			       "transient-for", window,
			       "destroy-with-parent", TRUE,
			       "use-header-bar", FALSE,
			       NULL);

	/* We want the Find/Replace/ReplaceAll buttons at the bottom,
	 * so we turn off the automatic header bar, but we check the
	 * setting and if a header bar should be used, we create it
	 * manually and use it for the close button.
	 */
	g_object_get (gtk_settings_get_default (),
	              "gtk-dialogs-use-header", &use_header,
	              NULL);

	if (use_header)
	{
		GtkWidget *header_bar;

		header_bar = gtk_header_bar_new ();
		gtk_header_bar_set_title (GTK_HEADER_BAR (header_bar), _("Find and Replace"));
		gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header_bar), TRUE);
		gtk_widget_show (header_bar);
		gtk_window_set_titlebar (GTK_WINDOW (dialog), header_bar);
	}
	else
	{
		gtk_widget_set_no_show_all (dialog->close_button, FALSE);
		gtk_widget_show (dialog->close_button);
	}

	return GTK_WIDGET (dialog);
}

const gchar *
gedit_replace_dialog_get_replace_text (GeditReplaceDialog *dialog)
{
	g_return_val_if_fail (GEDIT_IS_REPLACE_DIALOG (dialog), NULL);

	return gtk_entry_get_text (GTK_ENTRY (dialog->replace_text_entry));
}

gboolean
gedit_replace_dialog_get_backwards (GeditReplaceDialog *dialog)
{
	g_return_val_if_fail (GEDIT_IS_REPLACE_DIALOG (dialog), FALSE);

	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->backwards_checkbutton));
}

/* This function returns the original search text. The search text from the
 * search settings has been unescaped, and the escape function is not
 * reciprocal. So to avoid bugs, we have to deal with the original search text.
 */
const gchar *
gedit_replace_dialog_get_search_text (GeditReplaceDialog *dialog)
{
	g_return_val_if_fail (GEDIT_IS_REPLACE_DIALOG (dialog), NULL);

	return gtk_entry_get_text (GTK_ENTRY (dialog->search_text_entry));
}

/* ex:set ts=8 noet: */
