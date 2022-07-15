/*
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
 * Copyright (C) 2014 - Paolo Borelli
 * Copyright (C) 2014 - Jesse van den Kieboom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANWINDOWILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GEDIT_RECENT_OSX_H
#define GEDIT_RECENT_OSX_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* TODO: this code can be simplified, the struct can be made private, the dead
 * code can be removed, etc.
 */

typedef struct
{
	GtkRecentManager *manager;
	GtkRecentFilter *filter;

	gint limit;
	gchar *substring_filter;

	guint show_private : 1;
	guint show_not_found : 1;
	guint local_only : 1;
} GeditRecentConfiguration;

void		 gedit_recent_configuration_init_default (GeditRecentConfiguration *config);
void		 gedit_recent_configuration_destroy	 (GeditRecentConfiguration *config);
GList		*gedit_recent_get_items			 (GeditRecentConfiguration *config);

G_END_DECLS

#endif /* GEDIT_RECENT_OSX_H */

/* ex:set ts=8 noet: */
