/*
 * gedit-window.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
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

#include "gedit-window.h"

#include <time.h>
#include <sys/types.h>
#include <string.h>

#include <glib/gi18n.h>
#include <libpeas/peas-extension-set.h>
#include <tepl/tepl.h>

#include "gedit-window-private.h"
#include "gedit-app.h"
#include "gedit-app-private.h"
#include "gedit-notebook.h"
#include "gedit-notebook-popup-menu.h"
#include "gedit-multi-notebook.h"
#include "gedit-statusbar.h"
#include "gedit-tab.h"
#include "gedit-tab-private.h"
#include "gedit-view-frame.h"
#include "gedit-utils.h"
#include "gedit-commands.h"
#include "gedit-commands-private.h"
#include "gedit-debug.h"
#include "gedit-document.h"
#include "gedit-document-private.h"
#include "gedit-documents-panel.h"
#include "gedit-plugins-engine.h"
#include "gedit-window-activatable.h"
#include "gedit-enum-types.h"
#include "gedit-dirs.h"
#include "gedit-status-menu-button.h"
#include "gedit-settings.h"
#include "gedit-menu-stack-switcher.h"

enum
{
	PROP_0,
	PROP_STATE,
	LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

enum
{
	TAB_ADDED,
	TAB_REMOVED,
	TABS_REORDERED,
	ACTIVE_TAB_CHANGED,
	ACTIVE_TAB_STATE_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

enum
{
	TARGET_URI_LIST = 100,
	TARGET_XDNDDIRECTSAVE
};

static const GtkTargetEntry drop_types [] = {
	{ "XdndDirectSave0", 0, TARGET_XDNDDIRECTSAVE }, /* XDS Protocol Type */
	{ "text/uri-list", 0, TARGET_URI_LIST}
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditWindow, gedit_window, GTK_TYPE_APPLICATION_WINDOW)

/* Prototypes */
static void remove_actions (GeditWindow *window);

static void
gedit_window_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	GeditWindow *window = GEDIT_WINDOW (object);

	switch (prop_id)
	{
		case PROP_STATE:
			g_value_set_flags (value,
					   gedit_window_get_state (window));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
save_panels_state (GeditWindow *window)
{
	const gchar *panel_page;

	gedit_debug (DEBUG_WINDOW);

	if (window->priv->side_panel_size > 0)
	{
		g_settings_set_int (window->priv->window_settings,
				    GEDIT_SETTINGS_SIDE_PANEL_SIZE,
				    window->priv->side_panel_size);
	}

	panel_page = gtk_stack_get_visible_child_name (GTK_STACK (window->priv->side_panel));
	if (panel_page != NULL)
	{
		g_settings_set_string (window->priv->window_settings,
				       GEDIT_SETTINGS_SIDE_PANEL_ACTIVE_PAGE,
				       panel_page);
	}

	if (window->priv->bottom_panel_size > 0)
	{
		g_settings_set_int (window->priv->window_settings,
				    GEDIT_SETTINGS_BOTTOM_PANEL_SIZE,
				    window->priv->bottom_panel_size);
	}

	panel_page = gtk_stack_get_visible_child_name (GTK_STACK (window->priv->bottom_panel));
	if (panel_page != NULL)
	{
		g_settings_set_string (window->priv->window_settings,
				       GEDIT_SETTINGS_BOTTOM_PANEL_ACTIVE_PAGE,
				       panel_page);
	}

	g_settings_apply (window->priv->window_settings);
}

static void
save_window_state (GtkWidget *widget)
{
	GeditWindow *window = GEDIT_WINDOW (widget);

	if ((window->priv->window_state &
	     (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN)) == 0)
	{
		gtk_window_get_size (GTK_WINDOW (widget), &window->priv->width, &window->priv->height);

		g_settings_set (window->priv->window_settings, GEDIT_SETTINGS_WINDOW_SIZE,
				"(ii)", window->priv->width, window->priv->height);
	}
}

static void
gedit_window_dispose (GObject *object)
{
	GeditWindow *window;

	gedit_debug (DEBUG_WINDOW);

	window = GEDIT_WINDOW (object);

	/* Stop tracking removal of panels otherwise we always
	 * end up with thinking we had no panel active, since they
	 * should all be removed below */
	if (window->priv->bottom_panel_item_removed_handler_id != 0)
	{
		g_signal_handler_disconnect (window->priv->bottom_panel,
					     window->priv->bottom_panel_item_removed_handler_id);
		window->priv->bottom_panel_item_removed_handler_id = 0;
	}

	/* First of all, force collection so that plugins
	 * really drop some of the references.
	 */
	peas_engine_garbage_collect (PEAS_ENGINE (gedit_plugins_engine_get_default ()));

	/* save the panels position and make sure to deactivate plugins
	 * for this window, but only once */
	if (!window->priv->dispose_has_run)
	{
		save_window_state (GTK_WIDGET (window));
		save_panels_state (window);

		/* Note that unreffing the extensions will automatically remove
		   all extensions which in turn will deactivate the extension */
		g_object_unref (window->priv->extensions);

		peas_engine_garbage_collect (PEAS_ENGINE (gedit_plugins_engine_get_default ()));

		window->priv->dispose_has_run = TRUE;
	}

	g_clear_object (&window->priv->message_bus);
	g_clear_object (&window->priv->window_group);

	/* We must free the settings after saving the panels */
	g_clear_object (&window->priv->editor_settings);
	g_clear_object (&window->priv->ui_settings);
	g_clear_object (&window->priv->window_settings);

	/* Now that there have broken some reference loops,
	 * force collection again.
	 */
	peas_engine_garbage_collect (PEAS_ENGINE (gedit_plugins_engine_get_default ()));

	g_clear_object (&window->priv->side_stack_switcher);

	/* GTK+/GIO unref the action map in an idle. For the last GeditWindow,
	 * the application quits before the idle, so the action map is not
	 * unreffed, and some objects are not finalized on application shutdown
	 * (GeditView for example).
	 * So this is just for making the debugging of object references a bit
	 * nicer.
	 */
	remove_actions (window);

	window->priv->fullscreen_open_recent_button = NULL;

	G_OBJECT_CLASS (gedit_window_parent_class)->dispose (object);
}

static void
gedit_window_finalize (GObject *object)
{
	GeditWindow *window = GEDIT_WINDOW (object);

	g_free (window->priv->file_chooser_folder_uri);
	g_slist_free_full (window->priv->closed_docs_stack, (GDestroyNotify)g_object_unref);

	G_OBJECT_CLASS (gedit_window_parent_class)->finalize (object);
}

static void
update_fullscreen (GeditWindow *window,
                   gboolean     is_fullscreen)
{
	GAction *fullscreen_action;

	_gedit_multi_notebook_set_show_tabs (window->priv->multi_notebook, !is_fullscreen);

	if (is_fullscreen)
	{
		gtk_widget_hide (window->priv->statusbar);
	}
	else
	{
		if (g_settings_get_boolean (window->priv->ui_settings, "statusbar-visible"))
		{
			gtk_widget_show (window->priv->statusbar);
		}
	}

#ifndef OS_OSX
	if (is_fullscreen)
	{
		gtk_widget_show_all (window->priv->fullscreen_eventbox);
	}
	else
	{
		gtk_widget_hide (window->priv->fullscreen_eventbox);
	}
#endif

	fullscreen_action = g_action_map_lookup_action (G_ACTION_MAP (window),
	                                                "fullscreen");

	g_simple_action_set_state (G_SIMPLE_ACTION (fullscreen_action),
	                           g_variant_new_boolean (is_fullscreen));
}

static gboolean
gedit_window_window_state_event (GtkWidget           *widget,
				 GdkEventWindowState *event)
{
	GeditWindow *window = GEDIT_WINDOW (widget);

	window->priv->window_state = event->new_window_state;

	g_settings_set_int (window->priv->window_settings, GEDIT_SETTINGS_WINDOW_STATE,
			    window->priv->window_state);

	if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) != 0)
	{
		update_fullscreen (window, (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0);
	}

	return GTK_WIDGET_CLASS (gedit_window_parent_class)->window_state_event (widget, event);
}

static gboolean
gedit_window_configure_event (GtkWidget         *widget,
			      GdkEventConfigure *event)
{
	GeditWindow *window = GEDIT_WINDOW (widget);

	if (gtk_widget_get_realized (widget) &&
	    (window->priv->window_state &
	     (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN)) == 0)
	{
		save_window_state (widget);
	}

	return GTK_WIDGET_CLASS (gedit_window_parent_class)->configure_event (widget, event);
}

/*
 * GtkWindow catches keybindings for the menu items _before_ passing them to
 * the focused widget. This is unfortunate and means that pressing ctrl+V
 * in an entry on a panel ends up pasting text in the TextView.
 * Here we override GtkWindow's handler to do the same things that it
 * does, but in the opposite order and then we chain up to the grand
 * parent handler, skipping gtk_window_key_press_event.
 */
static gboolean
gedit_window_key_press_event (GtkWidget   *widget,
			      GdkEventKey *event)
{
	static gpointer grand_parent_class = NULL;

	GtkWindow *window = GTK_WINDOW (widget);
	gboolean handled = FALSE;

	if (grand_parent_class == NULL)
	{
		grand_parent_class = g_type_class_peek_parent (gedit_window_parent_class);
	}

	/* handle focus widget key events */
	if (!handled)
	{
		handled = gtk_window_propagate_key_event (window, event);
	}

	/* handle mnemonics and accelerators */
	if (!handled)
	{
		handled = gtk_window_activate_key (window, event);
	}

	/* Chain up, invokes binding set on window */
	if (!handled)
	{
		handled = GTK_WIDGET_CLASS (grand_parent_class)->key_press_event (widget, event);
	}

	if (!handled)
	{
		return gedit_app_process_window_event (GEDIT_APP (g_application_get_default ()),
		                                       GEDIT_WINDOW (widget),
		                                       (GdkEvent *)event);
	}

	return TRUE;
}

static void
gedit_window_tab_removed (GeditWindow *window,
			  GeditTab    *tab)
{
	peas_engine_garbage_collect (PEAS_ENGINE (gedit_plugins_engine_get_default ()));
}

static void
gedit_window_class_init (GeditWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	klass->tab_removed = gedit_window_tab_removed;

	object_class->dispose = gedit_window_dispose;
	object_class->finalize = gedit_window_finalize;
	object_class->get_property = gedit_window_get_property;

	widget_class->window_state_event = gedit_window_window_state_event;
	widget_class->configure_event = gedit_window_configure_event;
	widget_class->key_press_event = gedit_window_key_press_event;

	properties[PROP_STATE] =
		g_param_spec_flags ("state",
		                    "State",
		                    "The window's state",
		                    GEDIT_TYPE_WINDOW_STATE,
		                    GEDIT_WINDOW_STATE_NORMAL,
		                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, LAST_PROP, properties);

	signals[TAB_ADDED] =
		g_signal_new ("tab-added",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditWindowClass, tab_added),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_TAB);
	signals[TAB_REMOVED] =
		g_signal_new ("tab-removed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditWindowClass, tab_removed),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_TAB);
	signals[TABS_REORDERED] =
		g_signal_new ("tabs-reordered",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditWindowClass, tabs_reordered),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      0);
	signals[ACTIVE_TAB_CHANGED] =
		g_signal_new ("active-tab-changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditWindowClass, active_tab_changed),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_TAB);
	signals[ACTIVE_TAB_STATE_CHANGED] =
		g_signal_new ("active-tab-state-changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditWindowClass, active_tab_state_changed),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      0);

	/* Bind class to template */
	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/gedit/ui/gedit-window.ui");
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, titlebar_paned);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, side_headerbar);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, headerbar);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, new_button);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, gear_button);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, hpaned);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, side_panel);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, side_panel_inline_stack_switcher);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, vpaned);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, multi_notebook);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, bottom_panel_box);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, bottom_panel);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, statusbar);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, language_button);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, tab_width_button);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, line_col_button);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, fullscreen_eventbox);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, fullscreen_revealer);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, fullscreen_headerbar);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, fullscreen_new_button);
	gtk_widget_class_bind_template_child_private (widget_class, GeditWindow, fullscreen_gear_button);
}

static void
received_clipboard_contents (GtkClipboard     *clipboard,
			     GtkSelectionData *selection_data,
			     GeditWindow      *window)
{
	GeditTab *tab;
	gboolean enabled;
	GAction *action;

	/* getting clipboard contents is async, so we need to
	 * get the current tab and its state */

	tab = gedit_window_get_active_tab (window);

	if (tab != NULL)
	{
		GeditTabState state;
		gboolean state_normal;

		state = gedit_tab_get_state (tab);
		state_normal = (state == GEDIT_TAB_STATE_NORMAL);

		enabled = state_normal &&
		          gtk_selection_data_targets_include_text (selection_data);
	}
	else
	{
		enabled = FALSE;
	}

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "paste");

	/* Since this is emitted async, the disposal of the actions may have
	 * already happened. Ensure that we have an action before setting the
	 * state.
	 */
	if (action != NULL)
	{
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);
	}

	g_object_unref (window);
}

