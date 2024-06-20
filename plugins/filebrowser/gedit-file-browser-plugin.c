/*
 * gedit-file-browser-plugin.c - Gedit plugin providing easy file access
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

#include <string.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <gedit/gedit-app.h>
#include <gedit/gedit-commands.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>
#include <gedit/gedit-utils.h>
#include <tepl/tepl.h>

#include "gedit-file-browser-enum-types.h"
#include "gedit-file-browser-plugin.h"
#include "gedit-file-browser-utils.h"
#include "gedit-file-browser-error.h"
#include "gedit-file-browser-widget.h"
#include "gedit-file-browser-messages.h"

#define FILEBROWSER_BASE_SETTINGS	"org.gnome.gedit.plugins.filebrowser"
#define FILEBROWSER_TREE_VIEW		"tree-view"
#define FILEBROWSER_ROOT		"root"
#define FILEBROWSER_VIRTUAL_ROOT	"virtual-root"
#define FILEBROWSER_ENABLE_REMOTE	"enable-remote"
#define FILEBROWSER_OPEN_AT_FIRST_DOC	"open-at-first-doc"
#define FILEBROWSER_FILTER_MODE		"filter-mode"
#define FILEBROWSER_FILTER_PATTERN	"filter-pattern"
#define FILEBROWSER_BINARY_PATTERNS	"binary-patterns"

#define NAUTILUS_BASE_SETTINGS		"org.gnome.nautilus.preferences"
#define NAUTILUS_FALLBACK_SETTINGS	"org.gnome.gedit.plugins.filebrowser.nautilus"
#define NAUTILUS_CLICK_POLICY_KEY	"click-policy"

#define TERMINAL_BASE_SETTINGS		"org.gnome.desktop.default-applications.terminal"
#define TERMINAL_EXEC_KEY		"exec"

struct _GeditFileBrowserPluginPrivate
{
	GSettings              *settings;
	GSettings              *nautilus_settings;
	GSettings              *terminal_settings;

	GeditWindow            *window;

	GeditFileBrowserWidget *tree_widget;
	gboolean	        auto_root;
	gulong                  end_loading_handle;

	guint			click_policy_handle;

	TeplPanelItem          *side_panel_item;
};

enum
{
	PROP_0,
	PROP_WINDOW
};

static void gedit_window_activatable_iface_init	(GeditWindowActivatableInterface *iface);

static void on_location_activated_cb     (GeditFileBrowserWidget        *widget,
                                          GFile                         *location,
                                          GeditWindow                   *window);
static void on_error_cb                  (GeditFileBrowserWidget        *widget,
                                          guint                          code,
                                          gchar const                   *message,
                                          GeditFileBrowserPlugin        *plugin);
static void on_model_set_cb              (GeditFileBrowserView          *widget,
                                          GParamSpec                    *param,
                                          GeditFileBrowserPlugin        *plugin);
static void on_virtual_root_changed_cb   (GeditFileBrowserStore         *model,
                                          GParamSpec                    *param,
                                          GeditFileBrowserPlugin        *plugin);
static void on_rename_cb		 (GeditFileBrowserStore         *model,
					  GFile                         *oldfile,
					  GFile                         *newfile,
					  GeditWindow                   *window);
static void on_tab_added_cb              (GeditWindow                   *window,
                                          GeditTab                      *tab,
                                          GeditFileBrowserPlugin        *plugin);
static gboolean on_confirm_delete_cb     (GeditFileBrowserWidget        *widget,
                                          GeditFileBrowserStore         *store,
                                          GList                         *rows,
                                          GeditFileBrowserPlugin        *plugin);
static gboolean on_confirm_no_trash_cb   (GeditFileBrowserWidget        *widget,
                                          GList                         *files,
                                          GeditWindow                   *window);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditFileBrowserPlugin,
				gedit_file_browser_plugin,
				G_TYPE_OBJECT,
				0,
				G_ADD_PRIVATE_DYNAMIC (GeditFileBrowserPlugin)
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init)	\
													\
				gedit_file_browser_enum_and_flag_register_type	(type_module);		\
				_gedit_file_bookmarks_store_register_type	(type_module);		\
				_gedit_file_browser_store_register_type		(type_module);		\
				_gedit_file_browser_view_register_type		(type_module);		\
				_gedit_file_browser_widget_register_type	(type_module);		\
)

static GSettings *
create_nautilus_gsettings (void)
{
	GSettings *nautilus_settings = NULL;

	if (tepl_utils_can_use_gsettings_schema (NAUTILUS_BASE_SETTINGS))
	{
		nautilus_settings = g_settings_new (NAUTILUS_BASE_SETTINGS);

		if (tepl_utils_can_use_gsettings_key (nautilus_settings, NAUTILUS_CLICK_POLICY_KEY))
		{
			return nautilus_settings;
		}
	}

	g_clear_object (&nautilus_settings);
	return g_settings_new (NAUTILUS_FALLBACK_SETTINGS);
}

static void
gedit_file_browser_plugin_init (GeditFileBrowserPlugin *plugin)
{
	plugin->priv = gedit_file_browser_plugin_get_instance_private (plugin);

	plugin->priv->settings = g_settings_new (FILEBROWSER_BASE_SETTINGS);
	plugin->priv->terminal_settings = g_settings_new (TERMINAL_BASE_SETTINGS);
	plugin->priv->nautilus_settings = create_nautilus_gsettings ();
}

static void
gedit_file_browser_plugin_dispose (GObject *object)
{
	GeditFileBrowserPlugin *plugin = GEDIT_FILE_BROWSER_PLUGIN (object);

	g_clear_object (&plugin->priv->settings);
	g_clear_object (&plugin->priv->nautilus_settings);
	g_clear_object (&plugin->priv->terminal_settings);
	g_clear_object (&plugin->priv->window);

	G_OBJECT_CLASS (gedit_file_browser_plugin_parent_class)->dispose (object);
}

static void
gedit_file_browser_plugin_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
	GeditFileBrowserPlugin *plugin = GEDIT_FILE_BROWSER_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			plugin->priv->window = GEDIT_WINDOW (g_value_dup_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_file_browser_plugin_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
	GeditFileBrowserPlugin *plugin = GEDIT_FILE_BROWSER_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_value_set_object (value, plugin->priv->window);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
on_end_loading_cb (GeditFileBrowserStore  *store,
                   GtkTreeIter            *iter,
                   GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;

	/* Disconnect the signal */
	g_signal_handler_disconnect (store, priv->end_loading_handle);
	priv->end_loading_handle = 0;
	priv->auto_root = FALSE;
}

