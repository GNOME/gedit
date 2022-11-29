/*
 * gedit-close-confirmation-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2004-2005 GNOME Foundation
 * Copyright (C) 2015 Sébastien Wilmet
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

#include "gedit-close-confirmation-dialog.h"

#include <glib/gi18n.h>

#include <gedit/gedit-app.h>
#include <gedit/gedit-document.h>
#include <gedit/gedit-document-private.h>
#include <gedit/gedit-utils.h>
#include <gedit/gedit-window.h>

/* Mode */
enum
{
	SINGLE_DOC_MODE,
	MULTIPLE_DOCS_MODE
};

#define GET_MODE(dlg) (((dlg->unsaved_documents != NULL) && \
                       (dlg->unsaved_documents->next == NULL)) ? \
                       SINGLE_DOC_MODE : MULTIPLE_DOCS_MODE)

#define GEDIT_SAVE_DOCUMENT_KEY "gedit-save-document"

struct _GeditCloseConfirmationDialog
{
	GtkMessageDialog parent_instance;

	GList       *unsaved_documents;
	GList       *selected_documents;
	GtkWidget   *list_box;
};

enum
{
	PROP_0,
	PROP_UNSAVED_DOCUMENTS,
	LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

G_DEFINE_TYPE (GeditCloseConfirmationDialog,
	       gedit_close_confirmation_dialog,
	       GTK_TYPE_MESSAGE_DIALOG)

static void 	 set_unsaved_document 		(GeditCloseConfirmationDialog *dlg,
						 const GList                  *list);

static GList *
get_selected_docs (GtkWidget *list_box)
{
	GList *rows;
	GList *l;
	GList *ret = NULL;

	rows = gtk_container_get_children (GTK_CONTAINER (list_box));
	for (l = rows; l != NULL; l = l->next)
	{
		GtkWidget *row = l->data;
		GtkWidget *check_button;

		check_button = gtk_bin_get_child (GTK_BIN (row));
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_button)))
		{
			GeditDocument *doc;

			doc = g_object_get_data (G_OBJECT (row), GEDIT_SAVE_DOCUMENT_KEY);
			g_return_val_if_fail (doc != NULL, NULL);

			ret = g_list_prepend (ret, doc);
		}
	}

	g_list_free (rows);

	return g_list_reverse (ret);
}

/* Since we connect in the constructor we are sure this handler will be called
 * before the user ones.
 */
static void
response_cb (GeditCloseConfirmationDialog *dlg,
             gint                          response_id,
             gpointer                      data)
{
	g_return_if_fail (GEDIT_IS_CLOSE_CONFIRMATION_DIALOG (dlg));

	if (dlg->selected_documents != NULL)
	{
		g_list_free (dlg->selected_documents);
		dlg->selected_documents = NULL;
	}

	if (response_id == GTK_RESPONSE_YES)
	{
		if (GET_MODE (dlg) == SINGLE_DOC_MODE)
		{
			dlg->selected_documents = g_list_copy (dlg->unsaved_documents);
		}
		else
		{
			dlg->selected_documents = get_selected_docs (dlg->list_box);
		}
	}
}

static void
gedit_close_confirmation_dialog_init (GeditCloseConfirmationDialog *dlg)
{
	gtk_window_set_title (GTK_WINDOW (dlg), "");
	gtk_window_set_modal (GTK_WINDOW (dlg), TRUE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dlg), TRUE);

	g_signal_connect (dlg,
			  "response",
			  G_CALLBACK (response_cb),
			  NULL);
}

static void
gedit_close_confirmation_dialog_finalize (GObject *object)
{
	GeditCloseConfirmationDialog *dlg = GEDIT_CLOSE_CONFIRMATION_DIALOG (object);

	g_list_free (dlg->unsaved_documents);
	g_list_free (dlg->selected_documents);

	/* Call the parent's destructor */
	G_OBJECT_CLASS (gedit_close_confirmation_dialog_parent_class)->finalize (object);
}

