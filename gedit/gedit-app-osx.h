/*
 * gedit-app-osx.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Jesse van den Kieboom
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef GEDIT_APP_OSX_H
#define GEDIT_APP_OSX_H

#include "gedit-app.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_APP_OSX (gedit_app_osx_get_type ())

G_DECLARE_FINAL_TYPE (GeditAppOSX, gedit_app_osx, GEDIT, APP_OSX, GeditApp)

gboolean	 gedit_app_osx_show_url			(GeditAppOSX *app,
							 const gchar *url);

G_END_DECLS

#endif /* GEDIT_APP_OSX_H */

/* ex:set ts=8 noet: */
