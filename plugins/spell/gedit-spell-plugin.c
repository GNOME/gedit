/*
 * gedit-spell-plugin.c
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2015-2016 Sébastien Wilmet
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

#include "gedit-spell-plugin.h"

#include <glib/gi18n.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-app.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>
#include <gspell/gspell.h>
#include <libpeas-gtk/peas-gtk-configurable.h>

#include "gedit-spell-app-activatable.h"

#define GEDIT_METADATA_ATTRIBUTE_SPELL_LANGUAGE "gedit-spell-language"
#define GEDIT_METADATA_ATTRIBUTE_SPELL_ENABLED  "gedit-spell-enabled"

#define SPELL_ENABLED_STR "1"
#define SPELL_BASE_SETTINGS	"org.gnome.gedit.plugins.spell"
#define SETTINGS_KEY_HIGHLIGHT_MISSPELLED "highlight-misspelled"

static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);
static void peas_gtk_configurable_iface_init (PeasGtkConfigurableInterface *iface);

struct _GeditSpellPluginPrivate
{
	GeditWindow *window;
	GSettings   *settings;
};

enum
{
	PROP_0,
	PROP_WINDOW
};

typedef struct _SpellConfigureWidget SpellConfigureWidget;

struct _SpellConfigureWidget
{
	GtkWidget *content;
	GtkWidget *highlight_button;

	GSettings *settings;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditSpellPlugin,
				gedit_spell_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init)
				G_IMPLEMENT_INTERFACE_DYNAMIC (PEAS_GTK_TYPE_CONFIGURABLE,
							       peas_gtk_configurable_iface_init)
				G_ADD_PRIVATE_DYNAMIC (GeditSpellPlugin))

static void
gedit_spell_plugin_set_property (GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
	GeditSpellPlugin *plugin = GEDIT_SPELL_PLUGIN (object);

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
gedit_spell_plugin_get_property (GObject    *object,
				 guint       prop_id,
				 GValue     *value,
				 GParamSpec *pspec)
{
	GeditSpellPlugin *plugin = GEDIT_SPELL_PLUGIN (object);

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
gedit_spell_plugin_dispose (GObject *object)
{
	GeditSpellPlugin *plugin = GEDIT_SPELL_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS, "GeditSpellPlugin disposing");

	g_clear_object (&plugin->priv->window);
	g_clear_object (&plugin->priv->settings);

	G_OBJECT_CLASS (gedit_spell_plugin_parent_class)->dispose (object);
}

static void
gedit_spell_plugin_class_init (GeditSpellPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = gedit_spell_plugin_set_property;
	object_class->get_property = gedit_spell_plugin_get_property;
	object_class->dispose = gedit_spell_plugin_dispose;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
gedit_spell_plugin_class_finalize (GeditSpellPluginClass *klass)
{
}

static void
gedit_spell_plugin_init (GeditSpellPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditSpellPlugin initializing");

	plugin->priv = gedit_spell_plugin_get_instance_private (plugin);
	plugin->priv->settings = g_settings_new (SPELL_BASE_SETTINGS);
}

static GspellChecker *
get_spell_checker (GeditDocument *doc)
{
	GspellTextBuffer *gspell_buffer;

	gspell_buffer = gspell_text_buffer_get_from_gtk_text_buffer (GTK_TEXT_BUFFER (doc));
	return gspell_text_buffer_get_spell_checker (gspell_buffer);
}

static const GspellLanguage *
get_language_from_metadata (GeditDocument *doc)
{
	const GspellLanguage *lang = NULL;
	gchar *language_code = NULL;

	language_code = gedit_document_get_metadata (doc, GEDIT_METADATA_ATTRIBUTE_SPELL_LANGUAGE);

	if (language_code != NULL)
	{
		lang = gspell_language_lookup (language_code);
		g_free (language_code);
	}

	return lang;
}

static void
check_spell_cb (GSimpleAction *action,
		GVariant      *parameter,
		gpointer       data)
{
	GeditSpellPlugin *plugin = GEDIT_SPELL_PLUGIN (data);
	GeditSpellPluginPrivate *priv;
	GeditView *view;
	GspellNavigator *navigator;
	GtkWidget *dialog;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	view = gedit_window_get_active_view (priv->window);
	g_return_if_fail (view != NULL);

	navigator = gspell_navigator_text_view_new (GTK_TEXT_VIEW (view));
	dialog = gspell_checker_dialog_new (GTK_WINDOW (priv->window), navigator);

	gtk_widget_show (dialog);
}

static void
language_dialog_response_cb (GtkDialog *dialog,
			     gint       response_id,
			     gpointer   user_data)
{
	if (response_id == GTK_RESPONSE_HELP)
	{
		gedit_app_show_help (GEDIT_APP (g_application_get_default ()),
				     GTK_WINDOW (dialog),
				     NULL,
				     "gedit-spellcheck");
		return;
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
set_language_cb (GSimpleAction *action,
		 GVariant      *parameter,
		 gpointer       data)
{
	GeditSpellPlugin *plugin = GEDIT_SPELL_PLUGIN (data);
	GeditSpellPluginPrivate *priv;
	GeditDocument *doc;
	GspellChecker *checker;
	const GspellLanguage *lang;
	GtkWidget *dialog;
	GtkWindowGroup *window_group;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	doc = gedit_window_get_active_document (priv->window);
	g_return_if_fail (doc != NULL);

	checker = get_spell_checker (doc);
	g_return_if_fail (checker != NULL);

	lang = gspell_checker_get_language (checker);

	dialog = gspell_language_chooser_dialog_new (GTK_WINDOW (priv->window),
						     lang,
						     GTK_DIALOG_MODAL |
						     GTK_DIALOG_DESTROY_WITH_PARENT);

	g_object_bind_property (dialog, "language",
				checker, "language",
				G_BINDING_DEFAULT);

	window_group = gedit_window_get_group (priv->window);

	gtk_window_group_add_window (window_group, GTK_WINDOW (dialog));

	gtk_dialog_add_button (GTK_DIALOG (dialog),
			       _("_Help"),
			       GTK_RESPONSE_HELP);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (language_dialog_response_cb),
			  NULL);

	gtk_widget_show (dialog);
}

static void
inline_checker_activate_cb (GSimpleAction *action,
			    GVariant      *parameter,
			    gpointer       data)
{
	GeditSpellPlugin *plugin = GEDIT_SPELL_PLUGIN (data);
	GeditSpellPluginPrivate *priv = plugin->priv;
	GVariant *state;
	gboolean active;
	GeditView *view;

	gedit_debug (DEBUG_PLUGINS);

	state = g_action_get_state (G_ACTION (action));
	g_return_if_fail (state != NULL);

	active = g_variant_get_boolean (state);
	g_variant_unref (state);

	/* We must toggle ourself the value. */
	active = !active;
	g_action_change_state (G_ACTION (action), g_variant_new_boolean (active));

	view = gedit_window_get_active_view (priv->window);
	if (view != NULL)
	{
		GeditDocument *doc;

		doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));

		/* Set metadata in the "activate" handler, not in "change-state"
		 * because "change-state" is called every time the state
		 * changes, not specifically when the user has changed the state
		 * herself. For example "change-state" is called to initialize
		 * the sate to the default value specified in the GActionEntry.
		 */
		gedit_document_set_metadata (doc,
					     GEDIT_METADATA_ATTRIBUTE_SPELL_ENABLED,
					     active ? SPELL_ENABLED_STR : NULL,
					     NULL);
	}
}

