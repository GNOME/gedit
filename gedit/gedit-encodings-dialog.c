/*
 * gedit-encodings-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2002-2005 Paolo Maggi
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

#include "gedit-encodings-dialog.h"

#include <string.h>
#include <glib/gi18n.h>
#include <gtksourceview/gtksource.h>

#include "gedit-settings.h"

typedef enum _State
{
	STATE_UNMODIFIED,
	STATE_MODIFIED,
	STATE_RESET
} State;

struct _GeditEncodingsDialog
{
	GtkDialog parent_instance;

	GSettings *enc_settings;

	/* Available encodings */
	GtkListStore *liststore_available;
	GtkTreeModelSort *sort_available;
	GtkTreeView *treeview_available;
	GtkWidget *add_button;

	/* Chosen encodings */
	GtkListStore *liststore_chosen;
	GtkTreeView *treeview_chosen;
	GtkWidget *remove_button;
	GtkWidget *up_button;
	GtkWidget *down_button;
	GtkWidget *reset_button;

	State state;
};

enum
{
	COLUMN_NAME,
	COLUMN_CHARSET,
	COLUMN_ENCODING,
	N_COLUMNS
};

G_DEFINE_TYPE (GeditEncodingsDialog, gedit_encodings_dialog, GTK_TYPE_DIALOG)

static void
set_modified (GeditEncodingsDialog *dialog)
{
	dialog->state = STATE_MODIFIED;
	gtk_widget_set_sensitive (dialog->reset_button, TRUE);
}

static void
append_encoding (GtkListStore            *liststore,
		 const GtkSourceEncoding *encoding)
{
	GtkTreeIter iter;

	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter,
			    COLUMN_NAME, gtk_source_encoding_get_name (encoding),
			    COLUMN_ENCODING, encoding,
			    -1);

	if (encoding == gtk_source_encoding_get_current ())
	{
		gchar *charset = g_strdup_printf (_("%s (Current Locale)"),
						  gtk_source_encoding_get_charset (encoding));

		gtk_list_store_set (liststore, &iter,
				    COLUMN_CHARSET, charset,
				    -1);

		g_free (charset);
	}
	else
	{
		gtk_list_store_set (liststore, &iter,
				    COLUMN_CHARSET, gtk_source_encoding_get_charset (encoding),
				    -1);
	}
}

static void
init_liststores (GeditEncodingsDialog *dialog,
		 gboolean              reset)
{
	gboolean default_candidates;
	GSList *chosen_encodings;
	GSList *all_encodings;
	GSList *l;

	/* Chosen encodings */

	if (reset)
	{
		chosen_encodings = gtk_source_encoding_get_default_candidates ();
		default_candidates = TRUE;
	}
	else
	{
		chosen_encodings = gedit_settings_get_candidate_encodings (&default_candidates);
	}

	gtk_widget_set_sensitive (dialog->reset_button, !default_candidates);

	for (l = chosen_encodings; l != NULL; l = l->next)
	{
		const GtkSourceEncoding *cur_encoding = l->data;
		append_encoding (dialog->liststore_chosen, cur_encoding);
	}

	/* Available encodings */

	all_encodings = gtk_source_encoding_get_all ();

	for (l = chosen_encodings; l != NULL; l = l->next)
	{
		const GtkSourceEncoding *chosen_encoding = l->data;
		all_encodings = g_slist_remove (all_encodings, chosen_encoding);
	}

	for (l = all_encodings; l != NULL; l = l->next)
	{
		const GtkSourceEncoding *cur_encoding = l->data;
		append_encoding (dialog->liststore_available, cur_encoding);
	}

	g_slist_free (chosen_encodings);
	g_slist_free (all_encodings);
}

static void
reset_dialog_response_cb (GtkDialog            *msg_dialog,
			  gint                  response,
			  GeditEncodingsDialog *dialog)
{
	if (response == GTK_RESPONSE_ACCEPT)
	{
		gtk_list_store_clear (dialog->liststore_available);
		gtk_list_store_clear (dialog->liststore_chosen);

		init_liststores (dialog, TRUE);
		dialog->state = STATE_RESET;
	}

	gtk_widget_destroy (GTK_WIDGET (msg_dialog));
}

