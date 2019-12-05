
/*
 * gedit-file-browser-message-activation.c
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

#include "gedit-file-browser-message-activation.h"

enum
{
	PROP_0,

	PROP_ACTIVE,
};

struct _GeditFileBrowserMessageActivationPrivate
{
	gboolean active;
};

G_DEFINE_TYPE_EXTENDED (GeditFileBrowserMessageActivation,
                        gedit_file_browser_message_activation,
                        GEDIT_TYPE_MESSAGE,
                        0,
                        G_ADD_PRIVATE (GeditFileBrowserMessageActivation))

static void
gedit_file_browser_message_activation_get_property (GObject    *obj,
                                                    guint       prop_id,
                                                    GValue     *value,
                                                    GParamSpec *pspec)
{
	GeditFileBrowserMessageActivation *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_ACTIVATION (obj);

	switch (prop_id)
	{
		case PROP_ACTIVE:
			g_value_set_boolean (value, msg->priv->active);
			break;
	}
}

static void
gedit_file_browser_message_activation_set_property (GObject      *obj,
                                                    guint         prop_id,
                                                    GValue const *value,
                                                    GParamSpec   *pspec)
{
	GeditFileBrowserMessageActivation *msg;

	msg = GEDIT_FILE_BROWSER_MESSAGE_ACTIVATION (obj);

	switch (prop_id)
	{
		case PROP_ACTIVE:
			msg->priv->active = g_value_get_boolean (value);
			break;
	}
}

static void
gedit_file_browser_message_activation_class_init (GeditFileBrowserMessageActivationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->get_property = gedit_file_browser_message_activation_get_property;
	object_class->set_property = gedit_file_browser_message_activation_set_property;

	g_object_class_install_property (object_class,
	                                 PROP_ACTIVE,
	                                 g_param_spec_boolean ("active",
	                                                       "Active",
	                                                       "Active",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE |
	                                                       G_PARAM_CONSTRUCT |
	                                                       G_PARAM_STATIC_STRINGS));
}

static void
gedit_file_browser_message_activation_init (GeditFileBrowserMessageActivation *message)
{
	message->priv = gedit_file_browser_message_activation_get_instance_private (message);
}
