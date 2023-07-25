/*
 * gedit-multi-notebook.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
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
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "gedit-multi-notebook.h"

#include "gedit-enum-types-private.h"
#include "gedit-settings.h"
#include "gedit-tab-private.h"
#include "gedit-tab.h"

struct _GeditMultiNotebookPrivate
{
	GtkWidget *active_notebook;
	GList     *notebooks;
	gint       total_tabs;

	GeditTab  *active_tab;

	GeditNotebookShowTabsModeType show_tabs_mode;
	GSettings *ui_settings;

	guint      show_tabs : 1;
	guint      removing_notebook : 1;
};

enum
{
	PROP_0,
	PROP_ACTIVE_NOTEBOOK,
	PROP_ACTIVE_TAB,
	PROP_SHOW_TABS_MODE,
	LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

enum
{
	NOTEBOOK_ADDED,
	NOTEBOOK_REMOVED,
	TAB_ADDED,
	TAB_REMOVED,
	SWITCH_TAB,
	TAB_CLOSE_REQUEST,
	CREATE_WINDOW,
	PAGE_REORDERED,
	SHOW_POPUP_MENU,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_PRIVATE (GeditMultiNotebook, gedit_multi_notebook, GTK_TYPE_GRID)

static void	remove_notebook		(GeditMultiNotebook *mnb,
					 GtkWidget          *notebook);

static void	update_tabs_visibility	(GeditMultiNotebook *mnb);

static void
gedit_multi_notebook_get_property (GObject    *object,
				   guint       prop_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
	GeditMultiNotebook *mnb = GEDIT_MULTI_NOTEBOOK (object);

	switch (prop_id)
	{
		case PROP_ACTIVE_NOTEBOOK:
			g_value_set_object (value,
					    mnb->priv->active_notebook);
			break;
		case PROP_ACTIVE_TAB:
			g_value_set_object (value,
					    mnb->priv->active_tab);
			break;
		case PROP_SHOW_TABS_MODE:
			g_value_set_enum (value,
					  mnb->priv->show_tabs_mode);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_multi_notebook_set_property (GObject      *object,
				   guint         prop_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
	GeditMultiNotebook *mnb = GEDIT_MULTI_NOTEBOOK (object);

	switch (prop_id)
	{
		case PROP_SHOW_TABS_MODE:
			mnb->priv->show_tabs_mode = g_value_get_enum (value);
			update_tabs_visibility (mnb);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_multi_notebook_dispose (GObject *object)
{
	GeditMultiNotebook *mnb = GEDIT_MULTI_NOTEBOOK (object);

	g_clear_object (&mnb->priv->ui_settings);

	G_OBJECT_CLASS (gedit_multi_notebook_parent_class)->dispose (object);
}

static void
gedit_multi_notebook_finalize (GObject *object)
{
	GeditMultiNotebook *mnb = GEDIT_MULTI_NOTEBOOK (object);

	g_list_free (mnb->priv->notebooks);

	G_OBJECT_CLASS (gedit_multi_notebook_parent_class)->finalize (object);
}

static void
gedit_multi_notebook_class_init (GeditMultiNotebookClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_multi_notebook_dispose;
	object_class->finalize = gedit_multi_notebook_finalize;
	object_class->get_property = gedit_multi_notebook_get_property;
	object_class->set_property = gedit_multi_notebook_set_property;

	properties[PROP_ACTIVE_NOTEBOOK] =
		g_param_spec_object ("active-notebook",
		                     "Active Notebook",
		                     "The Active Notebook",
		                     GEDIT_TYPE_NOTEBOOK,
		                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
	properties[PROP_ACTIVE_TAB] =
		g_param_spec_object ("active-tab",
		                     "Active Tab",
		                     "The Active Tab",
		                     GEDIT_TYPE_TAB,
		                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
	properties[PROP_SHOW_TABS_MODE] =
		g_param_spec_enum ("show-tabs-mode",
		                   "Show Tabs Mode",
		                   "When tabs should be shown",
		                   GEDIT_TYPE_NOTEBOOK_SHOW_TABS_MODE_TYPE,
		                   GEDIT_NOTEBOOK_SHOW_TABS_ALWAYS,
		                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, LAST_PROP, properties);

	signals[NOTEBOOK_ADDED] =
		g_signal_new ("notebook-added",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditMultiNotebookClass, notebook_added),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_NOTEBOOK);
	signals[NOTEBOOK_REMOVED] =
		g_signal_new ("notebook-removed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditMultiNotebookClass, notebook_removed),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_NOTEBOOK);
	signals[TAB_ADDED] =
		g_signal_new ("tab-added",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditMultiNotebookClass, tab_added),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      2,
			      GEDIT_TYPE_NOTEBOOK,
			      GEDIT_TYPE_TAB);
	signals[TAB_REMOVED] =
		g_signal_new ("tab-removed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditMultiNotebookClass, tab_removed),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      2,
			      GEDIT_TYPE_NOTEBOOK,
			      GEDIT_TYPE_TAB);
	signals[SWITCH_TAB] =
		g_signal_new ("switch-tab",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditMultiNotebookClass, switch_tab),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      4,
			      GEDIT_TYPE_NOTEBOOK,
			      GEDIT_TYPE_TAB,
			      GEDIT_TYPE_NOTEBOOK,
			      GEDIT_TYPE_TAB);
	signals[TAB_CLOSE_REQUEST] =
		g_signal_new ("tab-close-request",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditMultiNotebookClass, tab_close_request),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      2,
			      GEDIT_TYPE_NOTEBOOK,
			      GEDIT_TYPE_TAB);
	signals[CREATE_WINDOW] =
		g_signal_new ("create-window",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (GeditMultiNotebookClass, create_window),
		              NULL, NULL, NULL,
		              GTK_TYPE_NOTEBOOK, 4,
		              GEDIT_TYPE_NOTEBOOK, GTK_TYPE_WIDGET,
		              G_TYPE_INT, G_TYPE_INT);
	signals[PAGE_REORDERED] =
		g_signal_new ("page-reordered",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (GeditMultiNotebookClass, page_reordered),
		              NULL, NULL, NULL,
		              G_TYPE_NONE,
		              3,
		              GEDIT_TYPE_NOTEBOOK, GTK_TYPE_WIDGET,
		              G_TYPE_INT);
	signals[SHOW_POPUP_MENU] =
		g_signal_new ("show-popup-menu",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditMultiNotebookClass, show_popup_menu),
			      NULL, NULL, NULL,
			      G_TYPE_NONE,
			      2,
			      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE,
			      GEDIT_TYPE_TAB);
}

static void
notebook_show_popup_menu (GtkNotebook        *notebook,
                          GdkEvent           *event,
                          GeditTab           *tab,
                          GeditMultiNotebook *mnb)
{
	g_signal_emit (G_OBJECT (mnb), signals[SHOW_POPUP_MENU], 0, event, tab);
}

static void
notebook_tab_close_request (GeditNotebook      *notebook,
			    GeditTab           *tab,
			    GeditMultiNotebook *mnb)
{
	g_signal_emit (G_OBJECT (mnb), signals[TAB_CLOSE_REQUEST], 0,
		       notebook, tab);
}

static GtkNotebook *
notebook_create_window (GeditNotebook      *notebook,
			GtkWidget          *child,
			gint                x,
			gint                y,
			GeditMultiNotebook *mnb)
{
	GtkNotebook *dest_notebook;

	g_signal_emit (G_OBJECT (mnb), signals[CREATE_WINDOW], 0,
	               notebook, child, x, y, &dest_notebook);

	return dest_notebook;
}

static void
notebook_page_reordered (GeditNotebook      *notebook,
		         GtkWidget          *child,
		         guint               page_num,
		         GeditMultiNotebook *mnb)
{
	g_signal_emit (G_OBJECT (mnb), signals[PAGE_REORDERED], 0, notebook,
	               child, page_num);
}

static void
set_active_tab (GeditMultiNotebook *mnb,
                GeditTab           *tab)
{
	mnb->priv->active_tab = tab;
	g_object_notify_by_pspec (G_OBJECT (mnb), properties[PROP_ACTIVE_TAB]);
}

static void
notebook_page_removed (GtkNotebook        *notebook,
		       GtkWidget          *child,
		       guint               page_num,
		       GeditMultiNotebook *mnb)
{
	GeditTab *tab = GEDIT_TAB (child);
	guint num_tabs;
	gboolean last_notebook;

	--mnb->priv->total_tabs;
	num_tabs = gtk_notebook_get_n_pages (notebook);
	last_notebook = (mnb->priv->notebooks->next == NULL);

	if (mnb->priv->total_tabs == 0)
	{
		set_active_tab (mnb, NULL);
	}

	g_signal_emit (G_OBJECT (mnb), signals[TAB_REMOVED], 0, notebook, tab);

	/* Not last notebook but last tab of the notebook, this means we have
	   to remove the current notebook */
	if (num_tabs == 0 && !mnb->priv->removing_notebook &&
	    !last_notebook)
	{
		remove_notebook (mnb, GTK_WIDGET (notebook));
	}

	update_tabs_visibility (mnb);
}

