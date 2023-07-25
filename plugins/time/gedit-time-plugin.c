/*
 * gedit-time-plugin.c
 *
 * Copyright (C) 2002-2005 - Paolo Maggi
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
#include <glib.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-utils.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-app-activatable.h>
#include <gedit/gedit-window-activatable.h>
#include <libpeas-gtk/peas-gtk-configurable.h>
#include <gedit/gedit-app.h>
#include "gedit-time-plugin.h"

/* gsettings keys */
#define TIME_BASE_SETTINGS	"org.gnome.gedit.plugins.time"
#define PROMPT_TYPE_KEY	"prompt-type"
#define SELECTED_FORMAT_KEY	"selected-format"
#define CUSTOM_FORMAT_KEY	"custom-format"

#define DEFAULT_CUSTOM_FORMAT "%d/%m/%Y %H:%M:%S"

static const gchar *formats[] =
{
	"%c",
	"%x",
	"%X",
	"%x %X",
	"%Y-%m-%d %H:%M:%S",
	"%a %b %d %H:%M:%S %Z %Y",
	"%a %b %d %H:%M:%S %Y",
	"%a %d %b %Y %H:%M:%S %Z",
	"%a %d %b %Y %H:%M:%S",
	"%d/%m/%Y",
	"%d/%m/%y",
	"%A %d %B %Y",
	"%A %B %d %Y",
	"%Y-%m-%d",
	"%d %B %Y",
	"%B %d, %Y",
	"%A %b %d",
	"%H:%M:%S",
	"%H:%M",
	"%I:%M:%S %p",
	"%I:%M %p",
	"%H.%M.%S",
	"%H.%M",
	"%I.%M.%S %p",
	"%I.%M %p",
	"%d/%m/%Y %H:%M:%S",
	"%d/%m/%y %H:%M:%S",
	"%a, %d %b %Y %H:%M:%S %z",
	NULL
};

enum
{
	COLUMN_FORMATS = 0,
	COLUMN_INDEX,
	NUM_COLUMNS
};

typedef enum
{
	PROMPT_SELECTED_FORMAT = 0,	/* Popup dialog with list preselected */
	PROMPT_CUSTOM_FORMAT,		/* Popup dialog with entry preselected */
	USE_SELECTED_FORMAT,		/* Use selected format directly */
	USE_CUSTOM_FORMAT		/* Use custom format directly */
} GeditTimePluginPromptType;

typedef struct _TimeConfigureWidget TimeConfigureWidget;

struct _TimeConfigureWidget
{
	GtkWidget *content;

	GtkWidget *list;

	/* Radio buttons to indicate what should be done */
	GtkWidget *prompt;
	GtkWidget *use_list;
	GtkWidget *custom;

	GtkWidget *custom_entry;
	GtkWidget *custom_format_example;

	GSettings *settings;
};

typedef struct _ChooseFormatDialog ChooseFormatDialog;

struct _ChooseFormatDialog
{
	GtkWidget *dialog;

	GtkWidget *list;

	/* Radio buttons to indicate what should be done */
	GtkWidget *use_list;
	GtkWidget *custom;

	GtkWidget *custom_entry;
	GtkWidget *custom_format_example;

	/* Info needed for the response handler */
	GtkTextBuffer   *buffer;

	GSettings *settings;
};

struct _GeditTimePluginPrivate
{
	GSettings      *settings;

	GSimpleAction  *action;
	GeditWindow    *window;

	GeditApp *app;
	GeditMenuExtension *menu_ext;
};

enum
{
	PROP_0,
	PROP_WINDOW,
	PROP_APP
};

static void gedit_app_activatable_iface_init (GeditAppActivatableInterface *iface);
static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);
static void peas_gtk_configurable_iface_init (PeasGtkConfigurableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditTimePlugin,
				gedit_time_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_APP_ACTIVATABLE,
							       gedit_app_activatable_iface_init)
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init)
				G_IMPLEMENT_INTERFACE_DYNAMIC (PEAS_GTK_TYPE_CONFIGURABLE,
							       peas_gtk_configurable_iface_init)
				G_ADD_PRIVATE_DYNAMIC (GeditTimePlugin))

static void time_cb (GAction *action, GVariant *parameter, GeditTimePlugin *plugin);

