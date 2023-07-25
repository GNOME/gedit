/*
 * gedit-file-browser-widget.c - Gedit plugin providing easy file access
 * from the sidepanel
 *
 * Copyright (C) 2006 - Jesse van den Kieboom <jesse@icecrew.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>
#include <gedit/gedit-utils.h>

#include "gedit-file-browser-utils.h"
#include "gedit-file-browser-error.h"
#include "gedit-file-browser-widget.h"
#include "gedit-file-browser-view.h"
#include "gedit-file-browser-store.h"
#include "gedit-file-bookmarks-store.h"
#include "gedit-file-browser-enum-types.h"

#define LOCATION_DATA_KEY "gedit-file-browser-widget-location"

enum
{
	BOOKMARKS_ID,
	SEPARATOR_CUSTOM_ID,
	SEPARATOR_ID,
	PATH_ID,
	NUM_DEFAULT_IDS
};

enum
{
	COLUMN_ICON,
	COLUMN_ICON_NAME,
	COLUMN_NAME,
	COLUMN_FILE,
	COLUMN_ID,
	N_COLUMNS
};

/* Properties */
enum
{
	PROP_0,

	PROP_FILTER_PATTERN,
};

/* Signals */
enum
{
	LOCATION_ACTIVATED,
	ERROR,
	CONFIRM_DELETE,
	CONFIRM_NO_TRASH,
	OPEN_IN_TERMINAL,
	SET_ACTIVE_ROOT,
	NUM_SIGNALS
};

static guint signals[NUM_SIGNALS] = { 0 };

typedef struct _SignalNode
{
	GObject *object;
	gulong   id;
} SignalNode;

typedef struct
{
	gulong                           id;
	GeditFileBrowserWidgetFilterFunc func;
	gpointer                         user_data;
	GDestroyNotify                   destroy_notify;
} FilterFunc;

typedef struct
{
	GFile *root;
	GFile *virtual_root;
} Location;

typedef struct
{
	gchar     *name;
	gchar     *icon_name;
	GdkPixbuf *icon;
} NameIcon;

struct _GeditFileBrowserWidgetPrivate
{
	GeditFileBrowserView    *treeview;
	GeditFileBrowserStore   *file_store;
	GeditFileBookmarksStore *bookmarks_store;

	GHashTable              *bookmarks_hash;

	GMenuModel              *dir_menu;
	GMenuModel              *bookmarks_menu;

	GtkWidget               *previous_button;
	GtkWidget               *next_button;

	GtkWidget               *locations_button;
	GtkWidget               *locations_popover;
	GtkWidget               *locations_treeview;
	GtkTreeViewColumn       *treeview_icon_column;
	GtkCellRenderer         *treeview_icon_renderer;
	GtkTreeSelection        *locations_treeview_selection;
	GtkWidget               *locations_button_arrow;
	GtkWidget               *locations_cellview;
	GtkListStore            *locations_model;

	GtkWidget               *location_entry;

	GtkWidget               *filter_entry_revealer;
	GtkWidget               *filter_entry;

	GSimpleActionGroup      *action_group;

	GSList                  *signal_pool;

	GSList                  *filter_funcs;
	gulong                   filter_id;
	gulong                   glob_filter_id;
	GPatternSpec            *filter_pattern;
	gchar                   *filter_pattern_str;

	GList                   *locations;
	GList                   *current_location;
	gboolean                 changing_location;
	GtkWidget               *location_previous_menu;
	GtkWidget               *location_next_menu;
	GtkWidget               *current_location_menu_item;

	GCancellable            *cancellable;

	GdkCursor               *busy_cursor;
};

static void on_model_set                       (GObject                *gobject,
						GParamSpec             *arg1,
						GeditFileBrowserWidget *obj);
static void on_treeview_error                  (GeditFileBrowserView   *tree_view,
						guint                   code,
						gchar                  *message,
						GeditFileBrowserWidget *obj);
static void on_file_store_error                (GeditFileBrowserStore  *store,
						guint                   code,
						gchar                  *message,
						GeditFileBrowserWidget *obj);
static gboolean on_file_store_no_trash 	       (GeditFileBrowserStore  *store,
						GList                  *files,
						GeditFileBrowserWidget *obj);
static gboolean on_location_button_press_event (GtkWidget              *button,
						GdkEventButton         *event,
						GeditFileBrowserWidget *obj);
static void on_location_entry_activate         (GtkEntry               *entry,
						GeditFileBrowserWidget *obj);
static gboolean
	    on_location_entry_focus_out_event  (GtkWidget              *entry,
						GdkEvent               *event,
						GeditFileBrowserWidget *obj);
static gboolean
	  on_location_entry_key_press_event    (GtkWidget              *entry,
						GdkEventKey            *event,
						GeditFileBrowserWidget *obj);
static gboolean on_treeview_popup_menu         (GeditFileBrowserView   *treeview,
						GeditFileBrowserWidget *obj);
static gboolean on_treeview_button_press_event (GeditFileBrowserView   *treeview,
						GdkEventButton         *event,
						GeditFileBrowserWidget *obj);
static gboolean on_treeview_key_press_event    (GeditFileBrowserView   *treeview,
						GdkEventKey            *event,
						GeditFileBrowserWidget *obj);
static void on_selection_changed               (GtkTreeSelection       *selection,
						GeditFileBrowserWidget *obj);

static void on_virtual_root_changed            (GeditFileBrowserStore  *model,
						GParamSpec             *param,
						GeditFileBrowserWidget *obj);

static gboolean on_entry_filter_activate       (GeditFileBrowserWidget *obj);
static void on_location_jump_activate          (GtkMenuItem            *item,
						GeditFileBrowserWidget *obj);
static void on_bookmarks_row_changed           (GtkTreeModel           *model,
                                                GtkTreePath            *path,
                                                GtkTreeIter            *iter,
                                                GeditFileBrowserWidget *obj);
static void on_bookmarks_row_deleted           (GtkTreeModel           *model,
                                                GtkTreePath            *path,
                                                GeditFileBrowserWidget *obj);
static void on_filter_mode_changed	       (GeditFileBrowserStore  *model,
                                                GParamSpec             *param,
                                                GeditFileBrowserWidget *obj);
