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

	TeplSignalGroup *tab_signal_group;
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

#define MAX_TITLE_LENGTH 100

static GParamSpec *properties[N_PROPERTIES];

G_DEFINE_TYPE_WITH_PRIVATE (GeditWindowTitles, _gedit_window_titles, G_TYPE_OBJECT)

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
	tepl_signal_group_clear (&titles->priv->tab_signal_group);
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
tab_name_notify_cb (GeditTab          *tab,
		    GParamSpec        *pspec,
		    GeditWindowTitles *titles)
{
	_gedit_window_titles_update (titles);
}

static void
file_read_only_notify_cb (GtkSourceFile     *file,
			  GParamSpec        *pspec,
			  GeditWindowTitles *titles)
{
	_gedit_window_titles_update (titles);
}

static void
active_tab_changed (GeditWindowTitles *titles)
{
	GeditTab *active_tab;
	GeditDocument *active_doc;
	GtkSourceFile *active_file;

	if (titles->priv->window == NULL)
	{
		return;
	}

	_gedit_window_titles_update (titles);

	tepl_signal_group_clear (&titles->priv->tab_signal_group);
	tepl_signal_group_clear (&titles->priv->file_signal_group);

	active_tab = gedit_window_get_active_tab (titles->priv->window);
	if (active_tab == NULL)
	{
		return;
	}

	/* Connect to GeditTab signals */

	titles->priv->tab_signal_group = tepl_signal_group_new (G_OBJECT (active_tab));

	tepl_signal_group_add (titles->priv->tab_signal_group,
			       g_signal_connect (active_tab,
						 "notify::name",
						 G_CALLBACK (tab_name_notify_cb),
						 titles));

	/* Connect to GtkSourceFile signals */

	active_doc = gedit_tab_get_document (active_tab);
	active_file = gedit_document_get_file (active_doc);

	titles->priv->file_signal_group = tepl_signal_group_new (G_OBJECT (active_file));

	tepl_signal_group_add (titles->priv->file_signal_group,
			       g_signal_connect (active_file,
						 "notify::read-only",
						 G_CALLBACK (file_read_only_notify_cb),
						 titles));
}

static void
active_tab_changed_simple_cb (GeditWindow       *window,
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
				 "active-tab-changed-simple",
				 G_CALLBACK (active_tab_changed_simple_cb),
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

static void
set_titles (GeditWindowTitles *titles,
	    const gchar       *single_title,
	    const gchar       *title,
	    const gchar       *subtitle)
{
	if (g_set_str (&titles->priv->single_title, single_title))
	{
		g_object_notify_by_pspec (G_OBJECT (titles), properties[PROP_SINGLE_TITLE]);
	}

	if (g_set_str (&titles->priv->title, title))
	{
		g_object_notify_by_pspec (G_OBJECT (titles), properties[PROP_TITLE]);
	}

	if (g_set_str (&titles->priv->subtitle, subtitle))
	{
		g_object_notify_by_pspec (G_OBJECT (titles), properties[PROP_SUBTITLE]);
	}
}

void
_gedit_window_titles_update (GeditWindowTitles *titles)
{
	GeditDocument *doc;
	GtkSourceFile *file;
	gchar *name;
	gchar *dirname = NULL;
	gchar *main_title = NULL;
	gchar *title = NULL;
	gchar *subtitle = NULL;
	gint len;

	g_return_if_fail (GEDIT_IS_WINDOW_TITLES (titles));

	if (titles->priv->window == NULL)
	{
		return;
	}

	doc = gedit_window_get_active_document (titles->priv->window);

	if (doc == NULL)
	{
		set_titles (titles,
			    g_get_application_name (),
			    g_get_application_name (),
			    NULL);
		return;
	}

	file = gedit_document_get_file (doc);

	name = tepl_file_get_short_name (tepl_buffer_get_file (TEPL_BUFFER (doc)));
	len = g_utf8_strlen (name, -1);

	/* if the name is awfully long, truncate it and be done with it,
	 * otherwise also show the directory (ellipsized if needed)
	 */
	if (len > MAX_TITLE_LENGTH)
	{
		gchar *truncated_name;

		truncated_name = tepl_utils_str_middle_truncate (name, MAX_TITLE_LENGTH);
		g_free (name);
		name = truncated_name;
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
			dirname = tepl_utils_str_middle_truncate (str, MAX (20, MAX_TITLE_LENGTH - len));
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
					 name,
					 _("Read-Only"));

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

	set_titles (titles, main_title, title, subtitle);

	g_free (dirname);
	g_free (name);
	g_free (title);
	g_free (main_title);
}
