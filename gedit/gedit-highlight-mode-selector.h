/*
 * gedit-highlight-mode-selector.h
 * This file is part of gedit
 *
 * Copyright (C) 2013 - Ignacio Casal Quinteiro
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
 * along with gedit. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GEDIT_HIGHLIGHT_MODE_SELECTOR_H
#define GEDIT_HIGHLIGHT_MODE_SELECTOR_H

#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_HIGHLIGHT_MODE_SELECTOR (gedit_highlight_mode_selector_get_type ())

G_DECLARE_FINAL_TYPE (GeditHighlightModeSelector, gedit_highlight_mode_selector, GEDIT, HIGHLIGHT_MODE_SELECTOR, GtkGrid)

GeditHighlightModeSelector *gedit_highlight_mode_selector_new             (void);

void                        gedit_highlight_mode_selector_select_language (GeditHighlightModeSelector *selector,
                                                                           GtkSourceLanguage          *language);

void                        gedit_highlight_mode_selector_activate_selected_language
                                                                          (GeditHighlightModeSelector *selector);

G_END_DECLS

#endif /* GEDIT_HIGHLIGHT_MODE_SELECTOR_H */

/* ex:set ts=8 noet: */