static void
gedit_time_plugin_init (GeditTimePlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditTimePlugin initializing");

	plugin->priv = gedit_time_plugin_get_instance_private (plugin);

	plugin->priv->settings = g_settings_new (TIME_BASE_SETTINGS);
}

static void
gedit_time_plugin_dispose (GObject *object)
{
	GeditTimePlugin *plugin = GEDIT_TIME_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS, "GeditTimePlugin disposing");

	g_clear_object (&plugin->priv->settings);
	g_clear_object (&plugin->priv->action);
	g_clear_object (&plugin->priv->window);
	g_clear_object (&plugin->priv->menu_ext);
	g_clear_object (&plugin->priv->app);

	G_OBJECT_CLASS (gedit_time_plugin_parent_class)->dispose (object);
}

static void
gedit_time_plugin_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
	GeditTimePlugin *plugin = GEDIT_TIME_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			plugin->priv->window = GEDIT_WINDOW (g_value_dup_object (value));
			break;
		case PROP_APP:
			plugin->priv->app = GEDIT_APP (g_value_dup_object (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_time_plugin_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
	GeditTimePlugin *plugin = GEDIT_TIME_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_value_set_object (value, plugin->priv->window);
			break;
		case PROP_APP:
			g_value_set_object (value, plugin->priv->app);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
update_ui (GeditTimePlugin *plugin)
{
	GeditView *view;

	gedit_debug (DEBUG_PLUGINS);

	view = gedit_window_get_active_view (plugin->priv->window);

	gedit_debug_message (DEBUG_PLUGINS, "View: %p", view);

	g_simple_action_set_enabled (plugin->priv->action,
	                             (view != NULL) &&
	                             gtk_text_view_get_editable (GTK_TEXT_VIEW (view)));
}

static void
add_keyboard_shortcut (GtkApplication *app)
{
	const gchar *accels[] = { "<Shift><Alt>D", NULL };
	gtk_application_set_accels_for_action (app, "win.time", accels);
}

static void
remove_keyboard_shortcut (GtkApplication *app)
{
	const gchar *accels[] = {  NULL };
	gtk_application_set_accels_for_action (app, "win.time", accels);
}

static void
gedit_time_plugin_app_activate (GeditAppActivatable *activatable)
{
	GeditTimePluginPrivate *priv;
	GMenuItem *item;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_TIME_PLUGIN (activatable)->priv;

	add_keyboard_shortcut (GTK_APPLICATION (priv->app));

	priv->menu_ext = gedit_app_activatable_extend_menu (activatable, "tools-section");
	item = g_menu_item_new (_("In_sert Date and Time…"), "win.time");
	gedit_menu_extension_append_menu_item (priv->menu_ext, item);
	g_object_unref (item);
}

static void
gedit_time_plugin_app_deactivate (GeditAppActivatable *activatable)
{
	GeditTimePluginPrivate *priv;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_TIME_PLUGIN (activatable)->priv;

	remove_keyboard_shortcut (GTK_APPLICATION (priv->app));

	g_clear_object (&priv->menu_ext);
}

static void
gedit_time_plugin_window_activate (GeditWindowActivatable *activatable)
{
	GeditTimePluginPrivate *priv;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_TIME_PLUGIN (activatable)->priv;

	priv->action = g_simple_action_new ("time", NULL);
	g_signal_connect (priv->action, "activate",
	                  G_CALLBACK (time_cb), activatable);
	g_action_map_add_action (G_ACTION_MAP (priv->window),
	                         G_ACTION (priv->action));

	update_ui (GEDIT_TIME_PLUGIN (activatable));
}

static void
gedit_time_plugin_window_deactivate (GeditWindowActivatable *activatable)
{
	GeditTimePluginPrivate *priv;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_TIME_PLUGIN (activatable)->priv;

	g_action_map_remove_action (G_ACTION_MAP (priv->window), "time");
}

static void
gedit_time_plugin_window_update_state (GeditWindowActivatable *activatable)
{
	gedit_debug (DEBUG_PLUGINS);

	update_ui (GEDIT_TIME_PLUGIN (activatable));
}

/* The selected format in the list */
static gchar *
get_selected_format (GeditTimePlugin *plugin)
{
	gchar *sel_format;

	sel_format = g_settings_get_string (plugin->priv->settings,
					    SELECTED_FORMAT_KEY);

	return sel_format ? sel_format : g_strdup (formats [0]);
}

/* the custom format in the entry */
static gchar *
get_custom_format (GeditTimePlugin *plugin)
{
	gchar *format;

	format = g_settings_get_string (plugin->priv->settings,
					CUSTOM_FORMAT_KEY);

	return format ? format : g_strdup (DEFAULT_CUSTOM_FORMAT);
}

static gchar *
get_time (const gchar *format)
{
	gchar *out;
	GDateTime *now;

	gedit_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (format != NULL, NULL);

	if (*format == '\0')
		return g_strdup (" ");

	now = g_date_time_new_now_local ();
	out = g_date_time_format (now, format);
	g_date_time_unref (now);

	return out;
}

static void
configure_widget_destroyed (GtkWidget *widget,
			    gpointer   data)
{
	TimeConfigureWidget *conf_widget = (TimeConfigureWidget *)data;

	gedit_debug (DEBUG_PLUGINS);

	g_object_unref (conf_widget->settings);
	g_slice_free (TimeConfigureWidget, data);

	gedit_debug_message (DEBUG_PLUGINS, "END");
}

static void
choose_format_dialog_destroyed (GtkWidget *widget,
				gpointer   dialog_pointer)
{
	gedit_debug (DEBUG_PLUGINS);

	g_slice_free (ChooseFormatDialog, dialog_pointer);

	gedit_debug_message (DEBUG_PLUGINS, "END");
}

static GtkTreeModel *
create_model (GtkWidget       *listview,
	      const gchar     *sel_format,
	      GeditTimePlugin *plugin)
{
	gint i = 0;
	GtkListStore *store;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PLUGINS);

	/* create list store */
	store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

	/* Set tree view model*/
	gtk_tree_view_set_model (GTK_TREE_VIEW (listview),
				 GTK_TREE_MODEL (store));
	g_object_unref (G_OBJECT (store));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (listview));
	g_return_val_if_fail (selection != NULL, GTK_TREE_MODEL (store));

	/* there should always be one line selected */
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	/* add data to the list store */
	while (formats[i] != NULL)
	{
		gchar *str;

		str = get_time (formats[i]);

		gedit_debug_message (DEBUG_PLUGINS, "%d : %s", i, str);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_FORMATS, str,
				    COLUMN_INDEX, i,
				    -1);
		g_free (str);

		if (sel_format && strcmp (formats[i], sel_format) == 0)
			gtk_tree_selection_select_iter (selection, &iter);

		++i;
	}

	/* fall back to select the first iter */
	if (!gtk_tree_selection_get_selected (selection, NULL, NULL) &&
	    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
	{
		gtk_tree_selection_select_iter (selection, &iter);
	}

	return GTK_TREE_MODEL (store);
}

