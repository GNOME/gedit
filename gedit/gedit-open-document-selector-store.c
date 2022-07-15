/*
 * gedit-open-document-selector-store.c
 * This file is part of gedit
 *
 * Copyright (C) 2015 - Sébastien Lafargue
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

/* You need to call gedit_open_document_selector_store_get_default()
 * to get a singleton #GeditOpenDocumentSelectorStore object.
 * #GeditOpenDocumentSelectorStore is responsible of managing
 * the recent files list and computing others lists.
 *
 * The lists returned are lists of FileItem structs.
 *
 * #GeditOpenDocumentSelectorStore is destroyed automaticly at
 * the end of your application.
 *
 * Call gedit_open_document_selector_store_update_list_async() with
 * the corresponding ListType, then in your callback, call
 * gedit_open_document_selector_store_update_list_finish() to get
 * in return the list of FileItem structs.
 *
 * The recent files list can be filtered by calling
 * gedit_open_document_selector_store_set_filter()
 * and you can get the actual filter by calling
 * gedit_open_document_selector_store_get_filter()
 * ( this is in addition to the text mime type filter)
 *
 * The recent files list is not capped by Gedit settings like
 * in gedit_recent_get_items() but you still can get the limit
 * by calling gedit_open_document_selector_store_get_recent_limit().
 *
 * The original setting is stored in gsettings at :
 * org.gnome.gedit.preferences.ui
 * with the key : max-recents
 */

#include "gedit-open-document-selector-store.h"

#include <time.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <glib.h>
#include <gio/gio.h>

#include "gedit-recent.h"
#include "gedit-utils.h"
#include "gedit-window.h"
#include "gedit-debug.h"

struct _GeditOpenDocumentSelectorStore
{
	GObject                   parent_instance;

	GSource                  *recent_source;

	GeditRecentConfiguration  recent_config;
	gchar                    *filter;
	GList                    *recent_items;
	gint                      recent_config_limit;
	gboolean                  recent_items_need_update;
};

G_LOCK_DEFINE_STATIC (recent_files_filter_lock);

G_DEFINE_TYPE (GeditOpenDocumentSelectorStore, gedit_open_document_selector_store, G_TYPE_OBJECT)

G_DEFINE_QUARK (gedit-open-document-selector-store-error-quark,
                gedit_open_document_selector_store_error)


static GDateTime *
_get_date_time (GFileInfo *info)
{
  guint32 time;
  guint32 usecs;
  GDateTime *dt = NULL, *dt2 = NULL;

  g_return_val_if_fail (G_IS_FILE_INFO (info), NULL);

  time = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_ACCESS);
  dt = g_date_time_new_from_unix_utc (time);

  usecs = g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_TIME_ACCESS_USEC);
  dt2 = g_date_time_add_seconds (dt, usecs / (gdouble) G_USEC_PER_SEC);

  g_date_time_unref (dt);

  return g_steal_pointer (&dt2);
}


static GList *
get_current_docs_list (GeditOpenDocumentSelectorStore *selector_store G_GNUC_UNUSED,
                       GeditOpenDocumentSelector      *selector)
{
	GeditWindow *window;
	GList *docs;
	GList *l;
	GFile *file;
	GFileInfo *info;
	FileItem *item;
	GList *file_items_list = NULL;

	window = gedit_open_document_selector_get_window (selector);

	docs = gedit_window_get_documents (window);
	for (l = docs; l != NULL; l = l->next)
	{
		file = gtk_source_file_get_location (gedit_document_get_file (l->data));
		if (file == NULL)
		{
			/* In case of not saved docs */
			continue;
		}

		info = g_file_query_info (file,
		                          "time::access,time::access-usec",
		                          G_FILE_QUERY_INFO_NONE,
		                          NULL,
		                          NULL);
		if (info == NULL)
		{
			continue;
		}

		item = gedit_open_document_selector_create_fileitem_item ();

		item->accessed = _get_date_time (info);
		item->uri = g_file_get_uri (file);

		file_items_list = g_list_prepend (file_items_list, item);

		g_object_unref (info);
	}

	g_list_free (docs);
	return file_items_list;
}

