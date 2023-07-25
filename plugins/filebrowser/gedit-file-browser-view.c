/*
 * gedit-file-browser-view.c - Gedit plugin providing easy file access
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

#include <string.h>

#include <glib-object.h>
#include <gio/gio.h>
#include <gdk/gdkkeysyms.h>

#include "gedit-file-browser-store.h"
#include "gedit-file-bookmarks-store.h"
#include "gedit-file-browser-view.h"
#include "gedit-file-browser-enum-types.h"

struct _GeditFileBrowserViewPrivate
{
	GtkTreeViewColumn               *column;
	GtkCellRenderer                 *pixbuf_renderer;
	GtkCellRenderer                 *text_renderer;

	GtkTreeModel                    *model;

	/* Used when renaming */
	gchar                           *orig_markup;
	GtkTreeRowReference             *editable;

	/* Click policy */
	GeditFileBrowserViewClickPolicy  click_policy;
	/* Both clicks in a double click need to be on the same row */
	GtkTreePath                     *double_click_path[2];
	GtkTreePath                     *hover_path;
	GdkCursor                       *hand_cursor;
	gboolean                         ignore_release;
	gboolean                         selected_on_button_down;
	gint                             drag_button;
	gboolean                         drag_started;

	gboolean                         restore_expand_state;
	gboolean                         is_refresh;
	GHashTable                      *expand_state;
};

/* Properties */
enum
{
	PROP_0,

	PROP_CLICK_POLICY,
	PROP_RESTORE_EXPAND_STATE
};

/* Signals */
enum
{
	ERROR,
	FILE_ACTIVATED,
	DIRECTORY_ACTIVATED,
	BOOKMARK_ACTIVATED,
	NUM_SIGNALS
};

static guint signals[NUM_SIGNALS] = { 0 };

static const GtkTargetEntry drag_source_targets[] = {
	{ (gchar *) "text/uri-list", 0, 0 }
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditFileBrowserView,
				gedit_file_browser_view,
				GTK_TYPE_TREE_VIEW,
				0,
				G_ADD_PRIVATE_DYNAMIC (GeditFileBrowserView))

static void on_cell_edited 		(GtkCellRendererText    *cell,
				 	 gchar                  *path,
				 	 gchar                  *new_text,
				 	 GeditFileBrowserView   *tree_view);

static void on_begin_refresh 		(GeditFileBrowserStore  *model,
					 GeditFileBrowserView   *view);
static void on_end_refresh 		(GeditFileBrowserStore  *model,
					 GeditFileBrowserView   *view);

static void on_unload			(GeditFileBrowserStore  *model,
					 GFile                  *location,
					 GeditFileBrowserView   *view);

static void on_row_inserted		(GeditFileBrowserStore  *model,
					 GtkTreePath            *path,
					 GtkTreeIter            *iter,
					 GeditFileBrowserView   *view);

static void
gedit_file_browser_view_finalize (GObject *object)
{
	GeditFileBrowserView *obj = GEDIT_FILE_BROWSER_VIEW (object);

	if (obj->priv->hand_cursor)
		g_object_unref (obj->priv->hand_cursor);

	if (obj->priv->hover_path)
		gtk_tree_path_free (obj->priv->hover_path);

	if (obj->priv->expand_state)
	{
		g_hash_table_destroy (obj->priv->expand_state);
		obj->priv->expand_state = NULL;
	}

	G_OBJECT_CLASS (gedit_file_browser_view_parent_class)->finalize (object);
}

static void
add_expand_state (GeditFileBrowserView *view,
		  GFile                *location)
{
	if (!location)
		return;

	if (view->priv->expand_state)
		g_hash_table_insert (view->priv->expand_state, location, g_object_ref (location));
}

static void
remove_expand_state (GeditFileBrowserView *view,
		     GFile                *location)
{
	if (!location)
		return;

	if (view->priv->expand_state)
		g_hash_table_remove (view->priv->expand_state, location);
}

static void
row_expanded (GtkTreeView *tree_view,
	      GtkTreeIter *iter,
	      GtkTreePath *path)
{
	GeditFileBrowserView *view = GEDIT_FILE_BROWSER_VIEW (tree_view);

	if (GTK_TREE_VIEW_CLASS (gedit_file_browser_view_parent_class)->row_expanded)
		GTK_TREE_VIEW_CLASS (gedit_file_browser_view_parent_class)->row_expanded (tree_view, iter, path);

	if (!GEDIT_IS_FILE_BROWSER_STORE (view->priv->model))
		return;

	if (view->priv->restore_expand_state)
	{
		GFile *location;

		gtk_tree_model_get (view->priv->model,
				    iter,
				    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &location,
				    -1);

		add_expand_state (view, location);

		if (location)
			g_object_unref (location);
	}

	_gedit_file_browser_store_iter_expanded (GEDIT_FILE_BROWSER_STORE (view->priv->model),
						 iter);
}