static void
reset_button_clicked_cb (GtkWidget            *button,
			 GeditEncodingsDialog *dialog)
{
	GtkDialog *msg_dialog;

	msg_dialog = GTK_DIALOG (gtk_message_dialog_new (GTK_WINDOW (dialog),
							 GTK_DIALOG_DESTROY_WITH_PARENT |
							 GTK_DIALOG_MODAL,
							 GTK_MESSAGE_QUESTION,
							 GTK_BUTTONS_NONE,
							 "%s",
							 _("Do you really want to reset the "
							   "character encodings’ preferences?")));

	gtk_dialog_add_buttons (msg_dialog,
				_("_Cancel"), GTK_RESPONSE_CANCEL,
				_("_Reset"), GTK_RESPONSE_ACCEPT,
				NULL);

	g_signal_connect (msg_dialog,
			  "response",
			  G_CALLBACK (reset_dialog_response_cb),
			  dialog);

	gtk_widget_show_all (GTK_WIDGET (msg_dialog));
}

static GSList *
get_chosen_encodings_list (GeditEncodingsDialog *dialog)
{
	GtkTreeModel *model = GTK_TREE_MODEL (dialog->liststore_chosen);
	GtkTreeIter iter;
	gboolean iter_set;
	GSList *ret = NULL;

	iter_set = gtk_tree_model_get_iter_first (model, &iter);

	while (iter_set)
	{
		const GtkSourceEncoding *encoding = NULL;

		gtk_tree_model_get (model, &iter,
				    COLUMN_ENCODING, &encoding,
				    -1);

		ret = g_slist_prepend (ret, (gpointer)encoding);

		iter_set = gtk_tree_model_iter_next (model, &iter);
	}

	return g_slist_reverse (ret);
}

static gchar **
encoding_list_to_strv (const GSList *enc_list)
{
	GSList *l;
	GPtrArray *array;

	array = g_ptr_array_sized_new (g_slist_length ((GSList *)enc_list) + 1);

	for (l = (GSList *)enc_list; l != NULL; l = l->next)
	{
		const GtkSourceEncoding *enc = l->data;
		const gchar *charset = gtk_source_encoding_get_charset (enc);

		g_return_val_if_fail (charset != NULL, NULL);

		g_ptr_array_add (array, g_strdup (charset));
	}

	g_ptr_array_add (array, NULL);

	return (gchar **)g_ptr_array_free (array, FALSE);
}

static void
apply_settings (GeditEncodingsDialog *dialog)
{
	switch (dialog->state)
	{
		case STATE_MODIFIED:
		{
			GSList *enc_list;
			gchar **enc_strv;

			enc_list = get_chosen_encodings_list (dialog);
			enc_strv = encoding_list_to_strv (enc_list);

			g_settings_set_strv (dialog->enc_settings,
					     GEDIT_SETTINGS_CANDIDATE_ENCODINGS,
					     (const gchar * const *)enc_strv);

			g_slist_free (enc_list);
			g_strfreev (enc_strv);
			break;
		}

		case STATE_RESET:
			g_settings_reset (dialog->enc_settings,
					  GEDIT_SETTINGS_CANDIDATE_ENCODINGS);
			break;

		case STATE_UNMODIFIED:
			/* Do nothing. */
			break;

		default:
			g_assert_not_reached ();

	}
}

static void
gedit_encodings_dialog_response (GtkDialog *gtk_dialog,
                                 gint       response_id)
{
	GeditEncodingsDialog *dialog = GEDIT_ENCODINGS_DIALOG (gtk_dialog);

	switch (response_id)
	{
		case GTK_RESPONSE_APPLY:
			apply_settings (dialog);
			break;

		case GTK_RESPONSE_CANCEL:
		default:
			/* Do nothing */
			break;
	}
}

static void
gedit_encodings_dialog_dispose (GObject *object)
{
	GeditEncodingsDialog *dialog = GEDIT_ENCODINGS_DIALOG (object);

	g_clear_object (&dialog->enc_settings);
	g_clear_object (&dialog->add_button);
	g_clear_object (&dialog->remove_button);
	g_clear_object (&dialog->up_button);
	g_clear_object (&dialog->down_button);
	g_clear_object (&dialog->reset_button);

	G_OBJECT_CLASS (gedit_encodings_dialog_parent_class)->dispose (object);
}