static void previous_location_activated        (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void next_location_activated            (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void up_activated                       (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void home_activated                     (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void new_folder_activated               (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void open_activated                     (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void new_file_activated                 (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void rename_activated                   (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void delete_activated                   (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void move_to_trash_activated            (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void refresh_view_activated             (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void view_folder_activated              (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void change_show_hidden_state           (GSimpleAction          *action,
                                                GVariant               *state,
                                                gpointer                user_data);
static void change_show_binary_state           (GSimpleAction          *action,
                                                GVariant               *state,
                                                gpointer                user_data);
static void change_show_match_filename         (GSimpleAction          *action,
                                                GVariant               *state,
                                                gpointer                user_data);
static void open_in_terminal_activated         (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void set_active_root_activated          (GSimpleAction          *action,
                                                GVariant               *parameter,
                                                gpointer                user_data);
static void on_locations_treeview_selection_changed (GtkTreeSelection       *treeselection,
                                                     GeditFileBrowserWidget *obj);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditFileBrowserWidget,
				gedit_file_browser_widget,
				GTK_TYPE_GRID,
				0,
				G_ADD_PRIVATE_DYNAMIC (GeditFileBrowserWidget))

static void
free_name_icon (gpointer data)
{
	NameIcon *item = (NameIcon *)(data);

	if (item == NULL)
		return;

	g_free (item->icon_name);
	g_free (item->name);

	if (item->icon)
		g_object_unref (item->icon);

	g_slice_free (NameIcon, item);
}

static FilterFunc *
filter_func_new (GeditFileBrowserWidget           *obj,
		 GeditFileBrowserWidgetFilterFunc  func,
		 gpointer                          user_data,
		 GDestroyNotify                    notify)
{
	FilterFunc *result = g_slice_new (FilterFunc);

	result->id = ++obj->priv->filter_id;
	result->func = func;
	result->user_data = user_data;
	result->destroy_notify = notify;
	return result;
}

static void
location_free (Location *loc)
{
	if (loc->root)
		g_object_unref (loc->root);

	if (loc->virtual_root)
		g_object_unref (loc->virtual_root);

	g_slice_free (Location, loc);
}

static gboolean
locations_find_by_id (GeditFileBrowserWidget *obj,
		      guint                   id,
		      GtkTreeIter            *iter)
{
	GtkTreeModel *model = GTK_TREE_MODEL (obj->priv->locations_model);
	guint checkid;

	if (iter == NULL)
		return FALSE;

	if (gtk_tree_model_get_iter_first (model, iter))
	{
		do
		{
			gtk_tree_model_get (model,
					    iter,
					    COLUMN_ID, &checkid,
					    -1);

			if (checkid == id)
				return TRUE;
		}
		while (gtk_tree_model_iter_next (model, iter));
	}

	return FALSE;
}

static void
cancel_async_operation (GeditFileBrowserWidget *widget)
{
	if (!widget->priv->cancellable)
		return;

	g_cancellable_cancel (widget->priv->cancellable);
	g_object_unref (widget->priv->cancellable);

	widget->priv->cancellable = NULL;
}

static void
filter_func_free (FilterFunc *func)
{
	g_slice_free (FilterFunc, func);
}

static void
gedit_file_browser_widget_finalize (GObject *object)
{
	GeditFileBrowserWidgetPrivate *priv = GEDIT_FILE_BROWSER_WIDGET (object)->priv;

	g_free (priv->filter_pattern_str);

	G_OBJECT_CLASS (gedit_file_browser_widget_parent_class)->finalize (object);
}

static void
clear_signals (GeditFileBrowserWidget *obj)
{
	GSList *item = obj->priv->signal_pool;
	SignalNode *node;

	while (item != NULL)
	{
		node = (SignalNode *) (item->data);
		item = g_slist_delete_link (item, item);

		g_signal_handler_disconnect (node->object, node->id);
		g_slice_free (SignalNode, node);
	}

	obj->priv->signal_pool = NULL;
}

static void
gedit_file_browser_widget_dispose (GObject *object)
{
	GeditFileBrowserWidget *obj = GEDIT_FILE_BROWSER_WIDGET (object);
	GeditFileBrowserWidgetPrivate *priv = obj->priv;

	clear_signals (obj);
	g_clear_object (&priv->file_store);
	g_clear_object (&priv->bookmarks_store);

	g_slist_free_full (priv->filter_funcs, (GDestroyNotify)filter_func_free);
	g_list_free_full (priv->locations, (GDestroyNotify)location_free);

	if (priv->bookmarks_hash != NULL)
	{
		g_hash_table_unref (priv->bookmarks_hash);
		priv->bookmarks_hash = NULL;
	}

	cancel_async_operation (obj);

	g_clear_object (&obj->priv->current_location_menu_item);
	g_clear_object (&priv->busy_cursor);
	g_clear_object (&priv->dir_menu);
	g_clear_object (&priv->bookmarks_menu);

	G_OBJECT_CLASS (gedit_file_browser_widget_parent_class)->dispose (object);
}

static void
gedit_file_browser_widget_get_property (GObject   *object,
			               guint       prop_id,
			               GValue     *value,
			               GParamSpec *pspec)
{
	GeditFileBrowserWidget *obj = GEDIT_FILE_BROWSER_WIDGET (object);

	switch (prop_id)
	{
		case PROP_FILTER_PATTERN:
			g_value_set_string (value, obj->priv->filter_pattern_str);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_file_browser_widget_set_property (GObject     *object,
			               guint         prop_id,
			               const GValue *value,
			               GParamSpec   *pspec)
{
	GeditFileBrowserWidget *obj = GEDIT_FILE_BROWSER_WIDGET (object);

	switch (prop_id)
	{
		case PROP_FILTER_PATTERN:
			gedit_file_browser_widget_set_filter_pattern (obj,
			                                              g_value_get_string (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_file_browser_widget_class_init (GeditFileBrowserWidgetClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = gedit_file_browser_widget_finalize;
	object_class->dispose = gedit_file_browser_widget_dispose;

	object_class->get_property = gedit_file_browser_widget_get_property;
	object_class->set_property = gedit_file_browser_widget_set_property;

	g_object_class_install_property (object_class, PROP_FILTER_PATTERN,
					 g_param_spec_string ("filter-pattern",
					 		      "Filter Pattern",
					 		      "The filter pattern",
					 		      "",
					 		      G_PARAM_READWRITE));

	signals[LOCATION_ACTIVATED] =
	    g_signal_new ("location-activated",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditFileBrowserWidgetClass, location_activated),
			  NULL, NULL, NULL,
			  G_TYPE_NONE, 1, G_TYPE_FILE);

	signals[ERROR] =
	    g_signal_new ("error", G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditFileBrowserWidgetClass, error),
			  NULL, NULL, NULL,
			  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);

	signals[CONFIRM_DELETE] =
	    g_signal_new ("confirm-delete", G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_LAST,
	                  G_STRUCT_OFFSET (GeditFileBrowserWidgetClass, confirm_delete),
	                  g_signal_accumulator_true_handled, NULL, NULL,
	                  G_TYPE_BOOLEAN, 2, G_TYPE_OBJECT, G_TYPE_POINTER);

	signals[CONFIRM_NO_TRASH] =
	    g_signal_new ("confirm-no-trash", G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_LAST,
	                  G_STRUCT_OFFSET (GeditFileBrowserWidgetClass, confirm_no_trash),
	                  g_signal_accumulator_true_handled, NULL, NULL,
	                  G_TYPE_BOOLEAN, 1, G_TYPE_POINTER);

	signals[OPEN_IN_TERMINAL] =
	    g_signal_new ("open-in-terminal",
	                  G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_LAST,
	                  G_STRUCT_OFFSET (GeditFileBrowserWidgetClass, open_in_terminal),
	                  NULL, NULL, NULL,
	                  G_TYPE_NONE, 1, G_TYPE_FILE);

	signals[SET_ACTIVE_ROOT] =
	    g_signal_new ("set-active-root",
	                  G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_LAST,
	                  G_STRUCT_OFFSET (GeditFileBrowserWidgetClass, set_active_root),
	                  NULL, NULL, NULL,
	                  G_TYPE_NONE, 0);

	/* Bind class to template */
	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/gedit/plugins/file-browser/ui/gedit-file-browser-widget.ui");
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, previous_button);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, next_button);

	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, locations_button);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, locations_popover);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, locations_treeview);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, locations_treeview_selection);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, treeview_icon_column);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, treeview_icon_renderer);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, locations_cellview);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, locations_button_arrow);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, locations_model);

	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, location_entry);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, treeview);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, filter_entry_revealer);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, filter_entry);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, location_previous_menu);
	gtk_widget_class_bind_template_child_private (widget_class, GeditFileBrowserWidget, location_next_menu);
}

static void
gedit_file_browser_widget_class_finalize (GeditFileBrowserWidgetClass *klass)
{
}

static void
add_signal (GeditFileBrowserWidget *obj,
	    gpointer                object,
	    gulong                  id)
{
	SignalNode *node = g_slice_new (SignalNode);

	node->object = G_OBJECT (object);
	node->id = id;

	obj->priv->signal_pool = g_slist_prepend (obj->priv->signal_pool, node);
}

static gboolean
separator_func (GtkTreeModel *model,
		GtkTreeIter  *iter,
		gpointer      data)
{
	guint id;

	gtk_tree_model_get (model, iter, COLUMN_ID, &id, -1);

	return (id == SEPARATOR_ID);
}

static gboolean
get_from_bookmark_file (GeditFileBrowserWidget  *obj,
			GFile                   *file,
			gchar                  **name,
			gchar                  **icon_name,
			GdkPixbuf              **icon)
{
	NameIcon *item = (NameIcon *)g_hash_table_lookup (obj->priv->bookmarks_hash, file);

	if (item == NULL)
		return FALSE;

	*name = g_strdup (item->name);
	*icon_name = g_strdup (item->icon_name);

	if (icon != NULL && item->icon != NULL)
	{
		*icon = g_object_ref (item->icon);
	}

	return TRUE;
}

static void
insert_path_item (GeditFileBrowserWidget *obj,
                  GFile                  *file,
		  GtkTreeIter            *after,
		  GtkTreeIter            *iter)
{
	gchar *unescape = NULL;
	gchar *icon_name = NULL;
	GdkPixbuf *icon = NULL;

	/* Try to get the icon and name from the bookmarks hash */
	if (!get_from_bookmark_file (obj, file, &unescape, &icon_name, &icon))
	{
		/* It's not a bookmark, fetch the name and the icon ourselves */
		unescape = gedit_file_browser_utils_file_basename (file);

		/* Get the icon */
		icon_name = gedit_file_browser_utils_symbolic_icon_name_from_file (file);
	}

	gtk_list_store_insert_after (obj->priv->locations_model, iter, after);

	gtk_list_store_set (obj->priv->locations_model,
	                    iter,
	                    COLUMN_ICON, icon,
	                    COLUMN_ICON_NAME, icon_name,
	                    COLUMN_NAME, unescape,
	                    COLUMN_FILE, file,
	                    COLUMN_ID, PATH_ID,
	                    -1);

	if (icon)
		g_object_unref (icon);

	g_free (icon_name);
	g_free (unescape);
}

static void
insert_separator_item (GeditFileBrowserWidget *obj)
{
	GtkTreeIter iter;

	gtk_list_store_insert (obj->priv->locations_model, &iter, 1);
	gtk_list_store_set (obj->priv->locations_model, &iter,
			    COLUMN_ICON, NULL,
			    COLUMN_ICON_NAME, NULL,
			    COLUMN_NAME, NULL,
			    COLUMN_ID, SEPARATOR_ID, -1);
}

static void
insert_location_path (GeditFileBrowserWidget *obj)
{
	GeditFileBrowserWidgetPrivate *priv = obj->priv;
	Location *loc;
	GFile *current = NULL;
	GFile *tmp;
	GtkTreeIter separator;
	GtkTreeIter iter;

	if (!priv->current_location)
	{
		g_message ("insert_location_path: no current location");
		return;
	}

	loc = (Location *)(priv->current_location->data);

	current = loc->virtual_root;
	locations_find_by_id (obj, SEPARATOR_ID, &separator);

	while (current != NULL)
	{
		insert_path_item (obj, current, &separator, &iter);

		if (current == loc->virtual_root)
		{

			g_signal_handlers_block_by_func (priv->locations_treeview,
			                                 on_locations_treeview_selection_changed,
			                                 obj);

			gtk_tree_selection_select_iter (priv->locations_treeview_selection, &iter);

			g_signal_handlers_unblock_by_func (priv->locations_treeview,
			                                   on_locations_treeview_selection_changed,
			                                   obj);
		}

		if (g_file_equal (current, loc->root) ||
		    !g_file_has_parent (current, NULL))
		{
			if (current != loc->virtual_root)
				g_object_unref (current);
			break;
		}

		tmp = g_file_get_parent (current);

		if (current != loc->virtual_root)
			g_object_unref (current);

		current = tmp;
	}
}

static void
remove_path_items (GeditFileBrowserWidget *obj)
{
	GtkTreeIter iter;

	while (locations_find_by_id (obj, PATH_ID, &iter))
	{
		gtk_list_store_remove (obj->priv->locations_model, &iter);
	}
}

static void
check_current_item (GeditFileBrowserWidget *obj,
		    gboolean                show_path)
{
	GtkTreeIter separator;
	gboolean has_sep;

	remove_path_items (obj);
	has_sep = locations_find_by_id (obj, SEPARATOR_ID, &separator);

	if (show_path)
	{
		if (!has_sep)
			insert_separator_item (obj);

		insert_location_path (obj);
	}
	else if (has_sep)
	{
		gtk_list_store_remove (obj->priv->locations_model, &separator);
	}
}

static void
fill_locations_model (GeditFileBrowserWidget *obj)
{
	GeditFileBrowserWidgetPrivate *priv = obj->priv;
	GtkTreeIter iter;

	gtk_list_store_append (priv->locations_model, &iter);
	gtk_list_store_set (priv->locations_model,
			    &iter,
			    COLUMN_ICON, NULL,
			    COLUMN_ICON_NAME, "user-bookmarks-symbolic",
			    COLUMN_NAME, _("Bookmarks"),
			    COLUMN_ID, BOOKMARKS_ID, -1);

	gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (priv->locations_treeview),
	                                      separator_func,
	                                      obj,
	                                      NULL);

	gtk_tree_selection_select_iter (priv->locations_treeview_selection, &iter);

	on_locations_treeview_selection_changed (priv->locations_treeview_selection, obj);
	gedit_file_browser_widget_show_bookmarks (obj);
}

