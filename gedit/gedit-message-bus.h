/*
 * gedit-message-bus.h
 * This file is part of gedit
 *
 * Copyright (C) 2008-2010 - Jesse van den Kieboom
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

#ifndef GEDIT_MESSAGE_BUS_H
#define GEDIT_MESSAGE_BUS_H

#include <glib-object.h>
#include <gedit/gedit-message.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_MESSAGE_BUS			(gedit_message_bus_get_type ())
#define GEDIT_MESSAGE_BUS(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_MESSAGE_BUS, GeditMessageBus))
#define GEDIT_MESSAGE_BUS_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_MESSAGE_BUS, GeditMessageBus const))
#define GEDIT_MESSAGE_BUS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_MESSAGE_BUS, GeditMessageBusClass))
#define GEDIT_IS_MESSAGE_BUS(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_MESSAGE_BUS))
#define GEDIT_IS_MESSAGE_BUS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_MESSAGE_BUS))
#define GEDIT_MESSAGE_BUS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_MESSAGE_BUS, GeditMessageBusClass))

typedef struct _GeditMessageBus		GeditMessageBus;
typedef struct _GeditMessageBusClass	GeditMessageBusClass;
typedef struct _GeditMessageBusPrivate	GeditMessageBusPrivate;

struct _GeditMessageBus
{
	GObject parent;

	GeditMessageBusPrivate *priv;
};

struct _GeditMessageBusClass
{
	GObjectClass parent_class;

	void (*dispatch)      (GeditMessageBus  *bus,
	                       GeditMessage     *message);
	void (*registered)    (GeditMessageBus  *bus,
	                       const gchar      *object_path,
	                       const gchar      *method);
	void (*unregistered)  (GeditMessageBus  *bus,
	                       const gchar      *object_path,
	                       const gchar      *method);
};

typedef void (* GeditMessageCallback)   (GeditMessageBus  *bus,
                                         GeditMessage     *message,
                                         gpointer          user_data);

typedef void (* GeditMessageBusForeach) (gchar const      *object_path,
                                         gchar const      *method,
                                         gpointer          user_data);

GEDIT_API
GType             gedit_message_bus_get_type           (void) G_GNUC_CONST;

GEDIT_API
GeditMessageBus  *gedit_message_bus_get_default        (void);
GEDIT_API
GeditMessageBus  *gedit_message_bus_new                (void);

GEDIT_API
GType             gedit_message_bus_lookup             (GeditMessageBus        *bus,
                                                        const gchar            *object_path,
                                                        const gchar            *method);

GEDIT_API
void              gedit_message_bus_register           (GeditMessageBus        *bus,
                                                        GType                   message_type,
                                                        const gchar            *object_path,
                                                        const gchar            *method);

GEDIT_API
void              gedit_message_bus_unregister         (GeditMessageBus        *bus,
                                                        const gchar            *object_path,
                                                        const gchar            *method);

GEDIT_API
void              gedit_message_bus_unregister_all     (GeditMessageBus        *bus,
                                                        const gchar            *object_path);

GEDIT_API
gboolean          gedit_message_bus_is_registered      (GeditMessageBus        *bus,
                                                        const gchar            *object_path,
                                                        const gchar            *method);

GEDIT_API
void              gedit_message_bus_foreach            (GeditMessageBus        *bus,
                                                        GeditMessageBusForeach  func,
                                                        gpointer                user_data);

GEDIT_API
guint             gedit_message_bus_connect            (GeditMessageBus        *bus,
                                                        const gchar            *object_path,
                                                        const gchar            *method,
                                                        GeditMessageCallback    callback,
                                                        gpointer                user_data,
                                                        GDestroyNotify          destroy_data);

GEDIT_API
void              gedit_message_bus_disconnect         (GeditMessageBus        *bus,
                                                        guint                   id);

GEDIT_API
void              gedit_message_bus_disconnect_by_func (GeditMessageBus        *bus,
                                                        const gchar            *object_path,
                                                        const gchar            *method,
                                                        GeditMessageCallback    callback,
                                                        gpointer                user_data);

GEDIT_API
void              gedit_message_bus_block              (GeditMessageBus        *bus,
                                                        guint                   id);
GEDIT_API
void              gedit_message_bus_block_by_func      (GeditMessageBus        *bus,
                                                        const gchar            *object_path,
                                                        const gchar            *method,
                                                        GeditMessageCallback    callback,
                                                        gpointer                user_data);

GEDIT_API
void              gedit_message_bus_unblock            (GeditMessageBus        *bus,
                                                        guint                   id);
GEDIT_API
void              gedit_message_bus_unblock_by_func    (GeditMessageBus        *bus,
                                                        const gchar            *object_path,
                                                        const gchar            *method,
                                                        GeditMessageCallback    callback,
                                                        gpointer                user_data);

GEDIT_API
void              gedit_message_bus_send_message       (GeditMessageBus        *bus,
                                                        GeditMessage           *message);
GEDIT_API
void              gedit_message_bus_send_message_sync  (GeditMessageBus        *bus,
                                                        GeditMessage           *message);

GEDIT_API
void              gedit_message_bus_send               (GeditMessageBus        *bus,
                                                        const gchar            *object_path,
                                                        const gchar            *method,
                                                        const gchar            *first_property,
                                                        ...) G_GNUC_NULL_TERMINATED;

GEDIT_API
GeditMessage     *gedit_message_bus_send_sync          (GeditMessageBus        *bus,
                                                        const gchar            *object_path,
                                                        const gchar            *method,
                                                        const gchar            *first_property,
                                                        ...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif /* GEDIT_MESSAGE_BUS_H */

/* ex:set ts=8 noet: */
