/* SPDX-FileCopyrightText: 2023 - Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "gedit-header-bar.h"

struct _GeditHeaderBarPrivate
{
	gint something;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditHeaderBar, _gedit_header_bar, G_TYPE_OBJECT)

static void
_gedit_header_bar_dispose (GObject *object)
{

	G_OBJECT_CLASS (_gedit_header_bar_parent_class)->dispose (object);
}

static void
_gedit_header_bar_class_init (GeditHeaderBarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = _gedit_header_bar_dispose;
}

static void
_gedit_header_bar_init (GeditHeaderBar *bar)
{
	bar->priv = _gedit_header_bar_get_instance_private (bar);
}

GeditHeaderBar *
_gedit_header_bar_new (void)
{
	return g_object_new (GEDIT_TYPE_HEADER_BAR, NULL);
}
