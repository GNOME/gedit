/*
 * gedit-open-links-plugin.h
 *
 * Copyright (C) 2020 James Seibel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GEDIT_OPEN_LINKS_PLUGIN_H
#define GEDIT_OPEN_LINKS_PLUGIN_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_OPEN_LINKS_PLUGIN	 		(gedit_open_links_plugin_get_type ())
#define GEDIT_OPEN_LINKS_PLUGIN(o)		 		(G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_TYPE_OPEN_LINKS_PLUGIN, GeditOpenLinksPlugin))
#define GEDIT_OPEN_LINKS_PLUGIN_CLASS(k)	 	(G_TYPE_CHECK_CLASS_CAST((k), GEDIT_TYPE_OPEN_LINKS_PLUGIN, GeditOpenLinksPluginClass))
#define GEDIT_IS_OPEN_LINKS_PLUGIN(o)	 		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_TYPE_OPEN_LINKS_PLUGIN))
#define GEDIT_IS_OPEN_LINKS_PLUGIN_CLASS(k) 	(G_TYPE_CHECK_CLASS_TYPE ((k), GEDIT_TYPE_OPEN_LINKS_PLUGIN))
#define GEDIT_OPEN_LINKS_GET_CLASS(o)	 		(G_TYPE_INSTANCE_GET_CLASS ((o), GEDIT_TYPE_OPEN_LINKS_PLUGIN, GeditOpenLinksPluginClass))

typedef struct _GeditOpenLinksPlugin			GeditOpenLinksPlugin;
typedef struct _GeditOpenLinksPluginPrivate		GeditOpenLinksPluginPrivate;
typedef struct _GeditOpenLinksPluginClass		GeditOpenLinksPluginClass;

struct _GeditOpenLinksPlugin
{
	PeasExtensionBase parent_instance;

	/* < private > */
	GeditOpenLinksPluginPrivate *priv;
};

struct _GeditOpenLinksPluginClass
{
	PeasExtensionBaseClass parent_class;
};

GType					gedit_open_links_plugin_get_type (void) G_GNUC_CONST;

G_MODULE_EXPORT void 	peas_register_types	(PeasObjectModule *module);

G_END_DECLS

#endif /* GEDIT_OPEN_LINKS_PLUGIN_H */

/* ex:set ts=8 noet: */