static void
notebook_page_added (GtkNotebook        *notebook,
		     GtkWidget          *child,
		     guint               page_num,
		     GeditMultiNotebook *mnb)
{
	GeditTab *tab = GEDIT_TAB (child);

	++mnb->priv->total_tabs;

	update_tabs_visibility (mnb);

	g_signal_emit (G_OBJECT (mnb), signals[TAB_ADDED], 0, notebook, tab);
}

static void
notebook_switch_page (GtkNotebook        *book,
		      GtkWidget          *pg,
		      gint                page_num,
		      GeditMultiNotebook *mnb)
{
	GeditTab *tab;

	/* When we switch a tab from a notebook that it is not the active one
	   the switch page is emitted before the set focus, so we do this check
	   and we avoid to call switch page twice */
	if (GTK_WIDGET (book) != mnb->priv->active_notebook)
		return;

	/* CHECK: I don't know why but it seems notebook_switch_page is called
	two times every time the user change the active tab */
	tab = GEDIT_TAB (gtk_notebook_get_nth_page (book, page_num));
	if (tab != mnb->priv->active_tab)
	{
		GeditTab *old_tab;

		old_tab = mnb->priv->active_tab;
		set_active_tab (mnb, tab);
		g_signal_emit (G_OBJECT (mnb), signals[SWITCH_TAB], 0,
			       mnb->priv->active_notebook, old_tab,
			       book, tab);
	}
}

