/*
 * This file is part of gedit
 *
 * Copyright (C) 2006 - Paolo Borelli
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

#ifndef GEDIT_HISTORY_ENTRY_H
#define GEDIT_HISTORY_ENTRY_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_HISTORY_ENTRY (gedit_history_entry_get_type ())

G_DECLARE_FINAL_TYPE (GeditHistoryEntry, gedit_history_entry,
		      GEDIT, HISTORY_ENTRY,
		      GtkComboBoxText)

GtkWidget *	gedit_history_entry_new				(const gchar *history_id,
								 gboolean     enable_completion);

void		gedit_history_entry_prepend_text		(GeditHistoryEntry *entry,
								 const gchar       *text);

void		gedit_history_entry_set_history_length		(GeditHistoryEntry *entry,
								 guint              max_saved);

guint		gedit_history_entry_get_history_length		(GeditHistoryEntry *entry);

void		gedit_history_entry_set_enable_completion	(GeditHistoryEntry *entry,
								 gboolean           enable);

gboolean	gedit_history_entry_get_enable_completion	(GeditHistoryEntry *entry);

GtkWidget *	gedit_history_entry_get_entry			(GeditHistoryEntry *entry);

G_END_DECLS

#endif /* GEDIT_HISTORY_ENTRY_H */

/* ex:set ts=8 noet: */