static void
prepare_auto_root (GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GeditFileBrowserStore *store;

	priv->auto_root = TRUE;

	store = gedit_file_browser_widget_get_browser_store (priv->tree_widget);

	if (priv->end_loading_handle != 0)
	{
		g_signal_handler_disconnect (store, priv->end_loading_handle);
		priv->end_loading_handle = 0;
	}

	priv->end_loading_handle = g_signal_connect (store,
	                                             "end-loading",
	                                             G_CALLBACK (on_end_loading_cb),
	                                             plugin);
}

static void
restore_default_location (GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gchar *root;
	gchar *virtual_root;
	gboolean bookmarks;
	gboolean remote;

	bookmarks = !g_settings_get_boolean (priv->settings,
					     FILEBROWSER_TREE_VIEW);

	if (bookmarks)
	{
		gedit_file_browser_widget_show_bookmarks (priv->tree_widget);
		return;
	}

	root = g_settings_get_string (priv->settings,
				      FILEBROWSER_ROOT);
	virtual_root = g_settings_get_string (priv->settings,
					      FILEBROWSER_VIRTUAL_ROOT);

	remote = g_settings_get_boolean (priv->settings,
					 FILEBROWSER_ENABLE_REMOTE);

	if (root != NULL && *root != '\0')
	{
		GFile *rootfile;
		GFile *vrootfile;

		rootfile = g_file_new_for_uri (root);
		vrootfile = g_file_new_for_uri (virtual_root);

		if (remote || g_file_is_native (rootfile))
		{
			if (virtual_root != NULL && *virtual_root != '\0')
			{
				prepare_auto_root (plugin);
				gedit_file_browser_widget_set_root_and_virtual_root (priv->tree_widget,
					                                             rootfile,
					                                             vrootfile);
			}
			else
			{
				prepare_auto_root (plugin);
				gedit_file_browser_widget_set_root (priv->tree_widget,
					                            rootfile,
					                            TRUE);
			}
		}

		g_object_unref (rootfile);
		g_object_unref (vrootfile);
	}

	g_free (root);
	g_free (virtual_root);
}