/* We need to figure out if the any of the internal widget of the notebook
   has got the focus to set the active notebook */
static void
notebook_set_focus (GtkContainer       *container,
		    GtkWidget          *widget,
		    GeditMultiNotebook *mnb)
{
	if (GEDIT_IS_NOTEBOOK (container) &&
	    GTK_WIDGET (container) != mnb->priv->active_notebook)
	{
		gint page_num;

		mnb->priv->active_notebook = GTK_WIDGET (container);

		page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (container));
		notebook_switch_page (GTK_NOTEBOOK (container), NULL,
				      page_num, mnb);

		g_object_notify_by_pspec (G_OBJECT (mnb), properties[PROP_ACTIVE_NOTEBOOK]);
	}
}

static void
show_tabs_changed (GObject     *object,
		   GParamSpec  *pspec,
		   gpointer    *data)
{
	update_tabs_visibility (GEDIT_MULTI_NOTEBOOK (data));
}

static void
update_tabs_visibility (GeditMultiNotebook *mnb)
{
	gboolean show_tabs;
	GList *l;

	if (mnb->priv->notebooks == NULL)
		return;

	if (!mnb->priv->show_tabs)
	{
		show_tabs = FALSE;
	}
	else if (mnb->priv->notebooks->next == NULL) /* only one notebook */
	{
		switch (mnb->priv->show_tabs_mode)
		{
			case GEDIT_NOTEBOOK_SHOW_TABS_NEVER:
				show_tabs = FALSE;
				break;
			case GEDIT_NOTEBOOK_SHOW_TABS_AUTO:
				show_tabs = gtk_notebook_get_n_pages (GTK_NOTEBOOK (mnb->priv->notebooks->data)) > 1;
				break;
			case GEDIT_NOTEBOOK_SHOW_TABS_ALWAYS:
			default:
				show_tabs = TRUE;
				break;
		}
	}
	else
	{
		show_tabs = (mnb->priv->show_tabs_mode != GEDIT_NOTEBOOK_SHOW_TABS_NEVER);
	}

	g_signal_handlers_block_by_func (mnb, show_tabs_changed, NULL);

	for (l = mnb->priv->notebooks; l != NULL; l = l->next)
	{
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK (l->data), show_tabs);
	}

	g_signal_handlers_unblock_by_func (mnb, show_tabs_changed, NULL);
}

