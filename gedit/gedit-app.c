/*
 * This file is part of gedit
 *
 * Copyright (C) 2005-2006 - Paolo Maggi
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

#include "gedit-app.h"
#include "gedit-app-private.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <libpeas/peas-extension-set.h>
#include <gfls/gfls.h>
#include <tepl/tepl.h>

#include "gedit-commands-private.h"
#include "gedit-notebook.h"
#include "gedit-debug.h"
#include "gedit-utils.h"
#include "gedit-enum-types.h"
#include "gedit-dirs.h"
#include "gedit-settings.h"
#include "gedit-app-activatable.h"
#include "gedit-plugins-engine.h"
#include "gedit-commands.h"
#include "gedit-preferences-dialog.h"
#include "gedit-tab.h"
#include "gedit-window-private.h"

/**
 * SECTION:gedit-app
 * @Title: GeditApp
 * @Short_description: The whole application
 *
 * #GeditApp is a subclass of #GtkApplication, representing the whole
 * application.
 *
 * It contains #GeditWindow's and other #GtkWindow's.
 */

#define GEDIT_PAGE_SETUP_FILE		"gedit-page-setup"
#define GEDIT_PRINT_SETTINGS_FILE	"gedit-print-settings"

typedef struct
{
	GeditPluginsEngine *engine;

	GtkCssProvider *theme_provider;

	GtkPageSetup      *page_setup;
	GtkPrintSettings  *print_settings;

	GMenuModel        *hamburger_menu;
	GMenuModel        *notebook_menu;
	GMenuModel        *tab_width_menu;

	PeasExtensionSet  *extensions;

	/* command line parsing */
	gboolean new_window;
	gboolean new_document;
	const GtkSourceEncoding *encoding;
	GInputStream *stdin_stream;
	GSList *file_list;
	gint line_position;
	gint column_position;
	GApplicationCommandLine *command_line;
} GeditAppPrivate;

static const GOptionEntry option_entries[] =
{
	/* Version */
	{
		"version", 'V', 0, G_OPTION_ARG_NONE, NULL,
		N_("Show the application’s version"), NULL
	},

	/* List available encodings */
	{
		"list-encodings", '\0', 0, G_OPTION_ARG_NONE, NULL,
		N_("Display list of possible values for the encoding option"),
		NULL
	},

	/* Encoding */
	{
		"encoding", '\0', 0, G_OPTION_ARG_STRING, NULL,
		N_("Set the character encoding to be used to open the files listed on the command line"),
		N_("ENCODING")
	},

	/* Open a new window */
	{
		"new-window", '\0', 0, G_OPTION_ARG_NONE, NULL,
		N_("Create a new top-level window in an existing instance of gedit"),
		NULL
	},

	/* Create a new empty document */
	{
		"new-document", '\0', 0, G_OPTION_ARG_NONE, NULL,
		N_("Create a new document in an existing instance of gedit"),
		NULL
	},

	/* Wait for closing documents */
	{
		"wait", 'w', 0, G_OPTION_ARG_NONE, NULL,
		N_("Open files and block process until files are closed"),
		NULL
	},

	/* New instance */
	{
		"standalone", 's', 0, G_OPTION_ARG_NONE, NULL,
		N_("Run gedit in standalone mode"),
		NULL
	},

	/* collects file arguments */
	{
		G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, NULL, NULL,
		N_("[FILE…] [+LINE[:COLUMN]]")
	},

	{NULL}
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditApp, gedit_app, GTK_TYPE_APPLICATION)

static void
gedit_app_dispose (GObject *object)
{
	GeditAppPrivate *priv;

	priv = gedit_app_get_instance_private (GEDIT_APP (object));

	g_clear_object (&priv->page_setup);
	g_clear_object (&priv->print_settings);

	/* Note that unreffing the extensions will automatically remove
	 * all extensions which in turn will deactivate the extension
	 */
	g_clear_object (&priv->extensions);

	g_clear_object (&priv->engine);
	g_clear_object (&priv->theme_provider);
	g_clear_object (&priv->hamburger_menu);
	g_clear_object (&priv->notebook_menu);
	g_clear_object (&priv->tab_width_menu);

	G_OBJECT_CLASS (gedit_app_parent_class)->dispose (object);
}

static gchar *
gedit_app_get_help_uri_impl (GeditApp    *app,
                             const gchar *name_of_user_manual,
                             const gchar *link_id_within_user_manual)
{
	if (link_id_within_user_manual != NULL)
	{
		return g_strdup_printf ("help:%s/%s",
					name_of_user_manual,
					link_id_within_user_manual);
	}
	else
	{
		return g_strdup_printf ("help:%s", name_of_user_manual);
	}
}

static gboolean
gedit_app_show_help_impl (GeditApp    *app,
                          GtkWindow   *parent_window,
                          const gchar *name_of_user_manual,
                          const gchar *link_id_within_user_manual)
{
	gchar *uri;
	gboolean ret;
	GError *error = NULL;

	if (name_of_user_manual == NULL)
	{
		name_of_user_manual = "gedit";
	}

	uri = GEDIT_APP_GET_CLASS (app)->get_help_uri (app,
						       name_of_user_manual,
						       link_id_within_user_manual);

	ret = gtk_show_uri_on_window (parent_window,
				      uri,
				      GDK_CURRENT_TIME,
				      &error);

	g_free (uri);

	if (error != NULL)
	{
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (parent_window,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("There was an error displaying the help."));

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  "%s", error->message);

		g_signal_connect (G_OBJECT (dialog),
				  "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);

		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

		gtk_widget_show (dialog);

		g_error_free (error);
	}

	return ret;
}