static void
row_collapsed (GtkTreeView *tree_view,
	       GtkTreeIter *iter,
	       GtkTreePath *path)
{
	GeditFileBrowserView *view = GEDIT_FILE_BROWSER_VIEW (tree_view);

	if (GTK_TREE_VIEW_CLASS (gedit_file_browser_view_parent_class)->row_collapsed)
		GTK_TREE_VIEW_CLASS (gedit_file_browser_view_parent_class)->row_collapsed (tree_view, iter, path);

	if (!GEDIT_IS_FILE_BROWSER_STORE (view->priv->model))
		return;

	if (view->priv->restore_expand_state)
	{
		GFile *location;

		gtk_tree_model_get (view->priv->model,
				    iter,
				    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &location,
				    -1);

		remove_expand_state (view, location);

		if (location)
			g_object_unref (location);
	}

	_gedit_file_browser_store_iter_collapsed (GEDIT_FILE_BROWSER_STORE (view->priv->model),
						  iter);
}

static gboolean
leave_notify_event (GtkWidget        *widget,
		    GdkEventCrossing *event)
{
	GeditFileBrowserView *view = GEDIT_FILE_BROWSER_VIEW (widget);

	if (view->priv->click_policy == GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_SINGLE &&
	    view->priv->hover_path != NULL)
	{
		gtk_tree_path_free (view->priv->hover_path);
		view->priv->hover_path = NULL;
	}

	/* Chainup */
	return GTK_WIDGET_CLASS (gedit_file_browser_view_parent_class)->leave_notify_event (widget, event);
}

static gboolean
enter_notify_event (GtkWidget        *widget,
		    GdkEventCrossing *event)
{
	GeditFileBrowserView *view = GEDIT_FILE_BROWSER_VIEW (widget);

	if (view->priv->click_policy == GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_SINGLE)
	{
		if (view->priv->hover_path != NULL)
			gtk_tree_path_free (view->priv->hover_path);

		gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
					       event->x, event->y,
					       &view->priv->hover_path,
					       NULL, NULL, NULL);

		if (view->priv->hover_path != NULL)
		{
			gdk_window_set_cursor (gtk_widget_get_window (widget),
					       view->priv->hand_cursor);
		}
	}

	/* Chainup */
	return GTK_WIDGET_CLASS (gedit_file_browser_view_parent_class)->enter_notify_event (widget, event);
}

static gboolean
motion_notify_event (GtkWidget *widget,
		     GdkEventMotion *event)
{
	GtkTreePath *old_hover_path;
	GeditFileBrowserView *view = GEDIT_FILE_BROWSER_VIEW (widget);

	if (view->priv->click_policy == GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_SINGLE)
	{
		old_hover_path = view->priv->hover_path;
		gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
					       event->x, event->y,
					       &view->priv->hover_path,
					       NULL, NULL, NULL);

		if ((old_hover_path != NULL) != (view->priv->hover_path != NULL))
		{
			if (view->priv->hover_path != NULL)
			{
				gdk_window_set_cursor (gtk_widget_get_window (widget),
						       view->priv->hand_cursor);
			}
			else
			{
				gdk_window_set_cursor (gtk_widget_get_window (widget),
						       NULL);
			}
		}

		if (old_hover_path != NULL)
			gtk_tree_path_free (old_hover_path);
	}

	/* Chainup */
	return GTK_WIDGET_CLASS (gedit_file_browser_view_parent_class)->motion_notify_event (widget, event);
}

static void
set_click_policy_property (GeditFileBrowserView            *obj,
			   GeditFileBrowserViewClickPolicy  click_policy)
{
	GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (obj));

	obj->priv->click_policy = click_policy;

	if (click_policy == GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_SINGLE)
	{
		if (obj->priv->hand_cursor == NULL)
			obj->priv->hand_cursor = gdk_cursor_new_from_name (display, "pointer");
	}
	else if (click_policy == GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_DOUBLE)
	{
		if (obj->priv->hover_path != NULL)
		{
			GtkTreeIter iter;

			if (gtk_tree_model_get_iter (GTK_TREE_MODEL (obj->priv->model),
						     &iter, obj->priv->hover_path))
			{
				gtk_tree_model_row_changed (GTK_TREE_MODEL (obj->priv->model),
							    obj->priv->hover_path, &iter);
			}

			gtk_tree_path_free (obj->priv->hover_path);
			obj->priv->hover_path = NULL;
		}

		if (gtk_widget_get_realized (GTK_WIDGET (obj)))
		{
			GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (obj));

			gdk_window_set_cursor (win, NULL);

			if (display != NULL)
				gdk_display_flush (display);
		}

		if (obj->priv->hand_cursor)
		{
			g_object_unref (obj->priv->hand_cursor);
			obj->priv->hand_cursor = NULL;
		}
	}
}

static void
directory_activated (GeditFileBrowserView *view,
		     GtkTreeIter          *iter)
{
	gedit_file_browser_store_set_virtual_root (GEDIT_FILE_BROWSER_STORE (view->priv->model), iter);
}

