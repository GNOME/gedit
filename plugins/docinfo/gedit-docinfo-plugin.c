/*
 * gedit-docinfo-plugin.c
 *
 * Copyright (C) 2002-2005 Paolo Maggi
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
 *
 */

#include "config.h"

#include "gedit-docinfo-plugin.h"

#include <string.h> /* For strlen (...) */

#include <glib/gi18n.h>
#include <pango/pango-break.h>
#include <gmodule.h>

#include <gedit/gedit-app.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-utils.h>
#include <gedit/gedit-menu-extension.h>
#include <gedit/gedit-app-activatable.h>
#include <gedit/gedit-window-activatable.h>

struct _GeditDocinfoPluginPrivate
{
	GeditWindow *window;

	GSimpleAction *action;

	GtkWidget *dialog;
	GtkWidget *header_bar;
	GtkWidget *lines_label;
	GtkWidget *words_label;
	GtkWidget *chars_label;
	GtkWidget *chars_ns_label;
	GtkWidget *bytes_label;
	GtkWidget *document_label;
	GtkWidget *document_lines_label;
	GtkWidget *document_words_label;
	GtkWidget *document_chars_label;
	GtkWidget *document_chars_ns_label;
	GtkWidget *document_bytes_label;
	GtkWidget *selection_label;
	GtkWidget *selected_lines_label;
	GtkWidget *selected_words_label;
	GtkWidget *selected_chars_label;
	GtkWidget *selected_chars_ns_label;
	GtkWidget *selected_bytes_label;

	GeditApp  *app;
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

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditDocinfoPlugin,
				gedit_docinfo_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_APP_ACTIVATABLE,
							       gedit_app_activatable_iface_init)
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init)
				G_ADD_PRIVATE_DYNAMIC (GeditDocinfoPlugin))

static void
calculate_info (GeditDocument *doc,
		GtkTextIter   *start,
		GtkTextIter   *end,
		gint          *chars,
		gint          *words,
		gint          *white_chars,
		gint          *bytes)
{
	gchar *text;

	gedit_debug (DEBUG_PLUGINS);

	text = gtk_text_buffer_get_slice (GTK_TEXT_BUFFER (doc),
					  start,
					  end,
					  TRUE);

	*chars = g_utf8_strlen (text, -1);
	*bytes = strlen (text);

	if (*chars > 0)
	{
		PangoLogAttr *attrs;
		gint i;

		attrs = g_new0 (PangoLogAttr, *chars + 1);

		pango_get_log_attrs (text,
				     -1,
				     0,
				     pango_language_from_string ("C"),
				     attrs,
				     *chars + 1);

		for (i = 0; i < (*chars); i++)
		{
			if (attrs[i].is_white)
				++(*white_chars);

			if (attrs[i].is_word_start)
				++(*words);
		}

		g_free (attrs);
	}
	else
	{
		*white_chars = 0;
		*words = 0;
	}

	g_free (text);
}