static void
inline_checker_change_state_cb (GSimpleAction *action,
				GVariant      *state,
				gpointer       data)
{
	GeditSpellPlugin *plugin = GEDIT_SPELL_PLUGIN (data);
	GeditSpellPluginPrivate *priv = plugin->priv;
	GeditView *view;
	gboolean active;

	gedit_debug (DEBUG_PLUGINS);

	active = g_variant_get_boolean (state);

	gedit_debug_message (DEBUG_PLUGINS, active ? "Inline Checker activated" : "Inline Checker deactivated");

	view = gedit_window_get_active_view (priv->window);
	if (view != NULL)
	{
		GspellTextView *gspell_view;

		gspell_view = gspell_text_view_get_from_gtk_text_view (GTK_TEXT_VIEW (view));
		gspell_text_view_set_inline_spell_checking (gspell_view, active);

		g_simple_action_set_state (action, g_variant_new_boolean (active));
	}
}

static void
update_ui (GeditSpellPlugin *plugin)
{
	GeditSpellPluginPrivate *priv;
	GeditTab *tab;
	GeditView *view = NULL;
	gboolean editable_view;
	GAction *check_spell_action;
	GAction *config_spell_action;
	GAction *inline_checker_action;

	gedit_debug (DEBUG_PLUGINS);

	priv = plugin->priv;

	tab = gedit_window_get_active_tab (priv->window);
	if (tab != NULL)
	{
		view = gedit_tab_get_view (tab);
	}

	editable_view = (view != NULL) && gtk_text_view_get_editable (GTK_TEXT_VIEW (view));

	check_spell_action = g_action_map_lookup_action (G_ACTION_MAP (priv->window),
	                                                 "check-spell");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (check_spell_action),
				     editable_view);

	config_spell_action = g_action_map_lookup_action (G_ACTION_MAP (priv->window),
	                                                  "config-spell");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (config_spell_action),
				     editable_view);

	inline_checker_action = g_action_map_lookup_action (G_ACTION_MAP (priv->window),
							    "inline-spell-checker");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (inline_checker_action),
				     editable_view);

	/* Update only on normal state to avoid garbage changes during e.g. file
	 * loading.
	 */
	if (tab != NULL &&
	    gedit_tab_get_state (tab) == GEDIT_TAB_STATE_NORMAL)
	{
		GspellTextView *gspell_view;
		gboolean inline_checking_enabled;

		gspell_view = gspell_text_view_get_from_gtk_text_view (GTK_TEXT_VIEW (view));
		inline_checking_enabled = gspell_text_view_get_inline_spell_checking (gspell_view);

		g_action_change_state (inline_checker_action,
				       g_variant_new_boolean (inline_checking_enabled));
	}
}

