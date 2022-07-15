/*
 * gedit-open-document-selector.c
 * This file is part of gedit
 *
 * Copyright (C) 2014 - Sébastien Lafargue
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

#include "gedit-open-document-selector.h"
#include "gedit-open-document-selector-store.h"
#include "gedit-open-document-selector-helper.h"

#include <time.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <glib/gstdio.h>
#include <gio/gio.h>

#include "gedit-recent.h"
#include "gedit-utils.h"
#include "gedit-window.h"
#include "gedit-debug.h"

struct _GeditOpenDocumentSelector
{
	GtkBox parent_instance;

	GeditWindow *window;
	GtkWidget *search_entry;

	GtkWidget *open_button;
	GtkWidget *treeview;
	GtkListStore *liststore;
	GtkCellRenderer *name_renderer;
	GtkCellRenderer *path_renderer;
	GtkWidget *placeholder_box;
	GtkWidget *scrolled_window;

	guint populate_listbox_id;

	GdkRGBA name_label_color;
	PangoFontDescription *name_font;
	GdkRGBA path_label_color;
	PangoFontDescription *path_font;
	gchar *match_markup_color;

	GeditOpenDocumentSelectorStore *selector_store;
	GList *recent_items;
	GList *home_dir_items;
	GList *desktop_dir_items;
	GList *local_bookmarks_dir_items;
	GList *file_browser_root_items;
	GList *active_doc_dir_items;
	GList *current_docs_items;
	GList *all_items;
};

typedef enum
{
	SELECTOR_TAG_NONE,
	SELECTOR_TAG_MATCH
} SelectorTag;

enum
{
	NAME_COLUMN,
	PATH_COLUMN,
	URI_COLUMN,
	N_COLUMNS
};

enum
{
	PROP_0,
	PROP_WINDOW,
	LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

enum
{
	SELECTOR_FILE_ACTIVATED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

/* Value 0xFF is reserved to mark the end of the array */
#define BYTE_ARRAY_END 0xFF

#define OPEN_DOCUMENT_SELECTOR_WIDTH 400
#define OPEN_DOCUMENT_SELECTOR_MAX_VISIBLE_ROWS 10

G_DEFINE_TYPE (GeditOpenDocumentSelector, gedit_open_document_selector, GTK_TYPE_BOX)

static inline const guint8 *
get_byte_run (const guint8 *byte_array,
              gsize        *count,
              SelectorTag  *tag)
{
	guint8 tag_found;
	gsize c = 1;

	tag_found = *byte_array++;

	while ( *byte_array != BYTE_ARRAY_END && *byte_array == tag_found)
	{
		c++;
		byte_array++;
	}

	*count = c;
	*tag = tag_found;

	return ( *byte_array != BYTE_ARRAY_END) ? byte_array : NULL;
}

static gchar*
get_markup_from_tagged_byte_array (GeditOpenDocumentSelector *selector,
                                   const gchar               *str,
                                   const guint8              *byte_array)
{
	gchar *txt;
	GString *string;
	gchar *result_str;
	SelectorTag tag;
	gsize count;

	string = g_string_sized_new (255);

	while (TRUE)
	{
		byte_array = get_byte_run (byte_array, &count, &tag);
		txt = g_markup_escape_text (str, count);
		if (tag == SELECTOR_TAG_MATCH)
		{
			g_string_append (string, selector->match_markup_color);
			g_string_append (string, txt);
			g_string_append (string, "</span>");
		}
		else
		{
			g_string_append (string, txt);
		}

		g_free (txt);

		if (!byte_array)
		{
			break;
		}

		str = (const gchar *)((gsize)str + count);
	}

	result_str = g_string_free (string, FALSE);
	return result_str;
}

static guint8 *
get_tagged_byte_array (const gchar *uri,
                       GRegex      *filter_regex)
{
	guint8 *byte_array;
	gsize uri_len;
	GMatchInfo *match_info;
	gboolean no_match = TRUE;

	g_return_val_if_fail (uri != NULL, NULL);

	uri_len = strlen (uri);
	byte_array = g_malloc0 (uri_len + 1);
	byte_array[uri_len] = BYTE_ARRAY_END;

	if (g_regex_match (filter_regex, uri, 0, &match_info) == TRUE)
	{
		while (g_match_info_matches (match_info) == TRUE)
		{
			guint8 *p;
			gint match_len;
			gint start_pos;
			gint end_pos;

			if (g_match_info_fetch_pos (match_info, 0, &start_pos, &end_pos) == TRUE)
			{
				match_len = end_pos - start_pos;
				no_match = FALSE;

				p = (guint8 *)((gsize)byte_array + start_pos);
				memset (p, SELECTOR_TAG_MATCH, match_len);
			}

			g_match_info_next (match_info, NULL);
		}
	}

	g_match_info_free (match_info);

	if (no_match)
	{
		g_free (byte_array);
		return NULL;
	}

	return byte_array;
}