static void
set_paste_sensitivity_according_to_clipboard (GeditWindow  *window,
					      GtkClipboard *clipboard)
{
	GdkDisplay *display;

	display = gtk_clipboard_get_display (clipboard);

	if (gdk_display_supports_selection_notification (display))
	{
		gtk_clipboard_request_contents (clipboard,
						gdk_atom_intern_static_string ("TARGETS"),
						(GtkClipboardReceivedFunc) received_clipboard_contents,
						g_object_ref (window));
	}
	else
	{
		GAction *action;

		action = g_action_map_lookup_action (G_ACTION_MAP (window), "paste");
		/* XFIXES extension not availbale, make
		 * Paste always sensitive */
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), TRUE);
	}
}

static void
extension_update_state (PeasExtensionSet *extensions,
		        PeasPluginInfo   *info,
		        PeasExtension    *exten,
		        GeditWindow      *window)
{
	gedit_window_activatable_update_state (GEDIT_WINDOW_ACTIVATABLE (exten));
}

static void
update_actions_sensitivity (GeditWindow *window)
{
	GeditNotebook *notebook;
	GeditTab *tab;
	gint num_notebooks;
	gint num_tabs;
	GeditTabState state = GEDIT_TAB_STATE_NORMAL;
	GeditDocument *doc = NULL;
	GtkSourceFile *file = NULL;
	GeditView *view = NULL;
	gint tab_number = -1;
	GAction *action;
	gboolean editable = FALSE;
	gboolean empty_search = FALSE;
	GtkClipboard *clipboard;
	gboolean enable_syntax_highlighting;

	gedit_debug (DEBUG_WINDOW);

	notebook = gedit_multi_notebook_get_active_notebook (window->priv->multi_notebook);
	tab = gedit_multi_notebook_get_active_tab (window->priv->multi_notebook);
	num_notebooks = gedit_multi_notebook_get_n_notebooks (window->priv->multi_notebook);
	num_tabs = gedit_multi_notebook_get_n_tabs (window->priv->multi_notebook);

	if (notebook != NULL && tab != NULL)
	{
		state = gedit_tab_get_state (tab);
		view = gedit_tab_get_view (tab);
		doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
		file = gedit_document_get_file (doc);
		tab_number = gtk_notebook_page_num (GTK_NOTEBOOK (notebook), GTK_WIDGET (tab));
		editable = gtk_text_view_get_editable (GTK_TEXT_VIEW (view));
		empty_search = _gedit_document_get_empty_search (doc);
	}

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (window), GDK_SELECTION_CLIPBOARD);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "save");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             ((state == GEDIT_TAB_STATE_NORMAL) ||
	                              (state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)) &&
	                             (file != NULL) && !gtk_source_file_is_readonly (file));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "save-as");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             ((state == GEDIT_TAB_STATE_NORMAL) ||
	                              (state == GEDIT_TAB_STATE_SAVING_ERROR) ||
	                              (state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)) &&
	                             (doc != NULL));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "revert");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             ((state == GEDIT_TAB_STATE_NORMAL) ||
	                              (state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)) &&
	                             (doc != NULL) && !gedit_document_is_untitled (doc));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "reopen-closed-tab");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), (window->priv->closed_docs_stack != NULL));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "print");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             ((state == GEDIT_TAB_STATE_NORMAL) ||
	                              (state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW)) &&
	                             (doc != NULL));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "close");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             (state != GEDIT_TAB_STATE_CLOSING) &&
	                             (state != GEDIT_TAB_STATE_SAVING) &&
	                             (state != GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW) &&
	                             (state != GEDIT_TAB_STATE_PRINTING) &&
	                             (state != GEDIT_TAB_STATE_SAVING_ERROR));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "undo");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             (state == GEDIT_TAB_STATE_NORMAL) &&
	                             (doc != NULL) && gtk_source_buffer_can_undo (GTK_SOURCE_BUFFER (doc)));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "redo");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             (state == GEDIT_TAB_STATE_NORMAL) &&
	                             (doc != NULL) && gtk_source_buffer_can_redo (GTK_SOURCE_BUFFER (doc)));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "cut");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             (state == GEDIT_TAB_STATE_NORMAL) &&
	                             editable &&
	                             (doc != NULL) && gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER (doc)));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "copy");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             ((state == GEDIT_TAB_STATE_NORMAL) ||
	                              (state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)) &&
	                             (doc != NULL) && gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER (doc)));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "paste");
	if (num_tabs > 0 && (state == GEDIT_TAB_STATE_NORMAL) && editable)
	{
		set_paste_sensitivity_according_to_clipboard (window, clipboard);
	}
	else
	{
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
	}

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "delete");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             (state == GEDIT_TAB_STATE_NORMAL) &&
	                             editable &&
	                             (doc != NULL) && gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER (doc)));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "overwrite-mode");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), doc != NULL);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "find");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             ((state == GEDIT_TAB_STATE_NORMAL) ||
	                              (state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)) &&
	                             (doc != NULL));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "replace");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             (state == GEDIT_TAB_STATE_NORMAL) &&
	                             (doc != NULL) && editable);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "find-next");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             ((state == GEDIT_TAB_STATE_NORMAL) ||
	                              (state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)) &&
	                              (doc != NULL) && !empty_search);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "find-prev");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             ((state == GEDIT_TAB_STATE_NORMAL) ||
	                              (state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)) &&
	                              (doc != NULL) && !empty_search);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "clear-highlight");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             ((state == GEDIT_TAB_STATE_NORMAL) ||
	                              (state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)) &&
	                              (doc != NULL) && !empty_search);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "goto-line");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             ((state == GEDIT_TAB_STATE_NORMAL) ||
	                              (state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)) &&
	                             (doc != NULL));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "highlight-mode");
	enable_syntax_highlighting = g_settings_get_boolean (window->priv->editor_settings,
	                                                     GEDIT_SETTINGS_SYNTAX_HIGHLIGHTING);
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             (state != GEDIT_TAB_STATE_CLOSING) &&
	                             (doc != NULL) && enable_syntax_highlighting);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "move-to-new-window");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             num_tabs > 1);

	action = g_action_map_lookup_action (G_ACTION_MAP (window),
	                                     "previous-document");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             tab_number > 0);

	action = g_action_map_lookup_action (G_ACTION_MAP (window),
	                                     "next-document");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             tab_number >= 0 &&
	                             tab_number < gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook)) - 1);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "new-tab-group");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             num_tabs > 0);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "previous-tab-group");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             num_notebooks > 1);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "next-tab-group");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             num_notebooks > 1);

	/* We disable File->Quit/SaveAll/CloseAll while printing to avoid to have two
	   operations (save and print/print preview) that uses the message area at
	   the same time (may be we can remove this limitation in the future) */
	/* We disable File->Quit/CloseAll if state is saving since saving cannot be
	   cancelled (may be we can remove this limitation in the future) */
	action = g_action_map_lookup_action (G_ACTION_MAP (g_application_get_default ()),
	                                     "quit");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             !(window->priv->state & GEDIT_WINDOW_STATE_SAVING) &&
	                             !(window->priv->state & GEDIT_WINDOW_STATE_PRINTING));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "save-all");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             !(window->priv->state & GEDIT_WINDOW_STATE_PRINTING) &&
	                             num_tabs > 0);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "close-all");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
	                             num_tabs > 0 &&
	                             !(window->priv->state & GEDIT_WINDOW_STATE_SAVING) &&
	                             !(window->priv->state & GEDIT_WINDOW_STATE_PRINTING) &&
	                             num_tabs > 0);

	peas_extension_set_foreach (window->priv->extensions,
	                            (PeasExtensionSetForeachFunc) extension_update_state,
	                            window);
}

static void
language_chooser_show_cb (TeplLanguageChooser *language_chooser,
			  GeditWindow         *window)
{
	GeditDocument *active_document;

	active_document = gedit_window_get_active_document (window);
	if (active_document != NULL)
	{
		GtkSourceLanguage *language;

		language = gedit_document_get_language (active_document);
		tepl_language_chooser_select_language (language_chooser, language);
	}
}

static void
language_activated_cb (TeplLanguageChooser *language_chooser,
		       GtkSourceLanguage   *language,
		       GeditWindow         *window)
{
	GeditDocument *active_document;

	active_document = gedit_window_get_active_document (window);
	if (active_document != NULL)
	{
		gedit_document_set_language (active_document, language);
	}

	gtk_widget_hide (window->priv->language_popover);
}

static void
setup_statusbar (GeditWindow *window)
{
	TeplLanguageChooserWidget *language_chooser;

	gedit_debug (DEBUG_WINDOW);

	window->priv->generic_message_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR (window->priv->statusbar), "generic_message");
	window->priv->tip_message_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR (window->priv->statusbar), "tip_message");
	window->priv->bracket_match_message_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR (window->priv->statusbar), "bracket_match_message");

	g_settings_bind (window->priv->ui_settings,
	                 "statusbar-visible",
	                 window->priv->statusbar,
	                 "visible",
	                 G_SETTINGS_BIND_GET);

	/* Line Col button */
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (window->priv->line_col_button),
	                                _gedit_app_get_line_col_menu (GEDIT_APP (g_application_get_default ())));

	/* Tab Width button */
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (window->priv->tab_width_button),
	                                _gedit_app_get_tab_width_menu (GEDIT_APP (g_application_get_default ())));

	/* Language button */
	window->priv->language_popover = gtk_popover_new (window->priv->language_button);
	gtk_menu_button_set_popover (GTK_MENU_BUTTON (window->priv->language_button),
	                             window->priv->language_popover);

	language_chooser = tepl_language_chooser_widget_new ();

	g_signal_connect (language_chooser,
	                  "show",
	                  G_CALLBACK (language_chooser_show_cb),
	                  window);

	g_signal_connect (language_chooser,
	                  "language-activated",
	                  G_CALLBACK (language_activated_cb),
	                  window);

	gtk_container_add (GTK_CONTAINER (window->priv->language_popover), GTK_WIDGET (language_chooser));
	gtk_widget_show (GTK_WIDGET (language_chooser));
}

static GeditWindow *
clone_window (GeditWindow *origin)
{
	GeditWindow *window;
	GdkScreen *screen;
	GeditApp  *app;
	const gchar *panel_page;

	gedit_debug (DEBUG_WINDOW);

	app = GEDIT_APP (g_application_get_default ());

	screen = gtk_window_get_screen (GTK_WINDOW (origin));
	window = gedit_app_create_window (app, screen);

	gtk_window_set_default_size (GTK_WINDOW (window),
				     origin->priv->width,
				     origin->priv->height);

	if ((origin->priv->window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0)
		gtk_window_maximize (GTK_WINDOW (window));
	else
		gtk_window_unmaximize (GTK_WINDOW (window));

	if ((origin->priv->window_state & GDK_WINDOW_STATE_STICKY) != 0)
		gtk_window_stick (GTK_WINDOW (window));
	else
		gtk_window_unstick (GTK_WINDOW (window));

	/* set the panels size, the paned position will be set when
	 * they are mapped */
	window->priv->side_panel_size = origin->priv->side_panel_size;
	window->priv->bottom_panel_size = origin->priv->bottom_panel_size;

	panel_page = gtk_stack_get_visible_child_name (GTK_STACK (origin->priv->side_panel));

	if (panel_page)
	{
		gtk_stack_set_visible_child_name (GTK_STACK (window->priv->side_panel), panel_page);
	}

	panel_page = gtk_stack_get_visible_child_name (GTK_STACK (origin->priv->bottom_panel));

	if (panel_page)
	{
		gtk_stack_set_visible_child_name (GTK_STACK (window->priv->bottom_panel), panel_page);
	}

	gtk_widget_set_visible (window->priv->side_panel,
	                        gtk_widget_get_visible (origin->priv->side_panel));
	gtk_widget_set_visible (window->priv->bottom_panel,
	                        gtk_widget_get_visible (origin->priv->bottom_panel));

	return window;
}

static void
bracket_matched_cb (GtkSourceBuffer           *buffer,
		    GtkTextIter               *iter,
		    GtkSourceBracketMatchType  result,
		    GeditWindow               *window)
{
	if (buffer != GTK_SOURCE_BUFFER (gedit_window_get_active_document (window)))
		return;

	switch (result)
	{
		case GTK_SOURCE_BRACKET_MATCH_NONE:
			gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
					   window->priv->bracket_match_message_cid);
			break;
		case GTK_SOURCE_BRACKET_MATCH_OUT_OF_RANGE:
			gedit_statusbar_flash_message (GEDIT_STATUSBAR (window->priv->statusbar),
						       window->priv->bracket_match_message_cid,
						       _("Bracket match is out of range"));
			break;
		case GTK_SOURCE_BRACKET_MATCH_NOT_FOUND:
			gedit_statusbar_flash_message (GEDIT_STATUSBAR (window->priv->statusbar),
						       window->priv->bracket_match_message_cid,
						       _("Bracket match not found"));
			break;
		case GTK_SOURCE_BRACKET_MATCH_FOUND:
			gedit_statusbar_flash_message (GEDIT_STATUSBAR (window->priv->statusbar),
						       window->priv->bracket_match_message_cid,
						       _("Bracket match found on line: %d"),
						       gtk_text_iter_get_line (iter) + 1);
			break;
		default:
			g_assert_not_reached ();
	}
}

