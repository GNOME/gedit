/* SPDX-FileCopyrightText: 2024 - Sébastien Wilmet
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "gedit-window-titles.h"
#include <glib/gi18n.h>
#include "gedit-utils.h"

struct _GeditWindowTitlesPrivate
{
	/* Weak ref */
	GeditWindow *window;

	gchar *single_title;
	gchar *title;
	gchar *subtitle;

	TeplSignalGroup *buffer_signal_group;
	TeplSignalGroup *file_signal_group;
};

enum
{
	PROP_0,
	PROP_SINGLE_TITLE,
	PROP_TITLE,
	PROP_SUBTITLE,
	N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

G_DEFINE_TYPE_WITH_PRIVATE (GeditWindowTitles, _gedit_window_titles, G_TYPE_OBJECT)

static void
set_single_title (GeditWindowTitles *titles,
		  const gchar       *single_title)
{
	if (g_set_str (&titles->priv->single_title, single_title))
	{
		g_object_notify_by_pspec (G_OBJECT (titles), properties[PROP_SINGLE_TITLE]);
	}
}

static void
set_title (GeditWindowTitles *titles,
	   const gchar       *title)
{
	if (g_set_str (&titles->priv->title, title))
	{
		g_object_notify_by_pspec (G_OBJECT (titles), properties[PROP_TITLE]);
	}
}

static void
set_subtitle (GeditWindowTitles *titles,
	      const gchar       *subtitle)
{
	if (g_set_str (&titles->priv->subtitle, subtitle))
	{
		g_object_notify_by_pspec (G_OBJECT (titles), properties[PROP_SUBTITLE]);
	}
}

static void
update_single_title (GeditWindowTitles *titles,
		     const gchar       *title,
		     const gchar       *dirname)
{
	GString *string;

	string = g_string_new (title);

	if (dirname != NULL)
	{
		g_string_append_printf (string, " (%s)", dirname);
	}

	g_string_append_printf (string, " - %s", g_get_application_name ());

	set_single_title (titles, string->str);

	g_string_free (string, TRUE);
}

static void
update_titles (GeditWindowTitles *titles)
{
	GeditDocument *doc;
	GtkSourceFile *source_file;
	TeplFile *tepl_file;
	GFile *location;
	gchar *short_title;
	gchar *title;
	gchar *dirname = NULL;

	if (titles->priv->window == NULL)
	{
		return;
	}

	doc = gedit_window_get_active_document (titles->priv->window);

	if (doc == NULL)
	{
		set_single_title (titles, g_get_application_name ());
		set_title (titles, g_get_application_name ());
		set_subtitle (titles, NULL);
		return;
	}

	short_title = tepl_buffer_get_short_title (TEPL_BUFFER (doc));

	source_file = gedit_document_get_file (doc);
	if (gtk_source_file_is_readonly (source_file))
	{
		title = g_strdup_printf ("%s [%s]", short_title, _("Read-Only"));
	}
	else
	{
		title = g_strdup (short_title);
	}

	tepl_file = tepl_buffer_get_file (TEPL_BUFFER (doc));
	location = tepl_file_get_location (tepl_file);
	if (location != NULL)
	{
		dirname = _gedit_utils_location_get_dirname_for_display (location);
	}

	update_single_title (titles, title, dirname);
	set_title (titles, title);
	set_subtitle (titles, dirname);

	g_free (short_title);
	g_free (title);
	g_free (dirname);
}

static void
_gedit_window_titles_get_property (GObject    *object,
				   guint       prop_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
	GeditWindowTitles *titles = GEDIT_WINDOW_TITLES (object);

	switch (prop_id)
	{
		case PROP_SINGLE_TITLE:
			g_value_set_string (value, _gedit_window_titles_get_single_title (titles));
			break;

		case PROP_TITLE:
			g_value_set_string (value, _gedit_window_titles_get_title (titles));
			break;

		case PROP_SUBTITLE:
			g_value_set_string (value, _gedit_window_titles_get_subtitle (titles));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
_gedit_window_titles_dispose (GObject *object)
{
	GeditWindowTitles *titles = GEDIT_WINDOW_TITLES (object);

	g_clear_weak_pointer (&titles->priv->window);
	tepl_signal_group_clear (&titles->priv->buffer_signal_group);
	tepl_signal_group_clear (&titles->priv->file_signal_group);

	G_OBJECT_CLASS (_gedit_window_titles_parent_class)->dispose (object);
}

static void
_gedit_window_titles_finalize (GObject *object)
{
	GeditWindowTitles *titles = GEDIT_WINDOW_TITLES (object);

	g_free (titles->priv->single_title);
	g_free (titles->priv->title);
	g_free (titles->priv->subtitle);

	G_OBJECT_CLASS (_gedit_window_titles_parent_class)->finalize (object);
}

static void
_gedit_window_titles_class_init (GeditWindowTitlesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = _gedit_window_titles_get_property;
	object_class->dispose = _gedit_window_titles_dispose;
	object_class->finalize = _gedit_window_titles_finalize;

	properties[PROP_SINGLE_TITLE] =
		g_param_spec_string ("single-title",
				     "single-title",
				     "",
				     NULL,
				     G_PARAM_READABLE |
				     G_PARAM_STATIC_STRINGS);

	properties[PROP_TITLE] =
		g_param_spec_string ("title",
				     "title",
				     "",
				     NULL,
				     G_PARAM_READABLE |
				     G_PARAM_STATIC_STRINGS);

	properties[PROP_SUBTITLE] =
		g_param_spec_string ("subtitle",
				     "subtitle",
				     "",
				     NULL,
				     G_PARAM_READABLE |
				     G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
_gedit_window_titles_init (GeditWindowTitles *titles)
{
	titles->priv = _gedit_window_titles_get_instance_private (titles);
}

static void
buffer_short_title_notify_cb (TeplBuffer        *buffer,
			      GParamSpec        *pspec,
			      GeditWindowTitles *titles)
{
	update_titles (titles);
}

static void
file_read_only_notify_cb (GtkSourceFile     *file,
			  GParamSpec        *pspec,
			  GeditWindowTitles *titles)
{
	update_titles (titles);
}

static void
active_tab_changed (GeditWindowTitles *titles)
{
	GeditDocument *active_doc;
	GtkSourceFile *active_file;

	if (titles->priv->window == NULL)
	{
		return;
	}

	update_titles (titles);

	tepl_signal_group_clear (&titles->priv->buffer_signal_group);
	tepl_signal_group_clear (&titles->priv->file_signal_group);

	active_doc = gedit_window_get_active_document (titles->priv->window);
	if (active_doc == NULL)
	{
		return;
	}

	/* Connect to GeditDocument signals */

	titles->priv->buffer_signal_group = tepl_signal_group_new (G_OBJECT (active_doc));

	tepl_signal_group_add (titles->priv->buffer_signal_group,
			       g_signal_connect (active_doc,
						 "notify::tepl-short-title",
						 G_CALLBACK (buffer_short_title_notify_cb),
						 titles));

	/* Connect to GtkSourceFile signals */

	active_file = gedit_document_get_file (active_doc);

	titles->priv->file_signal_group = tepl_signal_group_new (G_OBJECT (active_file));

	tepl_signal_group_add (titles->priv->file_signal_group,
			       g_signal_connect (active_file,
						 "notify::read-only",
						 G_CALLBACK (file_read_only_notify_cb),
						 titles));
}

static void
active_tab_changed_cb (GeditWindow       *window,
		       GeditWindowTitles *titles)
{
	active_tab_changed (titles);
}

GeditWindowTitles *
_gedit_window_titles_new (GeditWindow *window)
{
	GeditWindowTitles *titles;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	titles = g_object_new (GEDIT_TYPE_WINDOW_TITLES, NULL);

	g_set_weak_pointer (&titles->priv->window, window);

	g_signal_connect_object (titles->priv->window,
				 "active-tab-changed",
				 G_CALLBACK (active_tab_changed_cb),
				 titles,
				 G_CONNECT_DEFAULT);

	active_tab_changed (titles);

	return titles;
}

const gchar *
_gedit_window_titles_get_single_title (GeditWindowTitles *titles)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW_TITLES (titles), NULL);
	return titles->priv->single_title;
}

const gchar *
_gedit_window_titles_get_title (GeditWindowTitles *titles)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW_TITLES (titles), NULL);
	return titles->priv->title;
}

const gchar *
_gedit_window_titles_get_subtitle (GeditWindowTitles *titles)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW_TITLES (titles), NULL);
	return titles->priv->subtitle;
}
