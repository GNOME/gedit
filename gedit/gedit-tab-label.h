/*
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Paolo Borelli
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

#ifndef GEDIT_TAB_LABEL_H
#define GEDIT_TAB_LABEL_H

#include <gedit/gedit-tab.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_TAB_LABEL (gedit_tab_label_get_type ())

G_DECLARE_FINAL_TYPE (GeditTabLabel, gedit_tab_label,
		      GEDIT, TAB_LABEL,
		      GtkBox)

GtkWidget *	gedit_tab_label_new		(GeditTab *tab);

GeditTab *	gedit_tab_label_get_tab		(GeditTabLabel *tab_label);

G_END_DECLS

#endif /* GEDIT_TAB_LABEL_H */

/* ex:set ts=8 noet: */