static void
gedit_app_set_window_title_impl (GeditApp    *app,
                                 GeditWindow *window,
                                 const gchar *title)
{
	gtk_window_set_title (GTK_WINDOW (window), title);
}

static GeditWindow *
get_active_window (GtkApplication *app)
{
	GList *windows;
	GList *l;

	/* Gtk documentation says the window list is always in MRU order. */
	windows = gtk_application_get_windows (app);
	for (l = windows; l != NULL; l = l->next)
	{
		GtkWindow *window = l->data;

		if (GEDIT_IS_WINDOW (window))
		{
			return GEDIT_WINDOW (window);
		}
	}

	return NULL;
}

static void
set_command_line_wait (GeditApp *app,
		       GeditTab *tab)
{
	GeditAppPrivate *priv;

	priv = gedit_app_get_instance_private (app);

	g_object_set_data_full (G_OBJECT (tab),
	                        "GeditTabCommandLineWait",
	                        g_object_ref (priv->command_line),
	                        (GDestroyNotify)g_object_unref);
}

static void
set_command_line_wait_doc (GeditDocument *doc,
			   GeditApp      *app)
{
	GeditTab *tab = gedit_tab_get_from_document (doc);

	set_command_line_wait (app, tab);
}

static void
open_files (GApplication            *application,
	    gboolean                 new_window,
	    gboolean                 new_document,
	    gint                     line_position,
	    gint                     column_position,
	    const GtkSourceEncoding *encoding,
	    GInputStream            *stdin_stream,
	    GSList                  *file_list,
	    GApplicationCommandLine *command_line)
{
	GeditWindow *window = NULL;
	GeditTab *tab;
	gboolean doc_created = FALSE;

	if (!new_window)
	{
		window = get_active_window (GTK_APPLICATION (application));
	}

	if (window == NULL)
	{
		gedit_debug_message (DEBUG_APP, "Create main window");
		window = gedit_app_create_window (GEDIT_APP (application), NULL);

		gedit_debug_message (DEBUG_APP, "Show window");
		gtk_widget_show (GTK_WIDGET (window));
	}

	if (stdin_stream != NULL)
	{
		gedit_debug_message (DEBUG_APP, "Load stdin");

		tab = gedit_window_create_tab (window, TRUE);
		gedit_tab_load_stream (tab,
				       stdin_stream,
				       encoding,
				       line_position,
				       column_position);
		doc_created = TRUE;

		if (command_line != NULL)
		{
			set_command_line_wait (GEDIT_APP (application),
					       tab);
		}
		g_input_stream_close (stdin_stream, NULL, NULL);
	}

	if (file_list != NULL)
	{
		GSList *loaded;

		gedit_debug_message (DEBUG_APP, "Load files");
		loaded = _gedit_cmd_load_files_from_prompt (window,
		                                            file_list,
		                                            encoding,
		                                            line_position,
		                                            column_position);

		doc_created = doc_created || loaded != NULL;

		if (command_line)
		{
			g_slist_foreach (loaded, (GFunc)set_command_line_wait_doc, GEDIT_APP (application));
		}
		g_slist_free (loaded);
	}

	if (!doc_created || new_document)
	{
		gedit_debug_message (DEBUG_APP, "Create tab");
		tab = gedit_window_create_tab (window, TRUE);

		if (command_line)
		{
			set_command_line_wait (GEDIT_APP (application),
					       tab);
		}
	}

	gtk_window_present (GTK_WINDOW (window));
}

static void
new_window_activated (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
	GeditApp *app;
	GeditWindow *window;

	app = GEDIT_APP (user_data);
	window = gedit_app_create_window (app, NULL);

	gedit_debug_message (DEBUG_APP, "Show window");
	gtk_widget_show (GTK_WIDGET (window));

	gedit_debug_message (DEBUG_APP, "Create tab");
	gedit_window_create_tab (window, TRUE);

	gtk_window_present (GTK_WINDOW (window));
}

static void
new_document_activated (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
	GApplication *application = G_APPLICATION (user_data);

	open_files (application,
	            FALSE,
	            TRUE,
	            0,
	            0,
	            NULL,
	            NULL,
	            NULL,
	            NULL);
}

static void
preferences_activated (GSimpleAction *action,
		       GVariant      *parameter,
		       gpointer       user_data)
{
	GtkApplication *app = GTK_APPLICATION (user_data);
	GeditWindow *window;

	window = get_active_window (app);

	gedit_show_preferences_dialog (GTK_WINDOW (window));
}

static void
help_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
	GtkApplication *app;
	GeditWindow *window;

	app = GTK_APPLICATION (user_data);
	window = GEDIT_WINDOW (gtk_application_get_active_window (app));

	_gedit_cmd_help_contents (window);
}

static void
about_activated (GSimpleAction  *action,
                 GVariant       *parameter,
                 gpointer        user_data)
{
	GtkApplication *app;
	GeditWindow *window;

	app = GTK_APPLICATION (user_data);
	window = GEDIT_WINDOW (gtk_application_get_active_window (app));

	_gedit_cmd_help_about (window);
}

static GActionEntry app_entries[] = {
	{ "new-window", new_window_activated, NULL, NULL, NULL },
	{ "new-document", new_document_activated, NULL, NULL, NULL },
	{ "preferences", preferences_activated, NULL, NULL, NULL },
	{ "help", help_activated, NULL, NULL, NULL },
	{ "about", about_activated, NULL, NULL, NULL },
	{ "quit", _gedit_cmd_file_quit, NULL, NULL, NULL }
};