static void
activate_selected_files (GeditFileBrowserView *view)
{
	GtkTreeView *tree_view = GTK_TREE_VIEW (view);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);
	GList *rows = gtk_tree_selection_get_selected_rows (selection, &view->priv->model);
	GList *row;
	GtkTreePath *directory = NULL;
	GtkTreePath *path;
	GtkTreeIter iter;
	GeditFileBrowserStoreFlag flags;

	for (row = rows; row; row = row->next)
	{
		path = (GtkTreePath *)(row->data);

		/* Get iter from path */
		if (!gtk_tree_model_get_iter (view->priv->model, &iter, path))
			continue;

		gtk_tree_model_get (view->priv->model, &iter, GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags, -1);

		if (FILE_IS_DIR (flags) && directory == NULL)
			directory = path;
		else if (!FILE_IS_DUMMY (flags))
			g_signal_emit (view, signals[FILE_ACTIVATED], 0, &iter);
	}

	if (directory != NULL &&
	    gtk_tree_model_get_iter (view->priv->model, &iter, directory))
	{
		g_signal_emit (view, signals[DIRECTORY_ACTIVATED], 0, &iter);
	}

	g_list_free_full (rows, (GDestroyNotify) gtk_tree_path_free);
}

static void
activate_selected_bookmark (GeditFileBrowserView *view)
{
	GtkTreeView *tree_view = GTK_TREE_VIEW (view);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected (selection, &view->priv->model, &iter))
		g_signal_emit (view, signals[BOOKMARK_ACTIVATED], 0, &iter);
}

static void
activate_selected_items (GeditFileBrowserView *view)
{
	if (GEDIT_IS_FILE_BROWSER_STORE (view->priv->model))
		activate_selected_files (view);
	else if (GEDIT_IS_FILE_BOOKMARKS_STORE (view->priv->model))
		activate_selected_bookmark (view);
}

static void
expand_or_collapse_selected_item (GeditFileBrowserView *view,
				  gboolean              collapse)
{
	GtkTreeView *tree_view = GTK_TREE_VIEW (view);
	GtkTreePath *path = NULL;

	gtk_tree_view_get_cursor (tree_view, &path, NULL);

	if (path == NULL)
	{
		return;
	}

	if (collapse)
	{
		if (!gtk_tree_view_collapse_row (tree_view, path) &&
		    gtk_tree_path_get_depth (path) > 1 &&
		    gtk_tree_path_up (path))
		{
			gtk_tree_view_set_cursor (tree_view, path, NULL, FALSE);
		}
	}
	else
	{
		gtk_tree_view_expand_row (tree_view, path, FALSE);
	}

	gtk_tree_path_free (path);
}

static void
row_activated (GtkTreeView       *tree_view,
               GtkTreePath       *path,
               GtkTreeViewColumn *column)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);

	/* Make sure the activated row is the only one selected */
	gtk_tree_selection_unselect_all (selection);
	gtk_tree_selection_select_path (selection, path);

	activate_selected_items (GEDIT_FILE_BROWSER_VIEW (tree_view));
}

static void
toggle_hidden_filter (GeditFileBrowserView *view)
{
	GeditFileBrowserStoreFilterMode mode;

	if (GEDIT_IS_FILE_BROWSER_STORE (view->priv->model))
	{
		mode = gedit_file_browser_store_get_filter_mode (GEDIT_FILE_BROWSER_STORE (view->priv->model));
		mode ^=	GEDIT_FILE_BROWSER_STORE_FILTER_MODE_HIDE_HIDDEN;
		gedit_file_browser_store_set_filter_mode (GEDIT_FILE_BROWSER_STORE (view->priv->model), mode);
	}
}

static gboolean
button_event_modifies_selection (GdkEventButton *event)
{
	return (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) != 0;
}

static void
drag_begin (GtkWidget      *widget,
	    GdkDragContext *context)
{
	GeditFileBrowserView *view = GEDIT_FILE_BROWSER_VIEW (widget);

	view->priv->drag_button = 0;
	view->priv->drag_started = TRUE;

	/* Chain up */
	GTK_WIDGET_CLASS (gedit_file_browser_view_parent_class)->drag_begin (widget, context);
}

static void
did_not_drag (GeditFileBrowserView *view,
	      GdkEventButton       *event)
{
	GtkTreeView *tree_view = GTK_TREE_VIEW (view);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);
	GtkTreePath *path;

	if (gtk_tree_view_get_path_at_pos (tree_view, event->x, event->y, &path, NULL, NULL, NULL))
	{
		if ((view->priv->click_policy == GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_SINGLE) &&
		    !button_event_modifies_selection (event) &&
		    (event->button == 1 || event->button == 2))
		{
		    	/* Activate all selected items, and leave them selected */
			activate_selected_items (view);
		}
		else if ((event->button == 1 || event->button == 2) &&
		         ((event->state & GDK_CONTROL_MASK) != 0 || (event->state & GDK_SHIFT_MASK) == 0) &&
		         view->priv->selected_on_button_down)
		{
			if (!button_event_modifies_selection (event))
			{
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_path (selection, path);
			}
			else
			{
				gtk_tree_selection_unselect_path (selection, path);
			}
		}

		gtk_tree_path_free (path);
	}
}

static gboolean
button_release_event (GtkWidget      *widget,
		      GdkEventButton *event)
{
	GeditFileBrowserView *view = GEDIT_FILE_BROWSER_VIEW (widget);

	if (event->button == view->priv->drag_button)
	{
		view->priv->drag_button = 0;

		if (!view->priv->drag_started && !view->priv->ignore_release)
			did_not_drag (view, event);
	}

	/* Chain up */
	return GTK_WIDGET_CLASS (gedit_file_browser_view_parent_class)->button_release_event (widget, event);
}

