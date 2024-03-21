/* SPDX-FileCopyrightText: 2023-2024 - Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef GEDIT_WINDOW_PRIVATE_H
#define GEDIT_WINDOW_PRIVATE_H

#include "gedit-window.h"
#include "gedit-bottom-panel.h"
#include "gedit-multi-notebook.h"
#include "gedit-side-panel.h"
#include "gedit-window-titles.h"

G_BEGIN_DECLS

GeditMultiNotebook *	_gedit_window_get_multi_notebook	(GeditWindow *window);

GeditSidePanel *	_gedit_window_get_whole_side_panel	(GeditWindow *window);

GeditBottomPanel *	_gedit_window_get_whole_bottom_panel	(GeditWindow *window);

GeditWindowTitles *	_gedit_window_get_window_titles		(GeditWindow *window);

gboolean		_gedit_window_get_can_close		(GeditWindow *window);

G_END_DECLS

#endif /* GEDIT_WINDOW_PRIVATE_H */

/* ex:set ts=8 noet: */