static void
gedit_encodings_dialog_class_init (GeditEncodingsDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

	object_class->dispose = gedit_encodings_dialog_dispose;

	dialog_class->response = gedit_encodings_dialog_response;

	/* Bind class to template */
	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/gedit/ui/gedit-encodings-dialog.ui");
	gtk_widget_class_bind_template_child (widget_class, GeditEncodingsDialog, liststore_available);
	gtk_widget_class_bind_template_child (widget_class, GeditEncodingsDialog, liststore_chosen);
	gtk_widget_class_bind_template_child (widget_class, GeditEncodingsDialog, sort_available);
	gtk_widget_class_bind_template_child (widget_class, GeditEncodingsDialog, treeview_available);
	gtk_widget_class_bind_template_child (widget_class, GeditEncodingsDialog, treeview_chosen);
	gtk_widget_class_bind_template_child_full (widget_class, "scrolledwindow_available", FALSE, 0);
	gtk_widget_class_bind_template_child_full (widget_class, "scrolledwindow_chosen", FALSE, 0);
	gtk_widget_class_bind_template_child_full (widget_class, "toolbar_available", FALSE, 0);
	gtk_widget_class_bind_template_child_full (widget_class, "toolbar_chosen", FALSE, 0);
}

static void
update_add_button_sensitivity (GeditEncodingsDialog *dialog)
{
	GtkTreeSelection *selection;
	gint count;

	selection = gtk_tree_view_get_selection (dialog->treeview_available);
	count = gtk_tree_selection_count_selected_rows (selection);
	gtk_widget_set_sensitive (dialog->add_button, count > 0);
}

static void
update_remove_button_sensitivity (GeditEncodingsDialog *dialog)
{
	const GtkSourceEncoding *utf8_encoding;
	const GtkSourceEncoding *current_encoding;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *selected_rows;
	GList *l;
	gboolean sensitive;

	utf8_encoding = gtk_source_encoding_get_utf8 ();
	current_encoding = gtk_source_encoding_get_current ();

	selection = gtk_tree_view_get_selection (dialog->treeview_chosen);

	selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);
	g_return_if_fail (model == GTK_TREE_MODEL (dialog->liststore_chosen));

	sensitive = FALSE;
	for (l = selected_rows; l != NULL; l = l->next)
	{
		GtkTreePath *path = l->data;
		GtkTreeIter iter;
		const GtkSourceEncoding *encoding = NULL;

		if (!gtk_tree_model_get_iter (model, &iter, path))
		{
			g_warning ("Remove button: invalid path");
			continue;
		}

		gtk_tree_model_get (model, &iter,
				    COLUMN_ENCODING, &encoding,
				    -1);

		/* If at least one encoding other than UTF-8 or current is
		 * selected, set the Remove button as sensitive. But if UTF-8 or
		 * current is selected, it won't be removed. So Ctrl+A works
		 * fine to remove (almost) all encodings in one go.
		 */
		if (encoding != utf8_encoding &&
		    encoding != current_encoding)
		{
			sensitive = TRUE;
			break;
		}
	}

	gtk_widget_set_sensitive (dialog->remove_button, sensitive);

	g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
}

static void
update_up_down_buttons_sensitivity (GeditEncodingsDialog *dialog)
{
	GtkTreeSelection *selection;
	gint count;
	GList *selected_rows;
	GtkTreeModel *model;
	GtkTreePath *path;
	gint *indices;
	gint depth;
	gint items_count;
	gboolean first_item_selected;
	gboolean last_item_selected;

	selection = gtk_tree_view_get_selection (dialog->treeview_chosen);
	count = gtk_tree_selection_count_selected_rows (selection);

	if (count != 1)
	{
		gtk_widget_set_sensitive (dialog->up_button, FALSE);
		gtk_widget_set_sensitive (dialog->down_button, FALSE);
		return;
	}

	selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);
	g_assert (g_list_length (selected_rows) == 1);

	path = selected_rows->data;
	indices = gtk_tree_path_get_indices_with_depth (path, &depth);
	g_assert (depth == 1);

	items_count = gtk_tree_model_iter_n_children (model, NULL);

	first_item_selected = indices[0] == 0;
	last_item_selected = indices[0] == (items_count - 1);

	gtk_widget_set_sensitive (dialog->up_button, !first_item_selected);
	gtk_widget_set_sensitive (dialog->down_button, !last_item_selected);

	g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
}

static void
update_chosen_buttons_sensitivity (GeditEncodingsDialog *dialog)
{
	update_remove_button_sensitivity (dialog);
	update_up_down_buttons_sensitivity (dialog);
}