static gboolean
button_press_event (GtkWidget      *widget,
		    GdkEventButton *event)
{
	GtkWidgetClass *widget_parent = GTK_WIDGET_CLASS (gedit_file_browser_view_parent_class);
	GtkTreeView *tree_view = GTK_TREE_VIEW (widget);
	GeditFileBrowserView *view = GEDIT_FILE_BROWSER_VIEW (widget);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);
	int double_click_time;
	static int click_count = 0;
	static guint32 last_click_time = 0;
	GtkTreePath *path;
	int expander_size;
	int horizontal_separator;
	gboolean on_expander;
	gboolean call_parent;
	gboolean selected;

	/* Get double click time */
	g_object_get (G_OBJECT (gtk_widget_get_settings (widget)),
		      "gtk-double-click-time", &double_click_time,
		      NULL);

	/* Determine click count */
	if (event->time - last_click_time < double_click_time)
		click_count++;
	else
		click_count = 0;

	last_click_time = event->time;

	/* Ignore double click if we are in single click mode */
	if (view->priv->click_policy == GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_SINGLE &&
	    click_count >= 2)
	{
		return TRUE;
	}

	view->priv->ignore_release = FALSE;
	call_parent = TRUE;

	if (gtk_tree_view_get_path_at_pos (tree_view, event->x, event->y, &path, NULL, NULL, NULL))
	{
		/* Keep track of path of last click so double clicks only happen
		 * on the same item */
		if ((event->button == 1 || event->button == 2)  &&
		    event->type == GDK_BUTTON_PRESS)
		{
			if (view->priv->double_click_path[1])
				gtk_tree_path_free (view->priv->double_click_path[1]);

			view->priv->double_click_path[1] = view->priv->double_click_path[0];
			view->priv->double_click_path[0] = gtk_tree_path_copy (path);
		}

		if (event->type == GDK_2BUTTON_PRESS)
		{
			/* Do not chain up. The row-activated signal is normally
			 * already sent, which will activate the selected item
			 * and open the file.
			 */
		}
		else
		{
			/* We're going to filter out some situations where
			 * we can't let the default code run because all
			 * but one row would be deselected. We don't
			 * want that; we want the right click menu or single
			 * click to apply to everything that's currently selected. */
			selected = gtk_tree_selection_path_is_selected (selection, path);

			if (event->button == GDK_BUTTON_SECONDARY && selected)
				call_parent = FALSE;

			if ((event->button == 1 || event->button == 2) &&
			    ((event->state & GDK_CONTROL_MASK) != 0 ||
			     (event->state & GDK_SHIFT_MASK) == 0))
			{
				gtk_widget_style_get (widget,
						      "expander-size", &expander_size,
						      "horizontal-separator", &horizontal_separator,
						      NULL);
				on_expander = (event->x <= horizontal_separator / 2 +
					       gtk_tree_path_get_depth (path) * expander_size);

				view->priv->selected_on_button_down = selected;

				if (selected)
				{
					call_parent = on_expander || gtk_tree_selection_count_selected_rows (selection) == 1;
					view->priv->ignore_release = call_parent && view->priv->click_policy != GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_SINGLE;
				}
				else if  ((event->state & GDK_CONTROL_MASK) != 0)
				{
					call_parent = FALSE;
					gtk_tree_selection_select_path (selection, path);
				}
				else
				{
					view->priv->ignore_release = on_expander;
				}
			}

			if (call_parent)
			{
				/* Chain up */
				widget_parent->button_press_event (widget, event);
			}
			else if (selected)
			{
				gtk_widget_grab_focus (widget);
			}

			if ((event->button == 1 || event->button == 2) &&
			    event->type == GDK_BUTTON_PRESS)
			{
				view->priv->drag_started = FALSE;
				view->priv->drag_button = event->button;
			}
		}

		gtk_tree_path_free (path);
	}
	else
	{
		if ((event->button == 1 || event->button == 2)  &&
		    event->type == GDK_BUTTON_PRESS)
		{
			if (view->priv->double_click_path[1])
				gtk_tree_path_free (view->priv->double_click_path[1]);

			view->priv->double_click_path[1] = view->priv->double_click_path[0];
			view->priv->double_click_path[0] = NULL;
		}

		gtk_tree_selection_unselect_all (selection);
		/* Chain up */
		widget_parent->button_press_event (widget, event);
	}

	/* We already chained up if nescessary, so just return TRUE */
	return TRUE;
}

