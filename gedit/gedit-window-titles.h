/* SPDX-FileCopyrightText: 2024 - Sébastien Wilmet
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef GEDIT_WINDOW_TITLES_H
#define GEDIT_WINDOW_TITLES_H

#include "gedit-window.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_WINDOW_TITLES             (_gedit_window_titles_get_type ())
#define GEDIT_WINDOW_TITLES(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_WINDOW_TITLES, GeditWindowTitles))
#define GEDIT_WINDOW_TITLES_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_WINDOW_TITLES, GeditWindowTitlesClass))
#define GEDIT_IS_WINDOW_TITLES(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_WINDOW_TITLES))
#define GEDIT_IS_WINDOW_TITLES_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_WINDOW_TITLES))
#define GEDIT_WINDOW_TITLES_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_WINDOW_TITLES, GeditWindowTitlesClass))

typedef struct _GeditWindowTitles         GeditWindowTitles;
typedef struct _GeditWindowTitlesClass    GeditWindowTitlesClass;
typedef struct _GeditWindowTitlesPrivate  GeditWindowTitlesPrivate;

struct _GeditWindowTitles
{
	GObject parent;

	GeditWindowTitlesPrivate *priv;
};

struct _GeditWindowTitlesClass
{
	GObjectClass parent_class;
};

GType			_gedit_window_titles_get_type		(void);

GeditWindowTitles *	_gedit_window_titles_new		(GeditWindow *window);

const gchar *		_gedit_window_titles_get_single_title	(GeditWindowTitles *titles);

const gchar *		_gedit_window_titles_get_title		(GeditWindowTitles *titles);

const gchar *		_gedit_window_titles_get_subtitle	(GeditWindowTitles *titles);

G_END_DECLS

#endif /* GEDIT_WINDOW_TITLES_H */