static void
extension_added (PeasExtensionSet *extensions,
		 PeasPluginInfo   *info,
		 PeasExtension    *exten,
		 GeditApp         *app)
{
	gedit_app_activatable_activate (GEDIT_APP_ACTIVATABLE (exten));
}

static void
extension_removed (PeasExtensionSet *extensions,
		   PeasPluginInfo   *info,
		   PeasExtension    *exten,
		   GeditApp         *app)
{
	gedit_app_activatable_deactivate (GEDIT_APP_ACTIVATABLE (exten));
}

static void
load_accels (void)
{
	gchar *filename;

	filename = g_build_filename (gedit_dirs_get_user_config_dir (),
				     "accels",
				     NULL);
	if (filename != NULL)
	{
		gedit_debug_message (DEBUG_APP, "Loading keybindings from %s\n", filename);
		gtk_accel_map_load (filename);
		g_free (filename);
	}
}

static GtkCssProvider *
load_css_from_resource (const gchar *css_filename,
                        gboolean     required)
{
	GdkScreen *screen = gdk_screen_get_default ();
	gchar *resource_name;
	GFile *css_file;
	GtkCssProvider *provider;

	if (screen == NULL)
	{
		return NULL;
	}

	resource_name = g_strdup_printf ("resource:///org/gnome/gedit/css/%s", css_filename);
	css_file = g_file_new_for_uri (resource_name);
	g_free (resource_name);

	if (!required && !g_file_query_exists (css_file, NULL))
	{
		g_object_unref (css_file);
		return NULL;
	}

	provider = gtk_css_provider_new ();
	if (gtk_css_provider_load_from_file (provider, css_file, NULL))
	{
		gtk_style_context_add_provider_for_screen (screen,
							   GTK_STYLE_PROVIDER (provider),
							   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}
	else
	{
		g_warning ("Could not load css provider.");
	}

	g_object_unref (css_file);
	return provider;
}

static void
update_theme (GeditApp *app)
{
	GeditAppPrivate *priv = gedit_app_get_instance_private (app);
	GtkSettings *settings;
	gchar *theme_name = NULL;
	gchar *lowercase_theme_name;
	gchar *css_filename;

	settings = gtk_settings_get_default ();
	if (settings == NULL)
	{
		return;
	}

	g_object_get (settings,
		      "gtk-theme-name", &theme_name,
		      NULL);

	if (theme_name == NULL)
	{
		return;
	}

	lowercase_theme_name = g_ascii_strdown (theme_name, -1);
	g_free (theme_name);

	css_filename = g_strdup_printf ("gedit.%s.css", lowercase_theme_name);
	g_free (lowercase_theme_name);

	if (priv->theme_provider != NULL)
	{
		GdkScreen *screen = gdk_screen_get_default ();

		if (screen != NULL)
		{
			gtk_style_context_remove_provider_for_screen (screen,
								      GTK_STYLE_PROVIDER (priv->theme_provider));
		}

		g_clear_object (&priv->theme_provider);
	}

	priv->theme_provider = load_css_from_resource (css_filename, FALSE);
	g_free (css_filename);
}

static void
theme_name_notify_cb (GtkSettings *settings,
		      GParamSpec  *pspec,
		      GeditApp    *app)
{
	update_theme (app);
}

static void
setup_theme_extensions (GeditApp *app)
{
	GtkSettings *settings;

	settings = gtk_settings_get_default ();

	if (settings != NULL)
	{
		g_signal_connect_object (settings,
					 "notify::gtk-theme-name",
					 G_CALLBACK (theme_name_notify_cb),
					 app,
					 0);
	}

	update_theme (app);
}

static GMenuModel *
get_menu_model (GeditApp   *app,
                const char *id)
{
	GMenu *menu;

	menu = gtk_application_get_menu_by_id (GTK_APPLICATION (app), id);

	return menu ? G_MENU_MODEL (g_object_ref_sink (menu)) : NULL;
}

static void
add_accelerator (GtkApplication *app,
                 const gchar    *action_name,
                 const gchar    *accel)
{
	const gchar *vaccels[] = {
		accel,
		NULL
	};

	gtk_application_set_accels_for_action (app, action_name, vaccels);
}

static gboolean
show_menubar (void)
{
	GtkSettings *settings = gtk_settings_get_default ();
	gboolean result;

	g_object_get (settings,
	              "gtk-shell-shows-menubar", &result,
	              NULL);

	return result;
}

static void
init_tepl_settings (void)
{
	GeditSettings *gedit_settings;
	TeplSettings *tepl_settings;
	GSettings *editor_settings;
	GSettings *ui_settings;

	gedit_settings = _gedit_settings_get_singleton ();
	tepl_settings = tepl_settings_get_singleton ();

	editor_settings = _gedit_settings_peek_editor_settings (gedit_settings);
	ui_settings = _gedit_settings_peek_ui_settings (gedit_settings);

	tepl_settings_provide_font_settings (tepl_settings,
					     editor_settings,
					     GEDIT_SETTINGS_USE_DEFAULT_FONT,
					     GEDIT_SETTINGS_EDITOR_FONT);

	tepl_settings_handle_simple_theme_variant (tepl_settings,
						   ui_settings,
						   GEDIT_SETTINGS_THEME_VARIANT);
}

static gchar *
unsaved_document_title_cb (gint num)
{
	return g_strdup_printf (_("Untitled Document %d"), num);
}

static void
init_unsaved_document_titles (void)
{
	GflsUnsavedDocumentTitles *titles;

	titles = gfls_unsaved_document_titles_get_default ();
	gfls_unsaved_document_titles_set_title_callback (titles, unsaved_document_title_cb);
}

static void
gedit_app_startup (GApplication *application)
{
	GeditAppPrivate *priv = gedit_app_get_instance_private (GEDIT_APP (application));

	G_APPLICATION_CLASS (gedit_app_parent_class)->startup (application);

	/* Setup debugging */
	gedit_debug_init ();
	gedit_debug_message (DEBUG_APP, "Startup");

	/* Load/init settings */
	_gedit_settings_get_singleton ();
	init_tepl_settings ();
	init_unsaved_document_titles ();

	setup_theme_extensions (GEDIT_APP (application));

	g_action_map_add_action_entries (G_ACTION_MAP (application),
	                                 app_entries,
	                                 G_N_ELEMENTS (app_entries),
	                                 application);

	/* menus */
	if (!show_menubar ())
	{
		gtk_application_set_menubar (GTK_APPLICATION (application), NULL);
		priv->hamburger_menu = get_menu_model (GEDIT_APP (application),
		                                       "hamburger-menu");
	}

	priv->notebook_menu = get_menu_model (GEDIT_APP (application), "notebook-menu");
	priv->tab_width_menu = get_menu_model (GEDIT_APP (application), "tab-width-menu");

	/* Accelerators */
	add_accelerator (GTK_APPLICATION (application), "app.new-window", "<Primary>N");
	add_accelerator (GTK_APPLICATION (application), "app.quit", "<Primary>Q");
	add_accelerator (GTK_APPLICATION (application), "app.help", "F1");
	add_accelerator (GTK_APPLICATION (application), "app.preferences", "<Primary>comma");

	add_accelerator (GTK_APPLICATION (application), "win.hamburger-menu", "F10");
	add_accelerator (GTK_APPLICATION (application), "win.open", "<Primary>O");
	add_accelerator (GTK_APPLICATION (application), "win.save", "<Primary>S");
	add_accelerator (GTK_APPLICATION (application), "win.save-as", "<Primary><Shift>S");
	add_accelerator (GTK_APPLICATION (application), "win.save-all", "<Primary><Shift>L");
	add_accelerator (GTK_APPLICATION (application), "win.new-tab", "<Primary>T");
	add_accelerator (GTK_APPLICATION (application), "win.reopen-closed-tab", "<Primary><Shift>T");
	add_accelerator (GTK_APPLICATION (application), "win.close", "<Primary>W");
	add_accelerator (GTK_APPLICATION (application), "win.close-all", "<Primary><Shift>W");
	add_accelerator (GTK_APPLICATION (application), "win.print", "<Primary>P");
	add_accelerator (GTK_APPLICATION (application), "win.find", "<Primary>F");
	add_accelerator (GTK_APPLICATION (application), "win.find-next", "<Primary>G");
	add_accelerator (GTK_APPLICATION (application), "win.find-prev", "<Primary><Shift>G");
	add_accelerator (GTK_APPLICATION (application), "win.replace", "<Primary>H");
	add_accelerator (GTK_APPLICATION (application), "win.clear-highlight", "<Primary><Shift>K");
	add_accelerator (GTK_APPLICATION (application), "win.goto-line", "<Primary>I");
	add_accelerator (GTK_APPLICATION (application), "win.focus-active-view", "Escape");
	add_accelerator (GTK_APPLICATION (application), "win.side-panel", "F9");
	add_accelerator (GTK_APPLICATION (application), "win.bottom-panel", "<Primary>F9");
	add_accelerator (GTK_APPLICATION (application), "win.fullscreen", "F11");
	add_accelerator (GTK_APPLICATION (application), "win.new-tab-group", "<Primary><Alt>N");
	add_accelerator (GTK_APPLICATION (application), "win.previous-tab-group", "<Primary><Shift><Alt>Page_Up");
	add_accelerator (GTK_APPLICATION (application), "win.next-tab-group", "<Primary><Shift><Alt>Page_Down");
	add_accelerator (GTK_APPLICATION (application), "win.previous-document", "<Primary><Alt>Page_Up");
	add_accelerator (GTK_APPLICATION (application), "win.next-document", "<Primary><Alt>Page_Down");

	load_accels ();

	/* Load custom css */
	g_object_unref (load_css_from_resource ("gedit-style.css", TRUE));

	priv->engine = gedit_plugins_engine_get_default ();
	priv->extensions = peas_extension_set_new (PEAS_ENGINE (priv->engine),
	                                           GEDIT_TYPE_APP_ACTIVATABLE,
	                                           "app", GEDIT_APP (application),
	                                           NULL);

	g_signal_connect (priv->extensions,
	                  "extension-added",
	                  G_CALLBACK (extension_added),
	                  application);

	g_signal_connect (priv->extensions,
	                  "extension-removed",
	                  G_CALLBACK (extension_removed),
	                  application);

	peas_extension_set_foreach (priv->extensions,
	                            (PeasExtensionSetForeachFunc) extension_added,
	                            application);
}

static void
gedit_app_activate (GApplication *application)
{
	GeditAppPrivate *priv;

	priv = gedit_app_get_instance_private (GEDIT_APP (application));

	open_files (application,
	            priv->new_window,
	            priv->new_document,
	            priv->line_position,
	            priv->column_position,
	            priv->encoding,
	            priv->stdin_stream,
	            priv->file_list,
	            priv->command_line);
}

static void
clear_options (GeditApp *app)
{
	GeditAppPrivate *priv;

	priv = gedit_app_get_instance_private (app);

	g_clear_object (&priv->stdin_stream);
	g_slist_free_full (priv->file_list, g_object_unref);

	priv->new_window = FALSE;
	priv->new_document = FALSE;
	priv->encoding = NULL;
	priv->file_list = NULL;
	priv->line_position = 0;
	priv->column_position = 0;
	priv->command_line = NULL;
}

static void
get_line_column_position (const gchar *arg,
                          gint        *line,
                          gint        *column)
{
	gchar **split;

	split = g_strsplit (arg, ":", 2);

	if (split != NULL)
	{
		if (split[0] != NULL)
		{
			*line = atoi (split[0]);
		}

		if (split[1] != NULL)
		{
			*column = atoi (split[1]);
		}
	}

	g_strfreev (split);
}

static gint
gedit_app_command_line (GApplication            *application,
                        GApplicationCommandLine *cl)
{
	GeditAppPrivate *priv;
	GVariantDict *options;
	const gchar *encoding_charset;
	const gchar **remaining_args;

	priv = gedit_app_get_instance_private (GEDIT_APP (application));

	options = g_application_command_line_get_options_dict (cl);

	g_variant_dict_lookup (options, "new-window", "b", &priv->new_window);
	g_variant_dict_lookup (options, "new-document", "b", &priv->new_document);

	if (g_variant_dict_contains (options, "wait"))
	{
		priv->command_line = cl;
	}

	if (g_variant_dict_lookup (options, "encoding", "&s", &encoding_charset))
	{
		priv->encoding = gtk_source_encoding_get_from_charset (encoding_charset);

		if (priv->encoding == NULL)
		{
			g_application_command_line_printerr (cl,
							     _("%s: invalid encoding."),
							     encoding_charset);
		}
	}

	/* Parse filenames */
	if (g_variant_dict_lookup (options, G_OPTION_REMAINING, "^a&ay", &remaining_args))
	{
		gint i;

		for (i = 0; remaining_args[i]; i++)
		{
			if (*remaining_args[i] == '+')
			{
				if (*(remaining_args[i] + 1) == '\0')
				{
					/* goto the last line of the document */
					priv->line_position = G_MAXINT;
					priv->column_position = 0;
				}
				else
				{
					get_line_column_position (remaining_args[i] + 1,
								  &priv->line_position,
								  &priv->column_position);
				}
			}
			else if (*remaining_args[i] == '-' && *(remaining_args[i] + 1) == '\0')
			{
				priv->stdin_stream = g_application_command_line_get_stdin (cl);
			}
			else
			{
				GFile *file;

				file = g_application_command_line_create_file_for_arg (cl, remaining_args[i]);
				priv->file_list = g_slist_prepend (priv->file_list, file);
			}
		}

		priv->file_list = g_slist_reverse (priv->file_list);
		g_free (remaining_args);
	}

	g_application_activate (application);
	clear_options (GEDIT_APP (application));

	return 0;
}

static void
print_all_encodings (void)
{
	GSList *all_encodings;
	GSList *l;

	all_encodings = gtk_source_encoding_get_all ();

	for (l = all_encodings; l != NULL; l = l->next)
	{
		const GtkSourceEncoding *encoding = l->data;
		g_print ("%s\n", gtk_source_encoding_get_charset (encoding));
	}

	g_slist_free (all_encodings);
}

static gint
gedit_app_handle_local_options (GApplication *application,
                                GVariantDict *options)
{
	if (g_variant_dict_contains (options, "version"))
	{
		g_print ("%s - Version %s\n", g_get_application_name (), VERSION);
		return 0;
	}

	if (g_variant_dict_contains (options, "list-encodings"))
	{
		print_all_encodings ();
		return 0;
	}

	if (g_variant_dict_contains (options, "standalone"))
	{
		GApplicationFlags old_flags;

		old_flags = g_application_get_flags (application);
		g_application_set_flags (application, old_flags | G_APPLICATION_NON_UNIQUE);
	}

	if (g_variant_dict_contains (options, "wait"))
	{
		GApplicationFlags old_flags;

		old_flags = g_application_get_flags (application);
		g_application_set_flags (application, old_flags | G_APPLICATION_IS_LAUNCHER);
	}

	return -1;
}

/* Note: when launched from command line we do not reach this method
 * since we manually handle the command line parameters in order to
 * parse +LINE:COL, stdin, etc.
 * However this method is called when open() is called via dbus, for
 * instance when double clicking on a file in nautilus
 */
static void
gedit_app_open (GApplication  *application,
                GFile        **files,
                gint           n_files,
                const gchar   *hint)
{
	gint i;
	GSList *file_list = NULL;

	for (i = 0; i < n_files; i++)
	{
		file_list = g_slist_prepend (file_list, files[i]);
	}

	file_list = g_slist_reverse (file_list);

	open_files (application,
	            FALSE,
	            FALSE,
	            0,
	            0,
	            NULL,
	            NULL,
	            file_list,
	            NULL);

	g_slist_free (file_list);
}

static gboolean
ensure_user_config_dir (void)
{
	const gchar *config_dir;
	gboolean ret = TRUE;
	gint res;

	config_dir = gedit_dirs_get_user_config_dir ();
	if (config_dir == NULL)
	{
		g_warning ("Could not get config directory\n");
		return FALSE;
	}

	res = g_mkdir_with_parents (config_dir, 0755);
	if (res < 0)
	{
		g_warning ("Could not create config directory\n");
		ret = FALSE;
	}

	return ret;
}

static void
save_accels (void)
{
	gchar *filename;

	filename = g_build_filename (gedit_dirs_get_user_config_dir (),
				     "accels",
				     NULL);
	if (filename != NULL)
	{
		gedit_debug_message (DEBUG_APP, "Saving keybindings in %s\n", filename);
		gtk_accel_map_save (filename);
		g_free (filename);
	}
}

static gchar *
get_page_setup_file (void)
{
	const gchar *config_dir;
	gchar *setup = NULL;

	config_dir = gedit_dirs_get_user_config_dir ();

	if (config_dir != NULL)
	{
		setup = g_build_filename (config_dir,
					  GEDIT_PAGE_SETUP_FILE,
					  NULL);
	}

	return setup;
}

static void
save_page_setup (GeditApp *app)
{
	GeditAppPrivate *priv;

	priv = gedit_app_get_instance_private (app);

	if (priv->page_setup != NULL)
	{
		gchar *filename;
		GError *error = NULL;

		filename = get_page_setup_file ();

		gtk_page_setup_to_file (priv->page_setup,
					filename,
					&error);
		if (error)
		{
			g_warning ("%s", error->message);
			g_error_free (error);
		}

		g_free (filename);
	}
}

static gchar *
get_print_settings_file (void)
{
	const gchar *config_dir;
	gchar *settings = NULL;

	config_dir = gedit_dirs_get_user_config_dir ();

	if (config_dir != NULL)
	{
		settings = g_build_filename (config_dir,
					     GEDIT_PRINT_SETTINGS_FILE,
					     NULL);
	}

	return settings;
}

static void
save_print_settings (GeditApp *app)
{
	GeditAppPrivate *priv;

	priv = gedit_app_get_instance_private (app);

	if (priv->print_settings != NULL)
	{
		gchar *filename;
		GError *error = NULL;

		filename = get_print_settings_file ();

		gtk_print_settings_to_file (priv->print_settings,
					    filename,
					    &error);
		if (error)
		{
			g_warning ("%s", error->message);
			g_error_free (error);
		}

		g_free (filename);
	}
}

static void
gedit_app_shutdown (GApplication *app)
{
	gedit_debug_message (DEBUG_APP, "Quitting\n");

	/* Last window is gone... save some settings and exit */
	ensure_user_config_dir ();

	save_accels ();
	save_page_setup (GEDIT_APP (app));
	save_print_settings (GEDIT_APP (app));

	G_APPLICATION_CLASS (gedit_app_parent_class)->shutdown (app);
}

static gboolean
window_delete_event_cb (GeditWindow *window,
			GdkEvent    *event,
			gpointer     user_data)
{
	if (_gedit_window_get_can_close (window))
	{
		_gedit_cmd_file_close_window (window);
	}

	/* Do not destroy the window. */
	return GDK_EVENT_STOP;
}

static GeditWindow *
gedit_app_create_window_impl (GeditApp *app)
{
	GeditWindow *window;

	window = g_object_new (GEDIT_TYPE_WINDOW,
			       "application", app,
			       NULL);

	g_signal_connect (window,
			  "delete-event",
			  G_CALLBACK (window_delete_event_cb),
			  NULL);

	return window;
}

static void
gedit_app_class_init (GeditAppClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

	object_class->dispose = gedit_app_dispose;

	app_class->startup = gedit_app_startup;
	app_class->activate = gedit_app_activate;
	app_class->command_line = gedit_app_command_line;
	app_class->handle_local_options = gedit_app_handle_local_options;
	app_class->open = gedit_app_open;
	app_class->shutdown = gedit_app_shutdown;

	klass->show_help = gedit_app_show_help_impl;
	klass->get_help_uri = gedit_app_get_help_uri_impl;
	klass->set_window_title = gedit_app_set_window_title_impl;
	klass->create_window = gedit_app_create_window_impl;
}

static void
load_page_setup (GeditApp *app)
{
	GeditAppPrivate *priv;
	gchar *filename;
	GError *error = NULL;

	priv = gedit_app_get_instance_private (app);

	g_return_if_fail (priv->page_setup == NULL);

	filename = get_page_setup_file ();

	priv->page_setup = gtk_page_setup_new_from_file (filename, &error);
	if (error)
	{
		/* Ignore file not found error */
		if (error->domain != G_FILE_ERROR ||
		    error->code != G_FILE_ERROR_NOENT)
		{
			g_warning ("%s", error->message);
		}

		g_error_free (error);
	}

	g_free (filename);

	/* fall back to default settings */
	if (priv->page_setup == NULL)
	{
		priv->page_setup = gtk_page_setup_new ();
	}
}

static void
load_print_settings (GeditApp *app)
{
	GeditAppPrivate *priv;
	gchar *filename;
	GError *error = NULL;

	priv = gedit_app_get_instance_private (app);

	g_return_if_fail (priv->print_settings == NULL);

	filename = get_print_settings_file ();

	priv->print_settings = gtk_print_settings_new_from_file (filename, &error);
	if (error != NULL)
	{
		/* - Ignore file not found error.
		 * - Ignore empty file error, i.e. group not found. This happens
		 *   when we click on cancel in the print dialog, when using the
		 *   printing for the first time in gedit.
		 */
		if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT) &&
		    !g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND))
		{
			g_warning ("Load print settings error: %s", error->message);
		}

		g_error_free (error);
	}

	g_free (filename);

	/* fall back to default settings */
	if (priv->print_settings == NULL)
	{
		priv->print_settings = gtk_print_settings_new ();
	}
}

