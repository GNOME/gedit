
/*
 * gedit-file-browser-message-set-root.c
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

#include "gedit-file-browser-message-set-root.h"
#include "gio/gio.h"

enum
{
	PROP_0,

	PROP_LOCATION,
	PROP_VIRTUAL,
};

struct _GeditFileBrowserMessageSetRootPrivate
{
	GFile *location;
	gchar *virtual;
};

G_DEFINE_TYPE_EXTENDED (GeditFileBrowserMessageSetRoot,
                        gedit_file_browser_message_set_root,
                        GEDIT_TYPE_MESSAGE,
                        0,
                        G_ADD_PRIVATE (GeditFileBrowserMessageSetRoot))

static void
gedit_file_browser_message_set_root_finalize (GObject *obj)
{
	GeditFileBrowserMessageSetRoot *msg = GEDIT_FILE_BROWSER_MESSAGE_SET_ROOT (obj);

	if (msg->priv->location)
	{
		g_object_unref (msg->priv->location);
	}
	g_free (msg->priv->virtual);

	G_OBJECT_CLASS (gedit_file_browser_message_set_root_parent_class)->finalize (obj);
}

static void
gedit_file_browser_message_set_root_get_property (GObject    *obj,
                                                  guint       prop_id,
                                                  GValue     *value,
                                                  GParamSpec *pspec)
{
	GeditFileBrowserMessageSetRoot *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_SET_ROOT (obj);

	switch (prop_id)
	{
		case PROP_LOCATION:
			g_value_set_object (value, msg->priv->location);
			break;
		case PROP_VIRTUAL:
			g_value_set_string (value, msg->priv->virtual);
			break;
	}
}

static void
gedit_file_browser_message_set_root_set_property (GObject      *obj,
                                                  guint         prop_id,
                                                  GValue const *value,
                                                  GParamSpec   *pspec)
{
	GeditFileBrowserMessageSetRoot *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_SET_ROOT (obj);

	switch (prop_id)
	{
		case PROP_LOCATION:
		{
			if (msg->priv->location)
			{
				g_object_unref (msg->priv->location);
			}
			msg->priv->location = g_value_dup_object (value);
			break;
		}
		case PROP_VIRTUAL:
		{
			g_free (msg->priv->virtual);
			msg->priv->virtual = g_value_dup_string (value);
			break;
		}
	}
}

static void
gedit_file_browser_message_set_root_class_init (GeditFileBrowserMessageSetRootClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = gedit_file_browser_message_set_root_finalize;

	object_class->get_property = gedit_file_browser_message_set_root_get_property;
	object_class->set_property = gedit_file_browser_message_set_root_set_property;

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
	                                 PROP_VIRTUAL,
	                                 g_param_spec_string ("virtual",
	                                                      "Virtual",
	                                                      "Virtual",
	                                                      NULL,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));
}

static void
gedit_file_browser_message_set_root_init (GeditFileBrowserMessageSetRoot *message)
{
	message->priv = gedit_file_browser_message_set_root_get_instance_private (message);
}
