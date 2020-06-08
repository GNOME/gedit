/*
 * This file is part of gedit
 *
 * Copyright (C) 2020 Sébastien Wilmet <swilmet@gnome.org>
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

#ifndef GEDIT_FILE_CHOOSER_H
#define GEDIT_FILE_CHOOSER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL
void	_gedit_file_chooser_setup_filters	(GtkFileChooser *chooser);

G_END_DECLS

#endif /* GEDIT_FILE_CHOOSER_H */
