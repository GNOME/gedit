/*
 * This file is part of gedit
 *
 * Copyright (C) 2014 - Paolo Borelli
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

#ifndef GEDIT_NOTEBOOK_STACK_SWITCHER_H
#define GEDIT_NOTEBOOK_STACK_SWITCHER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_NOTEBOOK_STACK_SWITCHER             (gedit_notebook_stack_switcher_get_type())
#define GEDIT_NOTEBOOK_STACK_SWITCHER(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_NOTEBOOK_STACK_SWITCHER, GeditNotebookStackSwitcher))
#define GEDIT_NOTEBOOK_STACK_SWITCHER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_NOTEBOOK_STACK_SWITCHER, GeditNotebookStackSwitcherClass))
#define GEDIT_IS_NOTEBOOK_STACK_SWITCHER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_NOTEBOOK_STACK_SWITCHER))
#define GEDIT_IS_NOTEBOOK_STACK_SWITCHER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_NOTEBOOK_STACK_SWITCHER))
#define GEDIT_NOTEBOOK_STACK_SWITCHER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_NOTEBOOK_STACK_SWITCHER, GeditNotebookStackSwitcherClass))

typedef struct _GeditNotebookStackSwitcher        GeditNotebookStackSwitcher;
typedef struct _GeditNotebookStackSwitcherClass   GeditNotebookStackSwitcherClass;
typedef struct _GeditNotebookStackSwitcherPrivate GeditNotebookStackSwitcherPrivate;

struct _GeditNotebookStackSwitcher
{
	GtkBin parent;

	GeditNotebookStackSwitcherPrivate *priv;
};

struct _GeditNotebookStackSwitcherClass
{
	GtkBinClass parent_class;

	gpointer padding[12];
};

GType		gedit_notebook_stack_switcher_get_type	(void);

GtkWidget *	gedit_notebook_stack_switcher_new	(void);

void		gedit_notebook_stack_switcher_set_stack	(GeditNotebookStackSwitcher *switcher,
							 GtkStack                   *stack);

GtkStack *	gedit_notebook_stack_switcher_get_stack	(GeditNotebookStackSwitcher *switcher);

G_END_DECLS

#endif /* GEDIT_NOTEBOOK_STACK_SWITCHER_H */
