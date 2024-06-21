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

/**
 * GeditWindowState:
 * @GEDIT_WINDOW_STATE_NORMAL: No flags.
 * @GEDIT_WINDOW_STATE_SAVING: A tab is in saving state.
 * @GEDIT_WINDOW_STATE_PRINTING: There is a printing operation on a tab.
 * @GEDIT_WINDOW_STATE_LOADING: A tab is in loading or reverting state.
 * @GEDIT_WINDOW_STATE_ERROR: A tab is in an error state.
 *
 * Flags for the state of a #GeditWindow. The enumerators are flags and can be
 * combined. #GeditWindow combines and summarizes the state of its #GeditTab's
 * into one #GeditWindowState value. See #GeditTabState for the more precise
 * states.
 */
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

	GeditWindowPrivate *priv;
};

struct _GeditWindowClass
{
	GtkApplicationWindowClass parent_class;

	/* Signals */

	void	(* tab_added)	(GeditWindow *window,
				 GeditTab    *tab);

	void	(* tab_removed)	(GeditWindow *window,
				 GeditTab    *tab);
};

GType		gedit_window_get_type			(void) G_GNUC_CONST;

GeditTab *	gedit_window_create_tab			(GeditWindow *window,
							 gboolean     jump_to);

void		gedit_window_close_tab			(GeditWindow *window,
							 GeditTab    *tab);

void		gedit_window_close_all_tabs		(GeditWindow *window);

void		gedit_window_close_tabs			(GeditWindow *window,
							 const GList *tabs);

GeditTab *	gedit_window_get_active_tab		(GeditWindow *window);

void		gedit_window_set_active_tab		(GeditWindow *window,
							 GeditTab    *tab);

GeditView *	gedit_window_get_active_view		(GeditWindow *window);

GeditDocument *	gedit_window_get_active_document	(GeditWindow *window);

GList *		gedit_window_get_documents		(GeditWindow *window);

GList *		gedit_window_get_unsaved_documents	(GeditWindow *window);

GList *		gedit_window_get_views			(GeditWindow *window);

GtkWindowGroup *gedit_window_get_group			(GeditWindow *window);

TeplPanel *	gedit_window_get_side_panel		(GeditWindow *window);

TeplPanel *	gedit_window_get_bottom_panel		(GeditWindow *window);

GtkWidget *	gedit_window_get_statusbar		(GeditWindow *window);

GeditWindowState gedit_window_get_state 		(GeditWindow *window);

GeditTab *	gedit_window_get_tab_from_location	(GeditWindow *window,
							 GFile       *location);

GeditMessageBus *gedit_window_get_message_bus		(GeditWindow *window);

/* Non exported functions */

G_GNUC_INTERNAL
GtkWidget *	_gedit_window_get_notebook		(GeditWindow *window);

G_GNUC_INTERNAL
GMenuModel *	_gedit_window_get_hamburger_menu	(GeditWindow *window);

G_GNUC_INTERNAL
GeditWindow *	_gedit_window_move_tab_to_new_window	(GeditWindow *window,
							 GeditTab    *tab);

G_GNUC_INTERNAL
void		_gedit_window_move_tab_to_new_tab_group	(GeditWindow *window,
							 GeditTab    *tab);

G_GNUC_INTERNAL
gboolean	_gedit_window_is_removing_tabs		(GeditWindow *window);

G_GNUC_INTERNAL
const gchar *	_gedit_window_get_file_chooser_folder_uri
							(GeditWindow          *window,
							 GtkFileChooserAction  action);

G_GNUC_INTERNAL
void		_gedit_window_set_file_chooser_folder_uri
							(GeditWindow          *window,
							 GtkFileChooserAction  action,
							 const gchar          *folder_uri);

G_GNUC_INTERNAL
void		_gedit_window_fullscreen		(GeditWindow *window);

G_GNUC_INTERNAL
void		_gedit_window_unfullscreen		(GeditWindow *window);

G_GNUC_INTERNAL
gboolean	_gedit_window_is_fullscreen		(GeditWindow *window);

G_GNUC_INTERNAL
GList *		_gedit_window_get_all_tabs		(GeditWindow *window);

G_GNUC_INTERNAL
GFile *		_gedit_window_pop_last_closed_doc	(GeditWindow *window);

G_END_DECLS

#endif /* GEDIT_WINDOW_H */

/* ex:set ts=8 noet: */