static void
update_cursor_position_statusbar (GtkTextBuffer *buffer,
				  GeditWindow   *window)
{
	gint line, col;
	GtkTextIter iter;
	GeditView *view;
	gchar *msg = NULL;

	gedit_debug (DEBUG_WINDOW);

 	if (buffer != GTK_TEXT_BUFFER (gedit_window_get_active_document (window)))
 		return;

 	view = gedit_window_get_active_view (window);

	gtk_text_buffer_get_iter_at_mark (buffer,
					  &iter,
					  gtk_text_buffer_get_insert (buffer));

	line = 1 + gtk_text_iter_get_line (&iter);
	col = 1 + gtk_source_view_get_visual_column (GTK_SOURCE_VIEW (view), &iter);

	if ((line >= 0) || (col >= 0))
	{
		/* Translators: "Ln" is an abbreviation for "Line", Col is an abbreviation for "Column". Please,
		use abbreviations if possible to avoid space problems. */
		msg = g_strdup_printf (_("  Ln %d, Col %d"), line, col);
	}

	gedit_status_menu_button_set_label (GEDIT_STATUS_MENU_BUTTON (window->priv->line_col_button), msg);

	g_free (msg);
}

static void
set_overwrite_mode (GeditWindow *window,
                    gboolean     overwrite)
{
	GAction *action;

	gedit_statusbar_set_overwrite (GEDIT_STATUSBAR (window->priv->statusbar), overwrite);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "overwrite-mode");
	g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (overwrite));
}

static void
overwrite_mode_changed (GtkTextView *view,
			GParamSpec  *pspec,
			GeditWindow *window)
{
	if (view != GTK_TEXT_VIEW (gedit_window_get_active_view (window)))
		return;

	set_overwrite_mode (window, gtk_text_view_get_overwrite (view));
}

#define MAX_TITLE_LENGTH 100

static void
set_title (GeditWindow *window)
{
	GeditTab *tab;
	GeditDocument *doc = NULL;
	GtkSourceFile *file;
	gchar *name;
	gchar *dirname = NULL;
	gchar *main_title = NULL;
	gchar *title = NULL;
	gchar *subtitle = NULL;
	gint len;

	tab = gedit_window_get_active_tab (window);

	if (tab == NULL)
	{
		gedit_app_set_window_title (GEDIT_APP (g_application_get_default ()),
		                            window,
		                            "gedit");
		gtk_header_bar_set_title (GTK_HEADER_BAR (window->priv->headerbar),
		                          "gedit");
		gtk_header_bar_set_subtitle (GTK_HEADER_BAR (window->priv->headerbar),
		                             NULL);
		gtk_header_bar_set_title (GTK_HEADER_BAR (window->priv->fullscreen_headerbar),
		                          "gedit");
		gtk_header_bar_set_subtitle (GTK_HEADER_BAR (window->priv->fullscreen_headerbar),
		                             NULL);
		return;
	}

	doc = gedit_tab_get_document (tab);
	g_return_if_fail (doc != NULL);

	file = gedit_document_get_file (doc);

	name = gedit_document_get_short_name_for_display (doc);

	len = g_utf8_strlen (name, -1);

	/* if the name is awfully long, truncate it and be done with it,
	 * otherwise also show the directory (ellipsized if needed)
	 */
	if (len > MAX_TITLE_LENGTH)
	{
		gchar *tmp;

		tmp = tepl_utils_str_middle_truncate (name,
						      MAX_TITLE_LENGTH);
		g_free (name);
		name = tmp;
	}
	else
	{
		GFile *location = gtk_source_file_get_location (file);

		if (location != NULL)
		{
			gchar *str = gedit_utils_location_get_dirname_for_display (location);

			/* use the remaining space for the dir, but use a min of 20 chars
			 * so that we do not end up with a dirname like "(a...b)".
			 * This means that in the worst case when the filename is long 99
			 * we have a title long 99 + 20, but I think it's a rare enough
			 * case to be acceptable. It's justa darn title afterall :)
			 */
			dirname = tepl_utils_str_middle_truncate (str,
								  MAX (20, MAX_TITLE_LENGTH - len));
			g_free (str);
		}
	}

	if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)))
	{
		gchar *tmp_name;

		tmp_name = g_strdup_printf ("*%s", name);
		g_free (name);

		name = tmp_name;
	}

	if (gtk_source_file_is_readonly (file))
	{
		title = g_strdup_printf ("%s [%s]",
		                         name, _("Read-Only"));

		if (dirname != NULL)
		{
			main_title = g_strdup_printf ("%s [%s] (%s) - gedit",
			                              name,
			                              _("Read-Only"),
			                              dirname);
			subtitle = dirname;
		}
		else
		{
			main_title = g_strdup_printf ("%s [%s] - gedit",
			                              name,
			                              _("Read-Only"));
		}
	}
	else
	{
		title = g_strdup (name);

		if (dirname != NULL)
		{
			main_title = g_strdup_printf ("%s (%s) - gedit",
			                              name,
			                              dirname);
			subtitle = dirname;
		}
		else
		{
			main_title = g_strdup_printf ("%s - gedit",
			                              name);
		}
	}

	gedit_app_set_window_title (GEDIT_APP (g_application_get_default ()),
				    window,
				    main_title);

	gtk_header_bar_set_title (GTK_HEADER_BAR (window->priv->headerbar),
	                          title);
	gtk_header_bar_set_subtitle (GTK_HEADER_BAR (window->priv->headerbar),
	                             subtitle);
	gtk_header_bar_set_title (GTK_HEADER_BAR (window->priv->fullscreen_headerbar),
	                          title);
	gtk_header_bar_set_subtitle (GTK_HEADER_BAR (window->priv->fullscreen_headerbar),
	                             subtitle);

	g_free (dirname);
	g_free (name);
	g_free (title);
	g_free (main_title);
}

#undef MAX_TITLE_LENGTH

static void
tab_width_changed (GObject     *object,
		   GParamSpec  *pspec,
		   GeditWindow *window)
{
	guint new_tab_width;
	gchar *label;

	new_tab_width = gtk_source_view_get_tab_width (GTK_SOURCE_VIEW (object));

	label = g_strdup_printf (_("Tab Width: %u"), new_tab_width);
	gedit_status_menu_button_set_label (GEDIT_STATUS_MENU_BUTTON (window->priv->tab_width_button), label);
	g_free (label);
}

static void
language_changed (GObject     *object,
		  GParamSpec  *pspec,
		  GeditWindow *window)
{
	GtkSourceLanguage *new_language;
	const gchar *label;

	new_language = gtk_source_buffer_get_language (GTK_SOURCE_BUFFER (object));

	if (new_language)
		label = gtk_source_language_get_name (new_language);
	else
		label = _("Plain Text");

	gedit_status_menu_button_set_label (GEDIT_STATUS_MENU_BUTTON (window->priv->language_button), label);

	peas_extension_set_foreach (window->priv->extensions,
	                            (PeasExtensionSetForeachFunc) extension_update_state,
	                            window);
}

static void
update_statusbar_wrap_mode_checkbox_from_view (GeditWindow *window,
                                               GeditView   *view)
{
	GtkWrapMode wrap_mode;
	GSimpleAction *simple_action;

	wrap_mode = gtk_text_view_get_wrap_mode (GTK_TEXT_VIEW (view));

	simple_action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (window), "wrap-mode"));
	g_simple_action_set_state (simple_action, g_variant_new_boolean (wrap_mode != GTK_WRAP_NONE));
}
static void
on_view_wrap_mode_changed (GObject     *object,
                           GParamSpec  *pspec,
                           GeditWindow *window)
{
	GeditView *view = gedit_window_get_active_view (window);

	update_statusbar_wrap_mode_checkbox_from_view (window, view);
}

static void
_gedit_window_text_wrapping_change_state (GSimpleAction *simple,
                                          GVariant      *value,
                                          gpointer       window)
{
	gboolean result;
	GeditView *view;
	GtkWrapMode wrap_mode;
	GtkWrapMode current_wrap_mode;

	g_simple_action_set_state (simple, value);

	wrap_mode = g_settings_get_enum (GEDIT_WINDOW (window)->priv->editor_settings,
	                                 GEDIT_SETTINGS_WRAP_MODE);

	current_wrap_mode = wrap_mode;
	result = g_variant_get_boolean (value);

	if (result && wrap_mode == GTK_WRAP_NONE)
	{
		current_wrap_mode = g_settings_get_enum (GEDIT_WINDOW (window)->priv->editor_settings,
		                                         GEDIT_SETTINGS_WRAP_LAST_SPLIT_MODE);
	}
	else if (!result)
	{
		current_wrap_mode = GTK_WRAP_NONE;
	}

	view = gedit_window_get_active_view (GEDIT_WINDOW (window));

	g_signal_handler_block (view, GEDIT_WINDOW (window)->priv->wrap_mode_changed_id);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), current_wrap_mode);
	g_signal_handler_unblock (view, GEDIT_WINDOW (window)->priv->wrap_mode_changed_id);
}

static GActionEntry text_wrapping_entrie[] = {
	{"wrap-mode", NULL, NULL, "false", _gedit_window_text_wrapping_change_state},
};

static void
remove_actions (GeditWindow *window)
{
	g_action_map_remove_action (G_ACTION_MAP (window), "auto-indent");
	g_action_map_remove_action (G_ACTION_MAP (window), "tab-width");
	g_action_map_remove_action (G_ACTION_MAP (window), "use-spaces");
	g_action_map_remove_action (G_ACTION_MAP (window), "show-line-numbers");
	g_action_map_remove_action (G_ACTION_MAP (window), "display-right-margin");
	g_action_map_remove_action (G_ACTION_MAP (window), "highlight-current-line");
	g_action_map_remove_action (G_ACTION_MAP (window), "wrap-mode");
}

static void
sync_current_tab_actions (GeditWindow *window,
			  GeditView   *old_view,
			  GeditView   *new_view)
{
	if (old_view != NULL)
	{
		remove_actions (window);

		g_signal_handler_disconnect (old_view, window->priv->wrap_mode_changed_id);
	}

	if (new_view != NULL)
	{
		GPropertyAction *action;

		action = g_property_action_new ("auto-indent", new_view, "auto-indent");
		g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));
		g_object_unref (action);

		action = g_property_action_new ("tab-width", new_view, "tab-width");
		g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));
		g_object_unref (action);

		action = g_property_action_new ("use-spaces", new_view, "insert-spaces-instead-of-tabs");
		g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));
		g_object_unref (action);

		action = g_property_action_new ("show-line-numbers", new_view, "show-line-numbers");
		g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));
		g_object_unref (action);

		action = g_property_action_new ("display-right-margin", new_view, "show-right-margin");
		g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));
		g_object_unref (action);

		action = g_property_action_new ("highlight-current-line", new_view, "highlight-current-line");
		g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));
		g_object_unref (action);

		g_action_map_add_action_entries (G_ACTION_MAP (window),
		                                 text_wrapping_entrie,
		                                 G_N_ELEMENTS (text_wrapping_entrie),
		                                 window);

		update_statusbar_wrap_mode_checkbox_from_view (window, new_view);

		window->priv->wrap_mode_changed_id = g_signal_connect (new_view,
		                                                       "notify::wrap-mode",
		                                                       G_CALLBACK (on_view_wrap_mode_changed),
		                                                       window);
	}
}

static void
update_statusbar (GeditWindow *window,
		  GeditView   *old_view,
		  GeditView   *new_view)
{
	if (old_view)
	{
		if (window->priv->tab_width_id)
		{
			g_signal_handler_disconnect (old_view,
						     window->priv->tab_width_id);

			window->priv->tab_width_id = 0;
		}

		if (window->priv->language_changed_id)
		{
			g_signal_handler_disconnect (gtk_text_view_get_buffer (GTK_TEXT_VIEW (old_view)),
						     window->priv->language_changed_id);

			window->priv->language_changed_id = 0;
		}
	}

	if (new_view)
	{
		GeditDocument *doc;

		doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (new_view)));

		/* sync the statusbar */
		update_cursor_position_statusbar (GTK_TEXT_BUFFER (doc),
						  window);

		set_overwrite_mode (window, gtk_text_view_get_overwrite (GTK_TEXT_VIEW (new_view)));

		gtk_widget_show (window->priv->line_col_button);
		gtk_widget_show (window->priv->tab_width_button);
		gtk_widget_show (window->priv->language_button);

		window->priv->tab_width_id = g_signal_connect (new_view,
							       "notify::tab-width",
							       G_CALLBACK (tab_width_changed),
							       window);

		window->priv->language_changed_id = g_signal_connect (doc,
								      "notify::language",
								      G_CALLBACK (language_changed),
								      window);

		/* call it for the first time */
		tab_width_changed (G_OBJECT (new_view), NULL, window);
		language_changed (G_OBJECT (doc), NULL, window);
	}
}