/* Removes all @paths from @orig, and append them at the end of @dest in the
 * same order.
 */
static void
transfer_encodings (GList        *paths,
		    GtkListStore *orig,
		    GtkListStore *dest)
{
	GtkTreeModel *model_orig = GTK_TREE_MODEL (orig);
	GList *refs = NULL;
	GList *l;

	for (l = paths; l != NULL; l = l->next)
	{
		GtkTreePath *path = l->data;
		refs = g_list_prepend (refs, gtk_tree_row_reference_new (model_orig, path));
	}

	refs = g_list_reverse (refs);

	for (l = refs; l != NULL; l = l->next)
	{
		GtkTreeRowReference *ref = l->data;
		GtkTreePath *path;
		GtkTreeIter iter;
		const GtkSourceEncoding *encoding = NULL;

		path = gtk_tree_row_reference_get_path (ref);

		if (!gtk_tree_model_get_iter (model_orig, &iter, path))
		{
			gtk_tree_path_free (path);
			g_warning ("Remove encoding: invalid path");
			continue;
		}

		/* Transfer encoding */
		gtk_tree_model_get (model_orig, &iter,
				    COLUMN_ENCODING, &encoding,
				    -1);

		append_encoding (dest, encoding);

		gtk_list_store_remove (orig, &iter);

		gtk_tree_path_free (path);
	}

	g_list_free_full (refs, (GDestroyNotify) gtk_tree_row_reference_free);
}

static void
add_button_clicked_cb (GtkWidget            *button,
		       GeditEncodingsDialog *dialog)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *filter_paths;
	GList *children_paths = NULL;
	GList *l;

	selection = gtk_tree_view_get_selection (dialog->treeview_available);
	filter_paths = gtk_tree_selection_get_selected_rows (selection, &model);

	g_return_if_fail (model == GTK_TREE_MODEL (dialog->sort_available));

	for (l = filter_paths; l != NULL; l = l->next)
	{
		GtkTreePath *filter_path = l->data;
		GtkTreePath *child_path;

		child_path = gtk_tree_model_sort_convert_path_to_child_path (dialog->sort_available,
		                                                             filter_path);

		children_paths = g_list_prepend (children_paths, child_path);
	}

	children_paths = g_list_reverse (children_paths);

	transfer_encodings (children_paths,
	                    dialog->liststore_available,
	                    dialog->liststore_chosen);

	set_modified (dialog);

	/* For the treeview_available, it's more natural to unselect the added
	 * encodings.
	 * Note that when removing encodings from treeview_chosen, it is
	 * desirable to keep a selection, so we can remove several elements in a
	 * row (if the user doesn't know that several elements can be selected
	 * at once with Ctrl or Shift).
	 */
	gtk_tree_selection_unselect_all (selection);

	g_list_free_full (filter_paths, (GDestroyNotify) gtk_tree_path_free);
	g_list_free_full (children_paths, (GDestroyNotify) gtk_tree_path_free);
}

static void
remove_button_clicked_cb (GtkWidget            *button,
			  GeditEncodingsDialog *dialog)
{
	const GtkSourceEncoding *utf8_encoding;
	const GtkSourceEncoding *current_encoding;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *selected_rows;
	GList *to_remove = NULL;
	GList *l;

	utf8_encoding = gtk_source_encoding_get_utf8 ();
	current_encoding = gtk_source_encoding_get_current ();

	selection = gtk_tree_view_get_selection (dialog->treeview_chosen);
	selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);

	g_return_if_fail (model == GTK_TREE_MODEL (dialog->liststore_chosen));

	/* Ensure that UTF-8 and the current locale encodings cannot be removed. */
	for (l = selected_rows; l != NULL; l = l->next)
	{
		GtkTreePath *path = l->data;
		GtkTreeIter iter;
		const GtkSourceEncoding *encoding = NULL;

		if (!gtk_tree_model_get_iter (model, &iter, path))
		{
			gtk_tree_path_free (path);
			g_warning ("Remove button: invalid path");
			continue;
		}

		gtk_tree_model_get (model, &iter,
				    COLUMN_ENCODING, &encoding,
				    -1);

		if (encoding == utf8_encoding ||
		    encoding == current_encoding)
		{
			gtk_tree_path_free (path);
		}
		else
		{
			to_remove = g_list_prepend (to_remove, path);
		}
	}

	to_remove = g_list_reverse (to_remove);

	transfer_encodings (to_remove,
	                    dialog->liststore_chosen,
	                    dialog->liststore_available);

	set_modified (dialog);

	g_list_free (selected_rows);
	g_list_free_full (to_remove, (GDestroyNotify) gtk_tree_path_free);
}