/* Notice that a content-type attribute must have been query to work */
static gboolean
check_mime_type (GFileInfo *info)
{
	const gchar *content_type;
	G_GNUC_UNUSED gchar *mime_type;

	content_type = g_file_info_get_attribute_string (info, "standard::fast-content-type");
	if (content_type == NULL)
	{
		return FALSE;
	}

#ifdef G_OS_WIN32
	if (g_content_type_is_a (content_type, "text"))
	{
		return TRUE;
	}

	mime_type = g_content_type_get_mime_type (content_type);
	if (mime_type == NULL)
	{
		return FALSE;
	}

	if (g_strcmp0 (mime_type, "text/plain") == 0)
	{
		g_free (mime_type);
		return TRUE;
	}

	g_free (mime_type);
#else
	if (g_content_type_is_a (content_type, "text/plain"))
	{
		return TRUE;
	}
#endif
	return FALSE;
}

static GList *
get_children_from_dir (GeditOpenDocumentSelectorStore *selector_store G_GNUC_UNUSED,
                       GFile                          *dir)
{
	GList *file_items_list = NULL;
	GFileEnumerator *file_enum;
	GFileInfo *info;
	GFileType filetype;
	GFile *file;
	FileItem *item;
	gboolean is_text;
	gboolean is_correct_type;

	g_return_val_if_fail (G_IS_FILE (dir), NULL);

	file_enum = g_file_enumerate_children (dir,
	                                       "standard::name,"
	                                       "standard::type,"
	                                       "standard::fast-content-type,"
	                                       "time::access,time::access-usec",
	                                       G_FILE_QUERY_INFO_NONE,
	                                       NULL,
	                                       NULL);
	if (file_enum == NULL)
	{
		return NULL;
	}

	while ((info = g_file_enumerator_next_file (file_enum, NULL, NULL)))
	{
		filetype = g_file_info_get_file_type (info);
		is_text = check_mime_type (info);
		is_correct_type = (filetype == G_FILE_TYPE_REGULAR ||
		                   filetype == G_FILE_TYPE_SYMBOLIC_LINK ||
		                   filetype == G_FILE_TYPE_SHORTCUT);

		if (is_text &&
		    is_correct_type &&
		    (file = g_file_enumerator_get_child (file_enum, info)) != NULL)
		{
			item = gedit_open_document_selector_create_fileitem_item ();
			item->uri = g_file_get_uri (file);

			item->accessed = _get_date_time (info);

			file_items_list = g_list_prepend (file_items_list, item);
			g_object_unref (file);
		}

		g_object_unref (info);
	}

	g_file_enumerator_close (file_enum, NULL, NULL);
	g_object_unref (file_enum);

	return file_items_list;
}

static GList *
get_active_doc_dir_list (GeditOpenDocumentSelectorStore *selector_store,
                         GeditOpenDocumentSelector      *selector)
{
	GeditWindow *window;
	GeditDocument *active_doc;
	GtkSourceFile *source_file;
	GList *file_items_list = NULL;

	window = gedit_open_document_selector_get_window (selector);

	active_doc = gedit_window_get_active_document (window);

	if (active_doc == NULL)
	{
		return NULL;
	}

	source_file = gedit_document_get_file (active_doc);
	if (gtk_source_file_is_local (source_file))
	{
		GFile *location;
		GFile *parent_dir;

		location = gtk_source_file_get_location (source_file);
		parent_dir = g_file_get_parent (location);

		if (parent_dir != NULL)
		{
			file_items_list = get_children_from_dir (selector_store, parent_dir);
			g_object_unref (parent_dir);
		}
	}

	return file_items_list;
}

static GFile *
get_file_browser_root (GeditOpenDocumentSelectorStore *selector_store G_GNUC_UNUSED,
                       GeditOpenDocumentSelector      *selector)
{
	GeditWindow *window;
	GeditMessageBus *bus;
	GeditMessage *msg;
	GFile *root = NULL;

	window = gedit_open_document_selector_get_window (selector);

	bus = gedit_window_get_message_bus (window);
	if (gedit_message_bus_is_registered (bus, "/plugins/filebrowser", "get_root"))
	{
		msg = gedit_message_bus_send_sync (bus, "/plugins/filebrowser", "get_root", NULL, NULL);
		g_object_get (msg, "location", &root, NULL);
		g_object_unref (msg);
	}

	return root;
}