static void
gedit_app_init (GeditApp *app)
{
	TeplApplication *tepl_app;

	g_set_application_name ("gedit");
	gtk_window_set_default_icon_name ("org.gnome.gedit");

	g_application_add_main_option_entries (G_APPLICATION (app), option_entries);

	tepl_app = tepl_application_get_from_gtk_application (GTK_APPLICATION (app));
	tepl_application_handle_metadata (tepl_app);
}

/**
 * gedit_app_create_window:
 * @app: the #GeditApp.
 * @screen: (nullable): a #GdkScreen, or %NULL.
 *
 * Creates a new #GeditWindow part of @app.
 *
 * Returns: (transfer none): the new #GeditWindow.
 */
GeditWindow *
gedit_app_create_window (GeditApp  *app,
			 GdkScreen *screen)
{
	GeditWindow *window;
	GeditSettings *settings;
	GSettings *window_state_settings;
	gint width;
	gint height;
	GdkWindowState state;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);
	g_return_val_if_fail (screen == NULL || GDK_IS_SCREEN (screen), NULL);

	gedit_debug (DEBUG_APP);

	window = GEDIT_APP_GET_CLASS (app)->create_window (app);

	if (screen != NULL)
	{
		gtk_window_set_screen (GTK_WINDOW (window), screen);
	}

	settings = _gedit_settings_get_singleton ();
	window_state_settings = _gedit_settings_peek_window_state_settings (settings);

	g_settings_get (window_state_settings,
			GEDIT_SETTINGS_WINDOW_SIZE,
			"(ii)", &width, &height);

	gtk_window_set_default_size (GTK_WINDOW (window), width, height);

	state = g_settings_get_int (window_state_settings, GEDIT_SETTINGS_WINDOW_STATE);

	if (state & GDK_WINDOW_STATE_MAXIMIZED)
	{
		gtk_window_maximize (GTK_WINDOW (window));
	}
	else
	{
		gtk_window_unmaximize (GTK_WINDOW (window));
	}

	if (state & GDK_WINDOW_STATE_STICKY)
	{
		gtk_window_stick (GTK_WINDOW (window));
	}
	else
	{
		gtk_window_unstick (GTK_WINDOW (window));
	}

	return window;
}