static void
up_button_clicked_cb (GtkWidget            *button,
		      GeditEncodingsDialog *dialog)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *selected_rows;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeIter prev_iter;

	selection = gtk_tree_view_get_selection (dialog->treeview_chosen);
	selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);

	g_return_if_fail (model == GTK_TREE_MODEL (dialog->liststore_chosen));
	g_return_if_fail (g_list_length (selected_rows) == 1);

	path = selected_rows->data;
	if (!gtk_tree_model_get_iter (model, &iter, path))
	{
		g_return_if_reached ();
	}

	prev_iter = iter;
	if (!gtk_tree_model_iter_previous (model, &prev_iter))
	{
		g_return_if_reached ();
	}

	gtk_list_store_move_before (dialog->liststore_chosen,
	                            &iter,
	                            &prev_iter);

	set_modified (dialog);

	update_chosen_buttons_sensitivity (dialog);

	g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
}

static void
down_button_clicked_cb (GtkWidget            *button,
			GeditEncodingsDialog *dialog)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *selected_rows;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeIter next_iter;

	selection = gtk_tree_view_get_selection (dialog->treeview_chosen);
	selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);

	g_return_if_fail (model == GTK_TREE_MODEL (dialog->liststore_chosen));
	g_return_if_fail (g_list_length (selected_rows) == 1);

	path = selected_rows->data;
	if (!gtk_tree_model_get_iter (model, &iter, path))
	{
		g_return_if_reached ();
	}

	next_iter = iter;
	if (!gtk_tree_model_iter_next (model, &next_iter))
	{
		g_return_if_reached ();
	}

	gtk_list_store_move_after (dialog->liststore_chosen,
	                           &iter,
	                           &next_iter);

	set_modified (dialog);

	update_chosen_buttons_sensitivity (dialog);

	g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
}

static void
init_toolbar_available (GeditEncodingsDialog *dialog)
{
	GtkWidget *scrolled_window;
	GtkToolbar *toolbar;
	GtkStyleContext *context;

	scrolled_window = GTK_WIDGET (gtk_widget_get_template_child (GTK_WIDGET (dialog),
								     GEDIT_TYPE_ENCODINGS_DIALOG,
								     "scrolledwindow_available"));

	toolbar = GTK_TOOLBAR (gtk_widget_get_template_child (GTK_WIDGET (dialog),
							      GEDIT_TYPE_ENCODINGS_DIALOG,
							      "toolbar_available"));

	context = gtk_widget_get_style_context (scrolled_window);
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_BOTTOM);

	context = gtk_widget_get_style_context (GTK_WIDGET (toolbar));
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_TOP);
	gtk_style_context_add_class (context, GTK_STYLE_CLASS_INLINE_TOOLBAR);

	/* Add button */
	dialog->add_button = GTK_WIDGET (gtk_tool_button_new (NULL, NULL));
	g_object_ref_sink (dialog->add_button);

	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (dialog->add_button), "list-add-symbolic");
	gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (dialog->add_button), _("Add"));

	gtk_toolbar_insert (toolbar, GTK_TOOL_ITEM (dialog->add_button), -1);

	g_signal_connect_object (dialog->add_button,
				 "clicked",
				 G_CALLBACK (add_button_clicked_cb),
				 dialog,
				 0);

	gtk_widget_show_all (GTK_WIDGET (toolbar));
}