static void
get_markup_for_path_and_name (GeditOpenDocumentSelector  *selector,
                              GRegex                     *filter_regex,
                              const gchar                *src_path,
                              const gchar                *src_name,
                              gchar                     **dst_path,
                              gchar                     **dst_name)
{
	gchar *filename;
	gsize path_len;
	gsize name_len;
	gsize path_separator_len;
	guint8 *byte_array;
	guint8 *path_byte_array;
	guint8 *name_byte_array;

	filename = g_build_filename (src_path, src_name, NULL);

	path_len = g_utf8_strlen (src_path, -1);
	name_len = g_utf8_strlen (src_name, -1);
	path_separator_len = g_utf8_strlen (filename, -1) - ( path_len + name_len);

	byte_array = get_tagged_byte_array (filename, filter_regex);
	if (byte_array)
	{
		path_byte_array = g_memdup (byte_array, path_len + 1);
		path_byte_array[path_len] = BYTE_ARRAY_END;

		/* name_byte_array is part of byte_array, so released with it */
		name_byte_array = (guint8 *)((gsize)byte_array + path_len + path_separator_len);

		*dst_path = get_markup_from_tagged_byte_array (selector, src_path, path_byte_array);
		*dst_name = get_markup_from_tagged_byte_array (selector, src_name, name_byte_array);

		g_free (byte_array);
		g_free (path_byte_array);
	}
	else
	{
		*dst_path = g_strdup (src_path);
		*dst_name = g_strdup (src_name);
	}

	g_free (filename);
}

static void
create_row (GeditOpenDocumentSelector *selector,
            const FileItem            *item,
            GRegex                    *filter_regex)
{
	GtkTreeIter iter;
	gchar *uri;
	gchar *dst_path;
	gchar *dst_name;

	uri =item->uri;

	if (filter_regex)
	{
		get_markup_for_path_and_name (selector,
					      filter_regex,
		                              (const gchar *)item->path,
		                              (const gchar *)item->name,
		                              &dst_path,
		                              &dst_name);
	}
	else
	{
		dst_path = g_markup_escape_text (item->path, -1);
		dst_name = g_markup_escape_text (item->name, -1);
	}

	gtk_list_store_append (selector->liststore, &iter);
	gtk_list_store_set (selector->liststore, &iter,
	                    URI_COLUMN, uri,
	                    NAME_COLUMN, dst_name,
	                    PATH_COLUMN, dst_path,
	                    -1);

	g_free (dst_path);
	g_free (dst_name);
}

static gint
sort_items_by_mru (FileItem *a,
                   FileItem *b,
                   gpointer  unused G_GNUC_UNUSED)
{
	g_assert (a != NULL && b != NULL);

	return g_date_time_compare (b->accessed, a->accessed);
}

static GList *
compute_all_items_list (GeditOpenDocumentSelector *selector)
{
	GList *recent_items;
	GList *home_dir_items;
	GList *desktop_dir_items;
	GList *local_bookmarks_dir_items;
	GList *file_browser_root_items;
	GList *active_doc_dir_items;
	GList *current_docs_items;
	GList *all_items = NULL;

	/* Copy/concat the whole list */
	recent_items = gedit_open_document_selector_copy_file_items_list ((const GList *)selector->recent_items);
	home_dir_items = gedit_open_document_selector_copy_file_items_list ((const GList *)selector->home_dir_items);
	desktop_dir_items = gedit_open_document_selector_copy_file_items_list ((const GList *)selector->desktop_dir_items);
	local_bookmarks_dir_items = gedit_open_document_selector_copy_file_items_list ((const GList *)selector->local_bookmarks_dir_items);
	file_browser_root_items = gedit_open_document_selector_copy_file_items_list ((const GList *)selector->file_browser_root_items);
	active_doc_dir_items = gedit_open_document_selector_copy_file_items_list ((const GList *)selector->active_doc_dir_items);
	current_docs_items = gedit_open_document_selector_copy_file_items_list ((const GList *)selector->current_docs_items);

	if (selector->all_items)
	{
		gedit_open_document_selector_free_file_items_list (selector->all_items);
		selector->all_items = NULL;
	}

	all_items = g_list_concat (all_items, recent_items);
	all_items = g_list_concat (all_items, home_dir_items);
	all_items = g_list_concat (all_items, desktop_dir_items);
	all_items = g_list_concat (all_items, local_bookmarks_dir_items);
	all_items = g_list_concat (all_items, file_browser_root_items);
	all_items = g_list_concat (all_items, active_doc_dir_items);
	all_items = g_list_concat (all_items, current_docs_items);

	return all_items;
}

static GList *
clamp_recent_items_list (GList *recent_items,
                         gint   limit)
{
	GList *recent_items_capped = NULL;
	GList *l;
	FileItem *item;

	l = recent_items;
	while (limit > 0 && l != NULL)
	{
		item = gedit_open_document_selector_copy_fileitem_item (l->data);
		recent_items_capped = g_list_prepend (recent_items_capped, item);
		l = l->next;
		limit -= 1;
	}

	recent_items_capped = g_list_reverse (recent_items_capped);
	return recent_items_capped;
}