static void
connect_notebook_signals (GeditMultiNotebook *mnb,
			  GtkWidget          *notebook)
{
	g_signal_connect (notebook,
			  "set-focus-child",
			  G_CALLBACK (notebook_set_focus),
			  mnb);
	g_signal_connect (notebook,
			  "page-added",
			  G_CALLBACK (notebook_page_added),
			  mnb);
	g_signal_connect (notebook,
			  "page-removed",
			  G_CALLBACK (notebook_page_removed),
			  mnb);
	g_signal_connect (notebook,
			  "switch-page",
			  G_CALLBACK (notebook_switch_page),
			  mnb);
	g_signal_connect (notebook,
			  "page-reordered",
			  G_CALLBACK (notebook_page_reordered),
			  mnb);
	g_signal_connect (notebook,
			  "create-window",
			  G_CALLBACK (notebook_create_window),
			  mnb);
	g_signal_connect (notebook,
			  "tab-close-request",
			  G_CALLBACK (notebook_tab_close_request),
			  mnb);
	g_signal_connect (notebook,
			  "show-popup-menu",
			  G_CALLBACK (notebook_show_popup_menu),
			  mnb);
	g_signal_connect (notebook,
			  "notify::show-tabs",
			  G_CALLBACK (show_tabs_changed),
			  mnb);
}

static void
disconnect_notebook_signals (GeditMultiNotebook *mnb,
			     GtkWidget          *notebook)
{
	g_signal_handlers_disconnect_by_func (notebook, notebook_set_focus,
					      mnb);
	g_signal_handlers_disconnect_by_func (notebook, notebook_switch_page,
					      mnb);
	g_signal_handlers_disconnect_by_func (notebook, notebook_page_added,
					      mnb);
	g_signal_handlers_disconnect_by_func (notebook, notebook_page_removed,
					      mnb);
	g_signal_handlers_disconnect_by_func (notebook, notebook_page_reordered,
					      mnb);
	g_signal_handlers_disconnect_by_func (notebook, notebook_create_window,
					      mnb);
	g_signal_handlers_disconnect_by_func (notebook, notebook_tab_close_request,
					      mnb);
	g_signal_handlers_disconnect_by_func (notebook, notebook_show_popup_menu,
					      mnb);
	g_signal_handlers_disconnect_by_func (notebook, show_tabs_changed,
					      mnb);
}

