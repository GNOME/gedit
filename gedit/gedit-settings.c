/*
 * This file is part of gedit
 *
 * Copyright (C) 2002-2005 - Paolo Maggi
 * Copyright (C) 2009 - Ignacio Casal Quinteiro
 * Copyright (C) 2020 - Sébastien Wilmet <swilmet@gnome.org>
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
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "gedit-settings.h"
#include <gtksourceview/gtksource.h>
#include "gedit-app.h"

struct _GeditSettings
{
	GObject parent_instance;

	GSettings *settings_editor;
	GSettings *settings_ui;
	GSettings *settings_file_chooser_state;
	GSettings *settings_window_state;
};

static GeditSettings *singleton = NULL;

G_DEFINE_TYPE (GeditSettings, gedit_settings, G_TYPE_OBJECT)

static void
gedit_settings_dispose (GObject *object)
{
	GeditSettings *self = GEDIT_SETTINGS (object);

	g_clear_object (&self->settings_editor);
	g_clear_object (&self->settings_ui);
	g_clear_object (&self->settings_file_chooser_state);
	g_clear_object (&self->settings_window_state);

	G_OBJECT_CLASS (gedit_settings_parent_class)->dispose (object);
}

static void
gedit_settings_finalize (GObject *object)
{
	GeditSettings *self = GEDIT_SETTINGS (object);

	if (singleton == self)
	{
		singleton = NULL;
	}

	G_OBJECT_CLASS (gedit_settings_parent_class)->finalize (object);
}

static void
gedit_settings_class_init (GeditSettingsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_settings_dispose;
	object_class->finalize = gedit_settings_finalize;
}

static void
on_auto_save_changed (GSettings     *settings,
		      const gchar   *key,
		      GeditSettings *self)
{
	gboolean auto_save;
	GList *docs;
	GList *l;

	auto_save = g_settings_get_boolean (settings, key);

	docs = gedit_app_get_documents (GEDIT_APP (g_application_get_default ()));

	for (l = docs; l != NULL; l = l->next)
	{
		GeditTab *tab = gedit_tab_get_from_document (GEDIT_DOCUMENT (l->data));
		gedit_tab_set_auto_save_enabled (tab, auto_save);
	}

	g_list_free (docs);
}

static void
on_auto_save_interval_changed (GSettings     *settings,
			       const gchar   *key,
			       GeditSettings *self)
{
	guint auto_save_interval;
	GList *docs;
	GList *l;

	auto_save_interval = g_settings_get_uint (settings, key);

	docs = gedit_app_get_documents (GEDIT_APP (g_application_get_default ()));

	for (l = docs; l != NULL; l = l->next)
	{
		GeditTab *tab = gedit_tab_get_from_document (GEDIT_DOCUMENT (l->data));
		gedit_tab_set_auto_save_interval (tab, auto_save_interval);
	}

	g_list_free (docs);
}

static void
on_syntax_highlighting_changed (GSettings     *settings,
				const gchar   *key,
				GeditSettings *self)
{
	gboolean enable;
	GList *docs;
	GList *windows;
	GList *l;

	enable = g_settings_get_boolean (settings, key);

	docs = gedit_app_get_documents (GEDIT_APP (g_application_get_default ()));

	for (l = docs; l != NULL; l = l->next)
	{
		GtkSourceBuffer *buffer = GTK_SOURCE_BUFFER (l->data);
		gtk_source_buffer_set_highlight_syntax (buffer, enable);
	}

	g_list_free (docs);

	/* update the sensitivity of the Higlight Mode menu item */
	windows = gedit_app_get_main_windows (GEDIT_APP (g_application_get_default ()));

	for (l = windows; l != NULL; l = l->next)
	{
		GAction *action;

		action = g_action_map_lookup_action (G_ACTION_MAP (l->data), "highlight-mode");
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enable);
	}

	g_list_free (windows);
}

static void
gedit_settings_init (GeditSettings *self)
{
	self->settings_editor = g_settings_new ("org.gnome.gedit.preferences.editor");
	self->settings_ui = g_settings_new ("org.gnome.gedit.preferences.ui");
	self->settings_file_chooser_state = g_settings_new ("org.gnome.gedit.state.file-chooser");
	self->settings_window_state = g_settings_new ("org.gnome.gedit.state.window");

	g_signal_connect_object (self->settings_editor,
				 "changed::auto-save",
				 G_CALLBACK (on_auto_save_changed),
				 self,
				 0);

	g_signal_connect_object (self->settings_editor,
				 "changed::auto-save-interval",
				 G_CALLBACK (on_auto_save_interval_changed),
				 self,
				 0);

	g_signal_connect_object (self->settings_editor,
				 "changed::syntax-highlighting",
				 G_CALLBACK (on_syntax_highlighting_changed),
				 self,
				 0);
}