static gboolean
filter_real (GeditFileBrowserStore  *model,
	     GtkTreeIter            *iter,
	     GeditFileBrowserWidget *obj)
{
	GSList *item;

	for (item = obj->priv->filter_funcs; item; item = item->next)
	{
		FilterFunc *func = (FilterFunc *)(item->data);

		if (!func->func (obj, model, iter, func->user_data))
			return FALSE;
	}

	return TRUE;
}

static void
add_bookmark_hash (GeditFileBrowserWidget *obj,
		   GtkTreeIter            *iter)
{
	GtkTreeModel *model = GTK_TREE_MODEL (obj->priv->bookmarks_store);
	GdkPixbuf *pixbuf;
	gchar *name;
	gchar *icon_name;
	GFile *location;
	NameIcon *item;

	if (!(location = gedit_file_bookmarks_store_get_location (obj->priv->bookmarks_store, iter)))
		return;

	gtk_tree_model_get (model,
			    iter,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_ICON, &pixbuf,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_ICON_NAME, &icon_name,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_NAME, &name,
			    -1);

	item = g_slice_new (NameIcon);
	item->name = name;
	item->icon_name = icon_name;
	item->icon = pixbuf;

	g_hash_table_insert (obj->priv->bookmarks_hash,
			     location,
			     item);
}

static void
init_bookmarks_hash (GeditFileBrowserWidget *obj)
{
	GtkTreeModel *model = GTK_TREE_MODEL (obj->priv->bookmarks_store);
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_first (model, &iter))
		return;

	do
	{
		add_bookmark_hash (obj, &iter);
	}
	while (gtk_tree_model_iter_next (model, &iter));

	g_signal_connect (obj->priv->bookmarks_store,
		          "row-changed",
		          G_CALLBACK (on_bookmarks_row_changed),
		          obj);

	g_signal_connect (obj->priv->bookmarks_store,
		          "row-deleted",
		          G_CALLBACK (on_bookmarks_row_deleted),
		          obj);
}

static void
on_begin_loading (GeditFileBrowserStore  *model,
		  GtkTreeIter            *iter,
		  GeditFileBrowserWidget *obj)
{
	if (!GDK_IS_WINDOW (gtk_widget_get_window (GTK_WIDGET (obj->priv->treeview))))
		return;

	gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (obj)),
			       obj->priv->busy_cursor);
}

static void
on_end_loading (GeditFileBrowserStore  *model,
		GtkTreeIter            *iter,
		GeditFileBrowserWidget *obj)
{
	if (!GDK_IS_WINDOW (gtk_widget_get_window (GTK_WIDGET (obj->priv->treeview))))
		return;

	gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (obj)), NULL);
}

static void
on_locations_treeview_row_activated (GtkTreeView            *locations_treeview,
                                     GtkTreePath            *path,
                                     GtkTreeViewColumn      *column,
                                     GeditFileBrowserWidget *obj)
{
	GeditFileBrowserWidgetPrivate *priv = obj->priv;
	GtkTreeIter iter;
	guint id = G_MAXUINT;
	GFile *file;

	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->locations_model), &iter, path))
	{
		gtk_tree_model_get (GTK_TREE_MODEL (priv->locations_model), &iter, COLUMN_ID, &id, -1);
	}

	if (id == BOOKMARKS_ID)
	{
		gedit_file_browser_widget_show_bookmarks (obj);
	}
	else if (id == PATH_ID)
	{
		gtk_tree_model_get (GTK_TREE_MODEL (priv->locations_model),
			            &iter,
			            COLUMN_FILE, &file,
			            -1);

		gedit_file_browser_store_set_virtual_root_from_location (priv->file_store, file);

		g_object_unref (file);
		gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (priv->locations_cellview), path);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->locations_button), FALSE);
}

static GActionEntry browser_entries[] = {
	{ "open", open_activated },
	{ "set_active_root", set_active_root_activated },
	{ "new_folder", new_folder_activated },
	{ "new_file", new_file_activated },
	{ "rename", rename_activated },
	{ "move_to_trash", move_to_trash_activated },
	{ "delete", delete_activated },
	{ "refresh_view", refresh_view_activated },
	{ "view_folder", view_folder_activated },
	{ "open_in_terminal", open_in_terminal_activated },
	{ "show_hidden", NULL, NULL, "false", change_show_hidden_state },
	{ "show_binary", NULL, NULL, "false", change_show_binary_state },
	{ "show_match_filename", NULL, NULL, "false", change_show_match_filename },
	{ "previous_location", previous_location_activated },
	{ "next_location", next_location_activated },
	{ "up", up_activated },
	{ "home", home_activated }
};

static void
locations_icon_renderer_cb (GtkTreeViewColumn      *tree_column,
			    GtkCellRenderer        *cell,
			    GtkTreeModel           *tree_model,
			    GtkTreeIter            *iter,
			    GeditFileBrowserWidget *obj)
{
	GdkPixbuf *pixbuf;
	gchar *icon_name;

	gtk_tree_model_get (tree_model,
			    iter,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_ICON_NAME, &icon_name,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_ICON, &pixbuf,
			    -1);

	if (icon_name != NULL)
		g_object_set (cell, "icon-name", icon_name, NULL);
	else
		g_object_set (cell, "pixbuf", pixbuf, NULL);

	g_clear_object (&pixbuf);
	g_free (icon_name);
}

static void
gedit_file_browser_widget_init (GeditFileBrowserWidget *obj)
{
	GtkBuilder *builder;
	GdkDisplay *display;
	GAction *action;
	GError *error = NULL;

	obj->priv = gedit_file_browser_widget_get_instance_private (obj);

	obj->priv->filter_pattern_str = g_strdup ("");
	obj->priv->bookmarks_hash = g_hash_table_new_full (g_file_hash,
	                                                   (GEqualFunc)g_file_equal,
	                                                   g_object_unref,
	                                                   free_name_icon);

	display = gtk_widget_get_display (GTK_WIDGET (obj));
	obj->priv->busy_cursor = gdk_cursor_new_from_name (display, "progress");

	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_resource (builder,
	                                    "/org/gnome/gedit/plugins/file-browser/ui/gedit-file-browser-menus.ui",
	                                    &error))
	{
		g_warning ("loading menu builder file: %s", error->message);
		g_error_free (error);
	}
	else
	{
		obj->priv->dir_menu = G_MENU_MODEL (g_object_ref (gtk_builder_get_object (builder, "dir-menu")));
		obj->priv->bookmarks_menu = G_MENU_MODEL (g_object_ref (gtk_builder_get_object (builder, "bookmarks-menu")));
	}

	g_object_unref (builder);

	obj->priv->action_group = g_simple_action_group_new ();
	g_action_map_add_action_entries (G_ACTION_MAP (obj->priv->action_group),
	                                 browser_entries,
	                                 G_N_ELEMENTS (browser_entries),
	                                 obj);

	/* set initial sensitivity */
	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
	                                     "previous_location");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                             "next_location");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

	gtk_widget_insert_action_group (GTK_WIDGET (obj),
	                                "browser",
	                                G_ACTION_GROUP (obj->priv->action_group));

	gtk_widget_init_template (GTK_WIDGET (obj));

	g_signal_connect (obj->priv->previous_button, "button-press-event",
	                  G_CALLBACK (on_location_button_press_event), obj);
	g_signal_connect (obj->priv->next_button, "button-press-event",
	                  G_CALLBACK (on_location_button_press_event), obj);

	/* locations popover */
	gtk_tree_selection_set_mode (obj->priv->locations_treeview_selection, GTK_SELECTION_SINGLE);
	gtk_tree_view_column_set_cell_data_func (obj->priv->treeview_icon_column,
						 obj->priv->treeview_icon_renderer,
						 (GtkTreeCellDataFunc)locations_icon_renderer_cb,
						 obj,
						 NULL);
	fill_locations_model (obj);

	g_signal_connect (obj->priv->locations_treeview_selection, "changed",
	                  G_CALLBACK (on_locations_treeview_selection_changed), obj);
	g_signal_connect (obj->priv->locations_treeview, "row-activated",
	                  G_CALLBACK (on_locations_treeview_row_activated), obj);

	g_signal_connect (obj->priv->location_entry, "activate",
	                  G_CALLBACK (on_location_entry_activate), obj);
	g_signal_connect (obj->priv->location_entry, "focus-out-event",
	                  G_CALLBACK (on_location_entry_focus_out_event), obj);
	g_signal_connect (obj->priv->location_entry, "key-press-event",
	                  G_CALLBACK (on_location_entry_key_press_event), obj);

	/* tree view */
	obj->priv->file_store = gedit_file_browser_store_new (NULL);
	obj->priv->bookmarks_store = gedit_file_bookmarks_store_new ();

	gedit_file_browser_view_set_restore_expand_state (obj->priv->treeview, TRUE);

	gedit_file_browser_store_set_filter_mode (obj->priv->file_store,
						  GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN |
						  GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY);
	gedit_file_browser_store_set_filter_func (obj->priv->file_store,
						  (GeditFileBrowserStoreFilterFunc) filter_real,
						  obj);

	g_signal_connect (obj->priv->treeview, "notify::model",
			  G_CALLBACK (on_model_set), obj);
	g_signal_connect (obj->priv->treeview, "error",
			  G_CALLBACK (on_treeview_error), obj);
	g_signal_connect (obj->priv->treeview, "popup-menu",
			  G_CALLBACK (on_treeview_popup_menu), obj);
	g_signal_connect (obj->priv->treeview, "button-press-event",
			  G_CALLBACK (on_treeview_button_press_event),
			  obj);
	g_signal_connect (obj->priv->treeview, "key-press-event",
			  G_CALLBACK (on_treeview_key_press_event), obj);

	g_signal_connect (gtk_tree_view_get_selection
			  (GTK_TREE_VIEW (obj->priv->treeview)), "changed",
			  G_CALLBACK (on_selection_changed), obj);
	g_signal_connect (obj->priv->file_store, "notify::filter-mode",
			  G_CALLBACK (on_filter_mode_changed), obj);

	g_signal_connect (obj->priv->file_store, "notify::virtual-root",
			  G_CALLBACK (on_virtual_root_changed), obj);

	g_signal_connect (obj->priv->file_store, "begin-loading",
			  G_CALLBACK (on_begin_loading), obj);

	g_signal_connect (obj->priv->file_store, "end-loading",
			  G_CALLBACK (on_end_loading), obj);

	g_signal_connect (obj->priv->file_store, "error",
			  G_CALLBACK (on_file_store_error), obj);

	init_bookmarks_hash (obj);

	/* filter */
	g_signal_connect_swapped (obj->priv->filter_entry, "activate",
				  G_CALLBACK (on_entry_filter_activate),
				  obj);
	g_signal_connect_swapped (obj->priv->filter_entry, "focus_out_event",
				  G_CALLBACK (on_entry_filter_activate),
				  obj);
}

