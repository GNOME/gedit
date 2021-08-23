/*
 * gedit-message.h
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

#ifndef GEDIT_MESSAGE_H
#define GEDIT_MESSAGE_H

#include <glib-object.h>
#include <gedit/gedit-macros.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_MESSAGE			(gedit_message_get_type ())
#define GEDIT_MESSAGE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_MESSAGE, GeditMessage))
#define GEDIT_MESSAGE_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_MESSAGE, GeditMessage const))
#define GEDIT_MESSAGE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_MESSAGE, GeditMessageClass))
#define GEDIT_IS_MESSAGE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_MESSAGE))
#define GEDIT_IS_MESSAGE_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_MESSAGE))
#define GEDIT_MESSAGE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_MESSAGE, GeditMessageClass))

typedef struct _GeditMessage        GeditMessage;
typedef struct _GeditMessageClass   GeditMessageClass;
typedef struct _GeditMessagePrivate GeditMessagePrivate;

struct _GeditMessage
{
	GObject parent;

	GeditMessagePrivate *priv;
};

struct _GeditMessageClass
{
	GObjectClass parent_class;
};

GEDIT_API
GType        gedit_message_get_type             (void) G_GNUC_CONST;

GEDIT_API
const gchar *gedit_message_get_object_path      (GeditMessage *message);
GEDIT_API
const gchar *gedit_message_get_method           (GeditMessage *message);

GEDIT_API
gboolean     gedit_message_type_has             (GType         gtype,
                                                 const gchar  *propname);

GEDIT_API
gboolean     gedit_message_type_check           (GType         gtype,
                                                 const gchar  *propname,
                                                 GType         value_type);

GEDIT_API
gboolean     gedit_message_has                  (GeditMessage *message,
                                                 const gchar  *propname);

GEDIT_API
gboolean     gedit_message_is_valid_object_path (const gchar  *object_path);
GEDIT_API
gchar       *gedit_message_type_identifier      (const gchar  *object_path,
                                                 const gchar  *method);

G_END_DECLS

#endif /* GEDIT_MESSAGE_H */

/* ex:set ts=8 noet: */
