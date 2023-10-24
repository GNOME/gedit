/*
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Paolo Borelli
 * Copyright (C) 2023 - Sébastien Wilmet
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
#include <glib/gi18n.h>
#include "gedit-tab-private.h"

struct _GeditTabLabel
{
	GtkBox parent_instance;

	/* Weak ref */
	GeditTab *tab;

	GtkSpinner *spinner;
	GtkImage *icon;
	GtkLabel *label;
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
			g_set_weak_pointer (&tab_label->tab, GEDIT_TAB (g_value_get_object (value)));
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
			g_value_set_object (value, gedit_tab_label_get_tab (tab_label));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
close_button_clicked_cb (GtkButton     *close_button,
			 GeditTabLabel *tab_label)
{
	g_signal_emit (tab_label, signals[SIGNAL_CLOSE_CLICKED], 0);
}

static void
update_tooltip (GeditTabLabel *tab_label)
{
	gchar *str;

	if (tab_label->tab == NULL)
	{
		return;
	}

	str = _gedit_tab_get_tooltip (tab_label->tab);
	gtk_widget_set_tooltip_markup (GTK_WIDGET (tab_label), str);
	g_free (str);
}

static void
update_name (GeditTabLabel *tab_label)
{
	gchar *str;

	if (tab_label->tab == NULL)
	{
		return;
	}

	str = _gedit_tab_get_name (tab_label->tab);
	gtk_label_set_text (tab_label->label, str);
	g_free (str);

	update_tooltip (tab_label);
}

static void
tab_name_notify_cb (GeditTab      *tab,
		    GParamSpec    *pspec,
		    GeditTabLabel *tab_label)
{
	update_name (tab_label);
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
update_state (GeditTabLabel *tab_label)
{
	GeditTabState state;

	if (tab_label->tab == NULL)
	{
		return;
	}

	update_close_button_sensitivity (tab_label);

	state = gedit_tab_get_state (tab_label->tab);

	if ((state == GEDIT_TAB_STATE_LOADING) ||
	    (state == GEDIT_TAB_STATE_SAVING) ||
	    (state == GEDIT_TAB_STATE_REVERTING))
	{
		gtk_widget_hide (GTK_WIDGET (tab_label->icon));

		gtk_widget_show (GTK_WIDGET (tab_label->spinner));
		gtk_spinner_start (tab_label->spinner);
	}
	else
	{
		const gchar *icon_name;

		icon_name = _gedit_tab_get_icon_name (tab_label->tab);

		if (icon_name != NULL)
		{
			gtk_image_set_from_icon_name (tab_label->icon, icon_name, GTK_ICON_SIZE_MENU);
			gtk_widget_show (GTK_WIDGET (tab_label->icon));
		}
		else
		{
			gtk_widget_hide (GTK_WIDGET (tab_label->icon));
		}

		gtk_spinner_stop (tab_label->spinner);
		gtk_widget_hide (GTK_WIDGET (tab_label->spinner));
	}

	/* Update tooltip since encoding is known only after load/save end. */
	update_tooltip (tab_label);
}

static void
tab_state_notify_cb (GeditTab      *tab,
		     GParamSpec    *pspec,
		     GeditTabLabel *tab_label)
{
	update_state (tab_label);
}

static void
gedit_tab_label_constructed (GObject *object)
{
	GeditTabLabel *tab_label = GEDIT_TAB_LABEL (object);

	if (G_OBJECT_CLASS (gedit_tab_label_parent_class)->constructed != NULL)
	{
		G_OBJECT_CLASS (gedit_tab_label_parent_class)->constructed (object);
	}

	if (tab_label->tab == NULL)
	{
		return;
	}

	update_name (tab_label);
	update_state (tab_label);

	g_signal_connect_object (tab_label->tab,
				 "notify::name",
				 G_CALLBACK (tab_name_notify_cb),
				 tab_label,
				 G_CONNECT_DEFAULT);

	g_signal_connect_object (tab_label->tab,
				 "notify::state",
				 G_CALLBACK (tab_state_notify_cb),
				 tab_label,
				 G_CONNECT_DEFAULT);
}

static void
gedit_tab_label_dispose (GObject *object)
{
	GeditTabLabel *tab_label = GEDIT_TAB_LABEL (object);

	g_clear_weak_pointer (&tab_label->tab);

	G_OBJECT_CLASS (gedit_tab_label_parent_class)->dispose (object);
}

static void
gedit_tab_label_class_init (GeditTabLabelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = gedit_tab_label_set_property;
	object_class->get_property = gedit_tab_label_get_property;
	object_class->constructed = gedit_tab_label_constructed;
	object_class->dispose = gedit_tab_label_dispose;

	properties[PROP_TAB] =
		g_param_spec_object ("tab",
		                     "tab",
		                     "",
		                     GEDIT_TYPE_TAB,
		                     G_PARAM_READWRITE |
				     G_PARAM_CONSTRUCT_ONLY |
				     G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, N_PROPERTIES, properties);

	signals[SIGNAL_CLOSE_CLICKED] =
		g_signal_new ("close-clicked",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_FIRST,
			      0, NULL, NULL, NULL,
			      G_TYPE_NONE, 0);
}

static void
gedit_tab_label_init (GeditTabLabel *tab_label)
{
	/* Label */
	tab_label->label = GTK_LABEL (gtk_label_new (NULL));
	gtk_label_set_ellipsize (tab_label->label, PANGO_ELLIPSIZE_MIDDLE);
	gtk_label_set_width_chars (tab_label->label, 12);
	gtk_widget_show (GTK_WIDGET (tab_label->label));
	gtk_box_set_center_widget (GTK_BOX (tab_label), GTK_WIDGET (tab_label->label));

	/* Icon */
	tab_label->icon = GTK_IMAGE (gtk_image_new ());
	gtk_box_pack_start (GTK_BOX (tab_label), GTK_WIDGET (tab_label->icon), FALSE, FALSE, 0);

	/* Spinner */
	tab_label->spinner = GTK_SPINNER (gtk_spinner_new ());
	gtk_box_pack_start (GTK_BOX (tab_label), GTK_WIDGET (tab_label->spinner), FALSE, FALSE, 0);

	/* Close button */
	tab_label->close_button = tepl_utils_create_close_button ();
	gtk_widget_set_tooltip_text (tab_label->close_button, _("Close Document"));
	gtk_widget_show (tab_label->close_button);
	gtk_box_pack_end (GTK_BOX (tab_label), tab_label->close_button, FALSE, FALSE, 0);

	g_signal_connect (tab_label->close_button,
			  "clicked",
			  G_CALLBACK (close_button_clicked_cb),
			  tab_label);
}

GtkWidget *
gedit_tab_label_new (GeditTab *tab)
{
	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	return g_object_new (GEDIT_TYPE_TAB_LABEL,
			     "tab", tab,
			     NULL);
}

GeditTab *
gedit_tab_label_get_tab (GeditTabLabel *tab_label)
{
	g_return_val_if_fail (GEDIT_IS_TAB_LABEL (tab_label), NULL);

	return tab_label->tab;
}

/* ex:set ts=8 noet: */
