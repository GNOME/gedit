/*
 * gedit-multi-notebook.h
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


#ifndef GEDIT_MULTI_NOTEBOOK_H
#define GEDIT_MULTI_NOTEBOOK_H

#include <gtk/gtk.h>

#include "gedit-tab.h"
#include "gedit-notebook.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_MULTI_NOTEBOOK		(gedit_multi_notebook_get_type ())
#define GEDIT_MULTI_NOTEBOOK(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_MULTI_NOTEBOOK, GeditMultiNotebook))
#define GEDIT_MULTI_NOTEBOOK_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_MULTI_NOTEBOOK, GeditMultiNotebook const))
#define GEDIT_MULTI_NOTEBOOK_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_MULTI_NOTEBOOK, GeditMultiNotebookClass))
#define GEDIT_IS_MULTI_NOTEBOOK(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_MULTI_NOTEBOOK))
#define GEDIT_IS_MULTI_NOTEBOOK_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_MULTI_NOTEBOOK))
#define GEDIT_MULTI_NOTEBOOK_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_MULTI_NOTEBOOK, GeditMultiNotebookClass))

typedef struct _GeditMultiNotebook		GeditMultiNotebook;
typedef struct _GeditMultiNotebookClass		GeditMultiNotebookClass;
typedef struct _GeditMultiNotebookPrivate	GeditMultiNotebookPrivate;

struct _GeditMultiNotebook
{
	GtkGrid parent;

	GeditMultiNotebookPrivate *priv;
};

struct _GeditMultiNotebookClass
{
	GtkGridClass parent_class;

	/* Signals */
	void	(* notebook_added)		(GeditMultiNotebook *mnb,
						 GeditNotebook      *notebook);
	void	(* notebook_removed)		(GeditMultiNotebook *mnb,
						 GeditNotebook      *notebook);
	void	(* tab_added)			(GeditMultiNotebook *mnb,
						 GeditNotebook      *notebook,
						 GeditTab           *tab);
	void	(* tab_removed)			(GeditMultiNotebook *mnb,
						 GeditNotebook      *notebook,
						 GeditTab           *tab);
	void	(* switch_tab)			(GeditMultiNotebook *mnb,
						 GeditNotebook      *old_notebook,
						 GeditTab           *old_tab,
						 GeditNotebook      *new_notebook,
						 GeditTab           *new_tab);
	void	(* tab_close_request)		(GeditMultiNotebook *mnb,
						 GeditNotebook      *notebook,
						 GeditTab           *tab);
	GtkNotebook *	(* create_window)	(GeditMultiNotebook *mnb,
						 GeditNotebook      *notebook,
						 GtkWidget          *page,
						 gint                x,
						 gint                y);
	void	(* page_reordered)		(GeditMultiNotebook *mnb);
	void	(* show_popup_menu)		(GeditMultiNotebook *mnb,
						 GdkEvent           *event,
						 GeditTab           *tab);
};

G_MODULE_EXPORT
GType			 gedit_multi_notebook_get_type			(void) G_GNUC_CONST;

G_MODULE_EXPORT
GeditMultiNotebook	*gedit_multi_notebook_new			(void);

G_MODULE_EXPORT
GeditNotebook		*gedit_multi_notebook_get_active_notebook	(GeditMultiNotebook *mnb);

G_MODULE_EXPORT
gint			 gedit_multi_notebook_get_n_notebooks		(GeditMultiNotebook *mnb);

G_MODULE_EXPORT
GeditNotebook		*gedit_multi_notebook_get_nth_notebook		(GeditMultiNotebook *mnb,
									 gint                notebook_num);

G_MODULE_EXPORT
GeditNotebook		*gedit_multi_notebook_get_notebook_for_tab	(GeditMultiNotebook *mnb,
									 GeditTab           *tab);

G_MODULE_EXPORT
gint			 gedit_multi_notebook_get_notebook_num		(GeditMultiNotebook *mnb,
									 GeditNotebook      *notebook);

G_MODULE_EXPORT
gint			 gedit_multi_notebook_get_n_tabs		(GeditMultiNotebook *mnb);

G_MODULE_EXPORT
gint			 gedit_multi_notebook_get_page_num		(GeditMultiNotebook *mnb,
									 GeditTab           *tab);

G_MODULE_EXPORT
GeditTab		*gedit_multi_notebook_get_active_tab		(GeditMultiNotebook *mnb);
G_MODULE_EXPORT
void			 gedit_multi_notebook_set_active_tab		(GeditMultiNotebook *mnb,
									 GeditTab           *tab);

G_MODULE_EXPORT
void			 gedit_multi_notebook_set_current_page		(GeditMultiNotebook *mnb,
									 gint                page_num);

G_MODULE_EXPORT
GList			*gedit_multi_notebook_get_all_tabs		(GeditMultiNotebook *mnb);

G_MODULE_EXPORT
void			 gedit_multi_notebook_close_tabs		(GeditMultiNotebook *mnb,
									 const GList        *tabs);

G_MODULE_EXPORT
void			 gedit_multi_notebook_close_all_tabs		(GeditMultiNotebook *mnb);

G_MODULE_EXPORT
void			 gedit_multi_notebook_add_new_notebook		(GeditMultiNotebook *mnb);

G_MODULE_EXPORT
void			 gedit_multi_notebook_add_new_notebook_with_tab (GeditMultiNotebook *mnb,
									 GeditTab           *tab);

G_MODULE_EXPORT
void			 gedit_multi_notebook_remove_active_notebook	(GeditMultiNotebook *mnb);

G_MODULE_EXPORT
void			 gedit_multi_notebook_previous_notebook		(GeditMultiNotebook *mnb);
G_MODULE_EXPORT
void			 gedit_multi_notebook_next_notebook		(GeditMultiNotebook *mnb);

G_MODULE_EXPORT
void			 gedit_multi_notebook_foreach_notebook		(GeditMultiNotebook *mnb,
									 GtkCallback         callback,
									 gpointer            callback_data);

G_MODULE_EXPORT
void			 gedit_multi_notebook_foreach_tab		(GeditMultiNotebook *mnb,
									 GtkCallback         callback,
									 gpointer            callback_data);

G_MODULE_EXPORT
void			_gedit_multi_notebook_set_show_tabs		(GeditMultiNotebook *mnb,
									 gboolean            show);

G_END_DECLS

#endif /* GEDIT_MULTI_NOTEBOOK_H */

/* ex:set ts=8 noet: */
