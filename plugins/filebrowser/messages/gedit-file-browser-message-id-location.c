
/*
 * gedit-file-browser-message-id-location.c
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

#include "gedit-file-browser-message-id-location.h"
#include "gio/gio.h"

enum
{
	PROP_0,

	PROP_ID,
	PROP_NAME,
	PROP_LOCATION,
	PROP_IS_DIRECTORY,
};

struct _GeditFileBrowserMessageIdLocationPrivate
{
	gchar *id;
	gchar *name;
	GFile *location;
	gboolean is_directory;
};

G_DEFINE_TYPE_EXTENDED (GeditFileBrowserMessageIdLocation,
                        gedit_file_browser_message_id_location,
                        GEDIT_TYPE_MESSAGE,
                        0,
                        G_ADD_PRIVATE (GeditFileBrowserMessageIdLocation))

static void
gedit_file_browser_message_id_location_finalize (GObject *obj)
{
	GeditFileBrowserMessageIdLocation *msg = GEDIT_FILE_BROWSER_MESSAGE_ID_LOCATION (obj);

	g_free (msg->priv->id);
	g_free (msg->priv->name);
	if (msg->priv->location)
	{
		g_object_unref (msg->priv->location);
	}

	G_OBJECT_CLASS (gedit_file_browser_message_id_location_parent_class)->finalize (obj);
}

static void
gedit_file_browser_message_id_location_get_property (GObject    *obj,
                                                     guint       prop_id,
                                                     GValue     *value,
                                                     GParamSpec *pspec)
{
	GeditFileBrowserMessageIdLocation *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_ID_LOCATION (obj);

	switch (prop_id)
	{
		case PROP_ID:
			g_value_set_string (value, msg->priv->id);
			break;
		case PROP_NAME:
			g_value_set_string (value, msg->priv->name);
			break;
		case PROP_LOCATION:
			g_value_set_object (value, msg->priv->location);
			break;
		case PROP_IS_DIRECTORY:
			g_value_set_boolean (value, msg->priv->is_directory);
			break;
	}
}

static void
gedit_file_browser_message_id_location_set_property (GObject      *obj,
                                                     guint         prop_id,
                                                     GValue const *value,
                                                     GParamSpec   *pspec)
{
	GeditFileBrowserMessageIdLocation *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_ID_LOCATION (obj);

	switch (prop_id)
	{
		case PROP_ID:
		{
			g_free (msg->priv->id);
			msg->priv->id = g_value_dup_string (value);
			break;
		}
		case PROP_NAME:
		{
			g_free (msg->priv->name);
			msg->priv->name = g_value_dup_string (value);
			break;
		}
		case PROP_LOCATION:
		{
			if (msg->priv->location)
			{
				g_object_unref (msg->priv->location);
			}
			msg->priv->location = g_value_dup_object (value);
			break;
		}
		case PROP_IS_DIRECTORY:
			msg->priv->is_directory = g_value_get_boolean (value);
			break;
	}
}

static void
gedit_file_browser_message_id_location_class_init (GeditFileBrowserMessageIdLocationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = gedit_file_browser_message_id_location_finalize;

	object_class->get_property = gedit_file_browser_message_id_location_get_property;
	object_class->set_property = gedit_file_browser_message_id_location_set_property;

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
	                                 PROP_NAME,
	                                 g_param_spec_string ("name",
	                                                      "Name",
	                                                      "Name",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_LOCATION,
	                                 g_param_spec_object ("location",
	                                                      "Location",
	                                                      "Location",
	                                                      G_TYPE_FILE,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_IS_DIRECTORY,
	                                 g_param_spec_boolean ("is-directory",
	                                                       "Is Directory",
	                                                       "Is Directory",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE |
	                                                       G_PARAM_CONSTRUCT |
	                                                       G_PARAM_STATIC_STRINGS));
}

static void
gedit_file_browser_message_id_location_init (GeditFileBrowserMessageIdLocation *message)
{
	message->priv = gedit_file_browser_message_id_location_get_instance_private (message);
}
