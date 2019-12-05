
/*
 * gedit-file-browser-message-add-filter.c
 * This file is part of gedit
 *
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

#include "gedit-file-browser-message-add-filter.h"

enum
{
	PROP_0,

	PROP_OBJECT_PATH,
	PROP_METHOD,
	PROP_ID,
};

struct _GeditFileBrowserMessageAddFilterPrivate
{
	gchar *object_path;
	gchar *method;
	guint id;
};

G_DEFINE_TYPE_EXTENDED (GeditFileBrowserMessageAddFilter,
                        gedit_file_browser_message_add_filter,
                        GEDIT_TYPE_MESSAGE,
                        0,
                        G_ADD_PRIVATE (GeditFileBrowserMessageAddFilter))

static void
gedit_file_browser_message_add_filter_finalize (GObject *obj)
{
	GeditFileBrowserMessageAddFilter *msg = GEDIT_FILE_BROWSER_MESSAGE_ADD_FILTER (obj);

	g_free (msg->priv->object_path);
	g_free (msg->priv->method);

	G_OBJECT_CLASS (gedit_file_browser_message_add_filter_parent_class)->finalize (obj);
}

static void
gedit_file_browser_message_add_filter_get_property (GObject    *obj,
                                                    guint       prop_id,
                                                    GValue     *value,
                                                    GParamSpec *pspec)
{
	GeditFileBrowserMessageAddFilter *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_ADD_FILTER (obj);

	switch (prop_id)
	{
		case PROP_OBJECT_PATH:
			g_value_set_string (value, msg->priv->object_path);
			break;
		case PROP_METHOD:
			g_value_set_string (value, msg->priv->method);
			break;
		case PROP_ID:
			g_value_set_uint (value, msg->priv->id);
			break;
	}
}

static void
gedit_file_browser_message_add_filter_set_property (GObject      *obj,
                                                    guint         prop_id,
                                                    GValue const *value,
                                                    GParamSpec   *pspec)
{
	GeditFileBrowserMessageAddFilter *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_ADD_FILTER (obj);

	switch (prop_id)
	{
		case PROP_OBJECT_PATH:
		{
			g_free (msg->priv->object_path);
			msg->priv->object_path = g_value_dup_string (value);
			break;
		}
		case PROP_METHOD:
		{
			g_free (msg->priv->method);
			msg->priv->method = g_value_dup_string (value);
			break;
		}
		case PROP_ID:
			msg->priv->id = g_value_get_uint (value);
			break;
	}
}

static void
gedit_file_browser_message_add_filter_class_init (GeditFileBrowserMessageAddFilterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = gedit_file_browser_message_add_filter_finalize;

	object_class->get_property = gedit_file_browser_message_add_filter_get_property;
	object_class->set_property = gedit_file_browser_message_add_filter_set_property;

	g_object_class_install_property (object_class,
	                                 PROP_OBJECT_PATH,
	                                 g_param_spec_string ("object-path",
	                                                      "Object Path",
	                                                      "Object Path",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_METHOD,
	                                 g_param_spec_string ("method",
	                                                      "Method",
	                                                      "Method",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_ID,
	                                 g_param_spec_uint ("id",
	                                                    "Id",
	                                                    "Id",
	                                                    0,
	                                                    G_MAXUINT,
	                                                    0,
	                                                    G_PARAM_READWRITE |
	                                                    G_PARAM_CONSTRUCT |
	                                                    G_PARAM_STATIC_STRINGS));
}

static void
gedit_file_browser_message_add_filter_init (GeditFileBrowserMessageAddFilter *message)
{
	message->priv = gedit_file_browser_message_add_filter_get_instance_private (message);
}