static void
scroll_to_selected (GtkTreeView *tree_view)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PLUGINS);

	model = gtk_tree_view_get_model (tree_view);
	g_return_if_fail (model != NULL);

	/* Scroll to selected */
	selection = gtk_tree_view_get_selection (tree_view);
	g_return_if_fail (selection != NULL);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		GtkTreePath* path;

		path = gtk_tree_model_get_path (model, &iter);
		g_return_if_fail (path != NULL);

		gtk_tree_view_scroll_to_cell (tree_view,
					      path, NULL, TRUE, 1.0, 0.0);
		gtk_tree_path_free (path);
	}
}

static void
create_formats_list (GtkWidget       *listview,
		     const gchar     *sel_format,
		     GeditTimePlugin *plugin)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;

	gedit_debug (DEBUG_PLUGINS);

	g_return_if_fail (listview != NULL);
	g_return_if_fail (sel_format != NULL);

	/* the Available formats column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (
			_("Available formats"),
			cell,
			"text", COLUMN_FORMATS,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (listview), column);

	/* Create model, it also add model to the tree view */
	create_model (listview, sel_format, plugin);

	g_signal_connect (listview,
			  "realize",
			  G_CALLBACK (scroll_to_selected),
			  NULL);

	gtk_widget_show (listview);
}

static void
updated_custom_format_example (GtkEntry *format_entry,
			       GtkLabel *format_example)
{
	const gchar *format;
	gchar *time;
	gchar *str;
	gchar *escaped_time;

	gedit_debug (DEBUG_PLUGINS);

	g_return_if_fail (GTK_IS_ENTRY (format_entry));
	g_return_if_fail (GTK_IS_LABEL (format_example));

	format = gtk_entry_get_text (format_entry);

	time = get_time (format);
	escaped_time = g_markup_escape_text (time, -1);

	str = g_strdup_printf ("<span size=\"small\">%s</span>", escaped_time);

	gtk_label_set_markup (format_example, str);

	g_free (escaped_time);
	g_free (time);
	g_free (str);
}