static void
add_notebook (GeditMultiNotebook *mnb,
	      GtkWidget          *notebook,
	      gboolean            main_container)
{
	gtk_widget_set_hexpand (notebook, TRUE);
	gtk_widget_set_vexpand (notebook, TRUE);

	if (main_container)
	{
		gtk_container_add (GTK_CONTAINER (mnb), notebook);

		mnb->priv->notebooks = g_list_append (mnb->priv->notebooks,
		                                      notebook);
	}
	else
	{
		GtkWidget *paned;
		GtkWidget *parent;
		GtkAllocation allocation;
		GtkWidget *active_notebook = mnb->priv->active_notebook;
		gint active_nb_pos;

		paned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
		gtk_widget_show (paned);

		/* First we remove the active container from its parent to make
		   this we add a ref to it*/
		g_object_ref (active_notebook);
		parent = gtk_widget_get_parent (active_notebook);
		gtk_widget_get_allocation (active_notebook, &allocation);

		gtk_container_remove (GTK_CONTAINER (parent), active_notebook);
		gtk_container_add (GTK_CONTAINER (parent), paned);

		gtk_paned_pack1 (GTK_PANED (paned), active_notebook, TRUE, FALSE);
		g_object_unref (active_notebook);

		gtk_paned_pack2 (GTK_PANED (paned), notebook, FALSE, FALSE);

		/* We need to set the new paned in the right place */
		gtk_paned_set_position (GTK_PANED (paned),
		                        allocation.width / 2);

		active_nb_pos = g_list_index (mnb->priv->notebooks,
		                              active_notebook);
		mnb->priv->notebooks = g_list_insert (mnb->priv->notebooks,
		                                      notebook,
		                                      active_nb_pos + 1);
	}

	gtk_widget_show (notebook);

	connect_notebook_signals (mnb, notebook);

	g_signal_emit (G_OBJECT (mnb), signals[NOTEBOOK_ADDED], 0, notebook);
}

static void
remove_notebook (GeditMultiNotebook *mnb,
		 GtkWidget          *notebook)
{
	GtkWidget *parent;
	GtkWidget *grandpa;
	GList *children;
	GtkWidget *new_notebook;
	GList *current;

	if (mnb->priv->notebooks->next == NULL)
	{
		g_warning ("You are trying to remove the main notebook");
		return;
	}

	current = g_list_find (mnb->priv->notebooks,
			       notebook);

	if (current->next != NULL)
	{
		new_notebook = GTK_WIDGET (current->next->data);
	}
	else
	{
		new_notebook = GTK_WIDGET (mnb->priv->notebooks->data);
	}

	parent = gtk_widget_get_parent (notebook);

	/* Now we destroy the widget, we get the children of parent and we destroy
	  parent too as the parent is an useless paned. Finally we add the child
	  into the grand parent */
	g_object_ref (notebook);
	mnb->priv->removing_notebook = TRUE;

	gtk_widget_destroy (notebook);

	mnb->priv->notebooks = g_list_remove (mnb->priv->notebooks,
					      notebook);

	mnb->priv->removing_notebook = FALSE;

	children = gtk_container_get_children (GTK_CONTAINER (parent));
	if (children->next != NULL)
	{
		g_warning ("The parent is not a paned");
		return;
	}
	grandpa = gtk_widget_get_parent (parent);

	g_object_ref (children->data);
	gtk_container_remove (GTK_CONTAINER (parent),
			      GTK_WIDGET (children->data));
	gtk_widget_destroy (parent);
	gtk_container_add (GTK_CONTAINER (grandpa),
			   GTK_WIDGET (children->data));
	g_object_unref (children->data);
	g_list_free (children);

	disconnect_notebook_signals (mnb, notebook);

	g_signal_emit (G_OBJECT (mnb), signals[NOTEBOOK_REMOVED], 0, notebook);
	g_object_unref (notebook);

	/* Let's make the active notebook grab the focus */
	gtk_widget_grab_focus (new_notebook);
}

static void
gedit_multi_notebook_init (GeditMultiNotebook *mnb)
{
	GeditMultiNotebookPrivate *priv;

	mnb->priv = gedit_multi_notebook_get_instance_private (mnb);
	priv = mnb->priv;

	priv->removing_notebook = FALSE;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (mnb),
	                                GTK_ORIENTATION_VERTICAL);

	priv->show_tabs_mode = GEDIT_NOTEBOOK_SHOW_TABS_ALWAYS;
	priv->show_tabs = TRUE;

	priv->ui_settings = g_settings_new ("org.gnome.gedit.preferences.ui");
	g_settings_bind (priv->ui_settings,
			 GEDIT_SETTINGS_SHOW_TABS_MODE,
			 mnb,
			 "show-tabs-mode",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);

	priv->active_notebook = gedit_notebook_new ();
	add_notebook (mnb, priv->active_notebook, TRUE);
}

