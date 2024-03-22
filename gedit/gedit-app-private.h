/*
 * This file is part of gedit
 *
 * Copyright (C) 2015 - Sébastien Wilmet <swilmet@gnome.org>
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

#ifndef GEDIT_APP_PRIVATE_H
#define GEDIT_APP_PRIVATE_H

#include "gedit-app.h"
#include "gedit-menu-extension.h"

G_BEGIN_DECLS

/* global print config */
GtkPageSetup *		_gedit_app_get_default_page_setup	(GeditApp *app);

void			_gedit_app_set_default_page_setup	(GeditApp     *app,
								 GtkPageSetup *page_setup);

GtkPrintSettings *	_gedit_app_get_default_print_settings	(GeditApp *app);

void			_gedit_app_set_default_print_settings	(GeditApp         *app,
								 GtkPrintSettings *settings);

GMenuModel *		_gedit_app_get_hamburger_menu		(GeditApp *app);

GMenuModel *		_gedit_app_get_notebook_menu		(GeditApp *app);

GMenuModel *		_gedit_app_get_tab_width_menu		(GeditApp *app);

GeditMenuExtension *	_gedit_app_extend_menu			(GeditApp    *app,
								 const gchar *extension_point);

void			_gedit_app_set_window_title		(GeditApp    *app,
								 GeditWindow *window,
								 const gchar *title);

gboolean		_gedit_app_process_window_event		(GeditApp    *app,
								 GeditWindow *window,
								 GdkEvent    *event);

G_END_DECLS

#endif /* GEDIT_APP_PRIVATE_H */

/* ex:set ts=8 noet: */