/* Setup the fileitem, depending uri's scheme
 * Return a string to search in.
 */
static gchar *
fileitem_setup (FileItem *item)
{
	gchar *scheme;
	gchar *filename;
	gchar *normalized_filename = NULL;
	gchar *candidate = NULL;
	gchar *path;
	gchar *name;

	scheme = g_uri_parse_scheme (item->uri);
	if (g_strcmp0 (scheme, "file") == 0)
	{
		filename = g_filename_from_uri ((const gchar *)item->uri, NULL, NULL);
		if (filename)
		{
			path = g_path_get_dirname (filename);
			item->path = g_filename_to_utf8 (path, -1, NULL, NULL, NULL);
			g_free (path);

			name = g_path_get_basename (filename);
			item->name = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
			g_free (name);

			normalized_filename = g_utf8_normalize (filename, -1, G_NORMALIZE_ALL);
			g_free (filename);
		}
	}
	else
	{
		GFile *file;
		gchar *parse_name;

		file = g_file_new_for_uri (item->uri);
		item->path = gedit_utils_location_get_dirname_for_display (file);
		item->name  = gedit_utils_basename_for_display (file);
		parse_name = g_file_get_parse_name (file);
		g_object_unref (file);

		normalized_filename = g_utf8_normalize (parse_name, -1, G_NORMALIZE_ALL);
		g_free (parse_name);
	}

        if (normalized_filename)
	{
		candidate = g_utf8_casefold (normalized_filename, -1);
		g_free (normalized_filename);
	}

	g_free (scheme);

	return candidate;
}

static inline gboolean
is_filter_in_candidate (const gchar *candidate,
                        const gchar *filter)
{
	gchar *candidate_fold;
	gboolean ret;

	g_assert (candidate != NULL);
	g_assert (filter != NULL);

	candidate_fold = g_utf8_casefold (candidate, -1);
	ret = (strstr (candidate_fold, filter) != NULL);

	g_free (candidate_fold);
	return ret;
}

/* If filter == NULL then items are
 * not checked against the filter.
 */
static GList *
fileitem_list_filter (GList       *items,
                      const gchar *filter)
{
	GList *new_items = NULL;
	GList *l;
	gchar *filter_fold = NULL;

	if (filter != NULL)
		filter_fold = g_utf8_casefold (filter, -1);

	for (l = items; l != NULL; l = l->next)
	{
		FileItem *item;
		gchar *candidate;

		item = l->data;
		candidate = fileitem_setup (item);
		if (candidate != NULL)
		{
			if (filter == NULL || is_filter_in_candidate (candidate, filter_fold))
			{
				new_items = g_list_prepend (new_items,
					                    gedit_open_document_selector_copy_fileitem_item (item));
			}

			g_free (candidate);
		}
	}

	g_free (filter_fold);
	new_items = g_list_reverse (new_items);
	return new_items;
}

/* Remove duplicated, the HEAD of the list never change,
 * the list passed in is modified.
 */
static void
fileitem_list_remove_duplicates (GList *items)
{
	GList *l;
	G_GNUC_UNUSED GList *dummy_ptr;

	l = items;
	while (l != NULL)
	{
		gchar *l_uri, *l1_uri;
		GList *l1;

		if ((l1 = l->next) == NULL)
		{
			break;
		}

		l_uri = ((FileItem *)l->data)->uri;
		l1_uri = ((FileItem *)l1->data)->uri;
		if (g_strcmp0 (l_uri, l1_uri) == 0)
		{
			gedit_open_document_selector_free_fileitem_item ((FileItem *)l1->data);
			dummy_ptr = g_list_delete_link (items, l1);
		}
		else
		{
			l = l->next;
		}
	}
}