/**
 * gedit_app_get_main_windows:
 * @app: the #GeditApp.
 *
 * Returns all #GeditWindow's currently open in #GeditApp. This differs from
 * gtk_application_get_windows() since it does not include the preferences
 * dialog and other auxiliary windows.
 *
 * Returns: (element-type GeditWindow) (transfer container): a newly allocated
 *   list of #GeditWindow objects.
 */
GList *
gedit_app_get_main_windows (GeditApp *app)
{
	GList *res = NULL;
	GList *windows, *l;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	windows = gtk_application_get_windows (GTK_APPLICATION (app));
	for (l = windows; l != NULL; l = g_list_next (l))
	{
		if (GEDIT_IS_WINDOW (l->data))
		{
			res = g_list_prepend (res, l->data);
		}
	}

	return g_list_reverse (res);
}

/**
 * gedit_app_get_documents:
 * @app: the #GeditApp.
 *
 * Returns: (element-type GeditDocument) (transfer container): a newly allocated
 *   list of all the #GeditDocument's currently part of @app.
 */
GList *
gedit_app_get_documents	(GeditApp *app)
{
	GList *res = NULL;
	GList *windows, *l;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	windows = gtk_application_get_windows (GTK_APPLICATION (app));
	for (l = windows; l != NULL; l = g_list_next (l))
	{
		if (GEDIT_IS_WINDOW (l->data))
		{
			res = g_list_concat (res,
			                     gedit_window_get_documents (GEDIT_WINDOW (l->data)));
		}
	}

	return res;
}