static void
init_toolbar_chosen (GeditEncodingsDialog *dialog)
{
	GtkWidget *scrolled_window;
	GtkToolbar *toolbar;
	GtkStyleContext *context;
	GtkWidget *left_box;
	GtkWidget *right_box;
	GtkToolItem *left_group;
	GtkToolItem *right_group;
	GtkToolItem *separator;

	scrolled_window = GTK_WIDGET (gtk_widget_get_template_child (GTK_WIDGET (dialog),
								     GEDIT_TYPE_ENCODINGS_DIALOG,
								     "scrolledwindow_chosen"));

	toolbar = GTK_TOOLBAR (gtk_widget_get_template_child (GTK_WIDGET (dialog),
							      GEDIT_TYPE_ENCODINGS_DIALOG,
							      "toolbar_chosen"));

	context = gtk_widget_get_style_context (scrolled_window);
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_BOTTOM);

	context = gtk_widget_get_style_context (GTK_WIDGET (toolbar));
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_TOP);
	gtk_style_context_add_class (context, GTK_STYLE_CLASS_INLINE_TOOLBAR);

	/* Remove button */
	dialog->remove_button = gtk_button_new_from_icon_name ("list-remove-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
	g_object_ref_sink (dialog->remove_button);
	gtk_widget_set_tooltip_text (dialog->remove_button, _("Remove"));

	g_signal_connect_object (dialog->remove_button,
				 "clicked",
				 G_CALLBACK (remove_button_clicked_cb),
				 dialog,
				 0);

	/* Up button */
	dialog->up_button = gtk_button_new_from_icon_name ("go-up-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
	g_object_ref_sink (dialog->up_button);
	gtk_widget_set_tooltip_text (dialog->up_button, _("Move to a higher priority"));

	g_signal_connect_object (dialog->up_button,
				 "clicked",
				 G_CALLBACK (up_button_clicked_cb),
				 dialog,
				 0);

	/* Down button */
	dialog->down_button = gtk_button_new_from_icon_name ("go-down-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
	g_object_ref_sink (dialog->down_button);
	gtk_widget_set_tooltip_text (dialog->down_button, _("Move to a lower priority"));

	g_signal_connect_object (dialog->down_button,
				 "clicked",
				 G_CALLBACK (down_button_clicked_cb),
				 dialog,
				 0);

	/* Left group (with a trick for rounded borders) */
	left_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	left_group = gtk_tool_item_new ();
	gtk_box_pack_start (GTK_BOX (left_box), dialog->remove_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (left_box), dialog->up_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (left_box), dialog->down_button, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (left_group), left_box);
	gtk_toolbar_insert (toolbar, left_group, -1);

	/* Separator */
	separator = gtk_separator_tool_item_new ();
	gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (separator), FALSE);
	gtk_tool_item_set_expand (separator, TRUE);
	gtk_toolbar_insert (toolbar, separator, -1);

	/* Reset button */
	dialog->reset_button = gtk_button_new_with_mnemonic (_("_Reset"));
	g_object_ref_sink (dialog->reset_button);

	g_signal_connect_object (dialog->reset_button,
				 "clicked",
				 G_CALLBACK (reset_button_clicked_cb),
				 dialog,
				 0);

	/* Right group */
	right_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	right_group = gtk_tool_item_new ();
	gtk_box_pack_start (GTK_BOX (right_box), dialog->reset_button, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (right_group), right_box);
	gtk_toolbar_insert (toolbar, right_group, -1);

	gtk_widget_show_all (GTK_WIDGET (toolbar));
}

static void
gedit_encodings_dialog_init (GeditEncodingsDialog *dialog)
{
	GtkTreeSelection *selection;

	dialog->enc_settings = g_settings_new ("org.gnome.gedit.preferences.encodings");

	gtk_widget_init_template (GTK_WIDGET (dialog));

	init_toolbar_available (dialog);
	init_toolbar_chosen (dialog);
	init_liststores (dialog, FALSE);
	dialog->state = STATE_UNMODIFIED;

	/* Available encodings */
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (dialog->sort_available),
					      COLUMN_NAME,
					      GTK_SORT_ASCENDING);

	selection = gtk_tree_view_get_selection (dialog->treeview_available);

	g_signal_connect_swapped (selection,
				  "changed",
				  G_CALLBACK (update_add_button_sensitivity),
				  dialog);

	update_add_button_sensitivity (dialog);

	/* Chosen encodings */
	selection = gtk_tree_view_get_selection (dialog->treeview_chosen);

	g_signal_connect_swapped (selection,
				  "changed",
				  G_CALLBACK (update_chosen_buttons_sensitivity),
				  dialog);

	update_chosen_buttons_sensitivity (dialog);
}

GtkWidget *
gedit_encodings_dialog_new (void)
{
	return g_object_new (GEDIT_TYPE_ENCODINGS_DIALOG,
			     "use-header-bar", TRUE,
			     NULL);
}

/* ex:set ts=8 noet: */