static gboolean
real_populate_liststore (gpointer data)
{
	GeditOpenDocumentSelector *selector = GEDIT_OPEN_DOCUMENT_SELECTOR (data);
	GeditOpenDocumentSelectorStore *selector_store;
	GList *l;
	GList *filter_items = NULL;
	gchar *filter;
	GRegex *filter_regex = NULL;

	DEBUG_SELECTOR_TIMER_DECL
	DEBUG_SELECTOR_TIMER_NEW

	gtk_list_store_clear (selector->liststore);

	selector_store = selector->selector_store;
	filter = gedit_open_document_selector_store_get_filter (selector_store);
	if (filter && *filter != '\0')
	{
		DEBUG_SELECTOR (g_print ("Selector(%p): populate liststore: all lists\n", selector););

		filter_items = fileitem_list_filter (selector->all_items, (const gchar *)filter);
		filter_items = g_list_sort_with_data (filter_items, (GCompareDataFunc)sort_items_by_mru, NULL);
		fileitem_list_remove_duplicates (filter_items);

		filter_regex = g_regex_new (filter, G_REGEX_CASELESS, 0, NULL);
	}
	else
	{
		gint recent_limit;
		GList *recent_items;

		DEBUG_SELECTOR (g_print ("Selector(%p): populate liststore: recent files list\n", selector););

		recent_limit = gedit_open_document_selector_store_get_recent_limit (selector_store);

		if (recent_limit > 0 )
		{
			recent_items = fileitem_list_filter (selector->recent_items, NULL);
			filter_items = clamp_recent_items_list (recent_items, recent_limit);
			gedit_open_document_selector_free_file_items_list (recent_items);
		}
		else
		{
			filter_items = fileitem_list_filter (selector->recent_items, NULL);
		}
	}

	g_free (filter);

	DEBUG_SELECTOR (g_print ("Selector(%p): populate liststore: length:%i\n",
	                         selector, g_list_length (filter_items)););

	/* Show the placeholder if no results, show the treeview otherwise */
	gtk_widget_set_visible (selector->scrolled_window, (filter_items != NULL));
	gtk_widget_set_visible (selector->placeholder_box, (filter_items == NULL));

	for (l = filter_items; l != NULL; l = l->next)
	{
		FileItem *item;

		item = l->data;
		create_row (selector, (const FileItem *)item, filter_regex);
	}

	if (filter_regex)
	{
		g_regex_unref (filter_regex);
	}

	gedit_open_document_selector_free_file_items_list (filter_items);

	DEBUG_SELECTOR (g_print ("Selector(%p): populate liststore: time:%lf\n\n",
	                          selector, DEBUG_SELECTOR_TIMER_GET););
	DEBUG_SELECTOR_TIMER_DESTROY

	selector->populate_listbox_id = 0;
	return G_SOURCE_REMOVE;
}

static void
populate_liststore (GeditOpenDocumentSelector *selector)
{
	/* Populate requests are compressed */
	if (selector->populate_listbox_id != 0)
	{
		DEBUG_SELECTOR (g_print ("Selector(%p): populate liststore: idle\n", selector););
		return;
	}

	DEBUG_SELECTOR (g_print ("Selector(%p): populate liststore: scheduled\n", selector););
	selector->populate_listbox_id = gdk_threads_add_idle_full (G_PRIORITY_HIGH_IDLE + 30,
								   real_populate_liststore,
								   selector,
								   NULL);
}

static gboolean
on_treeview_key_press (GtkTreeView               *treeview,
                       GdkEventKey               *event,
                       GeditOpenDocumentSelector *selector)
{
	guint keyval;
	gboolean is_control_pressed;
	GtkTreeSelection *tree_selection;
	GtkTreePath *root_path;
	GdkModifierType modifiers;

	if (gdk_event_get_keyval ((GdkEvent *)event, &keyval) == TRUE)
	{
		tree_selection = gtk_tree_view_get_selection (treeview);
		root_path = gtk_tree_path_new_from_string ("0");

		modifiers = gtk_accelerator_get_default_mod_mask ();
		is_control_pressed = (event->state & modifiers) == GDK_CONTROL_MASK;

		if ((keyval == GDK_KEY_Up || keyval == GDK_KEY_KP_Up) &&
		    !is_control_pressed)
		{
			if (gtk_tree_selection_path_is_selected (tree_selection, root_path))
			{
				gtk_tree_selection_unselect_all (tree_selection);
				gtk_widget_grab_focus (selector->search_entry);

				return GDK_EVENT_STOP;
			}
		}
	}

	return GDK_EVENT_PROPAGATE;
}

static void
on_entry_changed (GtkEntry                  *entry,
                  GeditOpenDocumentSelector *selector)
{
	const gchar *entry_text;

	entry_text = gtk_entry_get_text (entry);
	gedit_open_document_selector_store_set_filter (selector->selector_store,
	                                               entry_text);

	if (gtk_widget_get_mapped ( GTK_WIDGET (selector)))
	{
		populate_liststore (selector);
	}
}

static void
on_entry_activated (GtkEntry                  *entry,
                    GeditOpenDocumentSelector *selector)
{
	const gchar *entry_text;
	GtkTreeSelection *selection;
	gchar *uri;
	GFile *file;
	gchar *scheme;

	entry_text = gtk_entry_get_text (entry);
	scheme = g_uri_parse_scheme (entry_text);
	if (!scheme)
	{
		const gchar *home_dir = g_get_home_dir ();

		if ( home_dir != NULL && g_str_has_prefix (entry_text, "~/"))
		{
			uri = g_strconcat ("file://", home_dir, "/", entry_text + 2, NULL);
		}
		else
		{
			uri = g_strconcat ("file://", entry_text, NULL);
		}
	}
	else
	{
		g_free (scheme);
		uri = g_strdup (entry_text);
	}

	file = g_file_new_for_uri (uri);
	if (g_file_query_exists (file, NULL))
	{
		DEBUG_SELECTOR (g_print ("Selector(%p): search entry activated : loading '%s'\n",
		                         selector, uri););

		gtk_entry_set_text (entry, "");
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (selector->treeview));
		gtk_tree_selection_unselect_all (selection);

		g_signal_emit (G_OBJECT (selector), signals[SELECTOR_FILE_ACTIVATED], 0, uri);
	}

	g_object_unref (file);
}

