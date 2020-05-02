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

#include "gedit-factory.h"

G_DEFINE_TYPE (GeditFactory, gedit_factory, TEPL_TYPE_ABSTRACT_FACTORY)

static void
gedit_factory_class_init (GeditFactoryClass *klass)
{
}

static void
gedit_factory_init (GeditFactory *factory)
{
}

GeditFactory *
gedit_factory_new (void)
{
	return g_object_new (GEDIT_TYPE_FACTORY, NULL);
}
