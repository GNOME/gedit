
/*
 * gedit-file-browser-message-get-root.c
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

#include "gedit-file-browser-message-get-root.h"
#include "gio/gio.h"

enum
{
	PROP_0,

	PROP_LOCATION,
};

struct _GeditFileBrowserMessageGetRootPrivate
{
	GFile *location;
};

G_DEFINE_TYPE_EXTENDED (GeditFileBrowserMessageGetRoot,
                        gedit_file_browser_message_get_root,
                        GEDIT_TYPE_MESSAGE,
                        0,
                        G_ADD_PRIVATE (GeditFileBrowserMessageGetRoot))

static void
gedit_file_browser_message_get_root_finalize (GObject *obj)
{
	GeditFileBrowserMessageGetRoot *msg = GEDIT_FILE_BROWSER_MESSAGE_GET_ROOT (obj);

	if (msg->priv->location)
	{
		g_object_unref (msg->priv->location);
	}

	G_OBJECT_CLASS (gedit_file_browser_message_get_root_parent_class)->finalize (obj);
}

static void
gedit_file_browser_message_get_root_get_property (GObject    *obj,
                                                  guint       prop_id,
                                                  GValue     *value,
                                                  GParamSpec *pspec)
{
	GeditFileBrowserMessageGetRoot *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_GET_ROOT (obj);

	switch (prop_id)
	{
		case PROP_LOCATION:
			g_value_set_object (value, msg->priv->location);
			break;
	}
}

static void
gedit_file_browser_message_get_root_set_property (GObject      *obj,
                                                  guint         prop_id,
                                                  GValue const *value,
                                                  GParamSpec   *pspec)
{
	GeditFileBrowserMessageGetRoot *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_GET_ROOT (obj);

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
	}
}

static void
gedit_file_browser_message_get_root_class_init (GeditFileBrowserMessageGetRootClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = gedit_file_browser_message_get_root_finalize;

	object_class->get_property = gedit_file_browser_message_get_root_get_property;
	object_class->set_property = gedit_file_browser_message_get_root_set_property;

	g_object_class_install_property (object_class,
	                                 PROP_LOCATION,
	                                 g_param_spec_object ("location",
	                                                      "Location",
	                                                      "Location",
	                                                      G_TYPE_FILE,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));
}

static void
gedit_file_browser_message_get_root_init (GeditFileBrowserMessageGetRoot *message)
{
	message->priv = gedit_file_browser_message_get_root_get_instance_private (message);
}