static void
tab_switched (GeditMultiNotebook *mnb,
	      GeditNotebook      *old_notebook,
	      GeditTab           *old_tab,
	      GeditNotebook      *new_notebook,
	      GeditTab           *new_tab,
	      GeditWindow        *window)
{
	GeditView *old_view, *new_view;

	old_view = old_tab ? gedit_tab_get_view (old_tab) : NULL;
	new_view = new_tab ? gedit_tab_get_view (new_tab) : NULL;

	sync_current_tab_actions (window, old_view, new_view);
	update_statusbar (window, old_view, new_view);

	if (new_tab == NULL || window->priv->dispose_has_run)
		return;

	set_title (window);
	update_actions_sensitivity (window);

	g_signal_emit (G_OBJECT (window),
		       signals[ACTIVE_TAB_CHANGED],
		       0,
		       new_tab);
}

static void
analyze_tab_state (GeditTab    *tab,
		   GeditWindow *window)
{
	GeditTabState ts;

	ts = gedit_tab_get_state (tab);

	switch (ts)
	{
		case GEDIT_TAB_STATE_LOADING:
		case GEDIT_TAB_STATE_REVERTING:
			window->priv->state |= GEDIT_WINDOW_STATE_LOADING;
			break;

		case GEDIT_TAB_STATE_SAVING:
			window->priv->state |= GEDIT_WINDOW_STATE_SAVING;
			break;

		case GEDIT_TAB_STATE_PRINTING:
			window->priv->state |= GEDIT_WINDOW_STATE_PRINTING;
			break;

		case GEDIT_TAB_STATE_LOADING_ERROR:
		case GEDIT_TAB_STATE_REVERTING_ERROR:
		case GEDIT_TAB_STATE_SAVING_ERROR:
		case GEDIT_TAB_STATE_GENERIC_ERROR:
			window->priv->state |= GEDIT_WINDOW_STATE_ERROR;
			++window->priv->num_tabs_with_error;
		default:
			/* NOP */
			break;
	}
}

static void
update_window_state (GeditWindow *window)
{
	GeditWindowState old_ws;
	gint old_num_of_errors;

	gedit_debug_message (DEBUG_WINDOW, "Old state: %x", window->priv->state);

	old_ws = window->priv->state;
	old_num_of_errors = window->priv->num_tabs_with_error;

	window->priv->state = 0;
	window->priv->num_tabs_with_error = 0;

	gedit_multi_notebook_foreach_tab (window->priv->multi_notebook,
					  (GtkCallback)analyze_tab_state,
					  window);

	gedit_debug_message (DEBUG_WINDOW, "New state: %x", window->priv->state);

	if (old_ws != window->priv->state)
	{
		update_actions_sensitivity (window);

		gedit_statusbar_set_window_state (GEDIT_STATUSBAR (window->priv->statusbar),
						  window->priv->state,
						  window->priv->num_tabs_with_error);

		g_object_notify_by_pspec (G_OBJECT (window), properties[PROP_STATE]);
	}
	else if (old_num_of_errors != window->priv->num_tabs_with_error)
	{
		gedit_statusbar_set_window_state (GEDIT_STATUSBAR (window->priv->statusbar),
						  window->priv->state,
						  window->priv->num_tabs_with_error);
	}
}

static void
update_can_close (GeditWindow *window)
{
	GeditWindowPrivate *priv = window->priv;
	GList *tabs;
	GList *l;
	gboolean can_close = TRUE;

	gedit_debug (DEBUG_WINDOW);

	tabs = gedit_multi_notebook_get_all_tabs (priv->multi_notebook);

	for (l = tabs; l != NULL; l = g_list_next (l))
	{
		GeditTab *tab = l->data;

		if (!_gedit_tab_get_can_close (tab))
		{
			can_close = FALSE;
			break;
		}
	}

	if (can_close && (priv->inhibition_cookie != 0))
	{
		gtk_application_uninhibit (GTK_APPLICATION (g_application_get_default ()),
					   priv->inhibition_cookie);
		priv->inhibition_cookie = 0;
	}
	else if (!can_close && (priv->inhibition_cookie == 0))
	{
		priv->inhibition_cookie = gtk_application_inhibit (GTK_APPLICATION (g_application_get_default ()),
		                                                   GTK_WINDOW (window),
		                                                   GTK_APPLICATION_INHIBIT_LOGOUT,
		                                                   _("There are unsaved documents"));
	}

	g_list_free (tabs);
}

static void
sync_state (GeditTab    *tab,
	    GParamSpec  *pspec,
	    GeditWindow *window)
{
	gedit_debug (DEBUG_WINDOW);

	update_window_state (window);

	if (tab == gedit_window_get_active_tab (window))
	{
		update_actions_sensitivity (window);

		g_signal_emit (G_OBJECT (window), signals[ACTIVE_TAB_STATE_CHANGED], 0);
	}
}

static void
sync_name (GeditTab    *tab,
	   GParamSpec  *pspec,
	   GeditWindow *window)
{
	if (tab == gedit_window_get_active_tab (window))
	{
		set_title (window);
		update_actions_sensitivity (window);
	}
}

static void
sync_can_close (GeditTab    *tab,
		GParamSpec  *pspec,
		GeditWindow *window)
{
	update_can_close (window);
}

static GeditWindow *
get_drop_window (GtkWidget *widget)
{
	GtkWidget *target_window;

	target_window = gtk_widget_get_toplevel (widget);
	g_return_val_if_fail (GEDIT_IS_WINDOW (target_window), NULL);

	return GEDIT_WINDOW (target_window);
}

static void
load_uris_from_drop (GeditWindow  *window,
		     gchar       **uri_list)
{
	GSList *locations = NULL;
	gint i;
	GSList *loaded;

	if (uri_list == NULL)
		return;

	for (i = 0; uri_list[i] != NULL; ++i)
	{
		locations = g_slist_prepend (locations, g_file_new_for_uri (uri_list[i]));
	}

	locations = g_slist_reverse (locations);
	loaded = gedit_commands_load_locations (window,
	                                        locations,
	                                        NULL,
	                                        0,
	                                        0);

	g_slist_free (loaded);
	g_slist_free_full (locations, g_object_unref);
}

/* Handle drops on the GeditWindow */
static void
drag_data_received_cb (GtkWidget        *widget,
		       GdkDragContext   *context,
		       gint              x,
		       gint              y,
		       GtkSelectionData *selection_data,
		       guint             info,
		       guint             timestamp,
		       gpointer          data)
{
	GeditWindow *window;
	gchar **uri_list;

	window = get_drop_window (widget);

	if (window == NULL)
		return;

	switch (info)
	{
		case TARGET_URI_LIST:
			uri_list = gedit_utils_drop_get_uris(selection_data);
			load_uris_from_drop (window, uri_list);
			g_strfreev (uri_list);

			gtk_drag_finish (context, TRUE, FALSE, timestamp);

			break;

		case TARGET_XDNDDIRECTSAVE:
			/* Indicate that we don't provide "F" fallback */
			if (gtk_selection_data_get_format (selection_data) == 8 &&
			    gtk_selection_data_get_length (selection_data) == 1 &&
			    gtk_selection_data_get_data (selection_data)[0] == 'F')
			{
				gdk_property_change (gdk_drag_context_get_source_window (context),
						     gdk_atom_intern ("XdndDirectSave0", FALSE),
						     gdk_atom_intern ("text/plain", FALSE), 8,
						     GDK_PROP_MODE_REPLACE, (const guchar *) "", 0);
			}
			else if (gtk_selection_data_get_format (selection_data) == 8 &&
				 gtk_selection_data_get_length (selection_data) == 1 &&
				 gtk_selection_data_get_data (selection_data)[0] == 'S' &&
				 window->priv->direct_save_uri != NULL)
			{
				gchar **uris;

				uris = g_new (gchar *, 2);
				uris[0] = window->priv->direct_save_uri;
				uris[1] = NULL;

				load_uris_from_drop (window, uris);
				g_free (uris);
			}

			g_free (window->priv->direct_save_uri);
			window->priv->direct_save_uri = NULL;

			gtk_drag_finish (context, TRUE, FALSE, timestamp);

			break;
	}
}

static void
drag_drop_cb (GtkWidget      *widget,
	      GdkDragContext *context,
	      gint            x,
	      gint            y,
	      guint           time,
	      gpointer        user_data)
{
	GeditWindow *window;
	GtkTargetList *target_list;
	GdkAtom target;

	window = get_drop_window (widget);

	target_list = gtk_drag_dest_get_target_list (widget);
	target = gtk_drag_dest_find_target (widget, context, target_list);

	if (target != GDK_NONE)
	{
		guint info;
		gboolean found;

		found = gtk_target_list_find (target_list, target, &info);
		g_assert (found);

		if (info == TARGET_XDNDDIRECTSAVE)
		{
			gchar *uri;
			uri = gedit_utils_set_direct_save_filename (context);

			if (uri != NULL)
			{
				g_free (window->priv->direct_save_uri);
				window->priv->direct_save_uri = uri;
			}
		}

		gtk_drag_get_data (GTK_WIDGET (widget), context,
				   target, time);
	}
}

/* Handle drops on the GeditView */
static void
drop_uris_cb (GtkWidget    *widget,
	      gchar       **uri_list,
	      GeditWindow  *window)
{
	load_uris_from_drop (window, uri_list);
}

static void
update_fullscreen_revealer_state (GeditWindow *window)
{
	gboolean open_recent_menu_is_active;
	gboolean hamburger_menu_is_active;

	open_recent_menu_is_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (window->priv->fullscreen_open_recent_button));
	hamburger_menu_is_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (window->priv->fullscreen_gear_button));

	gtk_revealer_set_reveal_child (window->priv->fullscreen_revealer,
				       (window->priv->in_fullscreen_eventbox ||
					open_recent_menu_is_active ||
					hamburger_menu_is_active));
}

static gboolean
on_fullscreen_eventbox_enter_notify_event (GtkWidget        *fullscreen_eventbox,
					   GdkEventCrossing *event,
					   GeditWindow      *window)
{
	window->priv->in_fullscreen_eventbox = TRUE;
	update_fullscreen_revealer_state (window);

	return GDK_EVENT_PROPAGATE;
}

static gboolean
on_fullscreen_eventbox_leave_notify_event (GtkWidget        *fullscreen_eventbox,
					   GdkEventCrossing *event,
					   GeditWindow      *window)
{
	if (-1.0 <= event->y && event->y <= 0.0)
	{
		/* Ignore the event.
		 *
		 * Leave notify events are received with -1 <= y <= 0
		 * coordinates, although the GeditWindow is in fullscreen mode
		 * and when there are no screens above (it's maybe a bug in an
		 * underlying library).
		 * If we hide the headerbar when those events happen, then it
		 * makes the headerbar to be shown/hidden a lot of time in a
		 * short period of time, i.e. a "stuttering". In other words
		 * lots of leave/enter events are received when moving the mouse
		 * upwards on the screen when the mouse is already at the top.
		 * The expected leave event has a positive event->y value being
		 * >= to the height of the headerbar (approximately
		 * 40 <= y <= 50). So clearly when we receive a leave event with
		 * event->y <= 0, it means that the mouse has left the eventbox
		 * on the wrong side.
		 * The -1.0 <= event->y is there (instead of just <= 0.0) in the
		 * case that there is another screen *above*, even if this
		 * heuristic/workaround is not perfect in that case. But that
		 * case is quite rare, so it's probably a good enough solution.
		 *
		 * Note that apparently the "stuttering" occurs only on an Xorg
		 * session, not on Wayland (tested with GNOME).
		 *
		 * If you see a better solution...
		 */
		return GDK_EVENT_PROPAGATE;
	}

	window->priv->in_fullscreen_eventbox = FALSE;
	update_fullscreen_revealer_state (window);

	return GDK_EVENT_PROPAGATE;
}

static void
setup_fullscreen_eventbox (GeditWindow *window)
{
	gtk_widget_set_size_request (window->priv->fullscreen_eventbox, -1, 1);
	gtk_widget_hide (window->priv->fullscreen_eventbox);

	g_signal_connect (window->priv->fullscreen_eventbox,
	                  "enter-notify-event",
	                  G_CALLBACK (on_fullscreen_eventbox_enter_notify_event),
	                  window);

	g_signal_connect (window->priv->fullscreen_eventbox,
	                  "leave-notify-event",
	                  G_CALLBACK (on_fullscreen_eventbox_leave_notify_event),
	                  window);
}

static void
empty_search_notify_cb (GeditDocument *doc,
			GParamSpec    *pspec,
			GeditWindow   *window)
{
	if (doc == gedit_window_get_active_document (window))
	{
		update_actions_sensitivity (window);
	}
}

static void
can_undo (GeditDocument *doc,
	  GParamSpec    *pspec,
	  GeditWindow   *window)
{
	if (doc == gedit_window_get_active_document (window))
	{
		update_actions_sensitivity (window);
	}
}

static void
can_redo (GeditDocument *doc,
	  GParamSpec    *pspec,
	  GeditWindow   *window)
{
	if (doc == gedit_window_get_active_document (window))
	{
		update_actions_sensitivity (window);
	}
}

