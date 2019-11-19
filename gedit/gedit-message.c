/*
 * gedit-message.c
 * This file is part of gedit
 *
 * Copyright (C) 2008, 2010 - Jesse van den Kieboom
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

#include "gedit-message.h"

#include <string.h>

/**
 * SECTION:gedit-message
 * @short_description: message bus message object
 * @include: gedit/gedit-message.h
 *
 * Communication on a #GeditMessageBus is done through messages. Messages are
 * sent over the bus and received by connecting callbacks on the message bus.
 * A #GeditMessage is an instantiation of a #GeditMessageType, containing
 * values for the arguments as specified in the message type.
 *
 * A message can be seen as a method call, or signal emission depending on
 * who is the sender and who is the receiver. There is no explicit distinction
 * between methods and signals.
 */

struct _GeditMessagePrivate
{
	gchar *object_path;
	gchar *method;
};

enum
{
	PROP_0,
	PROP_OBJECT_PATH,
	PROP_METHOD,
	LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

G_DEFINE_TYPE_WITH_PRIVATE (GeditMessage, gedit_message, G_TYPE_OBJECT)

static void
gedit_message_finalize (GObject *object)
{
	GeditMessage *message = GEDIT_MESSAGE (object);

	g_free (message->priv->object_path);
	g_free (message->priv->method);

	G_OBJECT_CLASS (gedit_message_parent_class)->finalize (object);
}

static void
gedit_message_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	GeditMessage *msg = GEDIT_MESSAGE (object);

	switch (prop_id)
	{
		case PROP_OBJECT_PATH:
			g_value_set_string (value,
			                    msg->priv->object_path);
			break;
		case PROP_METHOD:
			g_value_set_string (value,
			                    msg->priv->method);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_message_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
	GeditMessage *msg = GEDIT_MESSAGE (object);

	switch (prop_id)
	{
		case PROP_OBJECT_PATH:
			g_free (msg->priv->object_path);
			msg->priv->object_path = g_value_dup_string (value);
			break;
		case PROP_METHOD:
			g_free (msg->priv->method);
			msg->priv->method = g_value_dup_string (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_message_class_init (GeditMessageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = gedit_message_finalize;

	object_class->get_property = gedit_message_get_property;
	object_class->set_property = gedit_message_set_property;

	/**
	 * GeditMessage:object_path:
	 *
	 * The messages object path (e.g. /gedit/object/path).
	 *
	 */
	properties[PROP_OBJECT_PATH] =
		g_param_spec_string ("object-path",
		                     "OBJECT_PATH",
		                     "The message object path",
		                     NULL,
		                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * GeditMessage:method:
	 *
	 * The messages method.
	 *
	 */
	properties[PROP_METHOD] =
		g_param_spec_string ("method",
		                     "METHOD",
		                     "The message method",
		                     NULL,
		                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
gedit_message_init (GeditMessage *self)
{
	self->priv = gedit_message_get_instance_private (self);
}

/**
 * gedit_message_get_method:
 * @message: the #GeditMessage
 *
 * Get the message method.
 *
 * Return value: the message method
 *
 */
const gchar *
gedit_message_get_method (GeditMessage *message)
{
	g_return_val_if_fail (GEDIT_IS_MESSAGE (message), NULL);

	return message->priv->method;
}

/**
 * gedit_message_get_object_path:
 * @message: the #GeditMessage
 *
 * Get the message object path.
 *
 * Return value: the message object path
 *
 */
const gchar *
gedit_message_get_object_path (GeditMessage *message)
{
	g_return_val_if_fail (GEDIT_IS_MESSAGE (message), NULL);

	return message->priv->object_path;
}

/**
 * gedit_message_is_valid_object_path:
 * @object_path: (allow-none): the object path
 *
 * Returns whether @object_path is a valid object path
 *
 * Return value: %TRUE if @object_path is a valid object path
 *
 */
gboolean
gedit_message_is_valid_object_path (const gchar *object_path)
{
	if (!object_path)
	{
		return FALSE;
	}

	/* needs to start with / */
	if (*object_path != '/')
	{
		return FALSE;
	}

	while (*object_path)
	{
		if (*object_path == '/')
		{
			++object_path;

			if (!*object_path || !(g_ascii_isalpha (*object_path) || *object_path == '_'))
			{
				return FALSE;
			}
		}
		else if (!(g_ascii_isalnum (*object_path) || *object_path == '_'))
		{
			return FALSE;
		}

		++object_path;
	}

	return TRUE;
}

/**
 * gedit_message_type_identifier:
 * @object_path: (allow-none): the object path
 * @method: (allow-none): the method
 *
 * Get the string identifier for @method at @object_path.
 *
 * Return value: the identifier for @method at @object_path
 *
 */
gchar *
gedit_message_type_identifier (const gchar *object_path,
                               const gchar *method)
{
	return g_strconcat (object_path, ".", method, NULL);
}

/**
 * gedit_message_has:
 * @message: the #GeditMessage
 * @propname: the property name
 *
 * Check if a message has a certain property.
 *
 * Return Value: %TRUE if message has @propname, %FALSE otherwise
 *
 */
gboolean
gedit_message_has (GeditMessage *message,
                   const gchar  *propname)
{
	GObjectClass *klass;

	g_return_val_if_fail (GEDIT_IS_MESSAGE (message), FALSE);
	g_return_val_if_fail (propname != NULL, FALSE);

	klass = G_OBJECT_GET_CLASS (G_OBJECT (message));

	return g_object_class_find_property (klass, propname) != NULL;
}

gboolean
gedit_message_type_has (GType         gtype,
                        const gchar  *propname)
{
	GObjectClass *klass;
	gboolean ret;

	g_return_val_if_fail (g_type_is_a (gtype, GEDIT_TYPE_MESSAGE), FALSE);
	g_return_val_if_fail (propname != NULL, FALSE);

	klass = g_type_class_ref (gtype);
	ret = g_object_class_find_property (klass, propname) != NULL;
	g_type_class_unref (klass);

	return ret;
}

gboolean
gedit_message_type_check (GType         gtype,
                          const gchar  *propname,
                          GType         value_type)
{
	GObjectClass *klass;
	gboolean ret;
	GParamSpec *spec;

	g_return_val_if_fail (g_type_is_a (gtype, GEDIT_TYPE_MESSAGE), FALSE);
	g_return_val_if_fail (propname != NULL, FALSE);

	klass = g_type_class_ref (gtype);
	spec = g_object_class_find_property (klass, propname);
	ret = spec != NULL && spec->value_type == value_type;
	g_type_class_unref (klass);

	return ret;
}

/* ex:set ts=8 noet: */