/* Private */

static void
update_sensitivity (GeditFileBrowserWidget *obj)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GAction *action;
	gint mode;

	if (GEDIT_IS_FILE_BROWSER_STORE (model))
	{
		mode = gedit_file_browser_store_get_filter_mode (GEDIT_FILE_BROWSER_STORE (model));

		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                                     "show_hidden");

		g_action_change_state (action,
		                       g_variant_new_boolean (!(mode &
		                                                GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN)));

		/* sensitivity */
		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                                     "up");
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), TRUE);
		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                                     "home");
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), TRUE);

		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                                     "show_hidden");
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), TRUE);

		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                                     "show_binary");
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), TRUE);

		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                                     "show_match_filename");
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), TRUE);
	}
	else if (GEDIT_IS_FILE_BOOKMARKS_STORE (model))
	{
		/* Set the filter toggle to normal up state, just for visual pleasure */
		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                                     "show_hidden");

		g_action_change_state (action, g_variant_new_boolean (FALSE));

		/* sensitivity */
		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                                     "up");
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                                     "home");
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                                     "show_hidden");
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                                     "show_binary");
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group),
		                                     "show_match_filename");
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
	}

	on_selection_changed (gtk_tree_view_get_selection
			      GTK_TREE_VIEW (obj->priv->treeview),
			      obj);
}

static gboolean
gedit_file_browser_widget_get_first_selected (GeditFileBrowserWidget *obj,
					      GtkTreeIter            *iter)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (obj->priv->treeview));
	GtkTreeModel *model;
	GList *rows = gtk_tree_selection_get_selected_rows (selection, &model);
	gboolean result;

	if (!rows)
		return FALSE;

	result = gtk_tree_model_get_iter (model, iter, (GtkTreePath *)(rows->data));

	g_list_free_full (rows, (GDestroyNotify) gtk_tree_path_free);

	return result;
}

static gboolean
popup_menu (GeditFileBrowserWidget *obj,
	    GtkTreeView            *treeview,
	    GdkEventButton         *event,
	    GtkTreeModel           *model)
{
	GtkWidget *menu;
	GMenuModel *menu_model;

	if (GEDIT_IS_FILE_BROWSER_STORE (model))
		menu_model = obj->priv->dir_menu;
	else if (GEDIT_IS_FILE_BOOKMARKS_STORE (model))
		menu_model = obj->priv->bookmarks_menu;
	else
		return FALSE;

	menu = gtk_menu_new_from_model (menu_model);
	gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (obj), NULL);

	if (event != NULL)
	{
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (treeview);

		if (gtk_tree_selection_count_selected_rows (selection) <= 1)
		{
			GtkTreePath *path;

			if (gtk_tree_view_get_path_at_pos (treeview,
			                                   (gint)event->x, (gint)event->y,
			                                   &path, NULL, NULL, NULL))
			{
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_path (selection, path);
				gtk_tree_path_free (path);
			}
		}

		gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *)event);
	}
	else
	{
		GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (treeview));
		GdkGravity rect_gravity = GDK_GRAVITY_EAST;
		GdkGravity menu_gravity = GDK_GRAVITY_NORTH_WEST;
		GdkRectangle rect;

		if (gedit_utils_menu_position_under_tree_view (treeview, &rect))
		{
			if (gtk_widget_get_direction (GTK_WIDGET (treeview)) == GTK_TEXT_DIR_RTL)
			{
				rect_gravity = GDK_GRAVITY_WEST;
				menu_gravity = GDK_GRAVITY_NORTH_EAST;
			}

			gtk_menu_popup_at_rect (GTK_MENU (menu), window, &rect, rect_gravity, menu_gravity, NULL);
			gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), FALSE);
		}
	}

	return TRUE;
}

static gboolean
filter_glob (GeditFileBrowserWidget *obj,
	     GeditFileBrowserStore  *store,
	     GtkTreeIter            *iter,
	     gpointer                user_data)
{
	gchar *name;
	gboolean result;
	guint flags;

	if (obj->priv->filter_pattern == NULL)
		return TRUE;

	gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_NAME, &name,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    -1);

	if (FILE_IS_DIR (flags) || FILE_IS_DUMMY (flags))
		result = TRUE;
	else
		result = g_pattern_spec_match_string (obj->priv->filter_pattern, name);

	g_free (name);
	return result;
}

static void
rename_selected_file (GeditFileBrowserWidget *obj)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GtkTreeIter iter;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;

	if (gedit_file_browser_widget_get_first_selected (obj, &iter))
		gedit_file_browser_view_start_rename (obj->priv->treeview, &iter);
}

static GList *
get_deletable_files (GeditFileBrowserWidget *obj)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (obj->priv->treeview));
	GList *rows = gtk_tree_selection_get_selected_rows (selection, &model);
	GList *row;
	GList *paths = NULL;

	for (row = rows; row; row = row->next)
	{
		GtkTreePath *path = (GtkTreePath *)(row->data);
		GtkTreeIter iter;
		guint flags;

		if (!gtk_tree_model_get_iter (model, &iter, path))
			continue;

		gtk_tree_model_get (model,
				    &iter,
				    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
				    -1);

		if (FILE_IS_DUMMY (flags))
			continue;

		paths = g_list_append (paths, gtk_tree_path_copy (path));
	}

	g_list_free_full (rows, (GDestroyNotify) gtk_tree_path_free);

	return paths;
}

static gboolean
delete_selected_files (GeditFileBrowserWidget *obj,
		       gboolean                trash)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	gboolean confirm;
	GeditFileBrowserStoreResult result;
	GList *rows;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return FALSE;

	if (!(rows = get_deletable_files (obj)))
		return FALSE;

	if (!trash)
	{
		g_signal_emit (obj, signals[CONFIRM_DELETE], 0, model, rows, &confirm);

		if (!confirm)
			return FALSE;
	}

	result = gedit_file_browser_store_delete_all (GEDIT_FILE_BROWSER_STORE (model),
						      rows,
						      trash);

	g_list_free_full (rows, (GDestroyNotify) gtk_tree_path_free);

	return result == GEDIT_FILE_BROWSER_STORE_RESULT_OK;
}

static void
show_location_entry (GeditFileBrowserWidget *obj,
                     const gchar            *location)
{
	g_warn_if_fail (location != NULL);

	gtk_entry_set_text (GTK_ENTRY (obj->priv->location_entry), location);

	gtk_widget_show (obj->priv->location_entry);
	gtk_widget_grab_focus (obj->priv->location_entry);

	/* grab_focus() causes the entry's text to become
	 * selected so, unselect it and move the cursor to the end.
	 */
	gtk_editable_set_position (GTK_EDITABLE (obj->priv->location_entry), -1);
}

static gboolean
on_file_store_no_trash (GeditFileBrowserStore  *store,
			GList                  *files,
			GeditFileBrowserWidget *obj)
{
	gboolean confirm = FALSE;

	g_signal_emit (obj, signals[CONFIRM_NO_TRASH], 0, files, &confirm);

	return confirm;
}

static GFile *
get_topmost_file (GFile *file)
{
	GFile *current = g_object_ref (file);
	GFile *tmp;

	while ((tmp = g_file_get_parent (current)) != NULL)
	{
		g_object_unref (current);
		current = tmp;
	}

	return current;
}

static GtkWidget *
create_goto_menu_item (GeditFileBrowserWidget *obj,
		       GList                  *item)
{
	Location *loc = (Location *) (item->data);
	GtkWidget *result;
	gchar *icon_name = NULL;
	gchar *unescape = NULL;

	if (!get_from_bookmark_file (obj, loc->virtual_root, &unescape, &icon_name, NULL))
		unescape = gedit_file_browser_utils_file_basename (loc->virtual_root);

	result = gtk_menu_item_new_with_label (unescape);

	g_object_set_data (G_OBJECT (result), LOCATION_DATA_KEY, item);
	g_signal_connect (result, "activate",
			  G_CALLBACK (on_location_jump_activate), obj);

	gtk_widget_show (result);

	g_free (icon_name);
	g_free (unescape);

	return result;
}

static GList *
list_next_iterator (GList *list)
{
	if (!list)
		return NULL;

	return list->next;
}

static GList *
list_prev_iterator (GList *list)
{
	if (!list)
		return NULL;

	return list->prev;
}

static void
jump_to_location (GeditFileBrowserWidget *obj,
		  GList                  *item,
		  gboolean                previous)
{
	Location *loc;
	GtkWidget *widget;
	GList *children;
	GList *child;
	GList *(*iter_func) (GList *);
	GtkWidget *menu_from;
	GtkWidget *menu_to;

	if (!obj->priv->locations)
		return;

	if (previous)
	{
		iter_func = list_next_iterator;
		menu_from = obj->priv->location_previous_menu;
		menu_to = obj->priv->location_next_menu;
	}
	else
	{
		iter_func = list_prev_iterator;
		menu_from = obj->priv->location_next_menu;
		menu_to = obj->priv->location_previous_menu;
	}

	children = gtk_container_get_children (GTK_CONTAINER (menu_from));
	child = children;

	/* This is the menuitem for the current location, which is the first
	   to be added to the menu */
	widget = obj->priv->current_location_menu_item;

	while (obj->priv->current_location != item)
	{
		if (widget)
		{
			/* Prepend the menu item to the menu */
			gtk_menu_shell_prepend (GTK_MENU_SHELL (menu_to), widget);
			g_object_unref (widget);
		}

		widget = GTK_WIDGET (child->data);

		/* Make sure the widget isn't destroyed when removed */
		g_object_ref (widget);
		gtk_container_remove (GTK_CONTAINER (menu_from), widget);

		obj->priv->current_location_menu_item = widget;

		if (obj->priv->current_location == NULL)
		{
			obj->priv->current_location = obj->priv->locations;

			if (obj->priv->current_location == item)
				break;
		}
		else
		{
			obj->priv->current_location = iter_func (obj->priv->current_location);
		}

		child = child->next;
	}

	g_list_free (children);

	obj->priv->changing_location = TRUE;

	loc = (Location *) (obj->priv->current_location->data);

	/* Set the new root + virtual root */
	gedit_file_browser_widget_set_root_and_virtual_root (obj,
							     loc->root,
							     loc->virtual_root);

	obj->priv->changing_location = FALSE;
}