GeditMultiNotebook *
gedit_multi_notebook_new (void)
{
	return g_object_new (GEDIT_TYPE_MULTI_NOTEBOOK, NULL);
}

GeditNotebook *
gedit_multi_notebook_get_active_notebook (GeditMultiNotebook *mnb)
{
	g_return_val_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb), NULL);

	return GEDIT_NOTEBOOK (mnb->priv->active_notebook);
}

gint
gedit_multi_notebook_get_n_notebooks (GeditMultiNotebook *mnb)
{
	g_return_val_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb), 0);

	return g_list_length (mnb->priv->notebooks);
}

GeditNotebook *
gedit_multi_notebook_get_nth_notebook (GeditMultiNotebook *mnb,
				       gint                notebook_num)
{
	g_return_val_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb), NULL);

	return g_list_nth_data (mnb->priv->notebooks, notebook_num);
}

GeditNotebook *
gedit_multi_notebook_get_notebook_for_tab (GeditMultiNotebook *mnb,
                                           GeditTab           *tab)
{
	GList *l;
	gint page_num;

	g_return_val_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb), NULL);
	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	l = mnb->priv->notebooks;

	do
	{
		page_num = gtk_notebook_page_num (GTK_NOTEBOOK (l->data),
		                                  GTK_WIDGET (tab));
		if (page_num != -1)
			break;

		l = g_list_next (l);
	} while (l != NULL && page_num == -1);

	g_return_val_if_fail (page_num != -1, NULL);

	return GEDIT_NOTEBOOK (l->data);
}

gint
gedit_multi_notebook_get_notebook_num (GeditMultiNotebook *mnb,
				       GeditNotebook      *notebook)
{
	g_return_val_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb), -1);
	g_return_val_if_fail (GEDIT_IS_NOTEBOOK (notebook), -1);

	return g_list_index (mnb->priv->notebooks, notebook);
}

gint
gedit_multi_notebook_get_n_tabs (GeditMultiNotebook *mnb)
{
	g_return_val_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb), 0);

	return mnb->priv->total_tabs;
}

gint
gedit_multi_notebook_get_page_num (GeditMultiNotebook *mnb,
				   GeditTab           *tab)
{
	GList *l;
	gint real_page_num = 0;

	for (l = mnb->priv->notebooks; l != NULL; l = g_list_next (l))
	{
		gint page_num;

		page_num = gtk_notebook_page_num (GTK_NOTEBOOK (l->data),
						  GTK_WIDGET (tab));

		if (page_num != -1)
		{
			real_page_num += page_num;
			break;
		}

		real_page_num += gtk_notebook_get_n_pages (GTK_NOTEBOOK (l->data));
	}

	return real_page_num;
}

GeditTab *
gedit_multi_notebook_get_active_tab (GeditMultiNotebook *mnb)
{
	g_return_val_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb), NULL);

	return (mnb->priv->active_tab == NULL) ?
				NULL : GEDIT_TAB (mnb->priv->active_tab);
}

void
gedit_multi_notebook_set_active_tab (GeditMultiNotebook *mnb,
				     GeditTab           *tab)
{
	GList *l;
	gint page_num;

	g_return_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb));
	g_return_if_fail (GEDIT_IS_TAB (tab) || tab == NULL);

	/* use plain C cast since the active tab can be null */
	if (tab == (GeditTab *) mnb->priv->active_tab)
	{
		return;
	}

	if (tab == NULL)
	{
		set_active_tab (mnb, NULL);
		return;
	}

	l = mnb->priv->notebooks;

	do
	{
		page_num = gtk_notebook_page_num (GTK_NOTEBOOK (l->data),
		                                  GTK_WIDGET (tab));
		if (page_num != -1)
			break;

		l = g_list_next (l);
	} while (l != NULL && page_num == -1);

	g_return_if_fail (page_num != -1);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (l->data), page_num);

	if (GTK_WIDGET (l->data) != mnb->priv->active_notebook)
	{
		gtk_widget_grab_focus (GTK_WIDGET (l->data));
	}
}