static void
on_click_policy_changed (GSettings              *settings,
			 const gchar            *key,
			 GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GeditFileBrowserViewClickPolicy policy;
	GeditFileBrowserView *view;

	policy = g_settings_get_enum (settings, key);

	view = gedit_file_browser_widget_get_browser_view (priv->tree_widget);
	gedit_file_browser_view_set_click_policy (view, policy);
}

static void
install_nautilus_prefs (GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GeditFileBrowserViewClickPolicy policy;
	GeditFileBrowserView *view;

	/* Get click_policy */
	policy = g_settings_get_enum (priv->nautilus_settings,
				      NAUTILUS_CLICK_POLICY_KEY);

	view = gedit_file_browser_widget_get_browser_view (priv->tree_widget);
	gedit_file_browser_view_set_click_policy (view, policy);

	priv->click_policy_handle =
		g_signal_connect (priv->nautilus_settings,
				  "changed::" NAUTILUS_CLICK_POLICY_KEY,
				  G_CALLBACK (on_click_policy_changed),
				  plugin);
}

static void
set_root_from_doc (GeditFileBrowserPlugin *plugin,
                   GeditDocument          *doc)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GtkSourceFile *file;
	GFile *location;
	GFile *parent;

	if (doc == NULL)
	{
		return;
	}

	file = gedit_document_get_file (doc);
	location = gtk_source_file_get_location (file);
	if (location == NULL)
	{
		return;
	}

	parent = g_file_get_parent (location);

	if (parent != NULL)
	{
		gedit_file_browser_widget_set_root (priv->tree_widget,
				                    parent,
				                    TRUE);

		g_object_unref (parent);
	}
}

static void
set_active_root (GeditFileBrowserWidget *widget,
                 GeditFileBrowserPlugin *plugin)
{
	set_root_from_doc (plugin,
	                   gedit_window_get_active_document (plugin->priv->window));
}

static gchar *
get_terminal (GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gchar *terminal;

	terminal = g_settings_get_string (priv->terminal_settings,
					  TERMINAL_EXEC_KEY);

	if (terminal == NULL)
	{
		const gchar *term = g_getenv ("TERM");

		if (term != NULL)
			terminal = g_strdup (term);
		else
			terminal = g_strdup ("xterm");
	}

	return terminal;
}

static void
open_in_terminal (GeditFileBrowserWidget *widget,
                  GFile                  *location,
                  GeditFileBrowserPlugin *plugin)
{
	if (location)
	{
		gchar *terminal;
		gchar *local;
		gchar *argv[2];

		terminal = get_terminal (plugin);
		local = g_file_get_path (location);

		argv[0] = terminal;
		argv[1] = NULL;

		g_spawn_async (local,
			       argv,
			       NULL,
			       G_SPAWN_SEARCH_PATH,
			       NULL,
			       NULL,
			       NULL,
			       NULL);

		g_free (terminal);
		g_free (local);
	}
}

static void
gedit_file_browser_plugin_update_state (GeditWindowActivatable *activatable)
{
	GeditFileBrowserPluginPrivate *priv = GEDIT_FILE_BROWSER_PLUGIN (activatable)->priv;
	GeditDocument *doc;
	GFile *location = NULL;

	doc = gedit_window_get_active_document (priv->window);

	if (doc != NULL)
	{
		TeplFile *file;

		file = tepl_buffer_get_file (TEPL_BUFFER (doc));
		location = tepl_file_get_location (file);
	}

	gedit_file_browser_widget_set_active_root_enabled (priv->tree_widget, location != NULL);
}