static void
clear_next_locations (GeditFileBrowserWidget *obj)
{
	GAction *action;
	GList *children;
	GList *item;

	if (obj->priv->current_location == NULL)
		return;

	while (obj->priv->current_location->prev)
	{
		location_free ((Location *) (obj->priv->current_location->prev->data));
		obj->priv->locations = g_list_remove_link (obj->priv->locations,
							   obj->priv->current_location->prev);
	}

	children = gtk_container_get_children (GTK_CONTAINER (obj->priv->location_next_menu));

	for (item = children; item; item = item->next)
	{
		gtk_container_remove (GTK_CONTAINER
				      (obj->priv->location_next_menu),
				      GTK_WIDGET (item->data));
	}

	g_list_free (children);

	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "next_location");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
}

static void
update_filter_mode (GeditFileBrowserWidget          *obj,
                    GSimpleAction                   *action,
                    GVariant                        *state,
                    GeditFileBrowserStoreFilterMode  mode)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));

	if (GEDIT_IS_FILE_BROWSER_STORE (model))
	{
		gint now = gedit_file_browser_store_get_filter_mode (GEDIT_FILE_BROWSER_STORE (model));

		if (g_variant_get_boolean(state))
			now &= ~mode;
		else
			now |= mode;

		gedit_file_browser_store_set_filter_mode (GEDIT_FILE_BROWSER_STORE (model), now);

	}

	g_simple_action_set_state (action, state);
}

static void
set_filter_pattern_real (GeditFileBrowserWidget *obj,
                        gchar const             *pattern,
                        gboolean                 update_entry)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));

	if (pattern != NULL && *pattern == '\0')
		pattern = NULL;

	if (pattern == NULL && *obj->priv->filter_pattern_str == '\0')
		return;

	if (pattern != NULL && strcmp (pattern, obj->priv->filter_pattern_str) == 0)
		return;

	/* Free the old pattern */
	g_free (obj->priv->filter_pattern_str);

	if (pattern == NULL)
		obj->priv->filter_pattern_str = g_strdup ("");
	else
		obj->priv->filter_pattern_str = g_strdup (pattern);

	if (obj->priv->filter_pattern)
	{
		g_pattern_spec_free (obj->priv->filter_pattern);
		obj->priv->filter_pattern = NULL;
	}

	if (pattern == NULL)
	{
		if (obj->priv->glob_filter_id != 0)
		{
			gedit_file_browser_widget_remove_filter (obj, obj->priv->glob_filter_id);
			obj->priv->glob_filter_id = 0;
		}
	}
	else
	{
		obj->priv->filter_pattern = g_pattern_spec_new (pattern);

		if (obj->priv->glob_filter_id == 0)
			obj->priv->glob_filter_id = gedit_file_browser_widget_add_filter (obj,
											  filter_glob,
											  NULL,
											  NULL);
	}

	if (update_entry)
		gtk_entry_set_text (GTK_ENTRY (obj->priv->filter_entry), obj->priv->filter_pattern_str);

	if (GEDIT_IS_FILE_BROWSER_STORE (model))
		gedit_file_browser_store_refilter (GEDIT_FILE_BROWSER_STORE (model));

	g_object_notify (G_OBJECT (obj), "filter-pattern");
}


/* Public */

GtkWidget *
gedit_file_browser_widget_new (void)
{
	GeditFileBrowserWidget *obj = g_object_new (GEDIT_TYPE_FILE_BROWSER_WIDGET, NULL);

	gedit_file_browser_widget_show_bookmarks (obj);

	return GTK_WIDGET (obj);
}

void
gedit_file_browser_widget_show_bookmarks (GeditFileBrowserWidget *obj)
{
	GtkTreePath *path;
	GtkTreeIter iter;

	gtk_widget_set_sensitive (obj->priv->locations_button, FALSE);
	gtk_widget_hide (obj->priv->locations_button_arrow);
	locations_find_by_id (obj, BOOKMARKS_ID, &iter);

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (obj->priv->locations_model), &iter);
	gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (obj->priv->locations_cellview), path);
	gtk_tree_path_free (path);

	gedit_file_browser_view_set_model (obj->priv->treeview,
					   GTK_TREE_MODEL (obj->priv->bookmarks_store));
}

static void
show_files_real (GeditFileBrowserWidget *obj,
		 gboolean                do_root_changed)
{
	gtk_widget_set_sensitive (obj->priv->locations_button, TRUE);
	gtk_widget_show (obj->priv->locations_button_arrow);

	gedit_file_browser_view_set_model (obj->priv->treeview,
					   GTK_TREE_MODEL (obj->priv->file_store));

	if (do_root_changed)
		on_virtual_root_changed (obj->priv->file_store, NULL, obj);
}

void
gedit_file_browser_widget_show_files (GeditFileBrowserWidget *obj)
{
	show_files_real (obj, TRUE);
}

void
gedit_file_browser_widget_set_root_and_virtual_root (GeditFileBrowserWidget *obj,
						     GFile                  *root,
						     GFile                  *virtual_root)
{
	GeditFileBrowserStoreResult result;

	if (!virtual_root)
		result = gedit_file_browser_store_set_root_and_virtual_root (obj->priv->file_store,
									     root,
									     root);
	else
		result = gedit_file_browser_store_set_root_and_virtual_root (obj->priv->file_store,
									     root,
									     virtual_root);

	if (result == GEDIT_FILE_BROWSER_STORE_RESULT_NO_CHANGE)
		show_files_real (obj, TRUE);
}

void
gedit_file_browser_widget_set_root (GeditFileBrowserWidget *obj,
				    GFile                  *root,
				    gboolean                virtual_root)
{
	GFile *parent;

	if (!virtual_root)
	{
		gedit_file_browser_widget_set_root_and_virtual_root (obj,
								     root,
								     NULL);
		return;
	}

	if (!root)
		return;

	parent = get_topmost_file (root);
	gedit_file_browser_widget_set_root_and_virtual_root (obj, parent, root);
	g_object_unref (parent);
}

GeditFileBrowserStore *
gedit_file_browser_widget_get_browser_store (GeditFileBrowserWidget *obj)
{
	return obj->priv->file_store;
}

GeditFileBookmarksStore *
gedit_file_browser_widget_get_bookmarks_store (GeditFileBrowserWidget *obj)
{
	return obj->priv->bookmarks_store;
}

GeditFileBrowserView *
gedit_file_browser_widget_get_browser_view (GeditFileBrowserWidget *obj)
{
	return obj->priv->treeview;
}

GtkWidget *
gedit_file_browser_widget_get_filter_entry (GeditFileBrowserWidget *obj)
{
	return obj->priv->filter_entry;
}

gulong
gedit_file_browser_widget_add_filter (GeditFileBrowserWidget           *obj,
				      GeditFileBrowserWidgetFilterFunc  func,
				      gpointer                          user_data,
				      GDestroyNotify                    notify)
{
	FilterFunc *f = filter_func_new (obj, func, user_data, notify);
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));

	obj->priv->filter_funcs = g_slist_append (obj->priv->filter_funcs, f);

	if (GEDIT_IS_FILE_BROWSER_STORE (model))
		gedit_file_browser_store_refilter (GEDIT_FILE_BROWSER_STORE (model));

	return f->id;
}

void
gedit_file_browser_widget_remove_filter (GeditFileBrowserWidget *obj,
					 gulong                  id)
{
	GSList *item;

	for (item = obj->priv->filter_funcs; item; item = item->next)
	{
		FilterFunc *func = (FilterFunc *) (item->data);

		if (func->id == id)
		{
			if (func->destroy_notify)
				func->destroy_notify (func->user_data);

			obj->priv->filter_funcs = g_slist_remove_link (obj->priv->filter_funcs, item);

			filter_func_free (func);
			break;
		}
	}
}

void
gedit_file_browser_widget_set_filter_pattern (GeditFileBrowserWidget *obj,
                                              gchar const            *pattern)
{
	gboolean show;
	GAction *action;

	/* if pattern is not null, reveal the entry */
	show = pattern != NULL && *pattern != '\0';
	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "show_match_filename");
	g_action_change_state (action, g_variant_new_boolean (show));

	set_filter_pattern_real (obj, pattern, TRUE);
}

gboolean
gedit_file_browser_widget_get_selected_directory (GeditFileBrowserWidget *obj,
						  GtkTreeIter            *iter)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GtkTreeIter parent;
	guint flags;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return FALSE;

	if (!gedit_file_browser_widget_get_first_selected (obj, iter) &&
	    !gedit_file_browser_store_get_iter_virtual_root
	    		(GEDIT_FILE_BROWSER_STORE (model), iter))
	{
		return FALSE;
	}

	gtk_tree_model_get (model, iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    -1);

	if (!FILE_IS_DIR (flags))
	{
		/* Get the parent, because the selection is a file */
		gtk_tree_model_iter_parent (model, &parent, iter);
		*iter = parent;
	}

	return TRUE;
}

void
gedit_file_browser_widget_set_active_root_enabled (GeditFileBrowserWidget *widget,
                                                   gboolean                enabled)
{
	GAction *action;

	g_return_if_fail (GEDIT_IS_FILE_BROWSER_WIDGET (widget));

	action = g_action_map_lookup_action (G_ACTION_MAP (widget->priv->action_group), "set_active_root");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);
}

static guint
gedit_file_browser_widget_get_num_selected_files_or_directories (GeditFileBrowserWidget *obj,
								 guint                  *files,
								 guint                  *dirs)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (obj->priv->treeview));
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GList *rows, *row;
	guint result = 0;

	if (GEDIT_IS_FILE_BOOKMARKS_STORE (model))
		return 0;

	rows = gtk_tree_selection_get_selected_rows (selection, &model);

	for (row = rows; row; row = row->next)
	{
		GtkTreePath *path = (GtkTreePath *)(row->data);
		GeditFileBrowserStoreFlag flags;
		GtkTreeIter iter;

		/* Get iter from path */
		if (!gtk_tree_model_get_iter (model, &iter, path))
			continue;

		gtk_tree_model_get (model, &iter,
				    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
				    -1);

		if (!FILE_IS_DUMMY (flags))
		{
			if (!FILE_IS_DIR (flags))
				++(*files);
			else
				++(*dirs);

			++result;
		}
	}

	g_list_free_full (rows, (GDestroyNotify) gtk_tree_path_free);

	return result;
}