static void
setup_inline_checker_from_metadata (GeditSpellPlugin *plugin,
				    GeditView        *view)
{
	GeditDocument *doc;
	gboolean enabled;
	gchar *enabled_str;
	GspellTextView *gspell_view;
	GeditView *active_view;

	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));

	enabled = g_settings_get_boolean(plugin->priv->settings, SETTINGS_KEY_HIGHLIGHT_MISSPELLED);
	enabled_str = gedit_document_get_metadata (doc, GEDIT_METADATA_ATTRIBUTE_SPELL_ENABLED);
	if (enabled_str != NULL)
	{
		enabled = g_str_equal (enabled_str, SPELL_ENABLED_STR);
		g_free (enabled_str);
	}

	gspell_view = gspell_text_view_get_from_gtk_text_view (GTK_TEXT_VIEW (view));
	gspell_text_view_set_inline_spell_checking (gspell_view, enabled);

	/* In case that the view is the active one we mark the spell action */
	active_view = gedit_window_get_active_view (plugin->priv->window);

	if (active_view == view)
	{
		GAction *action;

		action = g_action_map_lookup_action (G_ACTION_MAP (plugin->priv->window),
		                                     "inline-spell-checker");
		g_action_change_state (action, g_variant_new_boolean (enabled));
	}
}

static void
language_notify_cb (GspellChecker *checker,
		    GParamSpec    *pspec,
		    GeditDocument *doc)
{
	const GspellLanguage *lang;
	const gchar *language_code;

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	lang = gspell_checker_get_language (checker);
	g_return_if_fail (lang != NULL);

	language_code = gspell_language_get_code (lang);
	g_return_if_fail (language_code != NULL);

	gedit_document_set_metadata (doc,
				     GEDIT_METADATA_ATTRIBUTE_SPELL_LANGUAGE, language_code,
				     NULL);
}

static void
on_document_loaded (GeditDocument    *doc,
		    GeditSpellPlugin *plugin)
{
	GspellChecker *checker;
	GeditTab *tab;
	GeditView *view;

	checker = get_spell_checker (doc);

	if (checker != NULL)
	{
		const GspellLanguage *lang;

		lang = get_language_from_metadata (doc);

		if (lang != NULL)
		{
			g_signal_handlers_block_by_func (checker, language_notify_cb, doc);
			gspell_checker_set_language (checker, lang);
			g_signal_handlers_unblock_by_func (checker, language_notify_cb, doc);
		}
	}

	tab = gedit_tab_get_from_document (doc);
	view = gedit_tab_get_view (tab);
	setup_inline_checker_from_metadata (plugin, view);
}

static void
on_document_saved (GeditDocument *doc,
		   gpointer       user_data)
{
	GeditTab *tab;
	GeditView *view;
	GspellChecker *checker;
	const gchar *language_code = NULL;
	GspellTextView *gspell_view;
	gboolean inline_checking_enabled;

	/* Make sure to save the metadata here too */

	checker = get_spell_checker (doc);

	if (checker != NULL)
	{
		const GspellLanguage *lang;

		lang = gspell_checker_get_language (checker);
		if (lang != NULL)
		{
			language_code = gspell_language_get_code (lang);
		}
	}

	tab = gedit_tab_get_from_document (doc);
	view = gedit_tab_get_view (tab);

	gspell_view = gspell_text_view_get_from_gtk_text_view (GTK_TEXT_VIEW (view));
	inline_checking_enabled = gspell_text_view_get_inline_spell_checking (gspell_view);

	gedit_document_set_metadata (doc,
	                             GEDIT_METADATA_ATTRIBUTE_SPELL_ENABLED,
				     inline_checking_enabled ? SPELL_ENABLED_STR : NULL,
	                             GEDIT_METADATA_ATTRIBUTE_SPELL_LANGUAGE,
	                             language_code,
	                             NULL);
}

