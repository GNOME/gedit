/*
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

#ifndef GEDIT_IO_ERROR_INFO_BAR_H
#define GEDIT_IO_ERROR_INFO_BAR_H

#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

GtkWidget *	gedit_io_loading_error_info_bar_new			(GFile                   *location,
									 const GtkSourceEncoding *encoding,
									 const GError            *error);

GtkWidget *	gedit_unrecoverable_reverting_error_info_bar_new	(GFile        *location,
									 const GError *error);

GtkWidget *	gedit_conversion_error_while_saving_info_bar_new	(GFile                   *location,
									 const GtkSourceEncoding *encoding);

const GtkSourceEncoding *
		gedit_conversion_error_info_bar_get_encoding		(GtkWidget *info_bar);

GtkWidget *	gedit_unrecoverable_saving_error_info_bar_new		(GFile        *location,
									 const GError *error);

G_END_DECLS

#endif /* GEDIT_IO_ERROR_INFO_BAR_H */

/* ex:set ts=8 noet: */