typedef struct
{
	GeditFileBrowserWidget *widget;
	GCancellable           *cancellable;
} AsyncData;

static AsyncData *
async_data_new (GeditFileBrowserWidget *widget)
{
	AsyncData *ret = g_slice_new (AsyncData);

	ret->widget = widget;

	cancel_async_operation (widget);
	widget->priv->cancellable = g_cancellable_new ();

	ret->cancellable = g_object_ref (widget->priv->cancellable);

	return ret;
}

static void
async_free (AsyncData *async)
{
	g_object_unref (async->cancellable);
	g_slice_free (AsyncData, async);
}

static void
set_busy (GeditFileBrowserWidget *obj,
	  gboolean                busy)
{
	GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (obj->priv->treeview));

	if (!GDK_IS_WINDOW (window))
		return;

	if (busy)
	{
		GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (obj));
		GdkCursor *cursor= gdk_cursor_new_from_name (display, "progress");

		gdk_window_set_cursor (window, cursor);
		g_clear_object (&cursor);
	}
	else
	{
		gdk_window_set_cursor (window, NULL);
	}
}

static void try_mount_volume (GeditFileBrowserWidget *widget, GVolume *volume);

static void
activate_mount (GeditFileBrowserWidget *widget,
		GVolume		       *volume,
		GMount		       *mount)
{
	GFile *root;

	if (!mount)
	{
		gchar *name = g_volume_get_name (volume);
		gchar *message = g_strdup_printf (_("No mount object for mounted volume: %s"), name);

		g_signal_emit (widget,
			       signals[ERROR],
			       0,
			       GEDIT_FILE_BROWSER_ERROR_SET_ROOT,
			       message);

		g_free (name);
		g_free (message);
		return;
	}

	root = g_mount_get_root (mount);

	gedit_file_browser_widget_set_root (widget, root, FALSE);

	g_object_unref (root);
}

static void
try_activate_drive (GeditFileBrowserWidget *widget,
		    GDrive 		   *drive)
{
	GList *volumes = g_drive_get_volumes (drive);
	GVolume *volume = G_VOLUME (volumes->data);
	GMount *mount = g_volume_get_mount (volume);

	if (mount)
	{
		/* try set the root of the mount */
		activate_mount (widget, volume, mount);
		g_object_unref (mount);
	}
	else
	{
		/* try to mount it then? */
		try_mount_volume (widget, volume);
	}

	g_list_free_full (volumes, g_object_unref);
}

static void
poll_for_media_cb (GDrive       *drive,
		   GAsyncResult *res,
		   AsyncData 	*async)
{
	GError *error = NULL;

	/* check for cancelled state */
	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_free (async);
		return;
	}

	/* finish poll operation */
	set_busy (async->widget, FALSE);

	if (g_drive_poll_for_media_finish (drive, res, &error) &&
	    g_drive_has_media (drive) &&
	    g_drive_has_volumes (drive))
	{
		try_activate_drive (async->widget, drive);
	}
	else
	{
		gchar *name = g_drive_get_name (drive);
		gchar *message = g_strdup_printf (_("Could not open media: %s"), name);

		g_signal_emit (async->widget,
			       signals[ERROR],
			       0,
			       GEDIT_FILE_BROWSER_ERROR_SET_ROOT,
			       message);

		g_free (name);
		g_free (message);

		g_error_free (error);
	}

	async_free (async);
}

static void
mount_volume_cb (GVolume      *volume,
		 GAsyncResult *res,
		 AsyncData    *async)
{
	GError *error = NULL;

	/* check for cancelled state */
	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_free (async);
		return;
	}

	if (g_volume_mount_finish (volume, res, &error))
	{
		GMount *mount = g_volume_get_mount (volume);

		activate_mount (async->widget, volume, mount);

		if (mount)
			g_object_unref (mount);
	}
	else
	{
		gchar *name = g_volume_get_name (volume);
		gchar *message = g_strdup_printf (_("Could not mount volume: %s"), name);

		g_signal_emit (async->widget,
			       signals[ERROR],
			       0,
			       GEDIT_FILE_BROWSER_ERROR_SET_ROOT,
			       message);

		g_free (name);
		g_free (message);

		g_error_free (error);
	}

	set_busy (async->widget, FALSE);
	async_free (async);
}

static void
activate_drive (GeditFileBrowserWidget *obj,
		GtkTreeIter	       *iter)
{
	GDrive *drive;
	AsyncData *async;

	gtk_tree_model_get (GTK_TREE_MODEL (obj->priv->bookmarks_store),
			    iter,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_OBJECT, &drive,
			    -1);

	/* most common use case is a floppy drive, we'll poll for media and
	   go from there */
	async = async_data_new (obj);
	g_drive_poll_for_media (drive,
				async->cancellable,
				(GAsyncReadyCallback)poll_for_media_cb,
				async);

	g_object_unref (drive);
	set_busy (obj, TRUE);
}

static void
try_mount_volume (GeditFileBrowserWidget *widget,
		  GVolume 		 *volume)
{
	GMountOperation *operation = gtk_mount_operation_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (widget))));
	AsyncData *async = async_data_new (widget);

	g_volume_mount (volume,
			G_MOUNT_MOUNT_NONE,
			operation,
			async->cancellable,
			(GAsyncReadyCallback)mount_volume_cb,
			async);

	g_object_unref (operation);
	set_busy (widget, TRUE);
}

static void
activate_volume (GeditFileBrowserWidget *obj,
		 GtkTreeIter	        *iter)
{
	GVolume *volume;

	gtk_tree_model_get (GTK_TREE_MODEL (obj->priv->bookmarks_store),
			    iter,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_OBJECT, &volume,
			    -1);

	/* see if we can mount the volume */
	try_mount_volume (obj, volume);
	g_object_unref (volume);
}

void
gedit_file_browser_widget_refresh (GeditFileBrowserWidget *obj)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));

	if (GEDIT_IS_FILE_BROWSER_STORE (model))
	{
		gedit_file_browser_store_refresh (GEDIT_FILE_BROWSER_STORE (model));
	}
	else if (GEDIT_IS_FILE_BOOKMARKS_STORE (model))
	{
		g_hash_table_ref (obj->priv->bookmarks_hash);
		g_hash_table_destroy (obj->priv->bookmarks_hash);

		gedit_file_bookmarks_store_refresh (GEDIT_FILE_BOOKMARKS_STORE (model));
	}
}

GeditMenuExtension *
gedit_file_browser_widget_extend_context_menu (GeditFileBrowserWidget *obj)
{
	guint i, n_items;
	GMenuModel *section = NULL;

	g_return_val_if_fail (GEDIT_IS_FILE_BROWSER_WIDGET (obj), NULL);

	n_items = g_menu_model_get_n_items (obj->priv->dir_menu);

	for (i = 0; i < n_items && !section; i++)
	{
		gchar *id = NULL;

		if (g_menu_model_get_item_attribute (obj->priv->dir_menu, i, "id", "s", &id) &&
		    strcmp (id, "extension-section") == 0)
		{
			section = g_menu_model_get_item_link (obj->priv->dir_menu, i, G_MENU_LINK_SECTION);
		}

		g_free (id);
	}

	return section != NULL ? gedit_menu_extension_new (G_MENU (section)) : NULL;
}

void
gedit_file_browser_widget_history_back (GeditFileBrowserWidget *obj)
{
	if (obj->priv->locations)
	{
		if (obj->priv->current_location)
			jump_to_location (obj, obj->priv->current_location->next, TRUE);
		else
			jump_to_location (obj, obj->priv->locations, TRUE);
	}
}

void
gedit_file_browser_widget_history_forward (GeditFileBrowserWidget *obj)
{
	if (obj->priv->locations)
		jump_to_location (obj, obj->priv->current_location->prev, FALSE);
}

static void
bookmark_open (GeditFileBrowserWidget *obj,
	       GtkTreeModel           *model,
	       GtkTreeIter            *iter)
{
	GFile *location;
	gint flags;

	gtk_tree_model_get (model,
			    iter,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS, &flags,
			    -1);

	if (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_DRIVE)
	{
		/* handle a drive node */
		gedit_file_browser_store_cancel_mount_operation (obj->priv->file_store);
		activate_drive (obj, iter);
		return;
	}
	else if (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_VOLUME)
	{
		/* handle a volume node */
		gedit_file_browser_store_cancel_mount_operation (obj->priv->file_store);
		activate_volume (obj, iter);
		return;
	}

	if ((location = gedit_file_bookmarks_store_get_location (GEDIT_FILE_BOOKMARKS_STORE (model), iter)))
	{
		/* here we check if the bookmark is a mount point, or if it
		   is a remote bookmark. If that's the case, we will set the
		   root to the uri of the bookmark and not try to set the
		   topmost parent as root (since that may as well not be the
		   mount point anymore) */
		if ((flags & GEDIT_FILE_BOOKMARKS_STORE_IS_MOUNT) ||
		    (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_REMOTE_BOOKMARK))
		{
			gedit_file_browser_widget_set_root (obj, location, FALSE);
		}
		else
		{
			gedit_file_browser_widget_set_root (obj, location, TRUE);
		}

		g_object_unref (location);
	}
	else
	{
		g_warning ("No uri!");
	}
}

static void
file_open (GeditFileBrowserWidget *obj,
	   GtkTreeModel           *model,
	   GtkTreeIter            *iter)
{
	GFile *location;
	gint flags;

	gtk_tree_model_get (model, iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &location,
			    -1);

	if (!FILE_IS_DIR (flags) && !FILE_IS_DUMMY (flags))
		g_signal_emit (obj, signals[LOCATION_ACTIVATED], 0, location);

	if (location)
		g_object_unref (location);
}