static void
selection_changed (GeditDocument *doc,
		   GParamSpec    *pspec,
		   GeditWindow   *window)
{
	if (doc == gedit_window_get_active_document (window))
	{
		update_actions_sensitivity (window);
	}
}

static void
readonly_changed (GtkSourceFile *file,
		  GParamSpec    *pspec,
		  GeditWindow   *window)
{
	update_actions_sensitivity (window);

	sync_name (gedit_window_get_active_tab (window), NULL, window);

	peas_extension_set_foreach (window->priv->extensions,
	                            (PeasExtensionSetForeachFunc) extension_update_state,
	                            window);
}

static void
editable_changed (GeditView  *view,
                  GParamSpec  *arg1,
                  GeditWindow *window)
{
	peas_extension_set_foreach (window->priv->extensions,
	                            (PeasExtensionSetForeachFunc) extension_update_state,
	                            window);
}

static void
on_tab_added (GeditMultiNotebook *multi,
	      GeditNotebook      *notebook,
	      GeditTab           *tab,
	      GeditWindow        *window)
{
	GeditView *view;
	GeditDocument *doc;
	GtkSourceFile *file;

	gedit_debug (DEBUG_WINDOW);

	update_actions_sensitivity (window);

	view = gedit_tab_get_view (tab);
	doc = gedit_tab_get_document (tab);
	file = gedit_document_get_file (doc);

	/* IMPORTANT: remember to disconnect the signal in notebook_tab_removed
	 * if a new signal is connected here */

	g_signal_connect (tab,
			 "notify::name",
			  G_CALLBACK (sync_name),
			  window);
	g_signal_connect (tab,
			 "notify::state",
			  G_CALLBACK (sync_state),
			  window);
	g_signal_connect (tab,
			  "notify::can-close",
			  G_CALLBACK (sync_can_close),
			  window);
	g_signal_connect (tab,
			  "drop_uris",
			  G_CALLBACK (drop_uris_cb),
			  window);
	g_signal_connect (doc,
			  "bracket-matched",
			  G_CALLBACK (bracket_matched_cb),
			  window);
	g_signal_connect (doc,
			  "tepl-cursor-moved",
			  G_CALLBACK (update_cursor_position_statusbar),
			  window);
	g_signal_connect (doc,
			  "notify::empty-search",
			  G_CALLBACK (empty_search_notify_cb),
			  window);
	g_signal_connect (doc,
			  "notify::can-undo",
			  G_CALLBACK (can_undo),
			  window);
	g_signal_connect (doc,
			  "notify::can-redo",
			  G_CALLBACK (can_redo),
			  window);
	g_signal_connect (doc,
			  "notify::has-selection",
			  G_CALLBACK (selection_changed),
			  window);
	g_signal_connect (view,
			  "notify::overwrite",
			  G_CALLBACK (overwrite_mode_changed),
			  window);
	g_signal_connect (view,
			  "notify::editable",
			  G_CALLBACK (editable_changed),
			  window);
	g_signal_connect (file,
			  "notify::read-only",
			  G_CALLBACK (readonly_changed),
			  window);

	update_window_state (window);
	update_can_close (window);

	g_signal_emit (G_OBJECT (window), signals[TAB_ADDED], 0, tab);
}

static void
push_last_closed_doc (GeditWindow   *window,
                      GeditDocument *doc)
{
	GeditWindowPrivate *priv = window->priv;
	GtkSourceFile *file = gedit_document_get_file (doc);
	GFile *location = gtk_source_file_get_location (file);

	if (location != NULL)
	{
		priv->closed_docs_stack = g_slist_prepend (priv->closed_docs_stack, location);
		g_object_ref (location);
	}
}

GFile *
_gedit_window_pop_last_closed_doc (GeditWindow *window)
{
	GeditWindowPrivate *priv = window->priv;
	GFile *f = NULL;

	if (window->priv->closed_docs_stack != NULL)
	{
		f = priv->closed_docs_stack->data;
		priv->closed_docs_stack = g_slist_remove (priv->closed_docs_stack, f);
	}

	return f;
}

static void
on_tab_removed (GeditMultiNotebook *multi,
		GeditNotebook      *notebook,
		GeditTab           *tab,
		GeditWindow        *window)
{
	GeditView *view;
	GeditDocument *doc;
	gint num_tabs;

	gedit_debug (DEBUG_WINDOW);

	num_tabs = gedit_multi_notebook_get_n_tabs (multi);

	view = gedit_tab_get_view (tab);
	doc = gedit_tab_get_document (tab);

	g_signal_handlers_disconnect_by_func (tab,
					      G_CALLBACK (sync_name),
					      window);
	g_signal_handlers_disconnect_by_func (tab,
					      G_CALLBACK (sync_state),
					      window);
	g_signal_handlers_disconnect_by_func (tab,
					      G_CALLBACK (sync_can_close),
					      window);
	g_signal_handlers_disconnect_by_func (tab,
					      G_CALLBACK (drop_uris_cb),
					      window);
	g_signal_handlers_disconnect_by_func (doc,
					      G_CALLBACK (bracket_matched_cb),
					      window);
	g_signal_handlers_disconnect_by_func (doc,
					      G_CALLBACK (update_cursor_position_statusbar),
					      window);
	g_signal_handlers_disconnect_by_func (doc,
					      G_CALLBACK (empty_search_notify_cb),
					      window);
	g_signal_handlers_disconnect_by_func (doc,
					      G_CALLBACK (can_undo),
					      window);
	g_signal_handlers_disconnect_by_func (doc,
					      G_CALLBACK (can_redo),
					      window);
	g_signal_handlers_disconnect_by_func (doc,
					      G_CALLBACK (selection_changed),
					      window);
	g_signal_handlers_disconnect_by_func (doc,
					      G_CALLBACK (readonly_changed),
					      window);
	g_signal_handlers_disconnect_by_func (view,
					      G_CALLBACK (overwrite_mode_changed),
					      window);
	g_signal_handlers_disconnect_by_func (view,
					      G_CALLBACK (editable_changed),
					      window);

	if (tab == gedit_multi_notebook_get_active_tab (multi))
	{
		if (window->priv->tab_width_id)
		{
			g_signal_handler_disconnect (view, window->priv->tab_width_id);
			window->priv->tab_width_id = 0;
		}

		if (window->priv->language_changed_id)
		{
			g_signal_handler_disconnect (doc, window->priv->language_changed_id);
			window->priv->language_changed_id = 0;
		}

		gedit_multi_notebook_set_active_tab (multi, NULL);
	}

	g_return_if_fail (num_tabs >= 0);
	if (num_tabs == 0)
	{
		set_title (window);

		gedit_statusbar_clear_overwrite (
				GEDIT_STATUSBAR (window->priv->statusbar));

		/* hide the combos */
		gtk_widget_hide (window->priv->line_col_button);
		gtk_widget_hide (window->priv->tab_width_button);
		gtk_widget_hide (window->priv->language_button);
	}

	if (!window->priv->dispose_has_run)
	{
		push_last_closed_doc (window, doc);

		if ((!window->priv->removing_tabs &&
		    gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook)) > 0) ||
		    num_tabs == 0)
		{
			update_actions_sensitivity (window);
		}
	}

	update_window_state (window);
	update_can_close (window);

	g_signal_emit (G_OBJECT (window), signals[TAB_REMOVED], 0, tab);
}

static void
on_page_reordered (GeditMultiNotebook *multi,
                   GeditNotebook      *notebook,
                   GtkWidget          *page,
                   gint                page_num,
                   GeditWindow        *window)
{
	update_actions_sensitivity (window);

	g_signal_emit (G_OBJECT (window), signals[TABS_REORDERED], 0);
}

static GtkNotebook *
on_notebook_create_window (GeditMultiNotebook *mnb,
                           GtkNotebook        *notebook,
                           GtkWidget          *page,
                           gint                x,
                           gint                y,
                           GeditWindow        *window)
{
	GeditWindow *new_window;
	GtkWidget *new_notebook;

	new_window = clone_window (window);

	gtk_window_move (GTK_WINDOW (new_window), x, y);
	gtk_widget_show (GTK_WIDGET (new_window));

	new_notebook = _gedit_window_get_notebook (GEDIT_WINDOW (new_window));

	return GTK_NOTEBOOK (new_notebook);
}

static void
on_tab_close_request (GeditMultiNotebook *multi,
		      GeditNotebook      *notebook,
		      GeditTab           *tab,
		      GtkWindow          *window)
{
	/* Note: we are destroying the tab before the default handler
	 * seems to be ok, but we need to keep an eye on this. */
	_gedit_cmd_file_close_tab (tab, GEDIT_WINDOW (window));
}

static void
on_show_popup_menu (GeditMultiNotebook *multi,
                    GdkEventButton     *event,
                    GeditTab           *tab,
                    GeditWindow        *window)
{
	GtkWidget *menu;

	if (event == NULL)
	{
		return;
	}

	menu = gedit_notebook_popup_menu_new (window, tab);

	g_signal_connect (menu,
			  "selection-done",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (menu);
	gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *)event);
}

static void
on_notebook_changed (GeditMultiNotebook *mnb,
		     GParamSpec         *pspec,
		     GeditWindow        *window)
{
	update_actions_sensitivity (window);
}

static void
on_notebook_removed (GeditMultiNotebook *mnb,
		     GeditNotebook      *notebook,
		     GeditWindow        *window)
{
	update_actions_sensitivity (window);
}

static void
on_fullscreen_toggle_button_toggled (GtkToggleButton *fullscreen_toggle_button,
				     GeditWindow     *window)
{
	update_fullscreen_revealer_state (window);
}

static void
side_panel_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation,
			  GeditWindow   *window)
{
	window->priv->side_panel_size = allocation->width;
}

static void
bottom_panel_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation,
			    GeditWindow   *window)
{
	window->priv->bottom_panel_size = allocation->height;
}

static void
hpaned_restore_position (GtkWidget   *widget,
			 GeditWindow *window)
{
	gint pos;

	gedit_debug_message (DEBUG_WINDOW,
			     "Restoring hpaned position: side panel size %d",
			     window->priv->side_panel_size);

	pos = MAX (100, window->priv->side_panel_size);
	gtk_paned_set_position (GTK_PANED (window->priv->hpaned), pos);

	/* start monitoring the size */
	g_signal_connect (window->priv->side_panel,
			  "size-allocate",
			  G_CALLBACK (side_panel_size_allocate),
			  window);

	/* run this only once */
	g_signal_handlers_disconnect_by_func (widget, hpaned_restore_position, window);
}

static void
vpaned_restore_position (GtkWidget   *widget,
			 GeditWindow *window)
{
	gint pos;
	GtkAllocation allocation;

	gedit_debug_message (DEBUG_WINDOW,
			     "Restoring vpaned position: bottom panel size %d",
			     window->priv->bottom_panel_size);

	gtk_widget_get_allocation (widget, &allocation);
	pos = allocation.height -
	      MAX (50, window->priv->bottom_panel_size);
	gtk_paned_set_position (GTK_PANED (window->priv->vpaned), pos);

	/* start monitoring the size */
	g_signal_connect (window->priv->bottom_panel,
			  "size-allocate",
			  G_CALLBACK (bottom_panel_size_allocate),
			  window);

	/* run this only once */
	g_signal_handlers_disconnect_by_func (widget, vpaned_restore_position, window);
}

static void
side_panel_visibility_changed (GtkWidget   *panel,
                               GParamSpec  *pspec,
                               GeditWindow *window)
{
	gboolean visible;
	GAction *action;
	gchar *layout_desc;

	visible = gtk_widget_get_visible (panel);

	g_settings_set_boolean (window->priv->ui_settings,
				GEDIT_SETTINGS_SIDE_PANEL_VISIBLE,
				visible);

	/* sync the action state if the panel visibility was changed programmatically */
	action = g_action_map_lookup_action (G_ACTION_MAP (window), "side-panel");
	g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (visible));

	/* focus the right widget and set the right styles */
	if (visible)
	{
		gtk_widget_grab_focus (window->priv->side_panel);
	}
	else
	{
		gtk_widget_grab_focus (GTK_WIDGET (window->priv->multi_notebook));
	}

	g_object_get (gtk_settings_get_default (),
		      "gtk-decoration-layout", &layout_desc,
		      NULL);
	if (visible)
	{
		gchar **tokens;

		tokens = g_strsplit (layout_desc, ":", 2);
		if (tokens)
		{
			gchar *layout_headerbar;

			layout_headerbar = g_strdup_printf ("%c%s", ':', tokens[1]);
			gtk_header_bar_set_decoration_layout (GTK_HEADER_BAR (window->priv->headerbar), layout_headerbar);
			gtk_header_bar_set_decoration_layout (GTK_HEADER_BAR (window->priv->side_headerbar), tokens[0]);

			g_free (layout_headerbar);
			g_strfreev (tokens);
		}
	}
	else
	{
		gtk_header_bar_set_decoration_layout (GTK_HEADER_BAR (window->priv->headerbar), layout_desc);
		gtk_header_bar_set_decoration_layout (GTK_HEADER_BAR (window->priv->side_headerbar), NULL);
	}

	g_free (layout_desc);
}

