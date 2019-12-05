
/*
 * gedit-file-browser-message-set-markup.c
 * This file is part of gedit
 *
 * Copyright (C) 2013 - Garrett Regier
 * Copyright (C) 2014 - Jesse van den Kieboom
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

#include "config.h"

#include "gedit-file-browser-message-set-markup.h"

enum
{
	PROP_0,

	PROP_ID,
	PROP_MARKUP,
};

struct _GeditFileBrowserMessageSetMarkupPrivate
{
	gchar *id;
	gchar *markup;
};

G_DEFINE_TYPE_EXTENDED (GeditFileBrowserMessageSetMarkup,
                        gedit_file_browser_message_set_markup,
                        GEDIT_TYPE_MESSAGE,
                        0,
                        G_ADD_PRIVATE (GeditFileBrowserMessageSetMarkup))

static void
gedit_file_browser_message_set_markup_finalize (GObject *obj)
{
	GeditFileBrowserMessageSetMarkup *msg = GEDIT_FILE_BROWSER_MESSAGE_SET_MARKUP (obj);

	g_free (msg->priv->id);
	g_free (msg->priv->markup);

	G_OBJECT_CLASS (gedit_file_browser_message_set_markup_parent_class)->finalize (obj);
}

static void
gedit_file_browser_message_set_markup_get_property (GObject    *obj,
                                                    guint       prop_id,
                                                    GValue     *value,
                                                    GParamSpec *pspec)
{
	GeditFileBrowserMessageSetMarkup *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_SET_MARKUP (obj);

	switch (prop_id)
	{
		case PROP_ID:
			g_value_set_string (value, msg->priv->id);
			break;
		case PROP_MARKUP:
			g_value_set_string (value, msg->priv->markup);
			break;
	}
}

static void
gedit_file_browser_message_set_markup_set_property (GObject      *obj,
                                                    guint         prop_id,
                                                    GValue const *value,
                                                    GParamSpec   *pspec)
{
	GeditFileBrowserMessageSetMarkup *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_SET_MARKUP (obj);

	switch (prop_id)
	{
		case PROP_ID:
		{
			g_free (msg->priv->id);
			msg->priv->id = g_value_dup_string (value);
			break;
		}
		case PROP_MARKUP:
		{
			g_free (msg->priv->markup);
			msg->priv->markup = g_value_dup_string (value);
			break;
		}
	}
}

static void
gedit_file_browser_message_set_markup_class_init (GeditFileBrowserMessageSetMarkupClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = gedit_file_browser_message_set_markup_finalize;

	object_class->get_property = gedit_file_browser_message_set_markup_get_property;
	object_class->set_property = gedit_file_browser_message_set_markup_set_property;

	g_object_class_install_property (object_class,
	                                 PROP_ID,
	                                 g_param_spec_string ("id",
	                                                      "Id",
	                                                      "Id",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_MARKUP,
	                                 g_param_spec_string ("markup",
	                                                      "Markup",
	                                                      "Markup",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));
}

static void
gedit_file_browser_message_set_markup_init (GeditFileBrowserMessageSetMarkup *message)
{
	message->priv = gedit_file_browser_message_set_markup_get_instance_private (message);
}