static gboolean
directory_open (GeditFileBrowserWidget *obj,
		GtkTreeModel           *model,
		GtkTreeIter            *iter)
{
	gboolean result = FALSE;
	GError *error = NULL;
	GFile *location;
	GeditFileBrowserStoreFlag flags;

	gtk_tree_model_get (model, iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &location,
			    -1);

	if (FILE_IS_DIR (flags) && location)
	{
		gchar *uri = g_file_get_uri (location);
		GtkWindow *window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (obj)));

		result = TRUE;

		if (!gtk_show_uri_on_window (window, uri, GDK_CURRENT_TIME, &error))
		{
			g_signal_emit (obj, signals[ERROR], 0,
				       GEDIT_FILE_BROWSER_ERROR_OPEN_DIRECTORY,
				       error->message);

			g_error_free (error);
			error = NULL;
		}

		g_free (uri);
		g_object_unref (location);
	}

	return result;
}

static void
on_bookmark_activated (GeditFileBrowserView   *tree_view,
		       GtkTreeIter            *iter,
		       GeditFileBrowserWidget *obj)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));

	bookmark_open (obj, model, iter);
}

static void
on_file_activated (GeditFileBrowserView   *tree_view,
		   GtkTreeIter            *iter,
		   GeditFileBrowserWidget *obj)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));

	file_open (obj, model, iter);
}

static gboolean
virtual_root_is_root (GeditFileBrowserWidget *obj,
                      GeditFileBrowserStore  *model)
{
	GtkTreeIter root;
	GtkTreeIter virtual_root;

	if (!gedit_file_browser_store_get_iter_root (model, &root))
		return TRUE;

	if (!gedit_file_browser_store_get_iter_virtual_root (model, &virtual_root))
		return TRUE;

	return gedit_file_browser_store_iter_equal (model, &root, &virtual_root);
}

static void
on_virtual_root_changed (GeditFileBrowserStore  *model,
			 GParamSpec             *param,
			 GeditFileBrowserWidget *obj)
{
	GtkTreeIter iter;

	if (gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview)) !=
	    GTK_TREE_MODEL (obj->priv->file_store))
	{
		show_files_real (obj, FALSE);
	}

	if (gedit_file_browser_store_get_iter_virtual_root (model, &iter))
	{
		GFile *location;
		GtkTreeIter root;

		gtk_tree_model_get (GTK_TREE_MODEL (model),
				    &iter,
				    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &location,
				    -1);

		if (gedit_file_browser_store_get_iter_root (model, &root))
		{
			GAction *action;

			if (!obj->priv->changing_location)
			{
				Location *loc;

				/* Remove all items from obj->priv->current_location on */
				if (obj->priv->current_location)
					clear_next_locations (obj);

				loc = g_slice_new (Location);
				loc->root = gedit_file_browser_store_get_root (model);
				loc->virtual_root = g_object_ref (location);

				if (obj->priv->current_location)
				{
					/* Add current location to the menu so we can go back
					   to it later */
					gtk_menu_shell_prepend (GTK_MENU_SHELL (obj->priv->location_previous_menu),
								obj->priv->current_location_menu_item);
				}

				obj->priv->locations = g_list_prepend (obj->priv->locations, loc);

				obj->priv->current_location = obj->priv->locations;
				obj->priv->current_location_menu_item = create_goto_menu_item (obj,
											       obj->priv->current_location);

				g_object_ref_sink (obj->priv->current_location_menu_item);
			}

			action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "up");
			g_simple_action_set_enabled (G_SIMPLE_ACTION (action), !virtual_root_is_root (obj, model));

			action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "previous_location");
			g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
			                             obj->priv->current_location != NULL &&
			                             obj->priv->current_location->next != NULL);

			action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "next_location");
			g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
			                             obj->priv->current_location != NULL &&
			                             obj->priv->current_location->prev != NULL);
		}

		check_current_item (obj, TRUE);

		if (location)
			g_object_unref (location);
	}
	else
	{
		g_message ("NO!");
	}
}

static void
on_model_set (GObject                *gobject,
              GParamSpec             *arg1,
              GeditFileBrowserWidget *obj)
{
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (gobject));

	clear_signals (obj);

	if (GEDIT_IS_FILE_BOOKMARKS_STORE (model))
	{
		clear_next_locations (obj);

		/* Add the current location to the back menu */
		if (obj->priv->current_location)
		{
			GAction *action;

			gtk_menu_shell_prepend (GTK_MENU_SHELL (obj->priv->location_previous_menu),
						obj->priv->current_location_menu_item);

			g_object_unref (obj->priv->current_location_menu_item);
			obj->priv->current_location = NULL;
			obj->priv->current_location_menu_item = NULL;

			action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "previous_location");
			g_simple_action_set_enabled (G_SIMPLE_ACTION (action), TRUE);
		}

		gtk_widget_hide (obj->priv->filter_entry_revealer);

		add_signal (obj,
			    gobject,
			    g_signal_connect (gobject, "bookmark-activated",
					      G_CALLBACK (on_bookmark_activated),
					      obj));
	}
	else if (GEDIT_IS_FILE_BROWSER_STORE (model))
	{
		/* make sure any async operation is cancelled */
		cancel_async_operation (obj);

		add_signal (obj,
			    gobject,
			    g_signal_connect (gobject, "file-activated",
					      G_CALLBACK (on_file_activated),
					      obj));

		add_signal (obj,
			    model,
		            g_signal_connect (model, "no-trash",
		                              G_CALLBACK (on_file_store_no_trash),
		                              obj));

		gtk_widget_show (obj->priv->filter_entry_revealer);
	}

	update_sensitivity (obj);
}

static void
on_file_store_error (GeditFileBrowserStore  *store,
		     guint                   code,
		     gchar                  *message,
		     GeditFileBrowserWidget *obj)
{
	g_signal_emit (obj, signals[ERROR], 0, code, message);
}

static void
on_treeview_error (GeditFileBrowserView   *tree_view,
		   guint                   code,
		   gchar                  *message,
		   GeditFileBrowserWidget *obj)
{
	g_signal_emit (obj, signals[ERROR], 0, code, message);
}

static gboolean
on_location_button_press_event (GtkWidget              *button,
				GdkEventButton         *event,
				GeditFileBrowserWidget *obj)
{
	GtkWidget *menu;

	if (event->button != GDK_BUTTON_SECONDARY)
		return FALSE;

	if (button == obj->priv->previous_button)
		menu = obj->priv->location_previous_menu;
	else
		menu = obj->priv->location_next_menu;

	gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *)event);

	return TRUE;
}

static void
on_locations_treeview_selection_changed (GtkTreeSelection       *treeview_selection,
                                         GeditFileBrowserWidget *obj)
{
	GeditFileBrowserWidgetPrivate *priv = obj->priv;
	GtkTreeModel *model = GTK_TREE_MODEL (priv->locations_model);
	GtkTreePath *path;
	GtkTreeIter iter;

	if (!gtk_tree_selection_get_selected (treeview_selection, &model, &iter))
		return;

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (obj->priv->locations_model), &iter);
	gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (obj->priv->locations_cellview), path);
	gtk_tree_path_free (path);
}

static void
on_location_entry_activate (GtkEntry               *entry,
                            GeditFileBrowserWidget *obj)
{
	gchar *location = g_strdup (gtk_entry_get_text (entry));
	GFile *root;
	gchar *cwd;
	GFile *new_root;

	if (g_str_has_prefix (location, "~/"))
	{
		gchar *tmp = location;

		location = g_strdup_printf ("%s/%s", g_get_home_dir (), tmp + strlen ("~/"));

		g_free (tmp);
	}

	root = gedit_file_browser_store_get_virtual_root (obj->priv->file_store);
	cwd = g_file_get_path (root);

	if (cwd == NULL)
		cwd = g_file_get_uri (root);

	new_root = g_file_new_for_commandline_arg_and_cwd (location, cwd);

	if (g_file_query_file_type (new_root, G_FILE_QUERY_INFO_NONE, NULL) != G_FILE_TYPE_DIRECTORY)
	{
		gchar *display_name = g_file_get_parse_name (new_root);
		gchar *msg = g_strdup_printf (_("Error when loading “%s”: No such directory"), display_name);

		g_signal_emit (obj, signals[ERROR], 0, GEDIT_FILE_BROWSER_ERROR_LOAD_DIRECTORY, msg);

		g_free (msg);
		g_free (display_name);
	}
	else
	{
		gtk_widget_grab_focus (GTK_WIDGET (obj->priv->treeview));
		gtk_widget_hide (obj->priv->location_entry);

		gedit_file_browser_widget_set_root (obj, new_root, TRUE);
	}

	g_object_unref (new_root);
	g_free (cwd);
	g_object_unref (root);
	g_free (location);
}

static gboolean
on_location_entry_focus_out_event (GtkWidget              *entry,
                                   GdkEvent               *event,
                                   GeditFileBrowserWidget *obj)
{
	gtk_widget_hide (entry);
	return FALSE;
}

static gboolean
on_location_entry_key_press_event (GtkWidget              *entry,
                                   GdkEventKey            *event,
                                   GeditFileBrowserWidget *obj)
{
	guint modifiers = gtk_accelerator_get_default_mod_mask ();

	if (event->keyval == GDK_KEY_Escape &&
	    (event->state & modifiers) == 0)
	{
		gtk_widget_grab_focus (GTK_WIDGET (obj->priv->treeview));
		gtk_widget_hide (entry);
		return TRUE;
	}

	return FALSE;
}

static gboolean
on_treeview_popup_menu (GeditFileBrowserView   *treeview,
			GeditFileBrowserWidget *obj)
{
	return popup_menu (obj, GTK_TREE_VIEW (treeview), NULL, gtk_tree_view_get_model (GTK_TREE_VIEW (treeview)));
}

static gboolean
on_treeview_button_press_event (GeditFileBrowserView   *treeview,
				GdkEventButton         *event,
				GeditFileBrowserWidget *obj)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY)
		return popup_menu (obj,
				   GTK_TREE_VIEW (treeview),
				   event,
				   gtk_tree_view_get_model (GTK_TREE_VIEW (treeview)));

	return FALSE;
}