static void
update_document_info (GeditDocinfoPlugin *plugin,
		      GeditDocument      *doc)
{
	GeditDocinfoPluginPrivate *priv;
	GtkTextIter start, end;
	gint words = 0;
	gint chars = 0;
	gint white_chars = 0;
	gint lines = 0;
	gint bytes = 0;
	gchar *doc_name;
	gchar *tmp_str;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc),
				    &start,
				    &end);

	lines = gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (doc));

	calculate_info (doc,
			&start, &end,
			&chars, &words, &white_chars, &bytes);

	if (chars == 0)
	{
		lines = 0;
	}

	gedit_debug_message (DEBUG_PLUGINS, "Chars: %d", chars);
	gedit_debug_message (DEBUG_PLUGINS, "Lines: %d", lines);
	gedit_debug_message (DEBUG_PLUGINS, "Words: %d", words);
	gedit_debug_message (DEBUG_PLUGINS, "Chars non-space: %d", chars - white_chars);
	gedit_debug_message (DEBUG_PLUGINS, "Bytes: %d", bytes);

	doc_name = tepl_file_get_short_name (tepl_buffer_get_file (TEPL_BUFFER (doc)));
	gtk_header_bar_set_subtitle (GTK_HEADER_BAR (priv->header_bar), doc_name);
	g_free (doc_name);

	tmp_str = g_strdup_printf("%d", lines);
	gtk_label_set_text (GTK_LABEL (priv->document_lines_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", words);
	gtk_label_set_text (GTK_LABEL (priv->document_words_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", chars);
	gtk_label_set_text (GTK_LABEL (priv->document_chars_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", chars - white_chars);
	gtk_label_set_text (GTK_LABEL (priv->document_chars_ns_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", bytes);
	gtk_label_set_text (GTK_LABEL (priv->document_bytes_label), tmp_str);
	g_free (tmp_str);
}

static void
update_selection_info (GeditDocinfoPlugin *plugin,
		       GeditDocument      *doc)
{
	GeditDocinfoPluginPrivate *priv;
	gboolean sel;
	GtkTextIter start, end;
	gint words = 0;
	gint chars = 0;
	gint white_chars = 0;
	gint lines = 0;
	gint bytes = 0;
	gchar *tmp_str;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	sel = gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
						    &start,
						    &end);

	if (sel)
	{
		lines = gtk_text_iter_get_line (&end) - gtk_text_iter_get_line (&start) + 1;

		calculate_info (doc,
				&start, &end,
				&chars, &words, &white_chars, &bytes);

		gedit_debug_message (DEBUG_PLUGINS, "Selected chars: %d", chars);
		gedit_debug_message (DEBUG_PLUGINS, "Selected lines: %d", lines);
		gedit_debug_message (DEBUG_PLUGINS, "Selected words: %d", words);
		gedit_debug_message (DEBUG_PLUGINS, "Selected chars non-space: %d", chars - white_chars);
		gedit_debug_message (DEBUG_PLUGINS, "Selected bytes: %d", bytes);

		gtk_widget_set_sensitive (priv->selection_label, TRUE);
		gtk_widget_set_sensitive (priv->selected_words_label, TRUE);
		gtk_widget_set_sensitive (priv->selected_bytes_label, TRUE);
		gtk_widget_set_sensitive (priv->selected_lines_label, TRUE);
		gtk_widget_set_sensitive (priv->selected_chars_label, TRUE);
		gtk_widget_set_sensitive (priv->selected_chars_ns_label, TRUE);
	}
	else
	{
		gedit_debug_message (DEBUG_PLUGINS, "Selection empty");

		gtk_widget_set_sensitive (priv->selection_label, FALSE);
		gtk_widget_set_sensitive (priv->selected_words_label, FALSE);
		gtk_widget_set_sensitive (priv->selected_bytes_label, FALSE);
		gtk_widget_set_sensitive (priv->selected_lines_label, FALSE);
		gtk_widget_set_sensitive (priv->selected_chars_label, FALSE);
		gtk_widget_set_sensitive (priv->selected_chars_ns_label, FALSE);
	}

	if (chars == 0)
		lines = 0;

	tmp_str = g_strdup_printf("%d", lines);
	gtk_label_set_text (GTK_LABEL (priv->selected_lines_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", words);
	gtk_label_set_text (GTK_LABEL (priv->selected_words_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", chars);
	gtk_label_set_text (GTK_LABEL (priv->selected_chars_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", chars - white_chars);
	gtk_label_set_text (GTK_LABEL (priv->selected_chars_ns_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", bytes);
	gtk_label_set_text (GTK_LABEL (priv->selected_bytes_label), tmp_str);
	g_free (tmp_str);
}

static void
docinfo_dialog_response_cb (GtkDialog          *widget,
			    gint                res_id,
			    GeditDocinfoPlugin *plugin)
{
	GeditDocinfoPluginPrivate *priv;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	switch (res_id)
	{
		case GTK_RESPONSE_CLOSE:
		{
			gedit_debug_message (DEBUG_PLUGINS, "GTK_RESPONSE_CLOSE");

			gtk_widget_destroy (priv->dialog);

			break;
		}

		case GTK_RESPONSE_OK:
		{
			GeditDocument *doc;

			gedit_debug_message (DEBUG_PLUGINS, "GTK_RESPONSE_OK");

			doc = gedit_window_get_active_document (priv->window);

			update_document_info (plugin, doc);
			update_selection_info (plugin, doc);

			break;
		}
	}
}

static void
create_docinfo_dialog (GeditDocinfoPlugin *plugin)
{
	GeditDocinfoPluginPrivate *priv;
	GtkBuilder *builder;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	builder = gtk_builder_new ();
	gtk_builder_add_from_resource (builder, "/org/gnome/gedit/plugins/docinfo/ui/gedit-docinfo-plugin.ui", NULL);
	priv->dialog = GTK_WIDGET (gtk_builder_get_object (builder, "dialog"));
	priv->header_bar = GTK_WIDGET (gtk_builder_get_object (builder, "header_bar"));
	priv->words_label = GTK_WIDGET (gtk_builder_get_object (builder, "words_label"));
	priv->bytes_label = GTK_WIDGET (gtk_builder_get_object (builder, "bytes_label"));
	priv->lines_label = GTK_WIDGET (gtk_builder_get_object (builder, "lines_label"));
	priv->chars_label = GTK_WIDGET (gtk_builder_get_object (builder, "chars_label"));
	priv->chars_ns_label = GTK_WIDGET (gtk_builder_get_object (builder, "chars_ns_label"));
	priv->document_label = GTK_WIDGET (gtk_builder_get_object (builder, "document_label"));
	priv->document_words_label = GTK_WIDGET (gtk_builder_get_object (builder, "document_words_label"));
	priv->document_bytes_label = GTK_WIDGET (gtk_builder_get_object (builder, "document_bytes_label"));
	priv->document_lines_label = GTK_WIDGET (gtk_builder_get_object (builder, "document_lines_label"));
	priv->document_chars_label = GTK_WIDGET (gtk_builder_get_object (builder, "document_chars_label"));
	priv->document_chars_ns_label = GTK_WIDGET (gtk_builder_get_object (builder, "document_chars_ns_label"));
	priv->selection_label = GTK_WIDGET (gtk_builder_get_object (builder, "selection_label"));
	priv->selected_words_label = GTK_WIDGET (gtk_builder_get_object (builder, "selected_words_label"));
	priv->selected_bytes_label = GTK_WIDGET (gtk_builder_get_object (builder, "selected_bytes_label"));
	priv->selected_lines_label = GTK_WIDGET (gtk_builder_get_object (builder, "selected_lines_label"));
	priv->selected_chars_label = GTK_WIDGET (gtk_builder_get_object (builder, "selected_chars_label"));
	priv->selected_chars_ns_label = GTK_WIDGET (gtk_builder_get_object (builder, "selected_chars_ns_label"));
	g_object_unref (builder);

	gtk_dialog_set_default_response (GTK_DIALOG (priv->dialog),
					 GTK_RESPONSE_OK);
	gtk_window_set_transient_for (GTK_WINDOW (priv->dialog),
				      GTK_WINDOW (priv->window));

	g_signal_connect (priv->dialog,
			  "destroy",
			  G_CALLBACK (gtk_widget_destroyed),
			  &priv->dialog);
	g_signal_connect (priv->dialog,
			  "response",
			  G_CALLBACK (docinfo_dialog_response_cb),
			  plugin);

	/* We set this explictely with code since glade does not
	 * save the can_focus property when set to false :(
	 * Making sure the labels are not focusable is needed to
	 * prevent loosing the selection in the document when
	 * creating the dialog.
	 */
	gtk_widget_set_can_focus (priv->words_label, FALSE);
	gtk_widget_set_can_focus (priv->bytes_label, FALSE);
	gtk_widget_set_can_focus (priv->lines_label, FALSE);
	gtk_widget_set_can_focus (priv->chars_label, FALSE);
	gtk_widget_set_can_focus (priv->chars_ns_label, FALSE);
	gtk_widget_set_can_focus (priv->document_label, FALSE);
	gtk_widget_set_can_focus (priv->document_words_label, FALSE);
	gtk_widget_set_can_focus (priv->document_bytes_label, FALSE);
	gtk_widget_set_can_focus (priv->document_lines_label, FALSE);
	gtk_widget_set_can_focus (priv->document_chars_label, FALSE);
	gtk_widget_set_can_focus (priv->document_chars_ns_label, FALSE);
	gtk_widget_set_can_focus (priv->selection_label, FALSE);
	gtk_widget_set_can_focus (priv->selected_words_label, FALSE);
	gtk_widget_set_can_focus (priv->selected_bytes_label, FALSE);
	gtk_widget_set_can_focus (priv->selected_lines_label, FALSE);
	gtk_widget_set_can_focus (priv->selected_chars_label, FALSE);
	gtk_widget_set_can_focus (priv->selected_chars_ns_label, FALSE);
}

static void
docinfo_cb (GAction            *action,
            GVariant           *parameter,
            GeditDocinfoPlugin *plugin)
{
	GeditDocinfoPluginPrivate *priv;
	GeditDocument *doc;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	doc = gedit_window_get_active_document (priv->window);

	if (priv->dialog != NULL)
	{
		gtk_window_present (GTK_WINDOW (priv->dialog));
		gtk_widget_grab_focus (GTK_WIDGET (priv->dialog));
	}
	else
	{
		create_docinfo_dialog (plugin);
		gtk_widget_show (GTK_WIDGET (priv->dialog));
	}

	update_document_info (plugin, doc);
	update_selection_info (plugin, doc);
}

static void
gedit_docinfo_plugin_init (GeditDocinfoPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditDocinfoPlugin initializing");

	plugin->priv = gedit_docinfo_plugin_get_instance_private (plugin);
}

static void
gedit_docinfo_plugin_dispose (GObject *object)
{
	GeditDocinfoPlugin *plugin = GEDIT_DOCINFO_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS, "GeditDocinfoPlugin dispose");

	g_clear_object (&plugin->priv->action);
	g_clear_object (&plugin->priv->window);
	g_clear_object (&plugin->priv->menu_ext);
	g_clear_object (&plugin->priv->app);

	G_OBJECT_CLASS (gedit_docinfo_plugin_parent_class)->dispose (object);
}


static void
gedit_docinfo_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditDocinfoPlugin finalizing");

	G_OBJECT_CLASS (gedit_docinfo_plugin_parent_class)->finalize (object);
}

static void
gedit_docinfo_plugin_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
	GeditDocinfoPlugin *plugin = GEDIT_DOCINFO_PLUGIN (object);

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
gedit_docinfo_plugin_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
	GeditDocinfoPlugin *plugin = GEDIT_DOCINFO_PLUGIN (object);

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
update_ui (GeditDocinfoPlugin *plugin)
{
	GeditDocinfoPluginPrivate *priv;
	GeditView *view;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	view = gedit_window_get_active_view (priv->window);

	g_simple_action_set_enabled (G_SIMPLE_ACTION (priv->action), view != NULL);

	if (priv->dialog != NULL)
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (priv->dialog),
						   GTK_RESPONSE_OK,
						   (view != NULL));
	}
}

static void
gedit_docinfo_plugin_app_activate (GeditAppActivatable *activatable)
{
	GeditDocinfoPluginPrivate *priv;
	GMenuItem *item;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_DOCINFO_PLUGIN (activatable)->priv;

	priv->menu_ext = gedit_app_activatable_extend_menu (activatable, "tools-section");
	item = g_menu_item_new (_("_Document Statistics"), "win.docinfo");
	gedit_menu_extension_append_menu_item (priv->menu_ext, item);
	g_object_unref (item);
}

static void
gedit_docinfo_plugin_app_deactivate (GeditAppActivatable *activatable)
{
	GeditDocinfoPluginPrivate *priv;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_DOCINFO_PLUGIN (activatable)->priv;

	g_clear_object (&priv->menu_ext);
}

static void
gedit_docinfo_plugin_window_activate (GeditWindowActivatable *activatable)
{
	GeditDocinfoPluginPrivate *priv;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_DOCINFO_PLUGIN (activatable)->priv;

	priv->action = g_simple_action_new ("docinfo", NULL);
	g_signal_connect (priv->action, "activate",
	                  G_CALLBACK (docinfo_cb), activatable);
	g_action_map_add_action (G_ACTION_MAP (priv->window),
	                         G_ACTION (priv->action));

	update_ui (GEDIT_DOCINFO_PLUGIN (activatable));
}

static void
gedit_docinfo_plugin_window_deactivate (GeditWindowActivatable *activatable)
{
	GeditDocinfoPluginPrivate *priv;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_DOCINFO_PLUGIN (activatable)->priv;

	g_action_map_remove_action (G_ACTION_MAP (priv->window), "docinfo");
}

static void
gedit_docinfo_plugin_window_update_state (GeditWindowActivatable *activatable)
{
	gedit_debug (DEBUG_PLUGINS);

	update_ui (GEDIT_DOCINFO_PLUGIN (activatable));
}

static void
gedit_docinfo_plugin_class_init (GeditDocinfoPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_docinfo_plugin_dispose;
	object_class->finalize = gedit_docinfo_plugin_finalize;
	object_class->set_property = gedit_docinfo_plugin_set_property;
	object_class->get_property = gedit_docinfo_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
	g_object_class_override_property (object_class, PROP_APP, "app");
}

static void
gedit_app_activatable_iface_init (GeditAppActivatableInterface *iface)
{
	iface->activate = gedit_docinfo_plugin_app_activate;
	iface->deactivate = gedit_docinfo_plugin_app_deactivate;
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_docinfo_plugin_window_activate;
	iface->deactivate = gedit_docinfo_plugin_window_deactivate;
	iface->update_state = gedit_docinfo_plugin_window_update_state;
}

static void
gedit_docinfo_plugin_class_finalize (GeditDocinfoPluginClass *klass)
{
}


G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_docinfo_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_APP_ACTIVATABLE,
						    GEDIT_TYPE_DOCINFO_PLUGIN);
	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_DOCINFO_PLUGIN);
}

/* ex:set ts=8 noet: */