GeditSettings *
_gedit_settings_get_singleton (void)
{
	if (singleton == NULL)
	{
		singleton = g_object_new (GEDIT_TYPE_SETTINGS, NULL);
	}

	return singleton;
}

void
gedit_settings_unref_singleton (void)
{
	if (singleton != NULL)
	{
		g_object_unref (singleton);
	}

	/* singleton is not set to NULL here, it is set to NULL in
	 * gedit_settings_finalize() (i.e. when we are sure that the ref count
	 * reaches 0).
	 */
}

GSettings *
_gedit_settings_peek_editor_settings (GeditSettings *self)
{
	g_return_val_if_fail (GEDIT_IS_SETTINGS (self), NULL);

	return self->settings_editor;
}

GSettings *
_gedit_settings_peek_ui_settings (GeditSettings *self)
{
	g_return_val_if_fail (GEDIT_IS_SETTINGS (self), NULL);

	return self->settings_ui;
}

GSettings *
_gedit_settings_peek_file_chooser_state_settings (GeditSettings *self)
{
	g_return_val_if_fail (GEDIT_IS_SETTINGS (self), NULL);

	return self->settings_file_chooser_state;
}

GSettings *
_gedit_settings_peek_window_state_settings (GeditSettings *self)
{
	g_return_val_if_fail (GEDIT_IS_SETTINGS (self), NULL);

	return self->settings_window_state;
}

static gboolean
strv_is_empty (gchar **strv)
{
	if (strv == NULL || strv[0] == NULL)
	{
		return TRUE;
	}

	/* Contains one empty string. */
	if (strv[1] == NULL && strv[0][0] == '\0')
	{
		return TRUE;
	}

	return FALSE;
}

static GSList *
encoding_strv_to_list (const gchar * const *encoding_strv)
{
	GSList *list = NULL;
	gchar **p;

	for (p = (gchar **)encoding_strv; p != NULL && *p != NULL; p++)
	{
		const gchar *charset = *p;
		const GtkSourceEncoding *encoding;

		encoding = gtk_source_encoding_get_from_charset (charset);

		if (encoding != NULL &&
		    g_slist_find (list, encoding) == NULL)
		{
			list = g_slist_prepend (list, (gpointer)encoding);
		}
	}

	return g_slist_reverse (list);
}

/* Take in priority the candidate encodings from GSettings. If the gsetting is
 * empty, take the default candidates of GtkSourceEncoding.
 * Also, ensure that UTF-8 and the current locale encoding are present.
 * Returns: a list of GtkSourceEncodings. Free with g_slist_free().
 */
GSList *
gedit_settings_get_candidate_encodings (gboolean *default_candidates)
{
	const GtkSourceEncoding *utf8_encoding;
	const GtkSourceEncoding *current_encoding;
	GSettings *settings;
	gchar **settings_strv;
	GSList *candidates;

	utf8_encoding = gtk_source_encoding_get_utf8 ();
	current_encoding = gtk_source_encoding_get_current ();

	settings = g_settings_new ("org.gnome.gedit.preferences.encodings");

	settings_strv = g_settings_get_strv (settings, GEDIT_SETTINGS_CANDIDATE_ENCODINGS);

	if (strv_is_empty (settings_strv))
	{
		if (default_candidates != NULL)
		{
			*default_candidates = TRUE;
		}

		candidates = gtk_source_encoding_get_default_candidates ();
	}
	else
	{
		if (default_candidates != NULL)
		{
			*default_candidates = FALSE;
		}

		candidates = encoding_strv_to_list ((const gchar * const *) settings_strv);

		/* Ensure that UTF-8 is present. */
		if (utf8_encoding != current_encoding &&
		    g_slist_find (candidates, utf8_encoding) == NULL)
		{
			candidates = g_slist_prepend (candidates, (gpointer)utf8_encoding);
		}

		/* Ensure that the current locale encoding is present (if not
		 * present, it must be the first encoding).
		 */
		if (g_slist_find (candidates, current_encoding) == NULL)
		{
			candidates = g_slist_prepend (candidates, (gpointer)current_encoding);
		}
	}

	g_object_unref (settings);
	g_strfreev (settings_strv);
	return candidates;
}

/* ex:set ts=8 noet: */
