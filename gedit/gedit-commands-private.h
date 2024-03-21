/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-commands.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi
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

#ifndef GEDIT_COMMANDS_PRIVATE_H
#define GEDIT_COMMANDS_PRIVATE_H

#include <gtksourceview/gtksource.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-notebook.h>

G_BEGIN_DECLS

/* Create titled documens for non-existing URIs */
GSList	       *_gedit_cmd_load_files_from_prompt	(GeditWindow             *window,
							 GSList                  *files,
							 const GtkSourceEncoding *encoding,
							 gint                     line_pos,
							 gint                     column_pos) G_GNUC_WARN_UNUSED_RESULT;

void		_gedit_cmd_file_new			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_file_open			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_file_reopen_closed_tab	(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_file_save			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_file_save_as			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_file_save_all		(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_file_revert			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_file_print			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_file_close			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_file_close_all		(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_file_close_window		(GeditWindow *window);
void		_gedit_cmd_file_quit			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);

void		_gedit_cmd_edit_undo			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_edit_redo			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_edit_cut			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_edit_copy			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_edit_paste			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_edit_delete			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_edit_select_all		(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_edit_overwrite_mode		(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);

void		_gedit_cmd_view_focus_active		(GSimpleAction *action,
							 GVariant      *state,
							 gpointer       user_data);
void		_gedit_cmd_view_toggle_side_panel	(GSimpleAction *action,
							 GVariant      *state,
							 gpointer       user_data);
void		_gedit_cmd_view_toggle_bottom_panel	(GSimpleAction *action,
							 GVariant      *state,
							 gpointer       user_data);
void		_gedit_cmd_view_toggle_fullscreen_mode	(GSimpleAction *action,
							 GVariant      *state,
							 gpointer       user_data);
void		_gedit_cmd_view_leave_fullscreen_mode	(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void            _gedit_cmd_view_highlight_mode          (GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);

void		_gedit_cmd_search_find			(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_search_find_next		(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_search_find_prev		(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_search_replace		(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_search_clear_highlight	(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_search_goto_line		(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);

void		_gedit_cmd_documents_previous_document	(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_documents_next_document	(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_documents_move_to_new_window	(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_documents_new_tab_group	(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_documents_previous_tab_group	(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);
void		_gedit_cmd_documents_next_tab_group	(GSimpleAction *action,
							 GVariant      *parameter,
							 gpointer       user_data);

void		_gedit_cmd_help_contents		(GeditWindow *window);
void		_gedit_cmd_help_about			(GeditWindow *window);

void		_gedit_cmd_file_close_tab 		(GeditTab    *tab,
							 GeditWindow *window);

void		_gedit_cmd_file_close_notebook		(GeditWindow   *window,
							 GeditNotebook *notebook);

G_END_DECLS

#endif /* GEDIT_COMMANDS_PRIVATE_H */
/* ex:set ts=8 noet: */