static GList *
get_file_browser_root_dir_list (GeditOpenDocumentSelectorStore *selector_store,
                                GeditOpenDocumentSelector      *selector)
{
	GFile *root;
	GList *file_items_list = NULL;

	root = get_file_browser_root (selector_store, selector);
	if (root != NULL && g_file_is_native (root))
	{
		file_items_list = get_children_from_dir (selector_store, root);
	}

	g_clear_object (&root);
	return file_items_list;
}

/* Taken and adapted from gtk+ gtkbookmarksmanager.c */
static GList *
read_bookmarks_file (GFile *file)
{
	gchar *contents;
	gchar **lines, *space;
	GList *uri_list = NULL;
	gint i;

	if (!g_file_load_contents (file, NULL, &contents, NULL, NULL, NULL))
	{
		return NULL;
	}

	lines = g_strsplit (contents, "\n", -1);

	for (i = 0; lines[i]; i++)
	{
		if (*lines[i] == '\0')
		{
			continue;
		}

		if (!g_utf8_validate (lines[i], -1, NULL))
		{
			continue;
		}

		if ((space = strchr (lines[i], ' ')) != NULL)
		{
			space[0] = '\0';
		}

		uri_list = g_list_prepend (uri_list, g_strdup (lines[i]));
	}

	g_strfreev (lines);
	g_free (contents);

	return uri_list;
}

static GList *
get_local_bookmarks_list (GeditOpenDocumentSelectorStore *selector_store,
                          GeditOpenDocumentSelector      *selector G_GNUC_UNUSED)
{
	GList *bookmarks_uri_list = NULL;
	GList *file_items_list = NULL;
	GList *new_file_items_list = NULL;
	GFile *bookmarks_file;
	GFile *file;
	gchar *filename;
	GList *l;

	filename = g_build_filename (g_get_user_config_dir (), "gtk-3.0", "bookmarks", NULL);
	bookmarks_file = g_file_new_for_path (filename);
	g_free (filename);

	bookmarks_uri_list = read_bookmarks_file (bookmarks_file);
	g_object_unref (bookmarks_file);

	for (l = bookmarks_uri_list; l != NULL; l = l->next)
	{
		file = g_file_new_for_uri (l->data);
		if (g_file_is_native (file))
		{
			new_file_items_list = get_children_from_dir (selector_store, file);
			file_items_list = g_list_concat (file_items_list, new_file_items_list);
		}

		g_object_unref (file);
	}

	g_list_free_full (bookmarks_uri_list, g_free);
	return file_items_list;
}

/* Taken and adapted from gtk+ gtkplacessidebar.c */
static gboolean
path_is_home_dir (const gchar *path)
{
	GFile *home_dir;
	GFile *location;
	const gchar *home_path;
	gboolean res;

	home_path = g_get_home_dir ();
	if (home_path == NULL)
	{
		return FALSE;
	}

	home_dir = g_file_new_for_path (home_path);
	location = g_file_new_for_path (path);
	res = g_file_equal (home_dir, location);

	g_object_unref (home_dir);
	g_object_unref (location);

	return res;
}

static GList *
get_desktop_dir_list (GeditOpenDocumentSelectorStore *selector_store,
                      GeditOpenDocumentSelector      *selector G_GNUC_UNUSED)
{
	GList *file_items_list = NULL;
	const gchar *desktop_dir_name;
	gchar *desktop_uri;
	GFile *desktop_file;

	desktop_dir_name = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);

	/* "To disable a directory, point it to the homedir."
	 * See http://freedesktop.org/wiki/Software/xdg-user-dirs
	 */
	if (path_is_home_dir (desktop_dir_name))
	{
		return NULL;
	}

	desktop_uri = g_strconcat ("file://", desktop_dir_name, NULL);
	desktop_file = g_file_new_for_uri (desktop_uri);
	file_items_list = get_children_from_dir (selector_store, desktop_file);

	g_free (desktop_uri);
	g_object_unref (desktop_file);

	return file_items_list;
}