static void
gedit_file_browser_plugin_activate (GeditWindowActivatable *activatable)
{
	GeditFileBrowserPlugin *plugin = GEDIT_FILE_BROWSER_PLUGIN (activatable);
	GeditFileBrowserPluginPrivate *priv;
	TeplPanel *side_panel;
	GeditFileBrowserStore *store;

	priv = plugin->priv;

	priv->tree_widget = GEDIT_FILE_BROWSER_WIDGET (gedit_file_browser_widget_new ());

	g_signal_connect (priv->tree_widget,
			  "location-activated",
			  G_CALLBACK (on_location_activated_cb), priv->window);

	g_signal_connect (priv->tree_widget,
			  "error", G_CALLBACK (on_error_cb), plugin);

	g_signal_connect (priv->tree_widget,
	                  "confirm-delete",
	                  G_CALLBACK (on_confirm_delete_cb),
	                  plugin);

	g_signal_connect (priv->tree_widget,
	                  "confirm-no-trash",
	                  G_CALLBACK (on_confirm_no_trash_cb),
	                  priv->window);

	g_signal_connect (priv->tree_widget,
	                  "open-in-terminal",
	                  G_CALLBACK (open_in_terminal),
	                  plugin);

	g_signal_connect (priv->tree_widget,
	                  "set-active-root",
	                  G_CALLBACK (set_active_root),
	                  plugin);

	g_settings_bind (priv->settings,
	                 FILEBROWSER_FILTER_PATTERN,
	                 priv->tree_widget,
	                 FILEBROWSER_FILTER_PATTERN,
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);

	g_clear_object (&priv->side_panel_item);
	priv->side_panel_item = tepl_panel_item_new (GTK_WIDGET (priv->tree_widget),
						     "GeditFileBrowserPanel",
						     _("File Browser"),
						     NULL,
						     0);

	side_panel = gedit_window_get_side_panel (priv->window);
	tepl_panel_add (side_panel, priv->side_panel_item);

	/* Install nautilus preferences */
	install_nautilus_prefs (plugin);

	/* Connect signals to store the last visited location */
	g_signal_connect (gedit_file_browser_widget_get_browser_view (priv->tree_widget),
	                  "notify::model",
	                  G_CALLBACK (on_model_set_cb),
	                  plugin);

	store = gedit_file_browser_widget_get_browser_store (priv->tree_widget);

	g_settings_bind (priv->settings,
	                 FILEBROWSER_FILTER_MODE,
	                 store,
	                 FILEBROWSER_FILTER_MODE,
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);

	g_settings_bind (priv->settings,
	                 FILEBROWSER_BINARY_PATTERNS,
	                 store,
	                 FILEBROWSER_BINARY_PATTERNS,
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);

	g_signal_connect (store,
	                  "notify::virtual-root",
	                  G_CALLBACK (on_virtual_root_changed_cb),
	                  plugin);

	g_signal_connect (store,
			  "rename",
			  G_CALLBACK (on_rename_cb),
			  priv->window);

	g_signal_connect (priv->window,
	                  "tab-added",
	                  G_CALLBACK (on_tab_added_cb),
	                  plugin);

	/* Register messages on the bus */
	gedit_file_browser_messages_register (priv->window, priv->tree_widget);

	gedit_file_browser_plugin_update_state (activatable);
}

static void
gedit_file_browser_plugin_deactivate (GeditWindowActivatable *activatable)
{
	GeditFileBrowserPlugin *plugin = GEDIT_FILE_BROWSER_PLUGIN (activatable);
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	TeplPanel *side_panel;

	/* Unregister messages from the bus */
	gedit_file_browser_messages_unregister (priv->window);

	/* Disconnect signals */
	g_signal_handlers_disconnect_by_func (priv->window,
	                                      G_CALLBACK (on_tab_added_cb),
	                                      plugin);

	if (priv->click_policy_handle)
	{
		g_signal_handler_disconnect (priv->nautilus_settings,
					     priv->click_policy_handle);
	}

	side_panel = gedit_window_get_side_panel (priv->window);
	tepl_panel_remove (side_panel, priv->side_panel_item);
	g_clear_object (&priv->side_panel_item);
}

static void
gedit_file_browser_plugin_class_init (GeditFileBrowserPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_file_browser_plugin_dispose;
	object_class->set_property = gedit_file_browser_plugin_set_property;
	object_class->get_property = gedit_file_browser_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_file_browser_plugin_activate;
	iface->deactivate = gedit_file_browser_plugin_deactivate;
	iface->update_state = gedit_file_browser_plugin_update_state;
}

static void
gedit_file_browser_plugin_class_finalize (GeditFileBrowserPluginClass *klass)
{
}

/* Callbacks */
static void
on_location_activated_cb (GeditFileBrowserWidget *tree_widget,
		          GFile                  *location,
		          GeditWindow            *window)
{
	gedit_commands_load_location (window, location, NULL, 0, 0);
}