static void
gedit_close_confirmation_dialog_set_property (GObject      *object,
					      guint         prop_id,
					      const GValue *value,
					      GParamSpec   *pspec)
{
	GeditCloseConfirmationDialog *dlg;

	dlg = GEDIT_CLOSE_CONFIRMATION_DIALOG (object);

	switch (prop_id)
	{
		case PROP_UNSAVED_DOCUMENTS:
			set_unsaved_document (dlg, g_value_get_pointer (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_close_confirmation_dialog_get_property (GObject    *object,
					      guint       prop_id,
					      GValue     *value,
					      GParamSpec *pspec)
{
	GeditCloseConfirmationDialog *dlg = GEDIT_CLOSE_CONFIRMATION_DIALOG (object);

	switch (prop_id)
	{
		case PROP_UNSAVED_DOCUMENTS:
			g_value_set_pointer (value, dlg->unsaved_documents);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_close_confirmation_dialog_class_init (GeditCloseConfirmationDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = gedit_close_confirmation_dialog_set_property;
	gobject_class->get_property = gedit_close_confirmation_dialog_get_property;
	gobject_class->finalize = gedit_close_confirmation_dialog_finalize;

	properties[PROP_UNSAVED_DOCUMENTS] =
		g_param_spec_pointer ("unsaved-documents",
		                      "Unsaved Documents",
		                      "List of Unsaved Documents",
		                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (gobject_class, LAST_PROP, properties);
}

GList *
gedit_close_confirmation_dialog_get_selected_documents (GeditCloseConfirmationDialog *dlg)
{
	g_return_val_if_fail (GEDIT_IS_CLOSE_CONFIRMATION_DIALOG (dlg), NULL);

	return g_list_copy (dlg->selected_documents);
}

GtkWidget *
gedit_close_confirmation_dialog_new (GtkWindow *parent,
				     GList     *unsaved_documents)
{
	GtkWidget *dlg;

	g_return_val_if_fail (unsaved_documents != NULL, NULL);

	dlg = g_object_new (GEDIT_TYPE_CLOSE_CONFIRMATION_DIALOG,
			    "unsaved-documents", unsaved_documents,
			    "message-type", GTK_MESSAGE_QUESTION,
			    NULL);

	if (parent != NULL)
	{
		gtk_window_group_add_window (gedit_window_get_group (GEDIT_WINDOW (parent)),
					     GTK_WINDOW (dlg));

		gtk_window_set_transient_for (GTK_WINDOW (dlg), parent);
	}

	return dlg;
}

GtkWidget *
gedit_close_confirmation_dialog_new_single (GtkWindow     *parent,
					    GeditDocument *doc)
{
	GtkWidget *dlg;
	GList *unsaved_documents;

	g_return_val_if_fail (doc != NULL, NULL);

	unsaved_documents = g_list_prepend (NULL, doc);
	dlg = gedit_close_confirmation_dialog_new (parent, unsaved_documents);
	g_list_free (unsaved_documents);

	return dlg;
}

static void
add_buttons (GeditCloseConfirmationDialog *dlg)
{
        GtkWidget *close_button;
	gboolean save_as = FALSE;

        close_button = gtk_dialog_add_button (GTK_DIALOG (dlg),
                                              _("Close _without Saving"),
                                              GTK_RESPONSE_NO);

        gtk_style_context_add_class (gtk_widget_get_style_context (close_button),
                                     "destructive-action");

	gtk_dialog_add_button (GTK_DIALOG (dlg), _("_Cancel"), GTK_RESPONSE_CANCEL);

	if (GET_MODE (dlg) == SINGLE_DOC_MODE)
	{
		GeditDocument *doc;
		GtkSourceFile *file;

		doc = GEDIT_DOCUMENT (dlg->unsaved_documents->data);
		file = gedit_document_get_file (doc);

		if (gtk_source_file_is_readonly (file) ||
		    _gedit_document_is_untitled (doc))
		{
			save_as = TRUE;
		}
	}

	gtk_dialog_add_button (GTK_DIALOG (dlg),
			       save_as ? _("_Save As…") : _("_Save"),
			       GTK_RESPONSE_YES);
	gtk_dialog_set_default_response	(GTK_DIALOG (dlg),
					 GTK_RESPONSE_YES);
}

static gchar *
get_text_secondary_label (GeditDocument *doc)
{
	glong  seconds;
	gchar *secondary_msg;

	seconds = MAX (1, _gedit_document_get_seconds_since_last_save_or_load (doc));

	if (seconds < 55)
	{
		secondary_msg = g_strdup_printf (
					ngettext ("If you don’t save, changes from the last %ld second "
					    	  "will be permanently lost.",
						  "If you don’t save, changes from the last %ld seconds "
					    	  "will be permanently lost.",
						  seconds),
					seconds);
	}
	else if (seconds < 75) /* 55 <= seconds < 75 */
	{
		secondary_msg = g_strdup (_("If you don’t save, changes from the last minute "
					    "will be permanently lost."));
	}
	else if (seconds < 110) /* 75 <= seconds < 110 */
	{
		secondary_msg = g_strdup_printf (
					ngettext ("If you don’t save, changes from the last minute and %ld "
						  "second will be permanently lost.",
						  "If you don’t save, changes from the last minute and %ld "
						  "seconds will be permanently lost.",
						  seconds - 60 ),
					seconds - 60);
	}
	else if (seconds < 3600)
	{
		secondary_msg = g_strdup_printf (
					ngettext ("If you don’t save, changes from the last %ld minute "
					    	  "will be permanently lost.",
						  "If you don’t save, changes from the last %ld minutes "
					    	  "will be permanently lost.",
						  seconds / 60),
					seconds / 60);
	}
	else if (seconds < 7200)
	{
		gint minutes;
		seconds -= 3600;

		minutes = seconds / 60;
		if (minutes < 5)
		{
			secondary_msg = g_strdup (_("If you don’t save, changes from the last hour "
						    "will be permanently lost."));
		}
		else
		{
			secondary_msg = g_strdup_printf (
					ngettext ("If you don’t save, changes from the last hour and %d "
						  "minute will be permanently lost.",
						  "If you don’t save, changes from the last hour and %d "
						  "minutes will be permanently lost.",
						  minutes),
					minutes);
		}
	}
	else
	{
		gint hours;

		hours = seconds / 3600;

		secondary_msg = g_strdup_printf (
					ngettext ("If you don’t save, changes from the last %d hour "
					    	  "will be permanently lost.",
						  "If you don’t save, changes from the last %d hours "
					    	  "will be permanently lost.",
						  hours),
					hours);
	}

	return secondary_msg;
}

static void
build_single_doc_dialog (GeditCloseConfirmationDialog *dlg)
{
	GeditDocument *doc;
	gchar *doc_name;
	gchar *str;
	gchar *markup_str;

	g_return_if_fail (dlg->unsaved_documents->data != NULL);
	doc = GEDIT_DOCUMENT (dlg->unsaved_documents->data);

	add_buttons (dlg);

	/* Primary message */
	doc_name = tepl_file_get_short_name (tepl_buffer_get_file (TEPL_BUFFER (doc)));
	str = g_markup_printf_escaped (_("Save changes to document “%s” before closing?"), doc_name);
	g_free (doc_name);

	markup_str = g_strconcat ("<span weight=\"bold\" size=\"larger\">", str, "</span>", NULL);
	g_free (str);

	gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dlg), markup_str);
	g_free (markup_str);

	/* Secondary message */
	str = get_text_secondary_label (doc);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), "%s", str);
	g_free (str);
}

static GtkWidget *
create_list_box (GeditCloseConfirmationDialog *dlg)
{
	GtkWidget *list_box;
	GList *l;

	list_box = gtk_list_box_new ();

	for (l = dlg->unsaved_documents; l != NULL; l = l->next)
	{
		GeditDocument *doc = l->data;
		gchar *name;
		GtkWidget *check_button;
		GtkWidget *row;

		name = tepl_file_get_short_name (tepl_buffer_get_file (TEPL_BUFFER (doc)));
		check_button = gtk_check_button_new_with_label (name);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), TRUE);
		gtk_widget_set_halign (check_button, GTK_ALIGN_START);
		g_free (name);

		row = gtk_list_box_row_new ();
		gtk_container_add (GTK_CONTAINER (row), check_button);
		gtk_widget_show_all (row);

		g_object_set_data_full (G_OBJECT (row),
		                        GEDIT_SAVE_DOCUMENT_KEY,
		                        g_object_ref (doc),
		                        (GDestroyNotify) g_object_unref);

		gtk_list_box_insert (GTK_LIST_BOX (list_box), row, -1);
	}

	return list_box;
}

static void
build_multiple_docs_dialog (GeditCloseConfirmationDialog *dlg)
{
	GtkWidget *content_area;
	GtkWidget *vbox;
	GtkWidget *select_label;
	GtkWidget *scrolledwindow;
	GtkWidget *secondary_label;
	gchar *str;
	gchar *markup_str;

	add_buttons (dlg);

	gtk_window_set_resizable (GTK_WINDOW (dlg), TRUE);

	/* Primary message */
	str = g_strdup_printf (
			ngettext ("There is %d document with unsaved changes. "
				  "Save changes before closing?",
				  "There are %d documents with unsaved changes. "
				  "Save changes before closing?",
				  g_list_length (dlg->unsaved_documents)),
			g_list_length (dlg->unsaved_documents));

	markup_str = g_strconcat ("<span weight=\"bold\" size=\"larger\">", str, "</span>", NULL);
	g_free (str);

	gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dlg), markup_str);
	g_free (markup_str);

	/* List of unsaved documents */
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dlg));
	gtk_box_set_spacing (GTK_BOX (content_area), 10);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
	gtk_widget_set_margin_start (vbox, 30);
	gtk_widget_set_margin_end (vbox, 30);
	gtk_widget_set_margin_bottom (vbox, 12);
	gtk_box_pack_start (GTK_BOX (content_area), vbox, TRUE, TRUE, 0);

	select_label = gtk_label_new_with_mnemonic (_("S_elect the documents you want to save:"));
	gtk_box_pack_start (GTK_BOX (vbox), select_label, FALSE, FALSE, 0);
	gtk_label_set_line_wrap (GTK_LABEL (select_label), TRUE);
	gtk_label_set_max_width_chars (GTK_LABEL (select_label), 72);
	gtk_widget_set_halign (select_label, GTK_ALIGN_START);

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
					     GTK_SHADOW_IN);
	gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scrolledwindow), 90);

	dlg->list_box = create_list_box (dlg);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), dlg->list_box);

	/* Secondary label */
	secondary_label = gtk_label_new (_("If you don’t save, "
					   "all your changes will be permanently lost."));
	gtk_box_pack_start (GTK_BOX (vbox), secondary_label, FALSE, FALSE, 0);
	gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
	gtk_widget_set_halign (secondary_label, GTK_ALIGN_CENTER);
	gtk_widget_set_valign (secondary_label, GTK_ALIGN_START);
	gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_max_width_chars (GTK_LABEL (secondary_label), 72);

	gtk_label_set_mnemonic_widget (GTK_LABEL (select_label), dlg->list_box);

	gtk_widget_show_all (vbox);
}

static void
set_unsaved_document (GeditCloseConfirmationDialog *dlg,
		      const GList                  *list)
{
	g_return_if_fail (list != NULL);

	g_return_if_fail (dlg->unsaved_documents == NULL);

	dlg->unsaved_documents = g_list_copy ((GList *)list);

	if (GET_MODE (dlg) == SINGLE_DOC_MODE)
	{
		build_single_doc_dialog (dlg);
	}
	else
	{
		build_multiple_docs_dialog (dlg);
	}
}

const GList *
gedit_close_confirmation_dialog_get_unsaved_documents (GeditCloseConfirmationDialog *dlg)
{
	g_return_val_if_fail (GEDIT_IS_CLOSE_CONFIRMATION_DIALOG (dlg), NULL);

	return dlg->unsaved_documents;
}

/* ex:set ts=8 noet: */