static GList *
get_home_dir_list (GeditOpenDocumentSelectorStore *selector_store,
                   GeditOpenDocumentSelector      *selector G_GNUC_UNUSED)
{
	GList *file_items_list = NULL;
	const gchar *home_name;
	gchar *home_uri;
	GFile *home_file;

	home_name = g_get_home_dir ();
	if (home_name == NULL)
	{
		return NULL;
	}

	home_uri = g_strconcat ("file://", home_name, NULL);
	home_file = g_file_new_for_uri (home_uri);
	file_items_list = get_children_from_dir (selector_store, home_file);

	g_free (home_uri);
	g_object_unref (home_file);

	return file_items_list;
}

static GList *
convert_recent_item_list_to_fileitem_list (GList *uri_list)
{
	GList *l;
	GList *fileitem_list = NULL;

	for (l = uri_list; l != NULL; l = l->next)
	{
		gchar *uri;
		FileItem *item;

		uri = g_strdup (gtk_recent_info_get_uri (l->data));

		item = gedit_open_document_selector_create_fileitem_item ();
		item->uri = uri;

		item->accessed = g_date_time_new_from_unix_local (gtk_recent_info_get_visited (l->data));

		fileitem_list = g_list_prepend (fileitem_list, item);
	}

	fileitem_list = g_list_reverse (fileitem_list);
	return fileitem_list;
}

static GList *
get_recent_files_list (GeditOpenDocumentSelectorStore *selector_store,
                       GeditOpenDocumentSelector      *selector G_GNUC_UNUSED)
{
	GList *recent_items_list;
	GList *file_items_list;

	G_LOCK (recent_files_filter_lock);
	recent_items_list = gedit_recent_get_items (&selector_store->recent_config);
	G_UNLOCK (recent_files_filter_lock);

	file_items_list = convert_recent_item_list_to_fileitem_list (recent_items_list);
	g_list_free_full (recent_items_list, (GDestroyNotify)gtk_recent_info_unref);

	return file_items_list;
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

	list = gedit_open_document_selector_store_update_list_finish (selector_store, res, &error);

	message = g_task_get_task_data (G_TASK (res));
	type = message->type;

	switch (type)
	{
		case GEDIT_OPEN_DOCUMENT_SELECTOR_RECENT_FILES_LIST:
			gedit_open_document_selector_free_file_items_list (selector_store->recent_items);
			selector_store->recent_items = list;

			DEBUG_SELECTOR (g_print ("\tStore(%p): update_list_cb: type:%s, length:%i\n",
			                         selector_store, list_type_string[type], g_list_length (list)););

			break;
		default:
			break;
	}
}

static void
on_recent_manager_changed (GtkRecentManager *manager G_GNUC_UNUSED,
                           gpointer          user_data)
{
	GeditOpenDocumentSelectorStore *selector_store = GEDIT_OPEN_DOCUMENT_SELECTOR_STORE (user_data);

	selector_store->recent_items_need_update = TRUE;
	gedit_open_document_selector_store_update_list_async (selector_store,
	                                                      NULL,
	                                                      NULL,
	                                                      (GAsyncReadyCallback)update_list_cb,
	                                                      GEDIT_OPEN_DOCUMENT_SELECTOR_RECENT_FILES_LIST,
	                                                      NULL);
}

static void
gedit_open_document_selector_store_dispose (GObject *object)
{
	GeditOpenDocumentSelectorStore *selector_store = GEDIT_OPEN_DOCUMENT_SELECTOR_STORE (object);

	gedit_recent_configuration_destroy (&selector_store->recent_config);

	g_clear_pointer (&selector_store->recent_source, g_source_destroy);
	g_clear_pointer (&selector_store->filter, g_free);

	if (selector_store->recent_items)
	{
		gedit_open_document_selector_free_file_items_list (selector_store->recent_items);
		selector_store->recent_items = NULL;
	}

	G_OBJECT_CLASS (gedit_open_document_selector_store_parent_class)->dispose (object);
}

static void
gedit_open_document_selector_store_class_init (GeditOpenDocumentSelectorStoreClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose = gedit_open_document_selector_store_dispose;
}

/* The order of functions pointers must be the same as in
 * ListType enum define in ./gedit-open-document-selector-helper.h
 */
static GList * (*list_func [])(GeditOpenDocumentSelectorStore *selector_store,
                               GeditOpenDocumentSelector      *selector) =
{
	get_recent_files_list,
	get_home_dir_list,
	get_desktop_dir_list,
	get_local_bookmarks_list,
	get_file_browser_root_dir_list,
	get_active_doc_dir_list,
	get_current_docs_list
};