static void
on_error_cb (GeditFileBrowserWidget *tree_widget,
	     guint                   code,
	     gchar const            *message,
	     GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gchar *title;
	GtkWidget *dlg;

	/* Do not show the error when the root has been set automatically */
	if (priv->auto_root && (code == GEDIT_FILE_BROWSER_ERROR_SET_ROOT ||
	                          code == GEDIT_FILE_BROWSER_ERROR_LOAD_DIRECTORY))
	{
		/* Show bookmarks */
		gedit_file_browser_widget_show_bookmarks (priv->tree_widget);
		return;
	}

	switch (code)
	{
		case GEDIT_FILE_BROWSER_ERROR_NEW_DIRECTORY:
			title = _("An error occurred while creating a new directory");
			break;
		case GEDIT_FILE_BROWSER_ERROR_NEW_FILE:
			title = _("An error occurred while creating a new file");
			break;
		case GEDIT_FILE_BROWSER_ERROR_RENAME:
			title = _("An error occurred while renaming a file or directory");
			break;
		case GEDIT_FILE_BROWSER_ERROR_DELETE:
			title = _("An error occurred while deleting a file or directory");
			break;
		case GEDIT_FILE_BROWSER_ERROR_OPEN_DIRECTORY:
			title = _("An error occurred while opening a directory in the file manager");
			break;
		case GEDIT_FILE_BROWSER_ERROR_SET_ROOT:
			title = _("An error occurred while setting a root directory");
			break;
		case GEDIT_FILE_BROWSER_ERROR_LOAD_DIRECTORY:
			title = _("An error occurred while loading a directory");
			break;
		default:
			title = _("An error occurred");
			break;
	}

	dlg = gtk_message_dialog_new (GTK_WINDOW (priv->window),
				      GTK_DIALOG_MODAL |
				      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				      "%s", title);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg),
						  "%s", message);

	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
}

static void
on_model_set_cb (GeditFileBrowserView   *widget,
                 GParamSpec             *param,
                 GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (gedit_file_browser_widget_get_browser_view (priv->tree_widget)));

	if (model == NULL)
		return;

	g_settings_set_boolean (priv->settings,
	                        FILEBROWSER_TREE_VIEW,
	                        GEDIT_IS_FILE_BROWSER_STORE (model));
}

static void
on_rename_cb (GeditFileBrowserStore *store,
	      GFile                 *oldfile,
	      GFile                 *newfile,
	      GeditWindow           *window)
{
	GList *documents;
	GList *item;

	/* Find all documents and set its uri to newuri where it matches olduri */
	documents = gedit_app_get_documents (GEDIT_APP (g_application_get_default ()));

	for (item = documents; item; item = item->next)
	{
		GeditDocument *doc;
		GtkSourceFile *source_file;
		GFile *docfile;

		doc = GEDIT_DOCUMENT (item->data);
		source_file = gedit_document_get_file (doc);
		docfile = gtk_source_file_get_location (source_file);

		if (docfile == NULL)
		{
			continue;
		}

		if (g_file_equal (docfile, oldfile))
		{
			gtk_source_file_set_location (source_file, newfile);
		}
		else
		{
			gchar *relative;

			relative = g_file_get_relative_path (oldfile, docfile);

			if (relative != NULL)
			{
				/* Relative now contains the part in docfile without
				   the prefix oldfile */

				docfile = g_file_get_child (newfile, relative);

				gtk_source_file_set_location (source_file, docfile);

				g_object_unref (docfile);
			}

			g_free (relative);
		}
	}

	g_list_free (documents);
}

