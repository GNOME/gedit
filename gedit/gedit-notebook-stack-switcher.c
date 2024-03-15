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

#include "gedit-notebook-stack-switcher.h"

/* This widget is a rather ugly kludge: it uses a GtkNotebook full of empty
 * pages to create a stack switcher for the bottom pane. This is needed
 * because we want to expose GtkStack in the API but we want the tabs styled
 * as notebook tabs. Hopefully Gtk itself will grow a "tabs" stack switcher...
 */

struct _GeditNotebookStackSwitcherPrivate
{
	GtkNotebook *notebook;
	GtkStack *stack;
};

enum
{
	PROP_0,
	PROP_STACK
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditNotebookStackSwitcher, gedit_notebook_stack_switcher, GTK_TYPE_BIN)

static void
gedit_notebook_stack_switcher_init (GeditNotebookStackSwitcher *switcher)
{
	GeditNotebookStackSwitcherPrivate *priv;

	switcher->priv = gedit_notebook_stack_switcher_get_instance_private (switcher);
	priv = switcher->priv;

	priv->notebook = GTK_NOTEBOOK (gtk_notebook_new ());

	gtk_notebook_set_tab_pos (priv->notebook, GTK_POS_BOTTOM);
	gtk_notebook_set_scrollable (priv->notebook, TRUE);
	gtk_notebook_set_show_border (priv->notebook, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (priv->notebook), 0);

	gtk_widget_show (GTK_WIDGET (priv->notebook));
	gtk_container_add (GTK_CONTAINER (switcher), GTK_WIDGET (priv->notebook));
}

static GtkWidget *
find_notebook_child (GeditNotebookStackSwitcher *switcher,
		     GtkWidget                  *stack_child)
{
	GeditNotebookStackSwitcherPrivate *priv = switcher->priv;
	GList *pages;
	GList *p;
	GtkWidget *ret = NULL;

	if (stack_child == NULL)
	{
		return NULL;
	}

	pages = gtk_container_get_children (GTK_CONTAINER (priv->notebook));
	for (p = pages; p != NULL; p = p->next)
	{
		GtkWidget *child;

		child = g_object_get_data (p->data, "stack-child");
		if (stack_child == child)
		{
			ret = p->data;
			break;
		}
	}

	g_list_free (pages);

	return ret;
}

static void
sync_label (GeditNotebookStackSwitcher *switcher,
	    GtkWidget                  *stack_child,
	    GtkWidget                  *notebook_child)
{
	GeditNotebookStackSwitcherPrivate *priv = switcher->priv;

	if (stack_child != NULL && notebook_child != NULL)
	{
		gchar *title;

		gtk_widget_set_visible (notebook_child,
		                        gtk_widget_get_visible (stack_child));

		gtk_container_child_get (GTK_CONTAINER (priv->stack), stack_child,
		                         "title", &title,
		                         NULL);

		gtk_notebook_set_tab_label_text (priv->notebook, notebook_child, title);

		g_free (title);
	}
}

static void
on_child_prop_changed (GtkWidget                  *widget,
                       GParamSpec                 *pspec,
                       GeditNotebookStackSwitcher *switcher)
{
	GtkWidget *nb_child;

	nb_child = find_notebook_child (switcher, widget);
	sync_label (switcher, widget, nb_child);
}

static void
on_child_changed (GtkWidget                  *widget,
                  GParamSpec                 *pspec,
                  GeditNotebookStackSwitcher *switcher)
{
	GtkNotebook *notebook;
	GtkWidget *child;
	GtkWidget *nb_child;
	gint nb_page;

	notebook = switcher->priv->notebook;

	child = gtk_stack_get_visible_child (GTK_STACK (widget));
	nb_child = find_notebook_child (switcher, child);

	nb_page = gtk_notebook_page_num (notebook, nb_child);

	g_signal_handlers_block_by_func (widget, on_child_prop_changed, switcher);
	gtk_notebook_set_current_page (notebook, nb_page);
	g_signal_handlers_unblock_by_func (widget, on_child_prop_changed, switcher);

	sync_label (switcher, child, nb_child);
}

static void
on_stack_child_added (GtkStack                   *stack,
                      GtkWidget                  *widget,
                      GeditNotebookStackSwitcher *switcher)
{
	GeditNotebookStackSwitcherPrivate *priv = switcher->priv;
	GtkWidget *dummy;

	dummy = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	g_object_set_data (G_OBJECT (dummy), "stack-child", widget);
	gtk_notebook_append_page (priv->notebook, dummy, NULL);

	g_signal_connect (widget, "notify::visible",
	                  G_CALLBACK (on_child_prop_changed), switcher);
	g_signal_connect (widget, "child-notify::title",
	                  G_CALLBACK (on_child_prop_changed), switcher);

	sync_label (switcher, widget, dummy);
}

static void
on_stack_child_removed (GtkStack                   *stack,
                        GtkWidget                  *widget,
                        GeditNotebookStackSwitcher *switcher)
{
	GeditNotebookStackSwitcherPrivate *priv = switcher->priv;
	GtkWidget *nb_child;

	g_signal_handlers_disconnect_by_func (widget, on_child_prop_changed, switcher);

	nb_child = find_notebook_child (switcher, widget);
	gtk_container_remove (GTK_CONTAINER (priv->notebook), nb_child);
}