static gboolean
key_press_event (GtkWidget   *widget,
		 GdkEventKey *event)
{
	GeditFileBrowserView *view = GEDIT_FILE_BROWSER_VIEW (widget);
	guint modifiers = gtk_accelerator_get_default_mod_mask ();
	gboolean handled = FALSE;

	switch (event->keyval)
	{
		case GDK_KEY_space:
			if (event->state & GDK_CONTROL_MASK)
			{
				handled = FALSE;
				break;
			}
			if (!gtk_widget_has_focus (widget))
			{
				handled = FALSE;
				break;
			}

			activate_selected_items (view);
			handled = TRUE;
			break;

		case GDK_KEY_Return:
		case GDK_KEY_KP_Enter:
			activate_selected_items (view);
			handled = TRUE;
			break;

		case GDK_KEY_h:
			if ((event->state & modifiers) == GDK_CONTROL_MASK)
			{
				toggle_hidden_filter (view);
				handled = TRUE;
			}
			break;

		case GDK_KEY_Left:
			expand_or_collapse_selected_item (view, TRUE);
			handled = TRUE;
			break;

		case GDK_KEY_Right:
			expand_or_collapse_selected_item (view, FALSE);
			handled = TRUE;
			break;

		default:
			handled = FALSE;
			break;
	}

	/* Chain up */
	if (!handled)
		return GTK_WIDGET_CLASS (gedit_file_browser_view_parent_class)->key_press_event (widget, event);

	return TRUE;
}

static void
fill_expand_state (GeditFileBrowserView *view,
		   GtkTreeIter          *iter)
{
	GtkTreePath *path;
	GtkTreeIter child;

	if (!gtk_tree_model_iter_has_child (view->priv->model, iter))
		return;

	path = gtk_tree_model_get_path (view->priv->model, iter);

	if (gtk_tree_view_row_expanded (GTK_TREE_VIEW (view), path))
	{
		GFile *location;

		gtk_tree_model_get (view->priv->model,
				    iter,
				    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &location,
				    -1);

		add_expand_state (view, location);

		if (location)
			g_object_unref (location);
	}

	if (gtk_tree_model_iter_children (view->priv->model, &child, iter))
	{
		do
		{
			fill_expand_state (view, &child);
		}
		while (gtk_tree_model_iter_next (view->priv->model, &child));
	}

	gtk_tree_path_free (path);
}

static void
uninstall_restore_signals (GeditFileBrowserView *tree_view,
			   GtkTreeModel         *model)
{
	g_signal_handlers_disconnect_by_func (model, on_begin_refresh, tree_view);
	g_signal_handlers_disconnect_by_func (model, on_end_refresh, tree_view);
	g_signal_handlers_disconnect_by_func (model, on_unload, tree_view);
	g_signal_handlers_disconnect_by_func (model, on_row_inserted, tree_view);
}

static void
install_restore_signals (GeditFileBrowserView *tree_view,
			 GtkTreeModel         *model)
{
	g_signal_connect (model, "begin-refresh", G_CALLBACK (on_begin_refresh), tree_view);
	g_signal_connect (model, "end-refresh", G_CALLBACK (on_end_refresh), tree_view);
	g_signal_connect (model, "unload", G_CALLBACK (on_unload), tree_view);
	g_signal_connect_after (model, "row-inserted", G_CALLBACK (on_row_inserted), tree_view);
}

static void
set_restore_expand_state (GeditFileBrowserView *view,
			  gboolean              state)
{
	if (state == view->priv->restore_expand_state)
		return;

	if (view->priv->expand_state)
	{
		g_hash_table_destroy (view->priv->expand_state);
		view->priv->expand_state = NULL;
	}

	if (state)
	{
		view->priv->expand_state = g_hash_table_new_full (g_file_hash,
								  (GEqualFunc)g_file_equal,
								  g_object_unref,
								  NULL);

		if (view->priv->model && GEDIT_IS_FILE_BROWSER_STORE (view->priv->model))
		{
			fill_expand_state (view, NULL);
			install_restore_signals (view, view->priv->model);
		}
	}
	else if (view->priv->model && GEDIT_IS_FILE_BROWSER_STORE (view->priv->model))
	{
		uninstall_restore_signals (view, view->priv->model);
	}

	view->priv->restore_expand_state = state;
}