static void
gedit_open_document_selector_dispose (GObject *object)
{
	GeditOpenDocumentSelector *selector = GEDIT_OPEN_DOCUMENT_SELECTOR (object);

	if (selector->populate_listbox_id != 0)
	{
		g_source_remove (selector->populate_listbox_id);
		selector->populate_listbox_id = 0;
	}

	g_clear_pointer (&selector->name_font, pango_font_description_free);
	g_clear_pointer (&selector->path_font, pango_font_description_free);
	g_clear_pointer (&selector->match_markup_color, g_free);

	if (selector->recent_items)
	{
		gedit_open_document_selector_free_file_items_list (selector->recent_items);
		selector->recent_items = NULL;
	}

	if (selector->home_dir_items)
	{
		gedit_open_document_selector_free_file_items_list (selector->home_dir_items);
		selector->home_dir_items = NULL;
	}

	if (selector->desktop_dir_items)
	{
		gedit_open_document_selector_free_file_items_list (selector->desktop_dir_items);
		selector->desktop_dir_items = NULL;
	}

	if (selector->local_bookmarks_dir_items)
	{
		gedit_open_document_selector_free_file_items_list (selector->local_bookmarks_dir_items);
		selector->local_bookmarks_dir_items = NULL;
	}

	if (selector->file_browser_root_items)
	{
		gedit_open_document_selector_free_file_items_list (selector->file_browser_root_items);
		selector->file_browser_root_items = NULL;
	}

	if (selector->active_doc_dir_items)
	{
		gedit_open_document_selector_free_file_items_list (selector->active_doc_dir_items);
		selector->active_doc_dir_items = NULL;
	}

	if (selector->current_docs_items)
	{
		gedit_open_document_selector_free_file_items_list (selector->current_docs_items);
		selector->current_docs_items = NULL;
	}

	if (selector->all_items)
	{
		gedit_open_document_selector_free_file_items_list (selector->all_items);
		selector->all_items = NULL;
	}

	G_OBJECT_CLASS (gedit_open_document_selector_parent_class)->dispose (object);
}

static void
on_row_activated (GtkTreeView               *treeview,
                  GtkTreePath               *path,
                  GtkTreeViewColumn         *column G_GNUC_UNUSED,
                  GeditOpenDocumentSelector *selector)
{
	GtkTreeModel *liststore = GTK_TREE_MODEL (selector->liststore);
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gchar *uri;

	g_return_if_fail (gtk_tree_model_get_iter (liststore, &iter, path));
	gtk_tree_model_get (liststore, &iter,
	                    URI_COLUMN, &uri,
	                    -1);

	selection = gtk_tree_view_get_selection (treeview);
	gtk_tree_selection_unselect_all (selection);

	/* Leak of uri */
	g_signal_emit (G_OBJECT (selector), signals[SELECTOR_FILE_ACTIVATED], 0, uri);
}

static void
update_list_cb (GeditOpenDocumentSelectorStore *selector_store,
                GAsyncResult                   *res,
                gpointer                        user_data G_GNUC_UNUSED)
{
	GList *list;
	GError *error;
	PushMessage *message;
	ListType type;
	GeditOpenDocumentSelector *selector;

	list = gedit_open_document_selector_store_update_list_finish (selector_store, res, &error);
	message = g_task_get_task_data (G_TASK (res));
	selector = message->selector;
	type = message->type;

	DEBUG_SELECTOR (g_print ("Selector(%p): update_list_cb - type:%s, length:%i\n",
	                         selector, list_type_string[type], g_list_length (list)););

	switch (type)
	{
		case GEDIT_OPEN_DOCUMENT_SELECTOR_RECENT_FILES_LIST:
			gedit_open_document_selector_free_file_items_list (selector->recent_items);
			selector->recent_items = list;
			break;

		case GEDIT_OPEN_DOCUMENT_SELECTOR_HOME_DIR_LIST:
			gedit_open_document_selector_free_file_items_list (selector->home_dir_items);
			selector->home_dir_items = list;
			break;

		case GEDIT_OPEN_DOCUMENT_SELECTOR_DESKTOP_DIR_LIST:
			gedit_open_document_selector_free_file_items_list (selector->desktop_dir_items);
			selector->desktop_dir_items = list;
			break;

		case GEDIT_OPEN_DOCUMENT_SELECTOR_LOCAL_BOOKMARKS_DIR_LIST:
			gedit_open_document_selector_free_file_items_list (selector->local_bookmarks_dir_items);
			selector->local_bookmarks_dir_items = list;
			break;

		case GEDIT_OPEN_DOCUMENT_SELECTOR_FILE_BROWSER_ROOT_DIR_LIST:
			gedit_open_document_selector_free_file_items_list (selector->file_browser_root_items);
			selector->file_browser_root_items = list;
			break;

		case GEDIT_OPEN_DOCUMENT_SELECTOR_ACTIVE_DOC_DIR_LIST:
			gedit_open_document_selector_free_file_items_list (selector->active_doc_dir_items);
			selector->active_doc_dir_items = list;
			break;

		case GEDIT_OPEN_DOCUMENT_SELECTOR_CURRENT_DOCS_LIST:
			gedit_open_document_selector_free_file_items_list (selector->current_docs_items);
			selector->current_docs_items = list;
			break;

		default:
			g_return_if_reached ();
	}

	selector->all_items = compute_all_items_list (selector);
	populate_liststore (selector);
}

