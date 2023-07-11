/* SPDX-FileCopyrightText: 2023 - Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef GEDIT_WINDOW_PRIVATE_H
#define GEDIT_WINDOW_PRIVATE_H

#include "gedit-window.h"
#include "gedit-multi-notebook.h"
#include "gedit-side-panel.h"

G_BEGIN_DECLS

GeditMultiNotebook *	_gedit_window_get_multi_notebook	(GeditWindow *window);

GeditSidePanel *	_gedit_window_get_whole_side_panel	(GeditWindow *window);

G_END_DECLS

#endif /* GEDIT_WINDOW_PRIVATE_H */

/* ex:set ts=8 noet: */