static void
on_side_panel_stack_children_number_changed (GtkStack    *stack,
                                             GtkWidget   *widget,
                                             GeditWindow *window)
{
	GeditWindowPrivate *priv = window->priv;
	GList *children;

	children = gtk_container_get_children (GTK_CONTAINER (priv->side_panel));

	if (children != NULL && children->next != NULL)
	{
		gtk_widget_show (priv->side_stack_switcher);

#ifndef OS_OSX
		gtk_header_bar_set_custom_title (GTK_HEADER_BAR (priv->side_headerbar), priv->side_stack_switcher);
#endif
	}
	else
	{
		/* side_stack_switcher can get NULL in dispose, before stack children
		   are being removed */
		if (priv->side_stack_switcher != NULL)
		{
			gtk_widget_hide (priv->side_stack_switcher);
		}

#ifndef OS_OSX
		gtk_header_bar_set_custom_title (GTK_HEADER_BAR (priv->side_headerbar), NULL);
#endif
	}

	g_list_free (children);
}

static void
setup_side_panel (GeditWindow *window)
{
	GeditWindowPrivate *priv = window->priv;
	GtkWidget *documents_panel;

	gedit_debug (DEBUG_WINDOW);

	g_signal_connect_after (priv->side_panel,
	                        "notify::visible",
	                        G_CALLBACK (side_panel_visibility_changed),
	                        window);

#ifdef OS_OSX
	priv->side_stack_switcher = priv->side_panel_inline_stack_switcher;
#else
	priv->side_stack_switcher = gedit_menu_stack_switcher_new ();
#endif

	gtk_button_set_relief (GTK_BUTTON (priv->side_stack_switcher), GTK_RELIEF_NONE);
	g_object_ref_sink (priv->side_stack_switcher);

	gedit_utils_set_atk_name_description (priv->side_stack_switcher, _("Change side panel page"),  NULL);

	gedit_menu_stack_switcher_set_stack (GEDIT_MENU_STACK_SWITCHER (priv->side_stack_switcher),
	                                     GTK_STACK (priv->side_panel));

	g_signal_connect (priv->side_panel,
	                  "add",
	                  G_CALLBACK (on_side_panel_stack_children_number_changed),
	                  window);

	g_signal_connect (priv->side_panel,
	                  "remove",
	                  G_CALLBACK (on_side_panel_stack_children_number_changed),
	                  window);

	documents_panel = gedit_documents_panel_new (window);
	gtk_widget_show_all (documents_panel);
	gtk_stack_add_titled (GTK_STACK (priv->side_panel),
	                      documents_panel,
	                      "GeditWindowDocumentsPanel",
	                      _("Documents"));
}

static void
bottom_panel_visibility_changed (GtkWidget   *panel_box,
                                 GParamSpec  *pspec,
                                 GeditWindow *window)
{
	gboolean visible;
	GAction *action;

	visible = gtk_widget_get_visible (panel_box);

	g_settings_set_boolean (window->priv->ui_settings,
				GEDIT_SETTINGS_BOTTOM_PANEL_VISIBLE,
				visible);

	/* sync the action state if the panel visibility was changed programmatically */
	action = g_action_map_lookup_action (G_ACTION_MAP (window), "bottom-panel");
	g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (visible));

	/* focus the right widget */
	if (visible)
	{
		gtk_widget_grab_focus (window->priv->side_panel);
	}
	else
	{
		gtk_widget_grab_focus (GTK_WIDGET (window->priv->multi_notebook));
	}
}

static void
bottom_panel_item_removed (GtkStack    *panel,
			   GtkWidget   *item,
			   GeditWindow *window)
{
	gtk_widget_set_visible (window->priv->bottom_panel,
				gtk_stack_get_visible_child (panel) != NULL);

	update_actions_sensitivity (window);
}

static void
bottom_panel_item_added (GtkStack    *panel,
			 GtkWidget   *item,
			 GeditWindow *window)
{
	GList *children;
	int n_children;

	children = gtk_container_get_children (GTK_CONTAINER (panel));
	n_children = g_list_length (children);
	g_list_free (children);

	/* First item added. */
	if (n_children == 1)
	{
		gboolean show;

		show = g_settings_get_boolean (window->priv->ui_settings,
		                               "bottom-panel-visible");
		if (show)
		{
			gtk_widget_show (window->priv->bottom_panel);
		}

		update_actions_sensitivity (window);
	}
}

static void
setup_bottom_panel (GeditWindow *window)
{
	gedit_debug (DEBUG_WINDOW);

	g_signal_connect_after (window->priv->bottom_panel,
	                        "notify::visible",
	                        G_CALLBACK (bottom_panel_visibility_changed),
	                        window);
}

static void
init_panels_visibility (GeditWindow *window)
{
	gchar *panel_page;
	GtkWidget *panel_child;
	gboolean side_panel_visible;
	gboolean bottom_panel_visible;

	gedit_debug (DEBUG_WINDOW);

	/* side panel */
	panel_page = g_settings_get_string (window->priv->window_settings,
	                                    GEDIT_SETTINGS_SIDE_PANEL_ACTIVE_PAGE);
	panel_child = gtk_stack_get_child_by_name (GTK_STACK (window->priv->side_panel),
	                                           panel_page);
	if (panel_child != NULL)
	{
		gtk_stack_set_visible_child (GTK_STACK (window->priv->side_panel),
		                             panel_child);
	}

	g_free (panel_page);

	side_panel_visible = g_settings_get_boolean (window->priv->ui_settings,
						    GEDIT_SETTINGS_SIDE_PANEL_VISIBLE);
	bottom_panel_visible = g_settings_get_boolean (window->priv->ui_settings,
						      GEDIT_SETTINGS_BOTTOM_PANEL_VISIBLE);

	if (side_panel_visible)
	{
		gtk_widget_show (window->priv->side_panel);
	}

	/* bottom pane, it can be empty */
	if (gtk_stack_get_visible_child (GTK_STACK (window->priv->bottom_panel)) != NULL)
	{
		panel_page = g_settings_get_string (window->priv->window_settings,
		                                    GEDIT_SETTINGS_BOTTOM_PANEL_ACTIVE_PAGE);
		panel_child = gtk_stack_get_child_by_name (GTK_STACK (window->priv->side_panel),
		                                           panel_page);
		if (panel_child)
		{
			gtk_stack_set_visible_child (GTK_STACK (window->priv->bottom_panel),
			                             panel_child);
		}

		if (bottom_panel_visible)
		{
			gtk_widget_show (window->priv->bottom_panel);
		}

		g_free (panel_page);
	}

	/* start track sensitivity after the initial state is set */
	window->priv->bottom_panel_item_removed_handler_id =
		g_signal_connect (window->priv->bottom_panel,
				  "remove",
				  G_CALLBACK (bottom_panel_item_removed),
				  window);

	g_signal_connect_after (window->priv->bottom_panel,
	                        "add",
	                        G_CALLBACK (bottom_panel_item_added),
	                        window);
}

static void
clipboard_owner_change (GtkClipboard        *clipboard,
			GdkEventOwnerChange *event,
			GeditWindow         *window)
{
	set_paste_sensitivity_according_to_clipboard (window,
						      clipboard);
}

static void
window_realized (GtkWidget *window,
		 gpointer  *data)
{
	GtkClipboard *clipboard;

	clipboard = gtk_widget_get_clipboard (window,
					      GDK_SELECTION_CLIPBOARD);

	g_signal_connect (clipboard,
			  "owner_change",
			  G_CALLBACK (clipboard_owner_change),
			  window);
}

static void
window_unrealized (GtkWidget *window,
		   gpointer  *data)
{
	GtkClipboard *clipboard;

	clipboard = gtk_widget_get_clipboard (window,
					      GDK_SELECTION_CLIPBOARD);

	g_signal_handlers_disconnect_by_func (clipboard,
					      G_CALLBACK (clipboard_owner_change),
					      window);
}

static void
extension_added (PeasExtensionSet *extensions,
		 PeasPluginInfo   *info,
		 PeasExtension    *exten,
		 GeditWindow      *window)
{
	gedit_window_activatable_activate (GEDIT_WINDOW_ACTIVATABLE (exten));
}

static void
extension_removed (PeasExtensionSet *extensions,
		   PeasPluginInfo   *info,
		   PeasExtension    *exten,
		   GeditWindow      *window)
{
	gedit_window_activatable_deactivate (GEDIT_WINDOW_ACTIVATABLE (exten));
}

static GActionEntry win_entries[] = {
	{ "new-tab", _gedit_cmd_file_new },
	{ "open", _gedit_cmd_file_open },
	{ "revert", _gedit_cmd_file_revert },
	{ "reopen-closed-tab", _gedit_cmd_file_reopen_closed_tab },
	{ "save", _gedit_cmd_file_save },
	{ "save-as", _gedit_cmd_file_save_as },
	{ "save-all", _gedit_cmd_file_save_all },
	{ "close", _gedit_cmd_file_close },
	{ "close-all", _gedit_cmd_file_close_all },
	{ "print", _gedit_cmd_file_print },
	{ "focus-active-view", NULL, NULL, "false", _gedit_cmd_view_focus_active },
	{ "side-panel", NULL, NULL, "false", _gedit_cmd_view_toggle_side_panel },
	{ "bottom-panel", NULL, NULL, "false", _gedit_cmd_view_toggle_bottom_panel },
	{ "fullscreen", NULL, NULL, "false", _gedit_cmd_view_toggle_fullscreen_mode },
	{ "leave-fullscreen", _gedit_cmd_view_leave_fullscreen_mode },
	{ "find", _gedit_cmd_search_find },
	{ "find-next", _gedit_cmd_search_find_next },
	{ "find-prev", _gedit_cmd_search_find_prev },
	{ "replace", _gedit_cmd_search_replace },
	{ "clear-highlight", _gedit_cmd_search_clear_highlight },
	{ "goto-line", _gedit_cmd_search_goto_line },
	{ "new-tab-group", _gedit_cmd_documents_new_tab_group },
	{ "previous-tab-group", _gedit_cmd_documents_previous_tab_group },
	{ "next-tab-group", _gedit_cmd_documents_next_tab_group },
	{ "previous-document", _gedit_cmd_documents_previous_document },
	{ "next-document", _gedit_cmd_documents_next_document },
	{ "move-to-new-window", _gedit_cmd_documents_move_to_new_window },
	{ "undo", _gedit_cmd_edit_undo },
	{ "redo", _gedit_cmd_edit_redo },
	{ "cut", _gedit_cmd_edit_cut },
	{ "copy", _gedit_cmd_edit_copy },
	{ "paste", _gedit_cmd_edit_paste },
	{ "delete", _gedit_cmd_edit_delete },
	{ "select-all", _gedit_cmd_edit_select_all },
	{ "highlight-mode", _gedit_cmd_view_highlight_mode },
	{ "overwrite-mode", NULL, NULL, "false", _gedit_cmd_edit_overwrite_mode }
};

static void
sync_fullscreen_actions (GeditWindow *window,
			 gboolean     fullscreen)
{
	GtkMenuButton *button;
	GPropertyAction *action;

	button = fullscreen ? window->priv->fullscreen_gear_button : window->priv->gear_button;
	g_action_map_remove_action (G_ACTION_MAP (window), "hamburger-menu");
	action = g_property_action_new ("hamburger-menu", button, "active");
	g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));
	g_object_unref (action);
}

static void
init_amtk_application_window (GeditWindow *gedit_window)
{
	AmtkApplicationWindow *amtk_window;

	amtk_window = amtk_application_window_get_from_gtk_application_window (GTK_APPLICATION_WINDOW (gedit_window));
	amtk_application_window_set_statusbar (amtk_window, GTK_STATUSBAR (gedit_window->priv->statusbar));
}

static void
open_recent_menu_item_activated_cb (GtkRecentChooser *recent_chooser,
				    gpointer          user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);
	gchar *uri;
	GFile *location;

	uri = gtk_recent_chooser_get_current_uri (recent_chooser);
	location = g_file_new_for_uri (uri);

	gedit_commands_load_location (window, location, NULL, 0, 0);

	g_free (uri);
	g_object_unref (location);
}