static gboolean
update_recent_list (gpointer user_data)
{
	GeditOpenDocumentSelectorStore *selector_store;
	GeditOpenDocumentSelector *selector;
	PushMessage *message;
	/* The type variable is only used when debug code activated */
	G_GNUC_UNUSED ListType type;
	GList *file_items_list;
	GTask *task = G_TASK(user_data);

	selector_store = g_task_get_source_object (task);
	message = g_task_get_task_data (task);
	selector = message->selector;
	type = message->type;

	DEBUG_SELECTOR_TIMER_DECL
	DEBUG_SELECTOR_TIMER_NEW
	DEBUG_SELECTOR (g_print ("\tStore(%p): store dispatcher: type:%s\n",
	                         selector, list_type_string[type]););

	/* Update the recent list only when it changes, copy otherwise but keep it the first time */
	if (selector_store->recent_items != NULL && selector_store->recent_items_need_update == FALSE)
	{
		file_items_list = gedit_open_document_selector_copy_file_items_list (selector_store->recent_items);

		DEBUG_SELECTOR (g_print ("\tStore(%p): store dispatcher: recent list copy\n", selector););
	}
	else
	{
		selector_store->recent_items_need_update = FALSE;
		file_items_list = get_recent_files_list (selector_store, selector);

		DEBUG_SELECTOR (g_print ("\tStore(%p): store dispatcher: recent list compute\n", selector););

		if (selector_store->recent_items == NULL)
		{
			selector_store->recent_items = gedit_open_document_selector_copy_file_items_list (file_items_list);
		}
	}

	g_task_return_pointer (task,
	                       file_items_list,
	                       (GDestroyNotify)gedit_open_document_selector_free_file_items_list);

	DEBUG_SELECTOR (g_print ("\tStore(%p): store dispatcher: type:%s, time:%lf\n",
	                         selector, list_type_string[type], DEBUG_SELECTOR_TIMER_GET););
	DEBUG_SELECTOR_TIMER_DESTROY

        selector_store->recent_source = NULL;
	return G_SOURCE_REMOVE;
}

static void
update_list_dispatcher (GTask        *task,
                        gpointer      source_object,
                        gpointer      task_data,
                        GCancellable *cancellable G_GNUC_UNUSED)
{
	GeditOpenDocumentSelectorStore *selector_store = source_object;
	GeditOpenDocumentSelector *selector;
	PushMessage *message;
	ListType type;
	GList *file_items_list;

	message = task_data;
	selector = message->selector;
	type = message->type;

	DEBUG_SELECTOR_TIMER_DECL
	DEBUG_SELECTOR_TIMER_NEW
	DEBUG_SELECTOR (g_print ("\tStore(%p): store dispatcher: Thread:%p, type:%s\n",
	                         selector, g_thread_self (), list_type_string[type]););

	if (type >= GEDIT_OPEN_DOCUMENT_SELECTOR_LIST_TYPE_NUM_OF_LISTS)
	{
		g_task_return_new_error (task,
		                         GEDIT_OPEN_DOCUMENT_SELECTOR_STORE_ERROR, TYPE_OUT_OF_RANGE,
		                         "List Type out of range");
		g_object_unref (task);
		return;
	}

	/* Here we call the corresponding list creator function */
	file_items_list = (*list_func[type]) (selector_store, selector);

	DEBUG_SELECTOR (g_print ("\tStore(%p): store dispatcher: Thread:%p, type:%s, time:%lf\n",
	                         selector, g_thread_self (), list_type_string[type], DEBUG_SELECTOR_TIMER_GET););
	DEBUG_SELECTOR_TIMER_DESTROY

	g_task_return_pointer (task,
	                       file_items_list,
	                       (GDestroyNotify)gedit_open_document_selector_free_file_items_list);
}

GList *
gedit_open_document_selector_store_update_list_finish (GeditOpenDocumentSelectorStore  *open_document_selector_store,
                                                       GAsyncResult                    *result,
                                                       GError                         **error)
{
	g_return_val_if_fail (GEDIT_IS_OPEN_DOCUMENT_SELECTOR_STORE (open_document_selector_store), NULL);
	g_return_val_if_fail (g_task_is_valid (result, open_document_selector_store), NULL);

	return g_task_propagate_pointer (G_TASK (result), error);
}