static void
choose_format_dialog_button_toggled (GtkToggleButton    *button,
				     ChooseFormatDialog *dialog)
{
	gedit_debug (DEBUG_PLUGINS);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->custom)))
	{
		gtk_widget_set_sensitive (dialog->list, FALSE);
		gtk_widget_set_sensitive (dialog->custom_entry, TRUE);
		gtk_widget_set_sensitive (dialog->custom_format_example, TRUE);
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->use_list)))
	{
		gtk_widget_set_sensitive (dialog->list, TRUE);
		gtk_widget_set_sensitive (dialog->custom_entry, FALSE);
		gtk_widget_set_sensitive (dialog->custom_format_example, FALSE);
	}
	else
	{
		g_return_if_reached ();
	}
}

static void
configure_widget_button_toggled (GtkToggleButton     *button,
				 TimeConfigureWidget *conf_widget)
{
	GeditTimePluginPromptType prompt_type;

	gedit_debug (DEBUG_PLUGINS);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (conf_widget->custom)))
	{
		gtk_widget_set_sensitive (conf_widget->list, FALSE);
		gtk_widget_set_sensitive (conf_widget->custom_entry, TRUE);
		gtk_widget_set_sensitive (conf_widget->custom_format_example, TRUE);

		prompt_type = USE_CUSTOM_FORMAT;
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (conf_widget->use_list)))
	{
		gtk_widget_set_sensitive (conf_widget->list, TRUE);
		gtk_widget_set_sensitive (conf_widget->custom_entry, FALSE);
		gtk_widget_set_sensitive (conf_widget->custom_format_example, FALSE);

		prompt_type = USE_SELECTED_FORMAT;
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (conf_widget->prompt)))
	{
		gtk_widget_set_sensitive (conf_widget->list, FALSE);
		gtk_widget_set_sensitive (conf_widget->custom_entry, FALSE);
		gtk_widget_set_sensitive (conf_widget->custom_format_example, FALSE);

		prompt_type = PROMPT_SELECTED_FORMAT;
	}
	else
	{
		g_return_if_reached ();
	}

	g_settings_set_enum (conf_widget->settings,
			     PROMPT_TYPE_KEY,
			     prompt_type);
}

static gint
get_format_from_list (GtkWidget *listview)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PLUGINS);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (listview));
	g_return_val_if_fail (model != NULL, 0);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (listview));
	g_return_val_if_fail (selection != NULL, 0);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gint selected_value;

		gtk_tree_model_get (model, &iter, COLUMN_INDEX, &selected_value, -1);

		gedit_debug_message (DEBUG_PLUGINS, "Sel value: %d", selected_value);

		return selected_value;
	}

	g_return_val_if_reached (0);
}

static void
on_configure_widget_selection_changed (GtkTreeSelection *selection,
				       TimeConfigureWidget *conf_widget)
{
	gint sel_format;

	sel_format = get_format_from_list (conf_widget->list);
	g_settings_set_string (conf_widget->settings,
			       SELECTED_FORMAT_KEY,
			       formats[sel_format]);
}