static void
gedit_open_document_selector_constructed (GObject *object)
{
	GeditOpenDocumentSelector *selector = GEDIT_OPEN_DOCUMENT_SELECTOR (object);

	G_OBJECT_CLASS (gedit_open_document_selector_parent_class)->constructed (object);

	DEBUG_SELECTOR (g_print ("Selector(%p): constructed - ask recent file list\n", selector););

	gedit_open_document_selector_store_update_list_async (selector->selector_store,
	                                                      selector,
	                                                      NULL,
	                                                      (GAsyncReadyCallback)update_list_cb,
	                                                      GEDIT_OPEN_DOCUMENT_SELECTOR_RECENT_FILES_LIST,
	                                                      selector);
}

static void
gedit_open_document_selector_mapped (GtkWidget *widget)
{
	GeditOpenDocumentSelector *selector = GEDIT_OPEN_DOCUMENT_SELECTOR (widget);
	ListType list_number;

	/* We update all the lists */
	DEBUG_SELECTOR (g_print ("Selector(%p): mapped - ask all lists\n", selector););

	for (list_number = 0; list_number < GEDIT_OPEN_DOCUMENT_SELECTOR_LIST_TYPE_NUM_OF_LISTS; list_number++)
	{
		gedit_open_document_selector_store_update_list_async (selector->selector_store,
		                                                      selector,
		                                                      NULL,
		                                                      (GAsyncReadyCallback)update_list_cb,
		                                                      list_number,
		                                                      selector);
	}

	GTK_WIDGET_CLASS (gedit_open_document_selector_parent_class)->map (widget);
}