static void
on_virtual_root_changed_cb (GeditFileBrowserStore  *store,
                            GParamSpec             *param,
                            GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	GFile *root;
	GFile *virtual_root;
	gchar *uri_root = NULL;

	root = gedit_file_browser_store_get_root (store);

	if (!root)
	{
		return;
	}
	else
	{
		uri_root = g_file_get_uri (root);
		g_object_unref (root);
	}

	g_settings_set_string (priv->settings,
	                       FILEBROWSER_ROOT,
	                       uri_root);

	virtual_root = gedit_file_browser_store_get_virtual_root (store);

	if (!virtual_root)
	{
		/* Set virtual to same as root then */
		g_settings_set_string (priv->settings,
		                       FILEBROWSER_VIRTUAL_ROOT,
		                       uri_root);
	}
	else
	{
		gchar *uri_vroot;

		uri_vroot = g_file_get_uri (virtual_root);

		g_settings_set_string (priv->settings,
		                       FILEBROWSER_VIRTUAL_ROOT,
		                       uri_vroot);
		g_free (uri_vroot);
		g_object_unref (virtual_root);
	}

	g_signal_handlers_disconnect_by_func (priv->window,
	                                      G_CALLBACK (on_tab_added_cb),
	                                      plugin);
	g_free (uri_root);
}

static void
on_tab_added_cb (GeditWindow            *window,
                 GeditTab               *tab,
                 GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gboolean open;
	gboolean load_default = TRUE;

	open = g_settings_get_boolean (priv->settings,
	                               FILEBROWSER_OPEN_AT_FIRST_DOC);

	if (open)
	{
		GeditDocument *doc;
		GtkSourceFile *file;
		GFile *location;

		doc = gedit_tab_get_document (tab);
		file = gedit_document_get_file (doc);
		location = gtk_source_file_get_location (file);

		if (location != NULL)
		{
			if (g_file_has_uri_scheme (location, "file"))
			{
				prepare_auto_root (plugin);
				set_root_from_doc (plugin, doc);
				load_default = FALSE;
			}
		}
	}

	if (load_default)
		restore_default_location (plugin);

	/* Disconnect this signal, it's only called once */
	g_signal_handlers_disconnect_by_func (window,
	                                      G_CALLBACK (on_tab_added_cb),
	                                      plugin);
}

static gchar *
get_filename_from_path (GtkTreeModel *model,
			GtkTreePath  *path)
{
	GtkTreeIter iter;
	GFile *location;
	gchar *ret = NULL;

	if (!gtk_tree_model_get_iter (model, &iter, path))
	{
		return NULL;
	}

	gtk_tree_model_get (model, &iter,
			    GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &location,
			    -1);

	if (location)
	{
		ret = gedit_file_browser_utils_file_basename (location);
		g_object_unref (location);
	}

	return ret;
}

static gboolean
on_confirm_no_trash_cb (GeditFileBrowserWidget *widget,
                        GList                  *files,
                        GeditWindow            *window)
{
	gchar *normal;
	gchar *message;
	gchar *secondary;
	gboolean result;

	message = _("Cannot move file to trash, do you\nwant to delete permanently?");

	if (files->next == NULL)
	{
		normal = gedit_file_browser_utils_file_basename (G_FILE (files->data));
	    	secondary = g_strdup_printf (_("The file “%s” cannot be moved to the trash."), normal);
		g_free (normal);
	}
	else
	{
		secondary = g_strdup (_("The selected files cannot be moved to the trash."));
	}

	result = gedit_file_browser_utils_confirmation_dialog (window,
	                                                       GTK_MESSAGE_QUESTION,
	                                                       message,
	                                                       secondary,
	                                                       _("_Delete"));
	g_free (secondary);

	return result;
}

static gboolean
on_confirm_delete_cb (GeditFileBrowserWidget *widget,
                      GeditFileBrowserStore  *store,
                      GList                  *paths,
                      GeditFileBrowserPlugin *plugin)
{
	GeditFileBrowserPluginPrivate *priv = plugin->priv;
	gchar *normal;
	gchar *message;
	gchar *secondary;
	gboolean result;

	if (paths->next == NULL)
	{
		normal = get_filename_from_path (GTK_TREE_MODEL (store), (GtkTreePath *)(paths->data));
		message = g_strdup_printf (_("Are you sure you want to permanently delete “%s”?"), normal);
		g_free (normal);
	}
	else
	{
		message = g_strdup (_("Are you sure you want to permanently delete the selected files?"));
	}

	secondary = _("If you delete an item, it is permanently lost.");

	result = gedit_file_browser_utils_confirmation_dialog (priv->window,
	                                                       GTK_MESSAGE_QUESTION,
	                                                       message,
	                                                       secondary,
	                                                       _("_Delete"));

	g_free (message);

	return result;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_file_browser_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_FILE_BROWSER_PLUGIN);
}

/* ex:set ts=8 noet: */