static void
get_property (GObject    *object,
	      guint       prop_id,
	      GValue     *value,
	      GParamSpec *pspec)
{
	GeditFileBrowserView *obj = GEDIT_FILE_BROWSER_VIEW (object);

	switch (prop_id)
	{
		case PROP_CLICK_POLICY:
			g_value_set_enum (value, obj->priv->click_policy);
			break;
		case PROP_RESTORE_EXPAND_STATE:
			g_value_set_boolean (value, obj->priv->restore_expand_state);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
set_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
	GeditFileBrowserView *obj = GEDIT_FILE_BROWSER_VIEW (object);

	switch (prop_id)
	{
		case PROP_CLICK_POLICY:
			set_click_policy_property (obj, g_value_get_enum (value));
			break;
		case PROP_RESTORE_EXPAND_STATE:
			set_restore_expand_state (obj, g_value_get_boolean (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_file_browser_view_class_init (GeditFileBrowserViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkTreeViewClass *tree_view_class = GTK_TREE_VIEW_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = gedit_file_browser_view_finalize;
	object_class->get_property = get_property;
	object_class->set_property = set_property;

	/* Event handlers */
	widget_class->motion_notify_event = motion_notify_event;
	widget_class->enter_notify_event = enter_notify_event;
	widget_class->leave_notify_event = leave_notify_event;
	widget_class->button_press_event = button_press_event;
	widget_class->button_release_event = button_release_event;
	widget_class->drag_begin = drag_begin;
	widget_class->key_press_event = key_press_event;

	/* Tree view handlers */
	tree_view_class->row_activated = row_activated;
	tree_view_class->row_expanded = row_expanded;
	tree_view_class->row_collapsed = row_collapsed;

	/* Default handlers */
	klass->directory_activated = directory_activated;

	g_object_class_install_property (object_class, PROP_CLICK_POLICY,
					 g_param_spec_enum ("click-policy",
					 		    "Click Policy",
					 		    "The click policy",
					 		     GEDIT_TYPE_FILE_BROWSER_VIEW_CLICK_POLICY,
					 		     GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_DOUBLE,
					 		     G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class, PROP_RESTORE_EXPAND_STATE,
					 g_param_spec_boolean ("restore-expand-state",
					 		       "Restore Expand State",
					 		       "Restore expanded state of loaded directories",
					 		       FALSE,
					 		       G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	signals[ERROR] =
	    g_signal_new ("error",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditFileBrowserViewClass, error),
			  NULL, NULL, NULL,
			  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);
	signals[FILE_ACTIVATED] =
	    g_signal_new ("file-activated",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditFileBrowserViewClass, file_activated),
			  NULL, NULL, NULL,
			  G_TYPE_NONE, 1, GTK_TYPE_TREE_ITER);
	signals[DIRECTORY_ACTIVATED] =
	    g_signal_new ("directory-activated",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditFileBrowserViewClass, directory_activated),
			  NULL, NULL, NULL,
			  G_TYPE_NONE, 1, GTK_TYPE_TREE_ITER);
	signals[BOOKMARK_ACTIVATED] =
	    g_signal_new ("bookmark-activated",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditFileBrowserViewClass, bookmark_activated),
			  NULL, NULL, NULL,
			  G_TYPE_NONE, 1, GTK_TYPE_TREE_ITER);
}

static void
gedit_file_browser_view_class_finalize (GeditFileBrowserViewClass *klass)
{
}

static void
cell_data_cb (GtkTreeViewColumn    *tree_column,
	      GtkCellRenderer      *cell,
	      GtkTreeModel         *tree_model,
	      GtkTreeIter          *iter,
	      GeditFileBrowserView *obj)
{
	GtkTreePath *path = gtk_tree_model_get_path (tree_model, iter);
	PangoUnderline underline = PANGO_UNDERLINE_NONE;
	gboolean editable = FALSE;

	if (obj->priv->click_policy == GEDIT_FILE_BROWSER_VIEW_CLICK_POLICY_SINGLE &&
	    obj->priv->hover_path != NULL &&
	    gtk_tree_path_compare (path, obj->priv->hover_path) == 0)
	{
		underline = PANGO_UNDERLINE_SINGLE;
	}

	if (GEDIT_IS_FILE_BROWSER_STORE (tree_model) &&
	    obj->priv->editable != NULL &&
	    gtk_tree_row_reference_valid (obj->priv->editable))
	{
		GtkTreePath *edpath = gtk_tree_row_reference_get_path (obj->priv->editable);

		editable = edpath && gtk_tree_path_compare (path, edpath) == 0;

		gtk_tree_path_free (edpath);
	}

	gtk_tree_path_free (path);
	g_object_set (cell, "editable", editable, "underline", underline, NULL);
}

static void
icon_renderer_cb (GtkTreeViewColumn    *tree_column,
                  GtkCellRenderer      *cell,
                  GtkTreeModel         *tree_model,
                  GtkTreeIter          *iter,
                  GeditFileBrowserView *obj)
{
	GdkPixbuf *pixbuf;
	gchar *icon_name;
	gboolean set_pixbuf = FALSE;

	gtk_tree_model_get (tree_model,
			    iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_ICON_NAME, &icon_name,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_ICON, &pixbuf,
			    -1);

	if (pixbuf != NULL && (GEDIT_IS_FILE_BROWSER_STORE (tree_model) || icon_name == NULL))
		set_pixbuf = TRUE;

	if (set_pixbuf)
		g_object_set (cell, "pixbuf", pixbuf, NULL);
	else
		g_object_set (cell, "icon-name", icon_name, NULL);

	g_clear_object (&pixbuf);
	g_free (icon_name);
}

static void
gedit_file_browser_view_init (GeditFileBrowserView *obj)
{
	obj->priv = gedit_file_browser_view_get_instance_private (obj);

	obj->priv->column = gtk_tree_view_column_new ();

	obj->priv->pixbuf_renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (obj->priv->column,
					 obj->priv->pixbuf_renderer,
					 FALSE);

	gtk_tree_view_column_set_cell_data_func (obj->priv->column,
						 obj->priv->pixbuf_renderer,
						 (GtkTreeCellDataFunc)icon_renderer_cb,
						 obj,
						 NULL);

	obj->priv->text_renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (obj->priv->column,
					 obj->priv->text_renderer, TRUE);
	gtk_tree_view_column_add_attribute (obj->priv->column,
					    obj->priv->text_renderer,
					    "markup",
					    GEDIT_FILE_BROWSER_STORE_COLUMN_MARKUP);

	g_signal_connect (obj->priv->text_renderer, "edited",
			  G_CALLBACK (on_cell_edited), obj);

	gtk_tree_view_append_column (GTK_TREE_VIEW (obj),
				     obj->priv->column);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (obj), FALSE);

	gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (obj),
						GDK_BUTTON1_MASK,
						drag_source_targets,
						G_N_ELEMENTS (drag_source_targets),
						GDK_ACTION_COPY);
}

static gboolean
bookmarks_separator_func (GtkTreeModel *model,
			  GtkTreeIter  *iter,
			  gpointer      user_data)
{
	guint flags;

	gtk_tree_model_get (model,
			    iter,
			    GEDIT_FILE_BOOKMARKS_STORE_COLUMN_FLAGS, &flags,
			    -1);

	return (flags & GEDIT_FILE_BOOKMARKS_STORE_IS_SEPARATOR);
}

/* Public */
GtkWidget *
gedit_file_browser_view_new (void)
{
	GeditFileBrowserView *obj = GEDIT_FILE_BROWSER_VIEW (g_object_new (GEDIT_TYPE_FILE_BROWSER_VIEW, NULL));

	return GTK_WIDGET (obj);
}

void
gedit_file_browser_view_set_model (GeditFileBrowserView *tree_view,
				   GtkTreeModel         *model)
{
	GtkTreeSelection *selection;
	gint search_column;

	if (tree_view->priv->model == model)
		return;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));

	if (GEDIT_IS_FILE_BOOKMARKS_STORE (model))
	{
		gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
		gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (tree_view), bookmarks_separator_func, NULL, NULL);
		gtk_tree_view_column_set_cell_data_func (tree_view->priv->column,
							 tree_view->priv->text_renderer,
							 (GtkTreeCellDataFunc)cell_data_cb,
							 tree_view, NULL);
		search_column = GEDIT_FILE_BOOKMARKS_STORE_COLUMN_NAME;
	}
	else
	{
		gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
		gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (tree_view), NULL, NULL, NULL);
		gtk_tree_view_column_set_cell_data_func (tree_view->priv->column,
							 tree_view->priv->text_renderer,
							 (GtkTreeCellDataFunc)cell_data_cb,
							 tree_view, NULL);
		search_column = GEDIT_FILE_BROWSER_STORE_COLUMN_NAME;

		if (tree_view->priv->restore_expand_state)
			install_restore_signals (tree_view, model);

	}

	if (tree_view->priv->hover_path != NULL)
	{
		gtk_tree_path_free (tree_view->priv->hover_path);
		tree_view->priv->hover_path = NULL;
	}

	if (GEDIT_IS_FILE_BROWSER_STORE (tree_view->priv->model) &&
	    tree_view->priv->restore_expand_state)
	{
		uninstall_restore_signals (tree_view, tree_view->priv->model);
	}

	tree_view->priv->model = model;
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), model);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (tree_view), search_column);
}