static TimeConfigureWidget *
get_configure_widget (GeditTimePlugin *plugin)
{
	TimeConfigureWidget *widget;
	GtkTreeSelection *selection;
	GtkWidget *viewport;
	GeditTimePluginPromptType prompt_type;
	gchar *sf;
	GtkBuilder *builder;
	const gchar *root_objects[] = {
		"time_dialog_content",
		NULL
	};

	gedit_debug (DEBUG_PLUGINS);

	widget = g_slice_new (TimeConfigureWidget);
	widget->settings = g_object_ref (plugin->priv->settings);

	builder = gtk_builder_new ();
	gtk_builder_add_objects_from_resource (builder, "/org/gnome/gedit/plugins/time/ui/gedit-time-setup-dialog.ui",
	                                       (gchar **)root_objects, NULL);
	widget->content = GTK_WIDGET (gtk_builder_get_object (builder, "time_dialog_content"));
	g_object_ref (widget->content);
	viewport = GTK_WIDGET (gtk_builder_get_object (builder, "formats_viewport"));
	widget->list = GTK_WIDGET (gtk_builder_get_object (builder, "formats_tree"));
	widget->prompt = GTK_WIDGET (gtk_builder_get_object (builder, "always_prompt"));
	widget->use_list = GTK_WIDGET (gtk_builder_get_object (builder, "never_prompt"));
	widget->custom = GTK_WIDGET (gtk_builder_get_object (builder, "use_custom"));
	widget->custom_entry = GTK_WIDGET (gtk_builder_get_object (builder, "custom_entry"));
	widget->custom_format_example = GTK_WIDGET (gtk_builder_get_object (builder, "custom_format_example"));
	g_object_unref (builder);

	sf = get_selected_format (plugin);
	create_formats_list (widget->list, sf, plugin);
	g_free (sf);

	prompt_type = g_settings_get_enum (plugin->priv->settings,
					   PROMPT_TYPE_KEY);

	g_settings_bind (widget->settings,
			 CUSTOM_FORMAT_KEY,
			 widget->custom_entry,
			 "text",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);

	if (prompt_type == USE_CUSTOM_FORMAT)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->custom), TRUE);

		gtk_widget_set_sensitive (widget->list, FALSE);
		gtk_widget_set_sensitive (widget->custom_entry, TRUE);
		gtk_widget_set_sensitive (widget->custom_format_example, TRUE);
	}
	else if (prompt_type == USE_SELECTED_FORMAT)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->use_list), TRUE);

		gtk_widget_set_sensitive (widget->list, TRUE);
		gtk_widget_set_sensitive (widget->custom_entry, FALSE);
		gtk_widget_set_sensitive (widget->custom_format_example, FALSE);
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->prompt), TRUE);

		gtk_widget_set_sensitive (widget->list, FALSE);
		gtk_widget_set_sensitive (widget->custom_entry, FALSE);
		gtk_widget_set_sensitive (widget->custom_format_example, FALSE);
	}

	updated_custom_format_example (GTK_ENTRY (widget->custom_entry),
				       GTK_LABEL (widget->custom_format_example));

	/* setup a window of a sane size. */
	gtk_widget_set_size_request (GTK_WIDGET (viewport), 10, 200);

	g_signal_connect (widget->custom,
			  "toggled",
			  G_CALLBACK (configure_widget_button_toggled),
			  widget);
	g_signal_connect (widget->prompt,
			  "toggled",
			  G_CALLBACK (configure_widget_button_toggled),
			  widget);
	g_signal_connect (widget->use_list,
			  "toggled",
			  G_CALLBACK (configure_widget_button_toggled),
			  widget);
	g_signal_connect (widget->content,
			  "destroy",
			  G_CALLBACK (configure_widget_destroyed),
			  widget);
	g_signal_connect (widget->custom_entry,
			  "changed",
			  G_CALLBACK (updated_custom_format_example),
			  widget->custom_format_example);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget->list));

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (on_configure_widget_selection_changed),
			  widget);

	return widget;
}

static void
real_insert_time (GtkTextBuffer *buffer,
		  const gchar   *the_time)
{
	gedit_debug_message (DEBUG_PLUGINS, "Insert: %s", the_time);

	gtk_text_buffer_begin_user_action (buffer);

	gtk_text_buffer_insert_at_cursor (buffer, the_time, -1);
	gtk_text_buffer_insert_at_cursor (buffer, " ", -1);

	gtk_text_buffer_end_user_action (buffer);
}

static void
choose_format_dialog_row_activated (GtkTreeView        *list,
				    GtkTreePath        *path,
				    GtkTreeViewColumn  *column,
				    ChooseFormatDialog *dialog)
{
	gint sel_format;
	gchar *the_time;

	sel_format = get_format_from_list (dialog->list);
	the_time = get_time (formats[sel_format]);

	g_settings_set_enum (dialog->settings,
			     PROMPT_TYPE_KEY,
			     PROMPT_SELECTED_FORMAT);
	g_settings_set_string (dialog->settings,
			       SELECTED_FORMAT_KEY,
			       formats[sel_format]);

	g_return_if_fail (the_time != NULL);

	real_insert_time (dialog->buffer, the_time);

	g_free (the_time);
}

