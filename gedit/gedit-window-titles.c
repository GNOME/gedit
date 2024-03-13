/* SPDX-FileCopyrightText: 2024 - Sébastien Wilmet
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "gedit-window-titles.h"

struct _GeditWindowTitlesPrivate
{
	/* Weak ref */
	GeditWindow *window;

	gchar *single_title;
	gchar *title;
	gchar *subtitle;
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

GeditWindowTitles *
_gedit_window_titles_new (GeditWindow *window)
{
	GeditWindowTitles *titles;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	titles = g_object_new (GEDIT_TYPE_WINDOW_TITLES, NULL);

	g_set_weak_pointer (&titles->priv->window, window);

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

void
_gedit_window_titles_update (GeditWindowTitles *titles)
{
	g_return_if_fail (GEDIT_IS_WINDOW_TITLES (titles));

	/* TODO */
}