void
gedit_multi_notebook_set_current_page (GeditMultiNotebook *mnb,
				       gint                page_num)
{
	GList *l;
	gint pages = 0;
	gint single_num = page_num;

	g_return_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb));

	for (l = mnb->priv->notebooks; l != NULL; l = g_list_next (l))
	{
		gint p;

		p = gtk_notebook_get_n_pages (GTK_NOTEBOOK (l->data));
		pages += p;

		if ((pages - 1) >= page_num)
			break;

		single_num -= p;
	}

	if (l == NULL)
		return;

	if (GTK_WIDGET (l->data) != mnb->priv->active_notebook)
	{
		gtk_widget_grab_focus (GTK_WIDGET (l->data));
	}

	gtk_notebook_set_current_page (GTK_NOTEBOOK (l->data), single_num);
}

GList *
gedit_multi_notebook_get_all_tabs (GeditMultiNotebook *mnb)
{
	GList *nbs;
	GList *ret = NULL;

	g_return_val_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb), NULL);

	for (nbs = mnb->priv->notebooks; nbs != NULL; nbs = g_list_next (nbs))
	{
		GList *l, *children;

		children = gtk_container_get_children (GTK_CONTAINER (nbs->data));

		for (l = children; l != NULL; l = g_list_next (l))
		{
			ret = g_list_prepend (ret, l->data);
		}

		g_list_free (children);
	}

	ret = g_list_reverse (ret);

	return ret;
}

void
gedit_multi_notebook_close_tabs (GeditMultiNotebook *mnb,
				 const GList        *tabs)
{
	GList *l;

	g_return_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb));

	for (l = (GList *)tabs; l != NULL; l = g_list_next (l))
	{
		GList *nbs;

		for (nbs = mnb->priv->notebooks; nbs != NULL; nbs = g_list_next (nbs))
		{
			gint n;

			n = gtk_notebook_page_num (GTK_NOTEBOOK (nbs->data),
			                           GTK_WIDGET (l->data));

			if (n != -1)
			{
				gtk_container_remove (GTK_CONTAINER (nbs->data),
				                      GTK_WIDGET (l->data));
				break;
			}
		}
	}
}

/**
 * gedit_multi_notebook_close_all_tabs:
 * @mnb: a #GeditMultiNotebook
 *
 * Closes all opened tabs.
 */
void
gedit_multi_notebook_close_all_tabs (GeditMultiNotebook *mnb)
{
	GList *nbs, *l;

	g_return_if_fail (GEDIT_MULTI_NOTEBOOK (mnb));

	/* We copy the list because the main one is going to have the items
	   removed */
	nbs = g_list_copy (mnb->priv->notebooks);

	for (l = nbs; l != NULL; l = g_list_next (l))
	{
		gedit_notebook_remove_all_tabs (GEDIT_NOTEBOOK (l->data));
	}

	g_list_free (nbs);
}

void
gedit_multi_notebook_add_new_notebook (GeditMultiNotebook *mnb)
{
	GtkWidget *notebook;
	GeditTab *tab;

	g_return_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb));

	notebook = gedit_notebook_new ();
	add_notebook (mnb, notebook, FALSE);

	tab = _gedit_tab_new ();
	gtk_widget_show (GTK_WIDGET (tab));

	/* When gtk_notebook_insert_page is called the focus is set in
	   the notebook, we don't want this to happen until the page is added.
	   Also we don't want to call switch_page when we add the tab
	   but when we switch the notebook. */
	g_signal_handlers_block_by_func (notebook, notebook_set_focus, mnb);
	g_signal_handlers_block_by_func (notebook, notebook_switch_page, mnb);

	gedit_notebook_add_tab (GEDIT_NOTEBOOK (notebook),
				tab,
				-1,
				TRUE);

	g_signal_handlers_unblock_by_func (notebook, notebook_switch_page, mnb);
	g_signal_handlers_unblock_by_func (notebook, notebook_set_focus, mnb);

	notebook_set_focus (GTK_CONTAINER (notebook), NULL, mnb);
}