static ChooseFormatDialog *
get_choose_format_dialog (GtkWindow                 *parent,
			  GeditTimePluginPromptType  prompt_type,
			  GeditTimePlugin           *plugin)
{
	ChooseFormatDialog *dialog;
	GtkBuilder *builder;
	gchar *sf, *cf;
	GtkWindowGroup *wg = NULL;

	if (parent != NULL)
		wg = gtk_window_get_group (parent);

	dialog = g_slice_new (ChooseFormatDialog);
	dialog->settings = plugin->priv->settings;

	builder = gtk_builder_new ();
	gtk_builder_add_from_resource (builder, "/org/gnome/gedit/plugins/time/ui/gedit-time-dialog.ui",
	                               NULL);
	dialog->dialog = GTK_WIDGET (gtk_builder_get_object (builder, "choose_format_dialog"));
	dialog->list = GTK_WIDGET (gtk_builder_get_object (builder, "choice_list"));
	dialog->use_list = GTK_WIDGET (gtk_builder_get_object (builder, "use_sel_format_radiobutton"));
	dialog->custom = GTK_WIDGET (gtk_builder_get_object (builder, "use_custom_radiobutton"));
	dialog->custom_entry = GTK_WIDGET (gtk_builder_get_object (builder, "custom_entry"));
	dialog->custom_format_example = GTK_WIDGET (gtk_builder_get_object (builder, "custom_format_example"));
	g_object_unref (builder);

	gtk_window_group_add_window (wg,
				     GTK_WINDOW (dialog->dialog));
	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog), parent);
	gtk_window_set_modal (GTK_WINDOW (dialog->dialog), TRUE);

	sf = get_selected_format (plugin);
	create_formats_list (dialog->list, sf, plugin);
	g_free (sf);

	cf = get_custom_format (plugin);
	gtk_entry_set_text (GTK_ENTRY(dialog->custom_entry), cf);
	g_free (cf);

	updated_custom_format_example (GTK_ENTRY (dialog->custom_entry),
				       GTK_LABEL (dialog->custom_format_example));

	if (prompt_type == PROMPT_CUSTOM_FORMAT)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->custom), TRUE);

		gtk_widget_set_sensitive (dialog->list, FALSE);
		gtk_widget_set_sensitive (dialog->custom_entry, TRUE);
		gtk_widget_set_sensitive (dialog->custom_format_example, TRUE);
	}
	else if (prompt_type == PROMPT_SELECTED_FORMAT)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->use_list), TRUE);

		gtk_widget_set_sensitive (dialog->list, TRUE);
		gtk_widget_set_sensitive (dialog->custom_entry, FALSE);
		gtk_widget_set_sensitive (dialog->custom_format_example, FALSE);
	}
	else
	{
		g_return_val_if_reached (NULL);
	}

	/* setup a window of a sane size. */
	gtk_widget_set_size_request (dialog->list, 10, 200);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_signal_connect (dialog->custom,
			  "toggled",
			  G_CALLBACK (choose_format_dialog_button_toggled),
			  dialog);
	g_signal_connect (dialog->use_list,
			  "toggled",
			  G_CALLBACK (choose_format_dialog_button_toggled),
			  dialog);
	g_signal_connect (dialog->dialog,
			  "destroy",
			  G_CALLBACK (choose_format_dialog_destroyed),
			  dialog);
	g_signal_connect (dialog->custom_entry,
			  "changed",
			  G_CALLBACK (updated_custom_format_example),
			  dialog->custom_format_example);
	g_signal_connect (dialog->list,
			  "row_activated",
			  G_CALLBACK (choose_format_dialog_row_activated),
			  dialog);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	return dialog;
}