void
gedit_open_document_selector_store_update_list_async (GeditOpenDocumentSelectorStore *selector_store,
                                                      GeditOpenDocumentSelector      *selector,
                                                      GCancellable                   *cancellable,
                                                      GAsyncReadyCallback             callback,
                                                      ListType                        type,
                                                      gpointer                        user_data)
{
	GTask *task;
	PushMessage *message;

	g_return_if_fail (GEDIT_IS_OPEN_DOCUMENT_SELECTOR_STORE (selector_store));
	g_return_if_fail (selector == NULL || GEDIT_IS_OPEN_DOCUMENT_SELECTOR (selector));

	message = g_new (PushMessage, 1);
	message->selector = selector;
	message->type = type;

	task = g_task_new (selector_store, cancellable, callback, user_data);
	g_task_set_source_tag (task, gedit_open_document_selector_store_update_list_async);
	g_task_set_priority (task, G_PRIORITY_DEFAULT);
	g_task_set_task_data (task, message, (GDestroyNotify)g_free);

	if (type == GEDIT_OPEN_DOCUMENT_SELECTOR_RECENT_FILES_LIST &&
	    selector_store->recent_source == NULL)
	{
		selector_store->recent_source = g_idle_source_new ();
		g_task_attach_source (task, selector_store->recent_source, update_recent_list);
	}
	else
	{
		g_task_run_in_thread (task, update_list_dispatcher);
	}

	g_object_unref (task);
}

static void
gedit_open_document_selector_store_init (GeditOpenDocumentSelectorStore *selector_store)
{
	gedit_recent_configuration_init_default (&selector_store->recent_config);

	/* We remove the recent files limit since we need the whole list but
	 * we back it up as gedit_open_document_selector_store_get_recent_limit
	 * use it
	 */
	selector_store->recent_config_limit = selector_store->recent_config.limit;
	selector_store->recent_config.limit = -1;

	g_signal_connect_object (selector_store->recent_config.manager,
	                         "changed",
	                         G_CALLBACK (on_recent_manager_changed),
	                         selector_store,
	                         0);

	selector_store->recent_items_need_update = TRUE;
}

gint
gedit_open_document_selector_store_get_recent_limit (GeditOpenDocumentSelectorStore *selector_store)
{
	g_return_val_if_fail (GEDIT_IS_OPEN_DOCUMENT_SELECTOR_STORE (selector_store), -1);

	return selector_store->recent_config_limit;
}

void
gedit_open_document_selector_store_set_filter (GeditOpenDocumentSelectorStore *selector_store,
                                               const gchar                    *filter)
{
	gchar *old_filter;

	g_return_if_fail (GEDIT_IS_OPEN_DOCUMENT_SELECTOR_STORE (selector_store));
	g_return_if_fail (filter != NULL);

	G_LOCK (recent_files_filter_lock);

	old_filter = selector_store->filter;
	selector_store->filter = g_strdup (filter);

	G_UNLOCK (recent_files_filter_lock);
	g_free (old_filter);
}

gchar *
gedit_open_document_selector_store_get_filter (GeditOpenDocumentSelectorStore *selector_store)
{
	gchar *recent_filter;

	g_return_val_if_fail (GEDIT_IS_OPEN_DOCUMENT_SELECTOR_STORE (selector_store), NULL);

	G_LOCK (recent_files_filter_lock);
	recent_filter = g_strdup (selector_store->filter);
	G_UNLOCK (recent_files_filter_lock);

	return recent_filter;
}

/* Gets a unique instance of #GeditOpenDocumentSelectorStore
 *
 * Returns: (transfer none): A unique #GeditOpenDocumentSelectorStore.
 * Do not ref or unref it, it will be destroyed at the end of the application.
 */
GeditOpenDocumentSelectorStore *
gedit_open_document_selector_store_get_default (void)
{
	static GeditOpenDocumentSelectorStore *instance;

	if (instance == NULL)
	{
		instance = g_object_new (GEDIT_TYPE_OPEN_DOCUMENT_SELECTOR_STORE, NULL);
		g_object_add_weak_pointer (G_OBJECT (instance), (gpointer) &instance);
	}

	return instance;
}

/* ex:set ts=8 noet: */