static void
activate_spell_checking_in_view (GeditSpellPlugin *plugin,
				 GeditView        *view)
{
	GeditDocument *doc;

	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));

	/* It is possible that a GspellChecker has already been set, for example
	 * if a GeditTab has moved to another window.
	 */
	if (get_spell_checker (doc) == NULL)
	{
		const GspellLanguage *lang;
		GspellChecker *checker;
		GspellTextBuffer *gspell_buffer;

		lang = get_language_from_metadata (doc);
		checker = gspell_checker_new (lang);

		g_signal_connect_object (checker,
					 "notify::language",
					 G_CALLBACK (language_notify_cb),
					 doc,
					 0);

		gspell_buffer = gspell_text_buffer_get_from_gtk_text_buffer (GTK_TEXT_BUFFER (doc));
		gspell_text_buffer_set_spell_checker (gspell_buffer, checker);
		g_object_unref (checker);

		setup_inline_checker_from_metadata (plugin, view);
	}

	g_signal_connect_object (doc,
				 "loaded",
				 G_CALLBACK (on_document_loaded),
				 plugin,
				 0);

	g_signal_connect_object (doc,
				 "saved",
				 G_CALLBACK (on_document_saved),
				 plugin,
				 0);
}

static void
disconnect_view (GeditSpellPlugin *plugin,
		 GeditView        *view)
{
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	/* It should still be the same buffer as the one where the signal
	 * handlers were connected. If not, we assume that the old buffer is
	 * finalized. And it is anyway safe to call
	 * g_signal_handlers_disconnect_by_func() if no signal handlers are
	 * found.
	 */
	g_signal_handlers_disconnect_by_func (buffer, on_document_loaded, plugin);
	g_signal_handlers_disconnect_by_func (buffer, on_document_saved, plugin);
}

static void
deactivate_spell_checking_in_view (GeditSpellPlugin *plugin,
				   GeditView        *view)
{
	GtkTextBuffer *gtk_buffer;
	GspellTextBuffer *gspell_buffer;
	GspellTextView *gspell_view;

	disconnect_view (plugin, view);

	gtk_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gspell_buffer = gspell_text_buffer_get_from_gtk_text_buffer (gtk_buffer);
	gspell_text_buffer_set_spell_checker (gspell_buffer, NULL);

	gspell_view = gspell_text_view_get_from_gtk_text_view (GTK_TEXT_VIEW (view));
	gspell_text_view_set_inline_spell_checking (gspell_view, FALSE);
}

static void
tab_added_cb (GeditWindow      *window,
	      GeditTab         *tab,
	      GeditSpellPlugin *plugin)
{
	activate_spell_checking_in_view (plugin, gedit_tab_get_view (tab));
}

static void
tab_removed_cb (GeditWindow      *window,
		GeditTab         *tab,
		GeditSpellPlugin *plugin)
{
	/* Don't deactivate completely the spell checking in @tab, since the tab
	 * can be moved to another window and we don't want to loose the spell
	 * checking settings (they are not saved in metadata for unsaved
	 * documents).
	 */
	disconnect_view (plugin, gedit_tab_get_view (tab));
}

static void
gedit_spell_plugin_activate (GeditWindowActivatable *activatable)
{
	GeditSpellPlugin *plugin;
	GeditSpellPluginPrivate *priv;
	GList *views;
	GList *l;

	const GActionEntry action_entries[] =
	{
		{ "check-spell", check_spell_cb },
		{ "config-spell", set_language_cb },
		{ "inline-spell-checker",
		  inline_checker_activate_cb,
		  NULL,
		  "false",
		  inline_checker_change_state_cb }
	};

	gedit_debug (DEBUG_PLUGINS);

	plugin = GEDIT_SPELL_PLUGIN (activatable);
	priv = plugin->priv;

	g_action_map_add_action_entries (G_ACTION_MAP (priv->window),
	                                 action_entries,
	                                 G_N_ELEMENTS (action_entries),
	                                 activatable);

	update_ui (plugin);

	views = gedit_window_get_views (priv->window);
	for (l = views; l != NULL; l = l->next)
	{
		activate_spell_checking_in_view (plugin, GEDIT_VIEW (l->data));
	}

	g_signal_connect (priv->window,
			  "tab-added",
			  G_CALLBACK (tab_added_cb),
			  activatable);

	g_signal_connect (priv->window,
			  "tab-removed",
			  G_CALLBACK (tab_removed_cb),
			  activatable);
}