static GtkWidget *
create_open_buttons (GeditWindow    *window,
		     GtkMenuButton **open_recent_button)
{
	GtkWidget *hbox;
	GtkStyleContext *style_context;
	GtkWidget *open_dialog_button;
	GtkWidget *my_open_recent_button;
	AmtkApplicationWindow *amtk_window;
	GtkRecentChooserMenu *recent_menu;

	/* It currently needs to be a GtkBox, not a GtkGrid, because GtkGrid and
	 * GTK_STYLE_CLASS_LINKED doesn't work as expected in a RTL locale.
	 * Probably a GtkGrid bug.
	 */
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	style_context = gtk_widget_get_style_context (hbox);
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_LINKED);

	open_dialog_button = gtk_button_new_with_mnemonic (_("_Open"));
	gtk_widget_set_tooltip_text (open_dialog_button, _("Open a file"));
	gtk_actionable_set_action_name (GTK_ACTIONABLE (open_dialog_button), "win.open");

	my_open_recent_button = gtk_menu_button_new ();
	gtk_widget_set_tooltip_text (my_open_recent_button, _("Open a recently used file"));

	recent_menu = amtk_application_window_create_open_recent_menu_base ();

	amtk_window = amtk_application_window_get_from_gtk_application_window (GTK_APPLICATION_WINDOW (window));
	amtk_application_window_connect_recent_chooser_menu_to_statusbar (amtk_window, recent_menu);

	g_signal_connect_object (recent_menu,
				 "item-activated",
				 G_CALLBACK (open_recent_menu_item_activated_cb),
				 window,
				 0);

	gtk_menu_button_set_popup (GTK_MENU_BUTTON (my_open_recent_button),
				   GTK_WIDGET (recent_menu));

	gtk_container_add (GTK_CONTAINER (hbox), open_dialog_button);
	gtk_container_add (GTK_CONTAINER (hbox), my_open_recent_button);
	gtk_widget_show_all (hbox);

	if (open_recent_button != NULL)
	{
		*open_recent_button = GTK_MENU_BUTTON (my_open_recent_button);
	}

	return hbox;
}

static void
init_open_buttons (GeditWindow *window)
{
	gtk_container_add_with_properties (GTK_CONTAINER (window->priv->headerbar),
					   create_open_buttons (window, NULL),
					   "position", 0, /* The first on the left. */
					   NULL);

	gtk_container_add_with_properties (GTK_CONTAINER (window->priv->fullscreen_headerbar),
					   create_open_buttons (window, &(window->priv->fullscreen_open_recent_button)),
					   "position", 0, /* The first on the left. */
					   NULL);

	g_signal_connect (GTK_TOGGLE_BUTTON (window->priv->fullscreen_open_recent_button),
	                  "toggled",
	                  G_CALLBACK (on_fullscreen_toggle_button_toggled),
	                  window);
}

static void
gedit_window_init (GeditWindow *window)
{
	GtkTargetList *tl;
	GMenuModel *hamburger_menu;

	gedit_debug (DEBUG_WINDOW);

	window->priv = gedit_window_get_instance_private (window);

	window->priv->removing_tabs = FALSE;
	window->priv->state = GEDIT_WINDOW_STATE_NORMAL;
	window->priv->inhibition_cookie = 0;
	window->priv->dispose_has_run = FALSE;
	window->priv->direct_save_uri = NULL;
	window->priv->closed_docs_stack = NULL;
	window->priv->editor_settings = g_settings_new ("org.gnome.gedit.preferences.editor");
	window->priv->ui_settings = g_settings_new ("org.gnome.gedit.preferences.ui");

	/* window settings are applied only once the window is closed. We do not
	   want to keep writing to disk when the window is dragged around */
	window->priv->window_settings = g_settings_new ("org.gnome.gedit.state.window");
	g_settings_delay (window->priv->window_settings);

	window->priv->message_bus = gedit_message_bus_new ();

	gtk_widget_init_template (GTK_WIDGET (window));
	init_amtk_application_window (window);
	init_open_buttons (window);

	g_action_map_add_action_entries (G_ACTION_MAP (window),
	                                 win_entries,
	                                 G_N_ELEMENTS (win_entries),
	                                 window);

	window->priv->window_group = gtk_window_group_new ();
	gtk_window_group_add_window (window->priv->window_group, GTK_WINDOW (window));

	setup_fullscreen_eventbox (window);
	sync_fullscreen_actions (window, FALSE);

	hamburger_menu = _gedit_app_get_hamburger_menu (GEDIT_APP (g_application_get_default ()));
	if (hamburger_menu)
	{
		gtk_menu_button_set_menu_model (window->priv->gear_button, hamburger_menu);
		gtk_menu_button_set_menu_model (window->priv->fullscreen_gear_button, hamburger_menu);
	}
	else
	{
		gtk_widget_hide (GTK_WIDGET (window->priv->gear_button));
		gtk_widget_hide (GTK_WIDGET (window->priv->fullscreen_gear_button));
		gtk_widget_set_no_show_all (GTK_WIDGET (window->priv->gear_button), TRUE);
		gtk_widget_set_no_show_all (GTK_WIDGET (window->priv->fullscreen_gear_button), TRUE);
	}

	g_signal_connect (GTK_TOGGLE_BUTTON (window->priv->fullscreen_gear_button),
	                  "toggled",
	                  G_CALLBACK (on_fullscreen_toggle_button_toggled),
	                  window);

	/* Setup status bar */
	setup_statusbar (window);

	/* Setup main area */
	g_signal_connect (window->priv->multi_notebook,
			  "notebook-removed",
			  G_CALLBACK (on_notebook_removed),
			  window);
	g_signal_connect (window->priv->multi_notebook,
			  "notify::active-notebook",
			  G_CALLBACK (on_notebook_changed),
			  window);

	g_signal_connect (window->priv->multi_notebook,
			  "tab-added",
			  G_CALLBACK (on_tab_added),
			  window);

	g_signal_connect (window->priv->multi_notebook,
			  "tab-removed",
			  G_CALLBACK (on_tab_removed),
			  window);

	g_signal_connect (window->priv->multi_notebook,
			  "switch-tab",
			  G_CALLBACK (tab_switched),
			  window);

	g_signal_connect (window->priv->multi_notebook,
			  "tab-close-request",
			  G_CALLBACK (on_tab_close_request),
			  window);

	g_signal_connect (window->priv->multi_notebook,
			  "page-reordered",
			  G_CALLBACK (on_page_reordered),
			  window);

	g_signal_connect (window->priv->multi_notebook,
	                  "create-window",
	                  G_CALLBACK (on_notebook_create_window),
	                  window);

	g_signal_connect (window->priv->multi_notebook,
			  "show-popup-menu",
			  G_CALLBACK (on_show_popup_menu),
			  window);

	/* side and bottom panels */
	setup_side_panel (window);
	setup_bottom_panel (window);

	/* panels' state must be restored after panels have been mapped,
	 * since the bottom panel position depends on the size of the vpaned. */
	window->priv->side_panel_size = g_settings_get_int (window->priv->window_settings,
							    GEDIT_SETTINGS_SIDE_PANEL_SIZE);
	window->priv->bottom_panel_size = g_settings_get_int (window->priv->window_settings,
							      GEDIT_SETTINGS_BOTTOM_PANEL_SIZE);

	g_signal_connect_after (window->priv->hpaned,
				"map",
				G_CALLBACK (hpaned_restore_position),
				window);
	g_signal_connect_after (window->priv->vpaned,
				"map",
				G_CALLBACK (vpaned_restore_position),
				window);

	/* Drag and drop support */
	gtk_drag_dest_set (GTK_WIDGET (window),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types,
			   G_N_ELEMENTS (drop_types),
			   GDK_ACTION_COPY);

	/* Add uri targets */
	tl = gtk_drag_dest_get_target_list (GTK_WIDGET (window));

	if (tl == NULL)
	{
		tl = gtk_target_list_new (drop_types, G_N_ELEMENTS (drop_types));
		gtk_drag_dest_set_target_list (GTK_WIDGET (window), tl);
		gtk_target_list_unref (tl);
	}

	gtk_target_list_add_uri_targets (tl, TARGET_URI_LIST);

	/* connect instead of override, so that we can
	 * share the cb code with the view */
	g_signal_connect (window,
			  "drag_data_received",
	                  G_CALLBACK (drag_data_received_cb),
	                  NULL);
	g_signal_connect (window,
			  "drag_drop",
	                  G_CALLBACK (drag_drop_cb),
	                  NULL);

	/* we can get the clipboard only after the widget
	 * is realized */
	g_signal_connect (window,
			  "realize",
			  G_CALLBACK (window_realized),
			  NULL);
	g_signal_connect (window,
			  "unrealize",
			  G_CALLBACK (window_unrealized),
			  NULL);

	gedit_debug_message (DEBUG_WINDOW, "Update plugins ui");

	window->priv->extensions = peas_extension_set_new (PEAS_ENGINE (gedit_plugins_engine_get_default ()),
							   GEDIT_TYPE_WINDOW_ACTIVATABLE,
							   "window", window,
							   NULL);
	g_signal_connect (window->priv->extensions,
			  "extension-added",
			  G_CALLBACK (extension_added),
			  window);
	g_signal_connect (window->priv->extensions,
			  "extension-removed",
			  G_CALLBACK (extension_removed),
			  window);
	peas_extension_set_foreach (window->priv->extensions,
	                            (PeasExtensionSetForeachFunc) extension_added,
	                            window);

	/* set visibility of panels.
	 * This needs to be done after plugins activatation */
	init_panels_visibility (window);

	update_actions_sensitivity (window);

	gedit_debug_message (DEBUG_WINDOW, "END");
}

/**
 * gedit_window_get_active_view:
 * @window: a #GeditWindow
 *
 * Gets the active #GeditView.
 *
 * Returns: (transfer none): the active #GeditView
 */
GeditView *
gedit_window_get_active_view (GeditWindow *window)
{
	GeditTab *tab;
	GeditView *view;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	tab = gedit_window_get_active_tab (window);

	if (tab == NULL)
		return NULL;

	view = gedit_tab_get_view (tab);

	return view;
}

/**
 * gedit_window_get_active_document:
 * @window: a #GeditWindow
 *
 * Gets the active #GeditDocument.
 *
 * Returns: (transfer none): the active #GeditDocument
 */
GeditDocument *
gedit_window_get_active_document (GeditWindow *window)
{
	GeditView *view;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	view = gedit_window_get_active_view (window);
	if (view == NULL)
		return NULL;

	return GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
}

GtkWidget *
_gedit_window_get_multi_notebook (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return GTK_WIDGET (window->priv->multi_notebook);
}

GtkWidget *
_gedit_window_get_notebook (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return GTK_WIDGET (gedit_multi_notebook_get_active_notebook (window->priv->multi_notebook));
}

static GeditTab *
process_create_tab (GeditWindow *window,
                    GtkWidget   *notebook,
                    GeditTab    *tab,
                    gboolean     jump_to)
{
	if (tab == NULL)
	{
		return NULL;
	}

	gedit_debug (DEBUG_WINDOW);

	gtk_widget_show (GTK_WIDGET (tab));
	gedit_notebook_add_tab (GEDIT_NOTEBOOK (notebook),
	                        tab,
	                        -1,
	                        jump_to);

	if (!gtk_widget_get_visible (GTK_WIDGET (window)))
	{
		gtk_window_present (GTK_WINDOW (window));
	}

	return tab;
}

/**
 * gedit_window_create_tab:
 * @window: a #GeditWindow
 * @jump_to: %TRUE to set the new #GeditTab as active
 *
 * Creates a new #GeditTab and adds the new tab to the #GtkNotebook.
 * In case @jump_to is %TRUE the #GtkNotebook switches to that new #GeditTab.
 *
 * Returns: (transfer none): a new #GeditTab
 */
GeditTab *
gedit_window_create_tab (GeditWindow *window,
			 gboolean     jump_to)
{
	GtkWidget *notebook;
	GeditTab *tab;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	gedit_debug (DEBUG_WINDOW);

	notebook = _gedit_window_get_notebook (window);
	tab = _gedit_tab_new ();
	gtk_widget_show (GTK_WIDGET (tab));

	return process_create_tab (window, notebook, tab, jump_to);
}

/**
 * gedit_window_create_tab_from_location:
 * @window: a #GeditWindow
 * @location: the location of the document
 * @encoding: (allow-none): a #GtkSourceEncoding, or %NULL
 * @line_pos: the line position to visualize
 * @column_pos: the column position to visualize
 * @create: %TRUE to create a new document in case @uri does exist
 * @jump_to: %TRUE to set the new #GeditTab as active
 *
 * Creates a new #GeditTab loading the document specified by @uri.
 * In case @jump_to is %TRUE the #GtkNotebook swithes to that new #GeditTab.
 * Whether @create is %TRUE, creates a new empty document if location does
 * not refer to an existing file
 *
 * Returns: (transfer none): a new #GeditTab
 */
GeditTab *
gedit_window_create_tab_from_location (GeditWindow             *window,
				       GFile                   *location,
				       const GtkSourceEncoding *encoding,
				       gint                     line_pos,
				       gint                     column_pos,
				       gboolean                 create,
				       gboolean                 jump_to)
{
	GtkWidget *notebook;
	GeditTab *tab;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	g_return_val_if_fail (G_IS_FILE (location), NULL);

	gedit_debug (DEBUG_WINDOW);

	tab = _gedit_tab_new ();

	_gedit_tab_load (tab,
			 location,
			 encoding,
			 line_pos,
			 column_pos,
			 create);

	notebook = _gedit_window_get_notebook (window);

	return process_create_tab (window, notebook, tab, jump_to);
}

/**
 * gedit_window_create_tab_from_stream:
 * @window: a #GeditWindow
 * @stream: a #GInputStream
 * @encoding: (allow-none): a #GtkSourceEncoding, or %NULL
 * @line_pos: the line position to visualize
 * @column_pos: the column position to visualize
 * @jump_to: %TRUE to set the new #GeditTab as active
 *
 * Returns: (transfer none): a new #GeditTab
 */