void
gedit_file_browser_view_start_rename (GeditFileBrowserView *tree_view,
				      GtkTreeIter          *iter)
{
	gchar *name;
	gchar *markup;
	guint flags;
	GValue name_escaped = G_VALUE_INIT;
	GtkTreeRowReference *rowref;
	GtkTreePath *path;

	g_return_if_fail (GEDIT_IS_FILE_BROWSER_VIEW (tree_view));
	g_return_if_fail (GEDIT_IS_FILE_BROWSER_STORE (tree_view->priv->model));
	g_return_if_fail (iter != NULL);

	gtk_tree_model_get (tree_view->priv->model,
			    iter,
	                    GEDIT_FILE_BROWSER_STORE_COLUMN_NAME, &name,
	                    GEDIT_FILE_BROWSER_STORE_COLUMN_MARKUP, &markup,
	                    GEDIT_FILE_BROWSER_STORE_COLUMN_FLAGS, &flags,
	                    -1);

	if (!(FILE_IS_DIR (flags) || !FILE_IS_DUMMY (flags)))
	{
		g_free (name);
		g_free (markup);
		return;
	}

	/* Restore the markup to the original
	 * name, a plugin might have changed the markup.
	 */
	g_value_init (&name_escaped, G_TYPE_STRING);
	g_value_take_string (&name_escaped, g_markup_escape_text (name, -1));
	gedit_file_browser_store_set_value (GEDIT_FILE_BROWSER_STORE (tree_view->priv->model),
					    iter,
	                                    GEDIT_FILE_BROWSER_STORE_COLUMN_MARKUP, &name_escaped);

	path = gtk_tree_model_get_path (tree_view->priv->model, iter);
	rowref = gtk_tree_row_reference_new (tree_view->priv->model, path);

	/* Start editing */
	gtk_widget_grab_focus (GTK_WIDGET (tree_view));

	if (gtk_tree_path_up (path))
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (tree_view), path);

	gtk_tree_path_free (path);

	tree_view->priv->orig_markup = markup;
	tree_view->priv->editable = rowref;

	/* grab focus on the text cell which is editable */
	gtk_tree_view_column_focus_cell (tree_view->priv->column, tree_view->priv->text_renderer);

	path = gtk_tree_row_reference_get_path (tree_view->priv->editable),
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree_view), path, tree_view->priv->column, TRUE);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tree_view),
				      path, tree_view->priv->column,
				      FALSE, 0.0, 0.0);

	gtk_tree_path_free (path);
	g_value_unset (&name_escaped);
	g_free (name);
}