static GtkSizeRequestMode
gedit_open_document_selector_get_request_mode (GtkWidget *widget G_GNUC_UNUSED)
{
	return GTK_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
gedit_open_document_selector_get_preferred_width (GtkWidget *widget G_GNUC_UNUSED,
                                                  gint      *minimum_width,
                                                  gint      *natural_width)
{
	*minimum_width = *natural_width = OPEN_DOCUMENT_SELECTOR_WIDTH;
}

static void
gedit_open_document_selector_set_property (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
	GeditOpenDocumentSelector *selector = GEDIT_OPEN_DOCUMENT_SELECTOR (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			selector->window = g_value_get_object (value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_open_document_selector_get_property (GObject    *object,
                                           guint       prop_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
	GeditOpenDocumentSelector *selector = GEDIT_OPEN_DOCUMENT_SELECTOR (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_value_set_object (value, selector->window);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_open_document_selector_file_activated (GeditOpenDocumentSelector *selector G_GNUC_UNUSED,
                                             const gchar               *uri      G_GNUC_UNUSED)
{
	/* Do nothing in the default handler */
}

static void
gedit_open_document_selector_class_init (GeditOpenDocumentSelectorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->constructed = gedit_open_document_selector_constructed;
	object_class->dispose = gedit_open_document_selector_dispose;

	object_class->get_property = gedit_open_document_selector_get_property;
	object_class->set_property = gedit_open_document_selector_set_property;

	widget_class->get_request_mode = gedit_open_document_selector_get_request_mode;
	widget_class->get_preferred_width = gedit_open_document_selector_get_preferred_width;
	widget_class->map = gedit_open_document_selector_mapped;

	properties[PROP_WINDOW] =
		g_param_spec_object ("window",
		                     "Window",
		                     "The GeditWindow this GeditOpenDocumentSelector is associated with",
		                     GEDIT_TYPE_WINDOW,
		                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, LAST_PROP, properties);

	signals[SELECTOR_FILE_ACTIVATED] =
		g_signal_new_class_handler ("file-activated",
		                            G_TYPE_FROM_CLASS (klass),
		                            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		                            G_CALLBACK (gedit_open_document_selector_file_activated),
		                            NULL, NULL, NULL,
		                            G_TYPE_NONE,
		                            1,
		                            G_TYPE_STRING);

	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/gedit/ui/gedit-open-document-selector.ui");

	gtk_widget_class_bind_template_child (widget_class, GeditOpenDocumentSelector, open_button);
	gtk_widget_class_bind_template_child (widget_class, GeditOpenDocumentSelector, treeview);
	gtk_widget_class_bind_template_child (widget_class, GeditOpenDocumentSelector, placeholder_box);
	gtk_widget_class_bind_template_child (widget_class, GeditOpenDocumentSelector, scrolled_window);
	gtk_widget_class_bind_template_child (widget_class, GeditOpenDocumentSelector, search_entry);
}

static void
on_treeview_allocate (GtkWidget                 *widget     G_GNUC_UNUSED,
                      GdkRectangle              *allocation G_GNUC_UNUSED,
                      GeditOpenDocumentSelector *selector)
{
	GeditOpenDocumentSelectorStore *selector_store;
	GtkStyleContext *context;
	gint name_renderer_natural_size;
	gint path_renderer_natural_size;
	GtkBorder padding;
	gint ypad;
	gint limit_capped;
	gint treeview_height;
	gint grid_line_width;
	gint row_height;
	gint recent_limit;

	selector_store = selector->selector_store;

	context = gtk_widget_get_style_context (selector->treeview);
	gtk_style_context_get_padding (context,
				       gtk_style_context_get_state (context),
				       &padding);

	/* Treeview height computation */
	gtk_cell_renderer_get_preferred_height (selector->name_renderer,
	                                        selector->treeview,
	                                        NULL,
	                                        &name_renderer_natural_size);

	gtk_cell_renderer_get_preferred_height (selector->path_renderer,
	                                        selector->treeview,
	                                        NULL,
	                                        &path_renderer_natural_size);

	gtk_cell_renderer_get_padding (selector->name_renderer, NULL, &ypad);
	gtk_widget_style_get (selector->treeview, "grid-line-width", &grid_line_width, NULL);

	recent_limit = gedit_open_document_selector_store_get_recent_limit (selector_store);

	limit_capped = (recent_limit > 0 ) ? MIN (recent_limit, OPEN_DOCUMENT_SELECTOR_MAX_VISIBLE_ROWS) :
	                                     OPEN_DOCUMENT_SELECTOR_MAX_VISIBLE_ROWS;

	row_height = name_renderer_natural_size +
	             path_renderer_natural_size +
	             2 * (padding.top + padding.bottom) +
	             ypad +
	             grid_line_width;

	treeview_height = row_height * limit_capped;
	gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (selector->scrolled_window),
	                                            treeview_height);
	gtk_scrolled_window_set_max_content_height (GTK_SCROLLED_WINDOW (selector->scrolled_window),
	                                            treeview_height);

	gtk_widget_set_size_request (selector->placeholder_box, -1, treeview_height);
}

static inline gchar *
rgba_to_hex8 (GdkRGBA *rgba)
{
	guint red = (guint)(0.5 + CLAMP (rgba->red, 0.0, 1.0) * 255.0);
	guint green = (guint)(0.5 + CLAMP (rgba->green, 0.0, 1.0) * 255.0);
	guint blue = (guint)(0.5 + CLAMP (rgba->blue, 0.0, 1.0) * 255.0);
	guint alpha = (guint)(0.5 + CLAMP (rgba->alpha, 0.0, 1.0) * 255.0);
	gchar *str = g_strdup_printf ("#%02X%02X%02X%02X", red, green, blue, alpha);

	return str;
}

static void
on_treeview_style_updated (GtkWidget                 *widget,
                           GeditOpenDocumentSelector *selector)
{
	GtkStyleContext *context;
	GdkRGBA match_foreground_rgba = {0.0, 0.0, 0.0, 0.0};
	GdkRGBA match_background_rgba = {0.0, 0.0, 0.0, 0.0};
	gchar *match_foreground_hex8;
	gchar *match_background_hex8;

	context = gtk_widget_get_style_context (widget);

	/* Name label foreground and font size styling */
	gtk_style_context_save (context);
	gtk_style_context_add_class (context, "open-document-selector-name-label");

	gtk_style_context_get_color (context,
				     gtk_style_context_get_state (context),
				     &selector->name_label_color);

	g_clear_pointer (&selector->name_font, pango_font_description_free);
	gtk_style_context_get (context,
			       gtk_style_context_get_state (context),
			       "font", &selector->name_font,
			       NULL);

	gtk_style_context_restore (context);

	/* Path label foreground and font size styling */
	gtk_style_context_save (context);
	gtk_style_context_add_class (context, "open-document-selector-path-label");

	gtk_style_context_get_color (context,
				     gtk_style_context_get_state (context),
				     &selector->path_label_color);

	g_clear_pointer (&selector->path_font, pango_font_description_free);
	gtk_style_context_get (context,
			       gtk_style_context_get_state (context),
			       "font", &selector->path_font,
			       NULL);

	gtk_style_context_restore (context);

	/* Match styling */
	gtk_style_context_save (context);
	gtk_style_context_add_class (context, "open-document-selector-match");

	gtk_style_context_get_color (context,
				     gtk_style_context_get_state (context),
				     &match_foreground_rgba);

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
	gtk_style_context_get_background_color (context,
						gtk_style_context_get_state (context),
						&match_background_rgba);
	G_GNUC_END_IGNORE_DEPRECATIONS;

	gtk_style_context_restore (context);
	g_free (selector->match_markup_color);

	match_foreground_hex8 = rgba_to_hex8 (&match_foreground_rgba);
	match_background_hex8 = rgba_to_hex8 (&match_background_rgba);

	selector->match_markup_color = g_strdup_printf ("<span weight =\"heavy\" foreground =\"%s\" background =\"%s\">",
							match_foreground_hex8,
							match_background_hex8);

	g_free (match_foreground_hex8);
	g_free (match_background_hex8);
}

static void
name_renderer_datafunc (GtkTreeViewColumn         *column        G_GNUC_UNUSED,
                        GtkCellRenderer           *name_renderer G_GNUC_UNUSED,
                        GtkTreeModel              *liststore     G_GNUC_UNUSED,
                        GtkTreeIter               *iter          G_GNUC_UNUSED,
                        GeditOpenDocumentSelector *selector)
{
	g_object_set (selector->name_renderer, "foreground-rgba", &selector->name_label_color, NULL);
	g_object_set (selector->name_renderer, "font-desc", selector->name_font, NULL);
}

static void
path_renderer_datafunc (GtkTreeViewColumn         *column        G_GNUC_UNUSED,
                        GtkCellRenderer           *path_renderer G_GNUC_UNUSED,
                        GtkTreeModel              *liststore     G_GNUC_UNUSED,
                        GtkTreeIter               *iter          G_GNUC_UNUSED,
                        GeditOpenDocumentSelector *selector)
{
	g_object_set (selector->path_renderer, "foreground-rgba", &selector->path_label_color, NULL);
	g_object_set (selector->path_renderer, "font-desc", selector->path_font, NULL);
}

static void
setup_treeview (GeditOpenDocumentSelector *selector)
{
	GtkTreeViewColumn *column;
	GtkCellArea *cell_area;
	GtkStyleContext *context;

	gtk_tree_view_set_model (GTK_TREE_VIEW (selector->treeview), GTK_TREE_MODEL (selector->liststore));
	g_object_unref(GTK_TREE_MODEL (selector->liststore));

	selector->name_renderer = gtk_cell_renderer_text_new ();
	selector->path_renderer = gtk_cell_renderer_text_new ();

	g_object_set (selector->name_renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	g_object_set (selector->path_renderer, "ellipsize", PANGO_ELLIPSIZE_START, NULL);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);

	gtk_tree_view_column_pack_start (column, selector->name_renderer, TRUE);
	gtk_tree_view_column_pack_start (column, selector->path_renderer, TRUE);

	gtk_tree_view_column_set_attributes (column, selector->name_renderer, "markup", NAME_COLUMN, NULL);
	gtk_tree_view_column_set_attributes (column, selector->path_renderer, "markup", PATH_COLUMN, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (selector->treeview), column);
	cell_area = gtk_cell_layout_get_area (GTK_CELL_LAYOUT (column));
	gtk_orientable_set_orientation (GTK_ORIENTABLE (cell_area), GTK_ORIENTATION_VERTICAL);

	context = gtk_widget_get_style_context (selector->treeview);
	gtk_style_context_add_class (context, "open-document-selector-treeview");

	gtk_tree_view_column_set_cell_data_func (column,
	                                         selector->name_renderer,
	                                         (GtkTreeCellDataFunc)name_renderer_datafunc,
	                                         selector,
	                                         NULL);

	gtk_tree_view_column_set_cell_data_func (column,
	                                         selector->path_renderer,
	                                         (GtkTreeCellDataFunc)path_renderer_datafunc,
	                                         selector,
	                                         NULL);
}

static void
gedit_open_document_selector_init (GeditOpenDocumentSelector *selector)
{
	gedit_debug (DEBUG_WINDOW);

	gtk_widget_init_template (GTK_WIDGET (selector));

	selector->selector_store = gedit_open_document_selector_store_get_default ();

	selector->liststore = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	setup_treeview (selector);

	g_signal_connect (selector->search_entry,
	                  "changed",
	                  G_CALLBACK (on_entry_changed),
	                  selector);

	g_signal_connect (selector->search_entry,
	                  "activate",
	                  G_CALLBACK (on_entry_activated),
	                  selector);

	g_signal_connect (selector->treeview,
	                  "row-activated",
	                  G_CALLBACK (on_row_activated),
	                  selector);

	g_signal_connect (selector->treeview,
	                  "size-allocate",
	                  G_CALLBACK (on_treeview_allocate),
	                  selector);

	g_signal_connect (selector->treeview,
	                  "key-press-event",
	                  G_CALLBACK (on_treeview_key_press),
	                  selector);

	g_signal_connect (selector->treeview,
	                  "style-updated",
	                  G_CALLBACK (on_treeview_style_updated),
	                  selector);
}

GeditOpenDocumentSelector *
gedit_open_document_selector_new (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return g_object_new (GEDIT_TYPE_OPEN_DOCUMENT_SELECTOR,
	                     "window", window,
	                     NULL);
}

GeditWindow *
gedit_open_document_selector_get_window (GeditOpenDocumentSelector *selector)
{
	g_return_val_if_fail (GEDIT_IS_OPEN_DOCUMENT_SELECTOR (selector), NULL);

	return selector->window;
}

GtkWidget *
gedit_open_document_selector_get_search_entry (GeditOpenDocumentSelector *selector)
{
	g_return_val_if_fail (GEDIT_IS_OPEN_DOCUMENT_SELECTOR (selector), NULL);

	return selector->search_entry;
}

/* ex:set ts=8 noet: */