/**
 * gedit_app_get_views:
 * @app: the #GeditApp.
 *
 * Returns: (element-type GeditView) (transfer container): a newly allocated
 *   list of all the #GeditView's currently part of @app.
 */
GList *
gedit_app_get_views (GeditApp *app)
{
	GList *res = NULL;
	GList *windows, *l;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	windows = gtk_application_get_windows (GTK_APPLICATION (app));
	for (l = windows; l != NULL; l = g_list_next (l))
	{
		if (GEDIT_IS_WINDOW (l->data))
		{
			res = g_list_concat (res,
			                     gedit_window_get_views (GEDIT_WINDOW (l->data)));
		}
	}

	return res;
}

/**
 * gedit_app_show_help:
 * @app: a #GeditApp.
 * @parent_window: (nullable): the #GtkWindow where the request originates from.
 * @name_of_user_manual: (nullable): %NULL for gedit's user manual, otherwise
 *   the name of another user manual (e.g., one from another application).
 * @link_id_within_user_manual: (nullable): a link ID within the user manual, or
 *   %NULL to show its start page.
 *
 * To show the user manual.
 *
 * As a useful information to know, the gedit user documentation is currently
 * written in Mallard. As such, this functionality can easily be tested with
 * Yelp on Linux:
 *
 * With @name_of_user_manual and @link_id_within_user_manual both %NULL, it is
 * equivalent to:
 *
 * `$ yelp 'help:gedit'`
 *
 * With @link_id_within_user_manual set to `"gedit-replace"` (a Mallard page
 * id):
 *
 * `$ yelp 'help:gedit/gedit-replace'`
 *
 * With @link_id_within_user_manual set to `"gedit-spellcheck#dict"` (it refers
 * to a section id within a page id):
 *
 * `$ yelp 'help:gedit/gedit-spellcheck#dict'`
 *
 * Returns: whether the operation was successful.
 */