GeditTab *
gedit_window_create_tab_from_stream (GeditWindow             *window,
				     GInputStream            *stream,
				     const GtkSourceEncoding *encoding,
				     gint                     line_pos,
				     gint                     column_pos,
				     gboolean                 jump_to)
{
	GtkWidget *notebook;
	GeditTab *tab;

	gedit_debug (DEBUG_WINDOW);

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	g_return_val_if_fail (G_IS_INPUT_STREAM (stream), NULL);

	tab = _gedit_tab_new ();

	_gedit_tab_load_stream (tab,
				stream,
				encoding,
				line_pos,
				column_pos);

	notebook = _gedit_window_get_notebook (window);

	return process_create_tab (window, notebook, tab, jump_to);
}

/**
 * gedit_window_get_active_tab:
 * @window: a GeditWindow
 *
 * Gets the active #GeditTab in the @window.
 *
 * Returns: (transfer none): the active #GeditTab in the @window.
 */
GeditTab *
gedit_window_get_active_tab (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return (window->priv->multi_notebook == NULL) ? NULL :
			gedit_multi_notebook_get_active_tab (window->priv->multi_notebook);
}

static void
add_document (GeditTab  *tab,
	      GList    **res)
{
	GeditDocument *doc;

	doc = gedit_tab_get_document (tab);

	*res = g_list_prepend (*res, doc);
}

/**
 * gedit_window_get_documents:
 * @window: a #GeditWindow
 *
 * Gets a newly allocated list with all the documents in the window.
 * This list must be freed.
 *
 * Returns: (element-type Gedit.Document) (transfer container): a newly
 * allocated list with all the documents in the window
 */
GList *
gedit_window_get_documents (GeditWindow *window)
{
	GList *res = NULL;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	gedit_multi_notebook_foreach_tab (window->priv->multi_notebook,
					  (GtkCallback)add_document,
					  &res);

	res = g_list_reverse (res);

	return res;
}

static void
add_view (GeditTab  *tab,
	  GList    **res)
{
	GeditView *view;

	view = gedit_tab_get_view (tab);

	*res = g_list_prepend (*res, view);
}

/**
 * gedit_window_get_views:
 * @window: a #GeditWindow
 *
 * Gets a list with all the views in the window. This list must be freed.
 *
 * Returns: (element-type Gedit.View) (transfer container): a newly allocated
 * list with all the views in the window
 */
GList *
gedit_window_get_views (GeditWindow *window)
{
	GList *res = NULL;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	gedit_multi_notebook_foreach_tab (window->priv->multi_notebook,
					  (GtkCallback)add_view,
					  &res);

	res = g_list_reverse (res);

	return res;
}

/**
 * gedit_window_close_tab:
 * @window: a #GeditWindow
 * @tab: the #GeditTab to close
 *
 * Closes the @tab.
 */
void
gedit_window_close_tab (GeditWindow *window,
			GeditTab    *tab)
{
	GList *tabs = NULL;

	g_return_if_fail (GEDIT_IS_WINDOW (window));
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail ((gedit_tab_get_state (tab) != GEDIT_TAB_STATE_SAVING) &&
			  (gedit_tab_get_state (tab) != GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW));

	tabs = g_list_append (tabs, tab);
	gedit_multi_notebook_close_tabs (window->priv->multi_notebook, tabs);
	g_list_free (tabs);
}

/**
 * gedit_window_close_all_tabs:
 * @window: a #GeditWindow
 *
 * Closes all opened tabs.
 */
void
gedit_window_close_all_tabs (GeditWindow *window)
{
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	g_return_if_fail (!(window->priv->state & GEDIT_WINDOW_STATE_SAVING));

	window->priv->removing_tabs = TRUE;

	gedit_multi_notebook_close_all_tabs (window->priv->multi_notebook);

	window->priv->removing_tabs = FALSE;
}

/**
 * gedit_window_close_tabs:
 * @window: a #GeditWindow
 * @tabs: (element-type Gedit.Tab): a list of #GeditTab
 *
 * Closes all tabs specified by @tabs.
 */
void
gedit_window_close_tabs (GeditWindow *window,
			 const GList *tabs)
{
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	g_return_if_fail (!(window->priv->state & GEDIT_WINDOW_STATE_SAVING));

	window->priv->removing_tabs = TRUE;

	gedit_multi_notebook_close_tabs (window->priv->multi_notebook, tabs);

	window->priv->removing_tabs = FALSE;
}

GeditWindow *
_gedit_window_move_tab_to_new_window (GeditWindow *window,
				      GeditTab    *tab)
{
	GeditWindow *new_window;
	GeditNotebook *old_notebook;
	GeditNotebook *new_notebook;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);
	g_return_val_if_fail (gedit_multi_notebook_get_n_notebooks (
	                        window->priv->multi_notebook) > 1 ||
	                      gedit_multi_notebook_get_n_tabs (
	                        window->priv->multi_notebook) > 1,
	                      NULL);

	new_window = clone_window (window);

	old_notebook = GEDIT_NOTEBOOK (gtk_widget_get_parent (GTK_WIDGET (tab)));
	new_notebook = gedit_multi_notebook_get_active_notebook (new_window->priv->multi_notebook);

	gedit_notebook_move_tab (old_notebook,
				 new_notebook,
				 tab,
				 -1);

	gtk_widget_show (GTK_WIDGET (new_window));

	return new_window;
}

void
_gedit_window_move_tab_to_new_tab_group (GeditWindow *window,
                                         GeditTab    *tab)
{
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	g_return_if_fail (GEDIT_IS_TAB (tab));

	gedit_multi_notebook_add_new_notebook_with_tab (window->priv->multi_notebook,
	                                                tab);
}

/**
 * gedit_window_set_active_tab:
 * @window: a #GeditWindow
 * @tab: a #GeditTab
 *
 * Switches to the tab that matches with @tab.
 */
void
gedit_window_set_active_tab (GeditWindow *window,
			     GeditTab    *tab)
{
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	gedit_multi_notebook_set_active_tab (window->priv->multi_notebook,
					     tab);
}

/**
 * gedit_window_get_group:
 * @window: a #GeditWindow
 *
 * Gets the #GtkWindowGroup in which @window resides.
 *
 * Returns: (transfer none): the #GtkWindowGroup
 */
GtkWindowGroup *
gedit_window_get_group (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return window->priv->window_group;
}

gboolean
_gedit_window_is_removing_tabs (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), FALSE);

	return window->priv->removing_tabs;
}

/**
 * gedit_window_get_side_panel:
 * @window: a #GeditWindow
 *
 * Gets the side panel of the @window.
 *
 * Returns: (transfer none): the side panel's #GtkStack.
 */
GtkWidget *
gedit_window_get_side_panel (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return window->priv->side_panel;
}

/**
 * gedit_window_get_bottom_panel:
 * @window: a #GeditWindow
 *
 * Gets the bottom panel of the @window.
 *
 * Returns: (transfer none): the bottom panel's #GtkStack.
 */
GtkWidget *
gedit_window_get_bottom_panel (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return window->priv->bottom_panel;
}

/**
 * gedit_window_get_statusbar:
 * @window: a #GeditWindow
 *
 * Gets the #GeditStatusbar of the @window.
 *
 * Returns: (transfer none): the #GeditStatusbar of the @window.
 */
GtkWidget *
gedit_window_get_statusbar (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), 0);

	return window->priv->statusbar;
}

/**
 * gedit_window_get_state:
 * @window: a #GeditWindow
 *
 * Retrieves the state of the @window.
 *
 * Returns: the current #GeditWindowState of the @window.
 */
GeditWindowState
gedit_window_get_state (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), GEDIT_WINDOW_STATE_NORMAL);

	return window->priv->state;
}

const gchar *
_gedit_window_get_file_chooser_folder_uri (GeditWindow          *window,
					   GtkFileChooserAction  action)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	g_return_val_if_fail ((action == GTK_FILE_CHOOSER_ACTION_OPEN) ||
			      (action == GTK_FILE_CHOOSER_ACTION_SAVE), NULL);

	if (action == GTK_FILE_CHOOSER_ACTION_OPEN)
	{
		GeditSettings *settings;
		GSettings *file_chooser_state_settings;

		settings = _gedit_settings_get_singleton ();
		file_chooser_state_settings = _gedit_settings_peek_file_chooser_state_settings (settings);

		if (g_settings_get_boolean (file_chooser_state_settings,
					    GEDIT_SETTINGS_FILE_CHOOSER_OPEN_RECENT))
		{
			return NULL;
		}
	}

	return window->priv->file_chooser_folder_uri;
}

void
_gedit_window_set_file_chooser_folder_uri (GeditWindow          *window,
					   GtkFileChooserAction  action,
					   const gchar          *folder_uri)
{
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	g_return_if_fail ((action == GTK_FILE_CHOOSER_ACTION_OPEN) ||
			  (action == GTK_FILE_CHOOSER_ACTION_SAVE));

	if (action == GTK_FILE_CHOOSER_ACTION_OPEN)
	{
		GeditSettings *settings;
		GSettings *file_chooser_state_settings;
		gboolean open_recent = folder_uri == NULL;

		settings = _gedit_settings_get_singleton ();
		file_chooser_state_settings = _gedit_settings_peek_file_chooser_state_settings (settings);

		g_settings_set_boolean (file_chooser_state_settings,
					GEDIT_SETTINGS_FILE_CHOOSER_OPEN_RECENT,
					open_recent);

		if (open_recent)
		{
			/* Do not set window->priv->file_chooser_folder_uri to
			 * NULL, to not lose the folder for the Save action.
			 */
			return;
		}
	}

	g_free (window->priv->file_chooser_folder_uri);
	window->priv->file_chooser_folder_uri = g_strdup (folder_uri);
}

static void
add_unsaved_doc (GeditTab *tab,
		 GList   **res)
{
	if (!_gedit_tab_get_can_close (tab))
	{
		GeditDocument *doc;

		doc = gedit_tab_get_document (tab);
		*res = g_list_prepend (*res, doc);
	}
}

/**
 * gedit_window_get_unsaved_documents:
 * @window: a #GeditWindow
 *
 * Gets the list of documents that need to be saved before closing the window.
 *
 * Returns: (element-type Gedit.Document) (transfer container): a list of
 * #GeditDocument that need to be saved before closing the window
 */
GList *
gedit_window_get_unsaved_documents (GeditWindow *window)
{
	GList *res = NULL;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	gedit_multi_notebook_foreach_tab (window->priv->multi_notebook,
					  (GtkCallback)add_unsaved_doc,
					  &res);

	return g_list_reverse (res);
}

GList *
_gedit_window_get_all_tabs (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return gedit_multi_notebook_get_all_tabs (window->priv->multi_notebook);
}

void
_gedit_window_fullscreen (GeditWindow *window)
{
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	if (_gedit_window_is_fullscreen (window))
		return;

	sync_fullscreen_actions (window, TRUE);

	/* Go to fullscreen mode and hide bars */
	gtk_window_fullscreen (GTK_WINDOW (&window->window));
}

void
_gedit_window_unfullscreen (GeditWindow *window)
{
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	if (!_gedit_window_is_fullscreen (window))
		return;

	sync_fullscreen_actions (window, FALSE);

	/* Unfullscreen and show bars */
	gtk_window_unfullscreen (GTK_WINDOW (&window->window));
}

gboolean
_gedit_window_is_fullscreen (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), FALSE);

	return window->priv->window_state & GDK_WINDOW_STATE_FULLSCREEN;
}

/**
 * gedit_window_get_tab_from_location:
 * @window: a #GeditWindow
 * @location: a #GFile
 *
 * Gets the #GeditTab that matches with the given @location.
 *
 * Returns: (transfer none): the #GeditTab that matches with the given @location.
 */
GeditTab *
gedit_window_get_tab_from_location (GeditWindow *window,
				    GFile       *location)
{
	GList *tabs;
	GList *l;
	GeditTab *ret = NULL;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	g_return_val_if_fail (G_IS_FILE (location), NULL);

	tabs = gedit_multi_notebook_get_all_tabs (window->priv->multi_notebook);

	for (l = tabs; l != NULL; l = g_list_next (l))
	{
		GeditDocument *doc;
		GtkSourceFile *file;
		GeditTab *tab;
		GFile *cur_location;

		tab = GEDIT_TAB (l->data);
		doc = gedit_tab_get_document (tab);
		file = gedit_document_get_file (doc);
		cur_location = gtk_source_file_get_location (file);

		if (cur_location != NULL)
		{
			gboolean found = g_file_equal (location, cur_location);

			if (found)
			{
				ret = tab;
				break;
			}
		}
	}

	g_list_free (tabs);

	return ret;
}

/**
 * gedit_window_get_message_bus:
 * @window: a #GeditWindow
 *
 * Gets the #GeditMessageBus associated with @window. The returned reference
 * is owned by the window and should not be unreffed.
 *
 * Return value: (transfer none): the #GeditMessageBus associated with @window
 */
GeditMessageBus *
gedit_window_get_message_bus (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return window->priv->message_bus;
}

/* ex:set ts=8 noet: */