void
gedit_multi_notebook_add_new_notebook_with_tab (GeditMultiNotebook *mnb,
                                                GeditTab           *tab)
{
	GtkWidget *notebook;
	GeditNotebook *old_notebook;

	g_return_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb));
	g_return_if_fail (GEDIT_IS_TAB (tab));

	notebook = gedit_notebook_new ();
	add_notebook (mnb, notebook, FALSE);

	old_notebook = gedit_multi_notebook_get_notebook_for_tab (mnb, tab);

	/* When gtk_notebook_insert_page is called the focus is set in
	   the notebook, we don't want this to happen until the page is added.
	   Also we don't want to call switch_page when we add the tab
	   but when we switch the notebook. */
	g_signal_handlers_block_by_func (old_notebook, notebook_set_focus, mnb);
	g_signal_handlers_block_by_func (old_notebook, notebook_switch_page, mnb);

	gedit_notebook_move_tab (old_notebook,
	                         GEDIT_NOTEBOOK (notebook),
	                         tab,
	                         -1);

	g_signal_handlers_unblock_by_func (old_notebook, notebook_switch_page, mnb);
	g_signal_handlers_unblock_by_func (old_notebook, notebook_set_focus, mnb);

	notebook_set_focus (GTK_CONTAINER (notebook), NULL, mnb);
}

void
gedit_multi_notebook_remove_active_notebook (GeditMultiNotebook *mnb)
{
	g_return_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb));

	gedit_notebook_remove_all_tabs (GEDIT_NOTEBOOK (mnb->priv->active_notebook));
}

void
gedit_multi_notebook_previous_notebook (GeditMultiNotebook *mnb)
{
	GList *current;
	GtkWidget *notebook;

	g_return_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb));

	current = g_list_find (mnb->priv->notebooks,
			       mnb->priv->active_notebook);

	if (current->prev != NULL)
		notebook = GTK_WIDGET (current->prev->data);
	else
		notebook = GTK_WIDGET (g_list_last (mnb->priv->notebooks)->data);

	gtk_widget_grab_focus (notebook);
}

void
gedit_multi_notebook_next_notebook (GeditMultiNotebook *mnb)
{
	GList *current;
	GtkWidget *notebook;

	g_return_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb));

	current = g_list_find (mnb->priv->notebooks,
			       mnb->priv->active_notebook);

	if (current->next != NULL)
		notebook = GTK_WIDGET (current->next->data);
	else
		notebook = GTK_WIDGET (mnb->priv->notebooks->data);

	gtk_widget_grab_focus (notebook);
}

void
gedit_multi_notebook_foreach_notebook (GeditMultiNotebook *mnb,
				       GtkCallback         callback,
				       gpointer            callback_data)
{
	GList *l;

	g_return_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb));

	for (l = mnb->priv->notebooks; l != NULL; l = g_list_next (l))
	{
		callback (GTK_WIDGET (l->data), callback_data);
	}
}

void
gedit_multi_notebook_foreach_tab (GeditMultiNotebook *mnb,
				  GtkCallback         callback,
				  gpointer            callback_data)
{
	GList *nb;

	g_return_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb));

	for (nb = mnb->priv->notebooks; nb != NULL; nb = g_list_next (nb))
	{
		GList *l, *children;

		children = gtk_container_get_children (GTK_CONTAINER (nb->data));

		for (l = children; l != NULL; l = g_list_next (l))
		{
			callback (GTK_WIDGET (l->data), callback_data);
		}

		g_list_free (children);
	}
}

/* We only use this to hide tabs in fullscreen mode so for now
 * we do not have a real property etc.
 */
void
_gedit_multi_notebook_set_show_tabs (GeditMultiNotebook *mnb,
				     gboolean           show)
{
	g_return_if_fail (GEDIT_IS_MULTI_NOTEBOOK (mnb));

	mnb->priv->show_tabs = show != FALSE;

	update_tabs_visibility (mnb);
}

/* ex:set ts=8 noet: */