static void
choose_format_dialog_response_cb (GtkWidget          *widget,
				  gint                response,
				  ChooseFormatDialog *dialog)
{
	switch (response)
	{
		case GTK_RESPONSE_HELP:
		{
			gedit_debug_message (DEBUG_PLUGINS, "GTK_RESPONSE_HELP");
			gedit_app_show_help (GEDIT_APP (g_application_get_default ()),
					     GTK_WINDOW (widget),
					     NULL,
					     "gedit-plugins-insert-date-time");
			break;
		}
		case GTK_RESPONSE_OK:
		{
			gchar *the_time;

			gedit_debug_message (DEBUG_PLUGINS, "GTK_RESPONSE_OK");

			/* Get the user's chosen format */
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->use_list)))
			{
				gint sel_format;

				sel_format = get_format_from_list (dialog->list);
				the_time = get_time (formats[sel_format]);

				g_settings_set_enum (dialog->settings,
						     PROMPT_TYPE_KEY,
						     PROMPT_SELECTED_FORMAT);
				g_settings_set_string (dialog->settings,
						       SELECTED_FORMAT_KEY,
						       formats[sel_format]);
			}
			else
			{
				const gchar *format;

				format = gtk_entry_get_text (GTK_ENTRY (dialog->custom_entry));
				the_time = get_time (format);

				g_settings_set_enum (dialog->settings,
						     PROMPT_TYPE_KEY,
						     PROMPT_CUSTOM_FORMAT);
				g_settings_set_string (dialog->settings,
						       CUSTOM_FORMAT_KEY,
						       format);
			}

			g_return_if_fail (the_time != NULL);

			real_insert_time (dialog->buffer, the_time);
			g_free (the_time);

			gtk_widget_destroy (dialog->dialog);
			break;
		}
		case GTK_RESPONSE_CANCEL:
			gedit_debug_message (DEBUG_PLUGINS, "GTK_RESPONSE_CANCEL");
			gtk_widget_destroy (dialog->dialog);
	}
}

static void
time_cb (GAction         *action,
         GVariant        *parameter,
         GeditTimePlugin *plugin)
{
	GeditTimePluginPrivate *priv;
	GtkTextBuffer *buffer;
	GeditTimePluginPromptType prompt_type;
	gchar *the_time = NULL;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	buffer = GTK_TEXT_BUFFER (gedit_window_get_active_document (priv->window));
	g_return_if_fail (buffer != NULL);

	prompt_type = g_settings_get_enum (plugin->priv->settings,
					   PROMPT_TYPE_KEY);

	if (prompt_type == USE_CUSTOM_FORMAT)
	{
		gchar *cf = get_custom_format (plugin);
		the_time = get_time (cf);
		g_free (cf);
	}
	else if (prompt_type == USE_SELECTED_FORMAT)
	{
		gchar *sf = get_selected_format (plugin);
		the_time = get_time (sf);
		g_free (sf);
	}
	else
	{
		ChooseFormatDialog *dialog;

		dialog = get_choose_format_dialog (GTK_WINDOW (priv->window),
						   prompt_type,
						   plugin);
		if (dialog != NULL)
		{
			dialog->buffer = buffer;
			dialog->settings = plugin->priv->settings;

			g_signal_connect (dialog->dialog,
					  "response",
					  G_CALLBACK (choose_format_dialog_response_cb),
					  dialog);

			gtk_widget_show (GTK_WIDGET (dialog->dialog));
		}

		return;
	}

	g_return_if_fail (the_time != NULL);

	real_insert_time (buffer, the_time);

	g_free (the_time);
}

static GtkWidget *
gedit_time_plugin_create_configure_widget (PeasGtkConfigurable *configurable)
{
	TimeConfigureWidget *widget;

	widget = get_configure_widget (GEDIT_TIME_PLUGIN (configurable));

	return widget->content;
}

static void
gedit_time_plugin_class_init (GeditTimePluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_time_plugin_dispose;
	object_class->set_property = gedit_time_plugin_set_property;
	object_class->get_property = gedit_time_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
	g_object_class_override_property (object_class, PROP_APP, "app");
}

static void
gedit_time_plugin_class_finalize (GeditTimePluginClass *klass)
{
}

static void
peas_gtk_configurable_iface_init (PeasGtkConfigurableInterface *iface)
{
	iface->create_configure_widget = gedit_time_plugin_create_configure_widget;
}

static void
gedit_app_activatable_iface_init (GeditAppActivatableInterface *iface)
{
	iface->activate = gedit_time_plugin_app_activate;
	iface->deactivate = gedit_time_plugin_app_deactivate;
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_time_plugin_window_activate;
	iface->deactivate = gedit_time_plugin_window_deactivate;
	iface->update_state = gedit_time_plugin_window_update_state;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_time_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_APP_ACTIVATABLE,
						    GEDIT_TYPE_TIME_PLUGIN);
	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_TIME_PLUGIN);
	peas_object_module_register_extension_type (module,
						    PEAS_GTK_TYPE_CONFIGURABLE,
						    GEDIT_TYPE_TIME_PLUGIN);
}

/* ex:set ts=8 noet: */