void
gedit_file_browser_view_set_click_policy (GeditFileBrowserView            *tree_view,
					  GeditFileBrowserViewClickPolicy  policy)
{
	g_return_if_fail (GEDIT_IS_FILE_BROWSER_VIEW (tree_view));

	set_click_policy_property (tree_view, policy);

	g_object_notify (G_OBJECT (tree_view), "click-policy");
}

void
gedit_file_browser_view_set_restore_expand_state (GeditFileBrowserView *tree_view,
						  gboolean              restore_expand_state)
{
	g_return_if_fail (GEDIT_IS_FILE_BROWSER_VIEW (tree_view));

	set_restore_expand_state (tree_view, restore_expand_state);
	g_object_notify (G_OBJECT (tree_view), "restore-expand-state");
}

/* Signal handlers */
static void
on_cell_edited (GtkCellRendererText  *cell,
		gchar                *path,
		gchar                *new_text,
		GeditFileBrowserView *tree_view)
{
	GtkTreePath *treepath = gtk_tree_path_new_from_string (path);
	GtkTreeIter iter;
	gboolean ret;
	GValue orig_markup = G_VALUE_INIT;
	GError *error = NULL;

	ret = gtk_tree_model_get_iter (GTK_TREE_MODEL (tree_view->priv->model), &iter, treepath);
	gtk_tree_path_free (treepath);

	if (ret)
	{
		/* Restore the original markup */
		g_value_init (&orig_markup, G_TYPE_STRING);
		g_value_set_string (&orig_markup, tree_view->priv->orig_markup);
		gedit_file_browser_store_set_value (GEDIT_FILE_BROWSER_STORE (tree_view->priv->model), &iter,
		                                    GEDIT_FILE_BROWSER_STORE_COLUMN_MARKUP, &orig_markup);

		if (new_text != NULL && *new_text != '\0' &&
		    gedit_file_browser_store_rename (GEDIT_FILE_BROWSER_STORE (tree_view->priv->model),
						     &iter,
						     new_text,
						     &error))
		{
			treepath = gtk_tree_model_get_path (GTK_TREE_MODEL (tree_view->priv->model), &iter);
			gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tree_view),
						      treepath, NULL,
						      FALSE, 0.0, 0.0);
			gtk_tree_path_free (treepath);
		}
		else if (error)
		{
			g_signal_emit (tree_view, signals[ERROR], 0, error->code, error->message);
			g_error_free (error);
		}

		g_value_unset (&orig_markup);
	}

	g_free (tree_view->priv->orig_markup);
	tree_view->priv->orig_markup = NULL;
	
	gtk_tree_row_reference_free (tree_view->priv->editable);
	tree_view->priv->editable = NULL;
}

static void
on_begin_refresh (GeditFileBrowserStore *model,
		  GeditFileBrowserView  *view)
{
	/* Store the refresh state, so we can handle unloading of nodes while
	   refreshing properly */
	view->priv->is_refresh = TRUE;
}

static void
on_end_refresh (GeditFileBrowserStore *model,
		GeditFileBrowserView  *view)
{
	/* Store the refresh state, so we can handle unloading of nodes while
	   refreshing properly */
	view->priv->is_refresh = FALSE;
}

static void
on_unload (GeditFileBrowserStore *model,
	   GFile                 *location,
	   GeditFileBrowserView  *view)
{
	/* Don't remove the expand state if we are refreshing */
	if (!view->priv->restore_expand_state || view->priv->is_refresh)
		return;

	remove_expand_state (view, location);
}

static void
restore_expand_state (GeditFileBrowserView  *view,
		      GeditFileBrowserStore *model,
		      GtkTreeIter           *iter)
{
	GFile *location;

	gtk_tree_model_get (GTK_TREE_MODEL (model),
			    iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &location,
			    -1);

	if (location)
	{
		GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), iter);

		if (g_hash_table_lookup (view->priv->expand_state, location))
			gtk_tree_view_expand_row (GTK_TREE_VIEW (view), path, FALSE);

		gtk_tree_path_free (path);
		g_object_unref (location);
	}
}

static void
on_row_inserted (GeditFileBrowserStore *model,
		 GtkTreePath           *path,
		 GtkTreeIter           *iter,
		 GeditFileBrowserView  *view)
{
	GtkTreeIter parent;
	GtkTreePath *copy;

	if (gtk_tree_model_iter_has_child (GTK_TREE_MODEL (model), iter))
		restore_expand_state (view, model, iter);

	copy = gtk_tree_path_copy (path);

	if (gtk_tree_path_up (copy) &&
	    (gtk_tree_path_get_depth (copy) != 0) &&
	    gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &parent, copy))
	{
		restore_expand_state (view, model, &parent);
	}

	gtk_tree_path_free (copy);
}

void
_gedit_file_browser_view_register_type (GTypeModule *type_module)
{
	gedit_file_browser_view_register_type (type_module);
}

/* ex:set ts=8 noet: */