gboolean
gedit_app_show_help (GeditApp    *app,
                     GtkWindow   *parent_window,
                     const gchar *name_of_user_manual,
                     const gchar *link_id_within_user_manual)
{
	g_return_val_if_fail (GEDIT_IS_APP (app), FALSE);
	g_return_val_if_fail (parent_window == NULL || GTK_IS_WINDOW (parent_window), FALSE);

	return GEDIT_APP_GET_CLASS (app)->show_help (app,
						     parent_window,
						     name_of_user_manual,
						     link_id_within_user_manual);
}

void
_gedit_app_set_window_title (GeditApp    *app,
			     GeditWindow *window,
			     const gchar *title)
{
	g_return_if_fail (GEDIT_IS_APP (app));
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	GEDIT_APP_GET_CLASS (app)->set_window_title (app, window, title);
}

gboolean
_gedit_app_process_window_event (GeditApp    *app,
				 GeditWindow *window,
				 GdkEvent    *event)
{
	g_return_val_if_fail (GEDIT_IS_APP (app), FALSE);
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), FALSE);

	if (GEDIT_APP_GET_CLASS (app)->process_window_event != NULL)
	{
		return GEDIT_APP_GET_CLASS (app)->process_window_event (app, window, event);
	}

	return FALSE;
}

