/*
 * gedit-progress-info-bar.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
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

#include "gedit-progress-info-bar.h"
#include <glib/gi18n.h>

enum {
	PROP_0,
	PROP_HAS_CANCEL_BUTTON,
	LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

struct _GeditProgressInfoBar
{
	GtkInfoBar parent_instance;

	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *progress;
};

G_DEFINE_TYPE (GeditProgressInfoBar, gedit_progress_info_bar, GTK_TYPE_INFO_BAR)

static void
gedit_progress_info_bar_set_has_cancel_button (GeditProgressInfoBar *bar,
					       gboolean              has_button)
{
	if (has_button)
	{
		gtk_info_bar_add_button (GTK_INFO_BAR (bar), _("_Cancel"), GTK_RESPONSE_CANCEL);
	}
}

static void
gedit_progress_info_bar_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
	GeditProgressInfoBar *bar;

	bar = GEDIT_PROGRESS_INFO_BAR (object);

	switch (prop_id)
	{
		case PROP_HAS_CANCEL_BUTTON:
			gedit_progress_info_bar_set_has_cancel_button (bar,
								       g_value_get_boolean (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_progress_info_bar_class_init (GeditProgressInfoBarClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	gobject_class->set_property = gedit_progress_info_bar_set_property;

	properties[PROP_HAS_CANCEL_BUTTON] =
		 g_param_spec_boolean ("has-cancel-button",
				       "Has Cancel Button",
				       "If the message bar has a cancel button",
				       TRUE,
				       G_PARAM_WRITABLE |
				       G_PARAM_CONSTRUCT_ONLY |
				       G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (gobject_class, LAST_PROP, properties);

	/* Bind class to template */
	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/gedit/ui/gedit-progress-info-bar.ui");
	gtk_widget_class_bind_template_child (widget_class, GeditProgressInfoBar, image);
	gtk_widget_class_bind_template_child (widget_class, GeditProgressInfoBar, label);
	gtk_widget_class_bind_template_child (widget_class, GeditProgressInfoBar, progress);
}

static void
gedit_progress_info_bar_init (GeditProgressInfoBar *bar)
{
	gtk_widget_init_template (GTK_WIDGET (bar));
}

GtkWidget *
gedit_progress_info_bar_new (const gchar *icon_name,
			     const gchar *markup,
			     gboolean     has_cancel)
{
	GeditProgressInfoBar *bar;

	g_return_val_if_fail (icon_name != NULL, NULL);
	g_return_val_if_fail (markup != NULL, NULL);

	bar = GEDIT_PROGRESS_INFO_BAR (g_object_new (GEDIT_TYPE_PROGRESS_INFO_BAR,
						     "has-cancel-button", has_cancel,
						     NULL));

	gedit_progress_info_bar_set_icon_name (bar, icon_name);
	gedit_progress_info_bar_set_markup (bar, markup);

	return GTK_WIDGET (bar);
}

void
gedit_progress_info_bar_set_icon_name (GeditProgressInfoBar *bar,
				       const gchar          *icon_name)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_INFO_BAR (bar));
	g_return_if_fail (icon_name != NULL);

	gtk_image_set_from_icon_name (GTK_IMAGE (bar->image),
				      icon_name,
				      GTK_ICON_SIZE_SMALL_TOOLBAR);
}

void
gedit_progress_info_bar_set_markup (GeditProgressInfoBar *bar,
				    const gchar          *markup)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_INFO_BAR (bar));
	g_return_if_fail (markup != NULL);

	gtk_label_set_markup (GTK_LABEL (bar->label), markup);
}

void
gedit_progress_info_bar_set_text (GeditProgressInfoBar *bar,
				  const gchar          *text)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_INFO_BAR (bar));
	g_return_if_fail (text != NULL);

	gtk_label_set_text (GTK_LABEL (bar->label), text);
}

void
gedit_progress_info_bar_set_fraction (GeditProgressInfoBar *bar,
				      gdouble               fraction)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_INFO_BAR (bar));

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar->progress), fraction);
}

void
gedit_progress_info_bar_pulse (GeditProgressInfoBar *bar)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_INFO_BAR (bar));

	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (bar->progress));
}

/* ex:set ts=8 noet: */
