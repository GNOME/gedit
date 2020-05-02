/*
 * This file is part of gedit
 *
 * Copyright (C) 2020 Sébastien Wilmet <swilmet@gnome.org>
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

#ifndef GEDIT_FACTORY_H
#define GEDIT_FACTORY_H

#include <tepl/tepl.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_FACTORY             (gedit_factory_get_type ())
#define GEDIT_FACTORY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_FACTORY, GeditFactory))
#define GEDIT_FACTORY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_FACTORY, GeditFactoryClass))
#define GEDIT_IS_FACTORY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_FACTORY))
#define GEDIT_IS_FACTORY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_FACTORY))
#define GEDIT_FACTORY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_FACTORY, GeditFactoryClass))

typedef struct _GeditFactory         GeditFactory;
typedef struct _GeditFactoryClass    GeditFactoryClass;

struct _GeditFactory
{
	TeplAbstractFactory parent;
};

struct _GeditFactoryClass
{
	TeplAbstractFactoryClass parent_class;
};

GType		gedit_factory_get_type	(void);

GeditFactory *	gedit_factory_new	(void);

G_END_DECLS

#endif /* GEDIT_FACTORY_H */