static GMenuModel *
find_extension_point_section (GMenuModel  *model,
                              const gchar *extension_point)
{
	gint i, n_items;
	GMenuModel *section = NULL;

	n_items = g_menu_model_get_n_items (model);

	for (i = 0; i < n_items && !section; i++)
	{
		gchar *id = NULL;

		if (g_menu_model_get_item_attribute (model, i, "id", "s", &id) &&
		    strcmp (id, extension_point) == 0)
		{
			section = g_menu_model_get_item_link (model, i, G_MENU_LINK_SECTION);
		}
		else
		{
			GMenuModel *subsection;
			GMenuModel *submenu;
			gint j, j_items;

			subsection = g_menu_model_get_item_link (model, i, G_MENU_LINK_SECTION);

			if (subsection == NULL)
			{
				subsection = model;
			}

			j_items = g_menu_model_get_n_items (subsection);

			for (j = 0; j < j_items && !section; j++)
			{
				submenu = g_menu_model_get_item_link (subsection, j, G_MENU_LINK_SUBMENU);
				if (submenu)
				{
					section = find_extension_point_section (submenu, extension_point);
				}
			}
		}

		g_free (id);
	}

	return section;
}

/* Returns a copy */
GtkPageSetup *
_gedit_app_get_default_page_setup (GeditApp *app)
{
	GeditAppPrivate *priv;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	priv = gedit_app_get_instance_private (app);

	if (priv->page_setup == NULL)
	{
		load_page_setup (app);
	}

	return gtk_page_setup_copy (priv->page_setup);
}

void
_gedit_app_set_default_page_setup (GeditApp     *app,
				   GtkPageSetup *page_setup)
{
	GeditAppPrivate *priv;

	g_return_if_fail (GEDIT_IS_APP (app));
	g_return_if_fail (GTK_IS_PAGE_SETUP (page_setup));

	priv = gedit_app_get_instance_private (app);

	g_set_object (&priv->page_setup, page_setup);
}

/* Returns a copy */
GtkPrintSettings *
_gedit_app_get_default_print_settings (GeditApp *app)
{
	GeditAppPrivate *priv;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	priv = gedit_app_get_instance_private (app);

	if (priv->print_settings == NULL)
	{
		load_print_settings (app);
	}

	return gtk_print_settings_copy (priv->print_settings);
}

void
_gedit_app_set_default_print_settings (GeditApp         *app,
				       GtkPrintSettings *settings)
{
	GeditAppPrivate *priv;

	g_return_if_fail (GEDIT_IS_APP (app));
	g_return_if_fail (GTK_IS_PRINT_SETTINGS (settings));

	priv = gedit_app_get_instance_private (app);

	if (priv->print_settings != NULL)
	{
		g_object_unref (priv->print_settings);
	}

	priv->print_settings = g_object_ref (settings);
}

GMenuModel *
_gedit_app_get_hamburger_menu (GeditApp *app)
{
	GeditAppPrivate *priv;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	priv = gedit_app_get_instance_private (app);

	return priv->hamburger_menu;
}

GMenuModel *
_gedit_app_get_notebook_menu (GeditApp *app)
{
	GeditAppPrivate *priv;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	priv = gedit_app_get_instance_private (app);

	return priv->notebook_menu;
}

GMenuModel *
_gedit_app_get_tab_width_menu (GeditApp *app)
{
	GeditAppPrivate *priv;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	priv = gedit_app_get_instance_private (app);

	return priv->tab_width_menu;
}

GeditMenuExtension *
_gedit_app_extend_menu (GeditApp    *app,
                        const gchar *extension_point)
{
	GeditAppPrivate *priv;
	GMenuModel *model;
	GMenuModel *section;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);
	g_return_val_if_fail (extension_point != NULL, NULL);

	priv = gedit_app_get_instance_private (app);

	/* First look in the gear or window menu */
	if (priv->hamburger_menu)
	{
		model = priv->hamburger_menu;
	}
	else
	{
		model = gtk_application_get_menubar (GTK_APPLICATION (app));
	}

	section = find_extension_point_section (model, extension_point);

	/* otherwise look in the app menu */
	if (section == NULL)
	{
		model = gtk_application_get_app_menu (GTK_APPLICATION (app));

		if (model != NULL)
		{
			section = find_extension_point_section (model, extension_point);
		}
	}

	return section != NULL ? gedit_menu_extension_new (G_MENU (section)) : NULL;
}

/* ex:set ts=8 noet: */