static gboolean
do_change_directory (GeditFileBrowserWidget *obj,
                     GdkEventKey            *event)
{
	GAction *action = NULL;

	if ((event->state &
	    (~GDK_CONTROL_MASK & ~GDK_SHIFT_MASK & ~GDK_MOD1_MASK)) ==
	     event->state && event->keyval == GDK_KEY_BackSpace)
	{
		action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "previous_location");
	}
	else if (!((event->state & GDK_MOD1_MASK) &&
		   (event->state & (~GDK_CONTROL_MASK & ~GDK_SHIFT_MASK)) == event->state))
	{
		return FALSE;
	}

	switch (event->keyval)
	{
		case GDK_KEY_Home:
			action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "home");
			break;
		case GDK_KEY_Left:
			action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "previous_location");
			break;
		case GDK_KEY_Right:
			action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "next_location");
			break;
		case GDK_KEY_Up:
			action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "up");
			break;
		default:
			break;
	}

	if (action != NULL)
	{
		g_action_activate (action, NULL);
		return TRUE;
	}

	return FALSE;
}

static gboolean
on_treeview_key_press_event (GeditFileBrowserView   *treeview,
			     GdkEventKey            *event,
			     GeditFileBrowserWidget *obj)
{
	GtkTreeModel *model;
	guint modifiers;

	if (do_change_directory (obj, event))
		return TRUE;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return FALSE;

	modifiers = gtk_accelerator_get_default_mod_mask ();

	if (event->keyval == GDK_KEY_Delete ||
	    event->keyval == GDK_KEY_KP_Delete)
	{

		if ((event->state & modifiers) == GDK_SHIFT_MASK)
		{
			delete_selected_files (obj, FALSE);
			return TRUE;
		}
		else if ((event->state & modifiers) == 0)
		{
			delete_selected_files (obj, TRUE);
			return TRUE;
		}
	}

	if ((event->keyval == GDK_KEY_F2) && (event->state & modifiers) == 0)
	{
		rename_selected_file (obj);
		return TRUE;
	}

	if (event->keyval == GDK_KEY_l &&
	    (event->state & modifiers) == GDK_CONTROL_MASK)
	{
		show_location_entry (obj, "");
		return TRUE;
	}

	if (event->keyval == GDK_KEY_slash ||
	    event->keyval == GDK_KEY_KP_Divide ||
#ifdef G_OS_WIN32
	    event->keyval == GDK_KEY_backslash ||
#endif
	    event->keyval == GDK_KEY_asciitilde)
	{
		gchar location[2] = {'\0', '\0'};

		location[0] = gdk_keyval_to_unicode (event->keyval);

		show_location_entry (obj, location);
		return TRUE;
	}

	return FALSE;
}

static void
on_selection_changed (GtkTreeSelection       *selection,
		      GeditFileBrowserWidget *obj)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (obj->priv->treeview));
	GAction *action;
	guint selected = 0;
	guint files = 0;
	guint dirs = 0;

	if (GEDIT_IS_FILE_BROWSER_STORE (model))
		selected = gedit_file_browser_widget_get_num_selected_files_or_directories (obj, &files, &dirs);

	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "move_to_trash");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), selected > 0);

	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "delete");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), selected > 0);

	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "open");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), (selected > 0) && (selected == files));

	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "rename");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), selected == 1);

	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "open_in_terminal");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), selected == 1);

	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "new_folder");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), selected <= 1);

	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "new_file");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), selected <= 1);
}

static gboolean
on_entry_filter_activate (GeditFileBrowserWidget *obj)
{
	gchar const *text = gtk_entry_get_text (GTK_ENTRY (obj->priv->filter_entry));
	set_filter_pattern_real (obj, text, FALSE);

	return FALSE;
}

static void
on_location_jump_activate (GtkMenuItem            *item,
			   GeditFileBrowserWidget *obj)
{
	GList *location = g_object_get_data (G_OBJECT (item), LOCATION_DATA_KEY);

	if (obj->priv->current_location)
	{
		jump_to_location (obj, location,
				  g_list_position (obj->priv->locations, location) >
				  g_list_position (obj->priv->locations, obj->priv->current_location));
	}
	else
	{
		jump_to_location (obj, location, TRUE);
	}
}

static void
on_bookmarks_row_changed (GtkTreeModel           *model,
                          GtkTreePath            *path,
                          GtkTreeIter            *iter,
                          GeditFileBrowserWidget *obj)
{
	add_bookmark_hash (obj, iter);
}

static void
on_bookmarks_row_deleted (GtkTreeModel           *model,
                          GtkTreePath            *path,
                          GeditFileBrowserWidget *obj)
{
	GtkTreeIter iter;
	GFile *location;

	if (!gtk_tree_model_get_iter (model, &iter, path))
		return;

	if (!(location = gedit_file_bookmarks_store_get_location (obj->priv->bookmarks_store, &iter)))
		return;

	g_hash_table_remove (obj->priv->bookmarks_hash, location);

	g_object_unref (location);
}

static void
on_filter_mode_changed (GeditFileBrowserStore  *model,
                        GParamSpec             *param,
                        GeditFileBrowserWidget *obj)
{
	gint mode = gedit_file_browser_store_get_filter_mode (model);
	GAction *action;
	GVariant *variant;
	gboolean active;

	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "show_hidden");
	active = !(mode & GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN);
	variant = g_action_get_state (action);

	if (active != g_variant_get_boolean (variant))
		g_action_change_state (action, g_variant_new_boolean (active));

	g_variant_unref (variant);

	action = g_action_map_lookup_action (G_ACTION_MAP (obj->priv->action_group), "show_binary");
	active = !(mode & GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY);
	variant = g_action_get_state (action);

	if (active != g_variant_get_boolean (variant))
		g_action_change_state (action, g_variant_new_boolean (active));

	g_variant_unref (variant);
}

static void
next_location_activated (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       user_data)
{
	gedit_file_browser_widget_history_forward (GEDIT_FILE_BROWSER_WIDGET (user_data));
}

static void
previous_location_activated (GSimpleAction *action,
                             GVariant      *parameter,
                             gpointer       user_data)
{
	gedit_file_browser_widget_history_back (GEDIT_FILE_BROWSER_WIDGET (user_data));
}

static void
up_activated (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget->priv->treeview));

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;

	gedit_file_browser_store_set_virtual_root_up (GEDIT_FILE_BROWSER_STORE (model));
}

static void
home_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget->priv->treeview));
	GFile *home_location;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;

	home_location = g_file_new_for_path (g_get_home_dir ());
	gedit_file_browser_widget_set_root  (widget, home_location, TRUE);

	g_object_unref (home_location);
}

static void
new_folder_activated (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget->priv->treeview));
	GtkTreeIter parent;
	GtkTreeIter iter;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;

	if (!gedit_file_browser_widget_get_selected_directory (widget, &parent))
		return;

	if (gedit_file_browser_store_new_directory (GEDIT_FILE_BROWSER_STORE (model), &parent, &iter))
		gedit_file_browser_view_start_rename (widget->priv->treeview, &iter);
}

static void
open_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget->priv->treeview));
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget->priv->treeview));
	GList *rows;
	GList *row;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;

	rows = gtk_tree_selection_get_selected_rows (selection, &model);
	for (row = rows; row; row = row->next)
	{
		GtkTreePath *path = (GtkTreePath *)(row->data);
		GtkTreeIter iter;

		if (gtk_tree_model_get_iter (model, &iter, path))
			file_open (widget, model, &iter);

		gtk_tree_path_free (path);
	}

	g_list_free (rows);
}

static void
new_file_activated (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget->priv->treeview));
	GtkTreeIter parent;
	GtkTreeIter iter;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;

	if (!gedit_file_browser_widget_get_selected_directory (widget, &parent))
		return;

	if (gedit_file_browser_store_new_file (GEDIT_FILE_BROWSER_STORE (model), &parent, &iter))
		gedit_file_browser_view_start_rename (widget->priv->treeview, &iter);
}

static void
rename_activated (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);

	rename_selected_file (widget);
}

static void
delete_activated (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);

	delete_selected_files (widget, FALSE);
}

static void
move_to_trash_activated (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);

	delete_selected_files (widget, TRUE);
}

static void
refresh_view_activated (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);

	gedit_file_browser_widget_refresh (widget);
}

static void
view_folder_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget->priv->treeview));
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget->priv->treeview));
	GtkTreeIter iter;
	GList *rows;
	GList *row;
	gboolean directory_opened = FALSE;

	if (!GEDIT_IS_FILE_BROWSER_STORE (model))
		return;

	rows = gtk_tree_selection_get_selected_rows (selection, &model);
	for (row = rows; row; row = row->next)
	{
		GtkTreePath *path = (GtkTreePath *)(row->data);

		if (gtk_tree_model_get_iter (model, &iter, path))
			directory_opened |= directory_open (widget, model, &iter);

		gtk_tree_path_free (path);
	}

	if (!directory_opened && gedit_file_browser_widget_get_selected_directory (widget, &iter))
		directory_open (widget, model, &iter);

	g_list_free (rows);
}

static void
change_show_hidden_state (GSimpleAction *action,
                          GVariant      *state,
                          gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);

	update_filter_mode (widget,
	                    action,
	                    state,
	                    GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN);
}

static void
change_show_binary_state (GSimpleAction *action,
                          GVariant      *state,
                          gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);

	update_filter_mode (widget,
	                    action,
	                    state,
	                    GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_BINARY);
}

static void
change_show_match_filename (GSimpleAction *action,
                            GVariant      *state,
                            gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);
	gboolean visible = g_variant_get_boolean (state);

	gtk_revealer_set_reveal_child (GTK_REVEALER (widget->priv->filter_entry_revealer), visible);

	if (visible)
		gtk_widget_grab_focus (widget->priv->filter_entry);
	else
		/* clear the filter */
		set_filter_pattern_real (widget, NULL, TRUE);

	g_simple_action_set_state (action, state);
}

static void
open_in_terminal_activated (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);
	GtkTreeIter iter;
	GFile *file;

	/* Get the current directory */
	if (!gedit_file_browser_widget_get_selected_directory (widget, &iter))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (widget->priv->file_store),
	                    &iter,
	                    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &file,
	                    -1);

	g_signal_emit (widget, signals[OPEN_IN_TERMINAL], 0, file);

	g_object_unref (file);
}

static void
set_active_root_activated (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	GeditFileBrowserWidget *widget = GEDIT_FILE_BROWSER_WIDGET (user_data);

	g_signal_emit (widget, signals[SET_ACTIVE_ROOT], 0);
}

void
_gedit_file_browser_widget_register_type (GTypeModule *type_module)
{
	gedit_file_browser_widget_register_type (type_module);
}

/* ex:set ts=8 noet: */
