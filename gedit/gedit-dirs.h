/*
 * gedit-dirs.h
 * This file is part of gedit
 *
 * Copyright (C) 2008 Ignacio Casal Quinteiro
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


#ifndef GEDIT_DIRS_H
#define GEDIT_DIRS_H

#include <glib.h>

G_BEGIN_DECLS

/* This function must be called before starting gedit */
void		 gedit_dirs_init			(void);
/* This function must be called before exiting gedit */
void		 gedit_dirs_shutdown 			(void);


const gchar	*gedit_dirs_get_user_cache_dir		(void);

const gchar	*gedit_dirs_get_user_config_dir		(void);

const gchar	*gedit_dirs_get_user_data_dir		(void);

const gchar	*gedit_dirs_get_user_styles_dir		(void);

const gchar	*gedit_dirs_get_user_plugins_dir	(void);

const gchar	*gedit_dirs_get_gedit_locale_dir	(void);

const gchar	*gedit_dirs_get_gedit_lib_dir		(void);

const gchar	*gedit_dirs_get_gedit_plugins_dir	(void);

const gchar	*gedit_dirs_get_gedit_plugins_data_dir	(void);

G_END_DECLS

#endif /* GEDIT_DIRS_H */

/* ex:set ts=8 noet: */
