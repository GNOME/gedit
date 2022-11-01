/*
 * This file is part of gedit
 *
 * Copyright (C) 2014 - Steve Frécinaux
 * Copyright (C) 2022 - Sébastien Wilmet
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

#ifndef TEPL_MENU_STACK_SWITCHER_H
#define TEPL_MENU_STACK_SWITCHER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TEPL_TYPE_MENU_STACK_SWITCHER             (tepl_menu_stack_switcher_get_type ())
#define TEPL_MENU_STACK_SWITCHER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEPL_TYPE_MENU_STACK_SWITCHER, TeplMenuStackSwitcher))
#define TEPL_MENU_STACK_SWITCHER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TEPL_TYPE_MENU_STACK_SWITCHER, TeplMenuStackSwitcherClass))
#define TEPL_IS_MENU_STACK_SWITCHER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEPL_TYPE_MENU_STACK_SWITCHER))
#define TEPL_IS_MENU_STACK_SWITCHER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TEPL_TYPE_MENU_STACK_SWITCHER))
#define TEPL_MENU_STACK_SWITCHER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TEPL_TYPE_MENU_STACK_SWITCHER, TeplMenuStackSwitcherClass))

typedef struct _TeplMenuStackSwitcher         TeplMenuStackSwitcher;
typedef struct _TeplMenuStackSwitcherClass    TeplMenuStackSwitcherClass;
typedef struct _TeplMenuStackSwitcherPrivate  TeplMenuStackSwitcherPrivate;

struct _TeplMenuStackSwitcher
{
	GtkMenuButton parent;

	TeplMenuStackSwitcherPrivate *priv;
};

struct _TeplMenuStackSwitcherClass
{
	GtkMenuButtonClass parent_class;

	gpointer padding[12];
};

GType		tepl_menu_stack_switcher_get_type	(void);

GtkWidget *	tepl_menu_stack_switcher_new		(void);

void		tepl_menu_stack_switcher_set_stack	(TeplMenuStackSwitcher *switcher,
							 GtkStack              *stack);

GtkStack *	tepl_menu_stack_switcher_get_stack	(TeplMenuStackSwitcher *switcher);

G_END_DECLS

#endif /* TEPL_MENU_STACK_SWITCHER_H */