static void
gedit_spell_plugin_deactivate (GeditWindowActivatable *activatable)
{
	GeditSpellPlugin *plugin;
	GeditSpellPluginPrivate *priv;
	GList *views;
	GList *l;

	gedit_debug (DEBUG_PLUGINS);

	plugin = GEDIT_SPELL_PLUGIN (activatable);
	priv = plugin->priv;

	g_action_map_remove_action (G_ACTION_MAP (priv->window), "check-spell");
	g_action_map_remove_action (G_ACTION_MAP (priv->window), "config-spell");
	g_action_map_remove_action (G_ACTION_MAP (priv->window), "inline-spell-checker");

	g_signal_handlers_disconnect_by_func (priv->window, tab_added_cb, activatable);
	g_signal_handlers_disconnect_by_func (priv->window, tab_removed_cb, activatable);

	views = gedit_window_get_views (priv->window);
	for (l = views; l != NULL; l = l->next)
	{
		deactivate_spell_checking_in_view (plugin, GEDIT_VIEW (l->data));
	}
}

static void
gedit_spell_plugin_update_state (GeditWindowActivatable *activatable)
{
	gedit_debug (DEBUG_PLUGINS);

	update_ui (GEDIT_SPELL_PLUGIN (activatable));
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_spell_plugin_activate;
	iface->deactivate = gedit_spell_plugin_deactivate;
	iface->update_state = gedit_spell_plugin_update_state;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_spell_plugin_register_type (G_TYPE_MODULE (module));
	gedit_spell_app_activatable_register (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_SPELL_PLUGIN);
	peas_object_module_register_extension_type (module,
						    PEAS_GTK_TYPE_CONFIGURABLE,
						    GEDIT_TYPE_SPELL_PLUGIN);
}

static void
highlight_button_toggled (GtkToggleButton     *button,
				 SpellConfigureWidget *widget)
{
	gboolean status = gtk_toggle_button_get_active (button);
	g_settings_set_boolean(widget->settings, SETTINGS_KEY_HIGHLIGHT_MISSPELLED, status);
}

static void
configure_widget_destroyed (GtkWidget *widget,
			    gpointer   data)
{
	SpellConfigureWidget *conf_widget = (SpellConfigureWidget *)data;

	gedit_debug (DEBUG_PLUGINS);

	g_object_unref (conf_widget->settings);
	g_slice_free (SpellConfigureWidget, data);

	gedit_debug_message (DEBUG_PLUGINS, "END");
}

static SpellConfigureWidget *
get_configure_widget (GeditSpellPlugin *plugin)
{
	SpellConfigureWidget *widget;
	GtkBuilder *builder;
	gboolean status;
	const gchar *root_objects[] = {
		"spell_dialog_content",
		NULL
	};

	gedit_debug (DEBUG_PLUGINS);

	widget = g_slice_new (SpellConfigureWidget);
	widget->settings = g_object_ref (plugin->priv->settings);

	builder = gtk_builder_new ();
	gtk_builder_add_objects_from_resource (builder, "/org/gnome/gedit/plugins/spell/ui/gedit-spell-setup-dialog.ui",
	                                       (gchar **)root_objects, NULL);
	widget->content = GTK_WIDGET (gtk_builder_get_object (builder, "spell_dialog_content"));
	g_object_ref (widget->content);

	widget->highlight_button = GTK_WIDGET (gtk_builder_get_object (builder, "highlight_button"));
	g_object_unref (builder);

	status = g_settings_get_boolean (widget->settings, SETTINGS_KEY_HIGHLIGHT_MISSPELLED);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->highlight_button), status);

	g_signal_connect (widget->highlight_button,
		          "toggled",
		          G_CALLBACK (highlight_button_toggled),
		          widget);

	g_signal_connect (widget->content,
			  "destroy",
			  G_CALLBACK (configure_widget_destroyed),
			  widget);

	return widget;
}


static GtkWidget *
gedit_spell_plugin_create_configure_widget (PeasGtkConfigurable *configurable)
{
	SpellConfigureWidget *widget;

	widget = get_configure_widget (GEDIT_SPELL_PLUGIN (configurable));

	return widget->content;
}

static void
peas_gtk_configurable_iface_init (PeasGtkConfigurableInterface *iface)
{
	iface->create_configure_widget = gedit_spell_plugin_create_configure_widget;
}

/* ex:set ts=8 noet: */