static void
on_notebook_switch_page (GtkNotebook                *notebook,
                         GtkWidget                  *page,
                         guint                       page_num,
                         GeditNotebookStackSwitcher *switcher)
{
	GeditNotebookStackSwitcherPrivate *priv = switcher->priv;
	GtkWidget *child;

	child = g_object_get_data (G_OBJECT (page), "stack-child");

	/* NOTE: we make the assumption here that if there is no visible child
	 * it means that the child does not contain any child already, this is
	 * to avoid an assertion when closing gedit, since the remove signal
	 * runs first on the stack handler so we try to set a visible child
	 * when the stack already does not handle that child
	 */
	if (child != NULL && gtk_stack_get_visible_child (priv->stack) != NULL)
	{
		gtk_stack_set_visible_child (priv->stack, child);
	}
}

static void
disconnect_signals (GeditNotebookStackSwitcher *switcher)
{
	GeditNotebookStackSwitcherPrivate *priv = switcher->priv;

	g_signal_handlers_disconnect_by_func (priv->stack, on_stack_child_added, switcher);
	g_signal_handlers_disconnect_by_func (priv->stack, on_stack_child_removed, switcher);
	g_signal_handlers_disconnect_by_func (priv->stack, on_child_changed, switcher);
	g_signal_handlers_disconnect_by_func (priv->stack, disconnect_signals, switcher);
	g_signal_handlers_disconnect_by_func (priv->notebook, on_notebook_switch_page, switcher);
}

static void
connect_signals (GeditNotebookStackSwitcher *switcher)
{
	GeditNotebookStackSwitcherPrivate *priv = switcher->priv;

	g_signal_connect (priv->stack, "add",
	                  G_CALLBACK (on_stack_child_added), switcher);
	g_signal_connect (priv->stack, "remove",
	                  G_CALLBACK (on_stack_child_removed), switcher);
	g_signal_connect (priv->stack, "notify::visible-child",
	                  G_CALLBACK (on_child_changed), switcher);
	g_signal_connect_swapped (priv->stack, "destroy",
	                          G_CALLBACK (disconnect_signals), switcher);
	g_signal_connect (priv->notebook, "switch-page",
	                  G_CALLBACK (on_notebook_switch_page), switcher);
}

void
gedit_notebook_stack_switcher_set_stack (GeditNotebookStackSwitcher *switcher,
                                         GtkStack                   *stack)
{
	GeditNotebookStackSwitcherPrivate *priv;

	g_return_if_fail (GEDIT_IS_NOTEBOOK_STACK_SWITCHER (switcher));
	g_return_if_fail (stack == NULL || GTK_IS_STACK (stack));

	priv = switcher->priv;

	if (priv->stack == stack)
		return;

	if (priv->stack)
	{
		disconnect_signals (switcher);
		g_clear_object (&priv->stack);
	}

	if (stack)
	{
		priv->stack = g_object_ref (stack);
		connect_signals (switcher);
	}

	g_object_notify (G_OBJECT (switcher), "stack");
}

GtkStack *
gedit_notebook_stack_switcher_get_stack (GeditNotebookStackSwitcher *switcher)
{
	g_return_val_if_fail (GEDIT_IS_NOTEBOOK_STACK_SWITCHER (switcher), NULL);

	return switcher->priv->stack;
}

static void
gedit_notebook_stack_switcher_get_property (GObject    *object,
                                            guint       prop_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
	GeditNotebookStackSwitcher *switcher = GEDIT_NOTEBOOK_STACK_SWITCHER (object);
	GeditNotebookStackSwitcherPrivate *priv = switcher->priv;

	switch (prop_id)
	{
		case PROP_STACK:
			g_value_set_object (value, priv->stack);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_notebook_stack_switcher_set_property (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
	GeditNotebookStackSwitcher *switcher = GEDIT_NOTEBOOK_STACK_SWITCHER (object);

	switch (prop_id)
	{
		case PROP_STACK:
			gedit_notebook_stack_switcher_set_stack (switcher, g_value_get_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_notebook_stack_switcher_dispose (GObject *object)
{
	GeditNotebookStackSwitcher *switcher = GEDIT_NOTEBOOK_STACK_SWITCHER (object);

	gedit_notebook_stack_switcher_set_stack (switcher, NULL);

	G_OBJECT_CLASS (gedit_notebook_stack_switcher_parent_class)->dispose (object);
}

static void
gedit_notebook_stack_switcher_class_init (GeditNotebookStackSwitcherClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = gedit_notebook_stack_switcher_get_property;
	object_class->set_property = gedit_notebook_stack_switcher_set_property;
	object_class->dispose = gedit_notebook_stack_switcher_dispose;

	g_object_class_install_property (object_class,
	                                 PROP_STACK,
	                                 g_param_spec_object ("stack",
	                                                      "Stack",
	                                                      "Stack",
	                                                      GTK_TYPE_STACK,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT));
}

GtkWidget *
gedit_notebook_stack_switcher_new (void)
{
	return g_object_new (GEDIT_TYPE_NOTEBOOK_STACK_SWITCHER, NULL);
}
