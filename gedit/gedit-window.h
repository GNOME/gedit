/*
 * gedit-window.h
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
 * MERCHANWINDOWILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GEDIT_WINDOW_H
#define GEDIT_WINDOW_H

#include <tepl/tepl.h>
#include <gedit/gedit-tab.h>
#include <gedit/gedit-message-bus.h>

G_BEGIN_DECLS

typedef enum
{
	GEDIT_WINDOW_STATE_NORMAL		= 0,
	GEDIT_WINDOW_STATE_SAVING		= 1 << 1,
	GEDIT_WINDOW_STATE_PRINTING		= 1 << 2,
	GEDIT_WINDOW_STATE_LOADING		= 1 << 3,
	GEDIT_WINDOW_STATE_ERROR		= 1 << 4
} GeditWindowState;

#define GEDIT_TYPE_WINDOW              (gedit_window_get_type())
#define GEDIT_WINDOW(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_WINDOW, GeditWindow))
#define GEDIT_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_WINDOW, GeditWindowClass))
#define GEDIT_IS_WINDOW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_WINDOW))
#define GEDIT_IS_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_WINDOW))
#define GEDIT_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_WINDOW, GeditWindowClass))

typedef struct _GeditWindow        GeditWindow;
typedef struct _GeditWindowClass   GeditWindowClass;
typedef struct _GeditWindowPrivate GeditWindowPrivate;

struct _GeditWindow
{
	GtkApplicationWindow window;

	/*< private > */
	GeditWindowPrivate *priv;
};

struct _GeditWindowClass
{
	GtkApplicationWindowClass parent_class;

	/* Signals */
	void	 (* tab_added)      	(GeditWindow *window,
					 GeditTab    *tab);
	void	 (* tab_removed)    	(GeditWindow *window,
					 GeditTab    *tab);
	void	 (* tabs_reordered) 	(GeditWindow *window);
	void	 (* active_tab_changed)	(GeditWindow *window,
				     	 GeditTab    *tab);
	void	 (* active_tab_state_changed)
					(GeditWindow *window);
};

/* Public methods */
GType 		 gedit_window_get_type 			(void) G_GNUC_CONST;

GeditTab	*gedit_window_create_tab		(GeditWindow         *window,
							 gboolean             jump_to);

void		 gedit_window_close_tab			(GeditWindow         *window,
							 GeditTab            *tab);

void		 gedit_window_close_all_tabs		(GeditWindow         *window);

void		 gedit_window_close_tabs		(GeditWindow         *window,
							 const GList         *tabs);

GeditTab	*gedit_window_get_active_tab		(GeditWindow         *window);

void		 gedit_window_set_active_tab		(GeditWindow         *window,
							 GeditTab            *tab);

/* Helper functions */
GeditView	*gedit_window_get_active_view		(GeditWindow         *window);
GeditDocument	*gedit_window_get_active_document	(GeditWindow         *window);

/* Returns a newly allocated list with all the documents in the window */
GList		*gedit_window_get_documents		(GeditWindow         *window);

/* Returns a newly allocated list with all the documents that need to be
   saved before closing the window */
GList		*gedit_window_get_unsaved_documents 	(GeditWindow         *window);

/* Returns a newly allocated list with all the views in the window */
GList		*gedit_window_get_views			(GeditWindow         *window);

GtkWindowGroup  *gedit_window_get_group			(GeditWindow         *window);

TeplPanel	*gedit_window_get_side_panel		(GeditWindow         *window);

GtkWidget	*gedit_window_get_bottom_panel		(GeditWindow         *window);

GtkWidget	*gedit_window_get_statusbar		(GeditWindow         *window);

GeditWindowState gedit_window_get_state 		(GeditWindow         *window);

GeditTab        *gedit_window_get_tab_from_location	(GeditWindow         *window,
							 GFile               *location);

/* Message bus */
GeditMessageBus	*gedit_window_get_message_bus		(GeditWindow         *window);

/*
 * Non exported functions
 */
GtkWidget	*_gedit_window_get_notebook		(GeditWindow         *window);

GMenuModel	*_gedit_window_get_hamburger_menu	(GeditWindow         *window);

GeditWindow	*_gedit_window_move_tab_to_new_window	(GeditWindow         *window,
							 GeditTab            *tab);
void             _gedit_window_move_tab_to_new_tab_group(GeditWindow         *window,
                                                         GeditTab            *tab);
gboolean	 _gedit_window_is_removing_tabs		(GeditWindow         *window);

const gchar	*_gedit_window_get_file_chooser_folder_uri
							(GeditWindow          *window,
							 GtkFileChooserAction  action);

void		 _gedit_window_set_file_chooser_folder_uri
							(GeditWindow          *window,
							 GtkFileChooserAction  action,
							 const gchar          *folder_uri);

void		 _gedit_window_fullscreen		(GeditWindow         *window);

void		 _gedit_window_unfullscreen		(GeditWindow         *window);

gboolean	 _gedit_window_is_fullscreen		(GeditWindow         *window);

GList		*_gedit_window_get_all_tabs		(GeditWindow         *window);

GFile		*_gedit_window_pop_last_closed_doc	(GeditWindow         *window);

G_END_DECLS

#endif  /* GEDIT_WINDOW_H  */

/* ex:set ts=8 noet: */
