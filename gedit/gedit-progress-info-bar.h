/*
 * gedit-progress-info-bar.h
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GEDIT_PROGRESS_INFO_BAR_H
#define GEDIT_PROGRESS_INFO_BAR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_PROGRESS_INFO_BAR (gedit_progress_info_bar_get_type ())
GEDIT_API
G_DECLARE_FINAL_TYPE (GeditProgressInfoBar, gedit_progress_info_bar, GEDIT, PROGRESS_INFO_BAR, GtkInfoBar)

GEDIT_API
GtkWidget	*gedit_progress_info_bar_new			(const gchar          *icon_name,
								 const gchar          *markup,
								 gboolean              has_cancel);

GEDIT_API
void		 gedit_progress_info_bar_set_icon_name		(GeditProgressInfoBar *bar,
								 const gchar          *icon_name);

GEDIT_API
void		 gedit_progress_info_bar_set_markup		(GeditProgressInfoBar *bar,
								 const gchar          *markup);

GEDIT_API
void		 gedit_progress_info_bar_set_text		(GeditProgressInfoBar *bar,
								 const gchar          *text);

GEDIT_API
void		 gedit_progress_info_bar_set_fraction		(GeditProgressInfoBar *bar,
								 gdouble               fraction);

GEDIT_API
void		 gedit_progress_info_bar_pulse			(GeditProgressInfoBar *bar);

G_END_DECLS

#endif  /* GEDIT_PROGRESS_INFO_BAR_H  */

/* ex:set ts=8 noet: */
