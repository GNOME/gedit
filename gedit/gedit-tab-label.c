/*
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Paolo Borelli
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

#include "gedit-tab-label.h"
#include "gedit-tab-private.h"

struct _GeditTabLabel
{
	GtkBox parent_instance;

	GeditTab *tab;

	GtkWidget *spinner;
	GtkWidget *icon;
	GtkWidget *label;
	GtkWidget *close_button;
};

enum
{
	PROP_0,
	PROP_TAB,
	N_PROPERTIES
};

enum
{
	SIGNAL_CLOSE_CLICKED,
	N_SIGNALS
};

static GParamSpec *properties[N_PROPERTIES];
static guint signals[N_SIGNALS];

G_DEFINE_TYPE (GeditTabLabel, gedit_tab_label, GTK_TYPE_BOX)

static void
gedit_tab_label_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GeditTabLabel *tab_label = GEDIT_TAB_LABEL (object);

	switch (prop_id)
	{
		case PROP_TAB:
			g_return_if_fail (tab_label->tab == NULL);
			tab_label->tab = GEDIT_TAB (g_value_get_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_tab_label_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	GeditTabLabel *tab_label = GEDIT_TAB_LABEL (object);

	switch (prop_id)
	{
		case PROP_TAB:
			g_value_set_object (value, tab_label->tab);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
close_button_clicked_cb (GtkWidget     *widget,
			 GeditTabLabel *tab_label)
{
	g_signal_emit (tab_label, signals[SIGNAL_CLOSE_CLICKED], 0, NULL);
}

static void
sync_tooltip (GeditTab      *tab,
	      GeditTabLabel *tab_label)
{
	gchar *str;

	str = _gedit_tab_get_tooltip (tab);
	g_return_if_fail (str != NULL);

	gtk_widget_set_tooltip_markup (GTK_WIDGET (tab_label), str);
	g_free (str);
}

static void
sync_name (GeditTab      *tab,
	   GParamSpec    *pspec,
	   GeditTabLabel *tab_label)
{
	gchar *str;

	g_return_if_fail (tab == tab_label->tab);

	str = _gedit_tab_get_name (tab);
	g_return_if_fail (str != NULL);

	gtk_label_set_text (GTK_LABEL (tab_label->label), str);
	g_free (str);

	sync_tooltip (tab, tab_label);
}

static void
update_close_button_sensitivity (GeditTabLabel *tab_label)
{
	GeditTabState state = gedit_tab_get_state (tab_label->tab);

	gtk_widget_set_sensitive (tab_label->close_button,
				  (state != GEDIT_TAB_STATE_CLOSING) &&
				  (state != GEDIT_TAB_STATE_SAVING)  &&
				  (state != GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW) &&
				  (state != GEDIT_TAB_STATE_PRINTING) &&
				  (state != GEDIT_TAB_STATE_SAVING_ERROR));
}

static void
sync_state (GeditTab      *tab,
	    GParamSpec    *pspec,
	    GeditTabLabel *tab_label)
{
	GeditTabState state;

	g_return_if_fail (tab == tab_label->tab);

	update_close_button_sensitivity (tab_label);

	state = gedit_tab_get_state (tab);

	if ((state == GEDIT_TAB_STATE_LOADING) ||
	    (state == GEDIT_TAB_STATE_SAVING) ||
	    (state == GEDIT_TAB_STATE_REVERTING))
	{
		gtk_widget_hide (tab_label->icon);

		gtk_widget_show (tab_label->spinner);
		gtk_spinner_start (GTK_SPINNER (tab_label->spinner));
	}
	else
	{
		GdkPixbuf *pixbuf;

		pixbuf = _gedit_tab_get_icon (tab);

		if (pixbuf != NULL)
		{
			gtk_image_set_from_pixbuf (GTK_IMAGE (tab_label->icon),
			                           pixbuf);

			g_clear_object (&pixbuf);

			gtk_widget_show (tab_label->icon);
		}
		else
		{
			gtk_widget_hide (tab_label->icon);
		}

		gtk_spinner_stop (GTK_SPINNER (tab_label->spinner));
		gtk_widget_hide (tab_label->spinner);
	}

	/* sync tip since encoding is known only after load/save end */
	sync_tooltip (tab, tab_label);
}

static void
gedit_tab_label_constructed (GObject *object)
{
	GeditTabLabel *tab_label = GEDIT_TAB_LABEL (object);

	if (tab_label->tab == NULL)
	{
		g_critical ("The tab label was not properly constructed");
		return;
	}

	sync_name (tab_label->tab, NULL, tab_label);
	sync_state (tab_label->tab, NULL, tab_label);

	g_signal_connect_object (tab_label->tab,
				 "notify::name",
				 G_CALLBACK (sync_name),
				 tab_label,
				 0);

	g_signal_connect_object (tab_label->tab,
				 "notify::state",
				 G_CALLBACK (sync_state),
				 tab_label,
				 0);

	G_OBJECT_CLASS (gedit_tab_label_parent_class)->constructed (object);
}

static void
gedit_tab_label_close_clicked (GeditTabLabel *tab_label)
{
}

static void
gedit_tab_label_class_init (GeditTabLabelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->set_property = gedit_tab_label_set_property;
	object_class->get_property = gedit_tab_label_get_property;
	object_class->constructed = gedit_tab_label_constructed;

	properties[PROP_TAB] =
		g_param_spec_object ("tab",
		                     "Tab",
		                     "The GeditTab",
		                     GEDIT_TYPE_TAB,
		                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties (object_class, N_PROPERTIES, properties);

	signals[SIGNAL_CLOSE_CLICKED] =
		g_signal_new_class_handler ("close-clicked",
		                            G_TYPE_FROM_CLASS (klass),
		                            G_SIGNAL_RUN_LAST,
		                            G_CALLBACK (gedit_tab_label_close_clicked),
		                            NULL, NULL, NULL,
		                            G_TYPE_NONE,
		                            0);

	/* Bind class to template */
	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/gedit/ui/gedit-tab-label.ui");
	gtk_widget_class_bind_template_child (widget_class, GeditTabLabel, spinner);
	gtk_widget_class_bind_template_child (widget_class, GeditTabLabel, icon);
	gtk_widget_class_bind_template_child (widget_class, GeditTabLabel, label);
	gtk_widget_class_bind_template_child (widget_class, GeditTabLabel, close_button);
}

static void
gedit_tab_label_init (GeditTabLabel *tab_label)
{
	gtk_widget_init_template (GTK_WIDGET (tab_label));

	g_signal_connect (tab_label->close_button,
	                  "clicked",
	                  G_CALLBACK (close_button_clicked_cb),
	                  tab_label);
}

GeditTab *
gedit_tab_label_get_tab (GeditTabLabel *tab_label)
{
	g_return_val_if_fail (GEDIT_IS_TAB_LABEL (tab_label), NULL);

	return tab_label->tab;
}

GtkWidget *
gedit_tab_label_new (GeditTab *tab)
{
	return g_object_new (GEDIT_TYPE_TAB_LABEL,
			     "tab", tab,
			     NULL);
}

/* ex:set ts=8 noet: */
