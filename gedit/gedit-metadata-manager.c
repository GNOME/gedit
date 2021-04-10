/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-metadata-manager.c
 * This file is part of gedit
 *
 * Copyright (C) 2003-2007  Paolo Maggi
 * Copyright (C) 2019 Canonical LTD
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

#include "gedit-metadata-manager.h"
#include <libxml/xmlreader.h>
#include "gedit-debug.h"

/*
#define GEDIT_METADATA_VERBOSE_DEBUG	1
*/

#define MAX_ITEMS 50

typedef struct _Item Item;

struct _Item
{
	/* Time of last access in seconds since January 1, 1970 UTC. */
	gint64	 	 atime;

	GHashTable	*values;
};

struct _GeditMetadataManager
{
	GObject parent_instance;

	/* It is true if the file has been read. */
	gboolean	 values_loaded;

	guint 		 timeout_id;

	GHashTable	*items;

	gchar		*metadata_filename;
};

enum
{
	PROP_0,
	PROP_METADATA_FILENAME,
	LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

G_DEFINE_TYPE (GeditMetadataManager, gedit_metadata_manager, G_TYPE_OBJECT);

static gboolean gedit_metadata_manager_save (GeditMetadataManager *self);

static void
item_free (gpointer data)
{
	Item *item;

	g_return_if_fail (data != NULL);

#ifdef GEDIT_METADATA_VERBOSE_DEBUG
	gedit_debug (DEBUG_METADATA);
#endif

	item = (Item *)data;

	if (item->values != NULL)
		g_hash_table_destroy (item->values);

	g_free (item);
}

static void
gedit_metadata_manager_arm_timeout (GeditMetadataManager *self)
{
	if (self->timeout_id == 0)
	{
		self->timeout_id =
			g_timeout_add_seconds_full (G_PRIORITY_DEFAULT_IDLE,
						    2,
						    (GSourceFunc)gedit_metadata_manager_save,
						    self,
						    NULL);
	}
}

static void
gedit_metadata_manager_parse_item (GeditMetadataManager *self,
				   xmlDocPtr             doc,
				   xmlNodePtr            cur)
{
	Item *item;

	xmlChar *uri;
	xmlChar *atime;

#ifdef GEDIT_METADATA_VERBOSE_DEBUG
	gedit_debug (DEBUG_METADATA);
#endif

	if (xmlStrcmp (cur->name, (const xmlChar *)"document") != 0)
			return;

	uri = xmlGetProp (cur, (const xmlChar *)"uri");
	if (uri == NULL)
		return;

	atime = xmlGetProp (cur, (const xmlChar *)"atime");
	if (atime == NULL)
	{
		xmlFree (uri);
		return;
	}

	item = g_new0 (Item, 1);

	item->atime = g_ascii_strtoll ((char *)atime, NULL, 0);

	item->values = g_hash_table_new_full (g_str_hash,
					      g_str_equal,
					      g_free,
					      g_free);

	cur = cur->xmlChildrenNode;

	while (cur != NULL)
	{
		if (xmlStrcmp (cur->name, (const xmlChar *)"entry") == 0)
		{
			xmlChar *key;
			xmlChar *value;

			key = xmlGetProp (cur, (const xmlChar *)"key");
			value = xmlGetProp (cur, (const xmlChar *)"value");

			if ((key != NULL) && (value != NULL))
			{
				g_hash_table_insert (item->values,
						     g_strdup ((gchar *)key),
						     g_strdup ((gchar *)value));
			}

			if (key != NULL)
				xmlFree (key);
			if (value != NULL)
				xmlFree (value);
		}

		cur = cur->next;
	}

	g_hash_table_insert (self->items,
			     g_strdup ((gchar *)uri),
			     item);

	xmlFree (uri);
	xmlFree (atime);
}

/* Returns FALSE in case of error. */
static gboolean
gedit_metadata_manager_load_values (GeditMetadataManager *self)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	gedit_debug (DEBUG_METADATA);

	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (self->values_loaded == FALSE, FALSE);

	self->values_loaded = TRUE;

	xmlKeepBlanksDefault (0);

	if (self->metadata_filename == NULL)
	{
		return FALSE;
	}

	/* TODO: avoid races */
	if (!g_file_test (self->metadata_filename, G_FILE_TEST_EXISTS))
	{
		return TRUE;
	}

	doc = xmlParseFile (self->metadata_filename);

	if (doc == NULL)
	{
		return FALSE;
	}

	cur = xmlDocGetRootElement (doc);
	if (cur == NULL)
	{
		g_message ("The metadata file '%s' is empty",
		           g_path_get_basename (self->metadata_filename));
		xmlFreeDoc (doc);

		return TRUE;
	}

	if (xmlStrcmp (cur->name, (const xmlChar *) "metadata"))
	{
		g_message ("File '%s' is of the wrong type",
		           g_path_get_basename (self->metadata_filename));
		xmlFreeDoc (doc);

		return FALSE;
	}

	cur = xmlDocGetRootElement (doc);
	cur = cur->xmlChildrenNode;

	while (cur != NULL)
	{
		gedit_metadata_manager_parse_item (self, doc, cur);

		cur = cur->next;
	}

	xmlFreeDoc (doc);

	return TRUE;
}

/**
 * gedit_metadata_manager_get:
 * @self: a #GeditMetadataManager.
 * @location: a #GFile.
 * @key: a key.
 *
 * Gets the value associated with the specified @key for the file @location.
 */
gchar *
gedit_metadata_manager_get (GeditMetadataManager *self,
			    GFile                *location,
			    const gchar          *key)
{
	Item *item;
	gchar *value;
	gchar *uri;

	g_return_val_if_fail (GEDIT_IS_METADATA_MANAGER (self), NULL);
	g_return_val_if_fail (G_IS_FILE (location), NULL);
	g_return_val_if_fail (key != NULL, NULL);

	uri = g_file_get_uri (location);

	gedit_debug_message (DEBUG_METADATA, "URI: %s --- key: %s", uri, key );

	if (!self->values_loaded)
	{
		gboolean res;

		res = gedit_metadata_manager_load_values (self);

		if (!res)
		{
			g_free (uri);
			return NULL;
		}
	}

	item = (Item *)g_hash_table_lookup (self->items, uri);

	g_free (uri);

	if (item == NULL)
		return NULL;

	item->atime = g_get_real_time () / 1000;

	if (item->values == NULL)
		return NULL;

	value = g_hash_table_lookup (item->values, key);

	if (value == NULL)
		return NULL;
	else
		return g_strdup (value);
}

/**
 * gedit_metadata_manager_set:
 * @self: a #GeditMetadataManager.
 * @location: a #GFile.
 * @key: a key.
 * @value: the value associated with the @key.
 *
 * Sets the @key to contain the given @value for the file @location.
 */
void
gedit_metadata_manager_set (GeditMetadataManager *self,
			    GFile                *location,
			    const gchar          *key,
			    const gchar          *value)
{
	Item *item;
	gchar *uri;

	g_return_if_fail (GEDIT_IS_METADATA_MANAGER (self));
	g_return_if_fail (G_IS_FILE (location));
	g_return_if_fail (key != NULL);

	uri = g_file_get_uri (location);

	gedit_debug_message (DEBUG_METADATA, "URI: %s --- key: %s --- value: %s", uri, key, value);

	if (!self->values_loaded)
	{
		gboolean ok;

		ok = gedit_metadata_manager_load_values (self);

		if (!ok)
		{
			g_free (uri);
			return;
		}
	}

	item = (Item *)g_hash_table_lookup (self->items, uri);

	if (item == NULL)
	{
		item = g_new0 (Item, 1);

		g_hash_table_insert (self->items,
				     g_strdup (uri),
				     item);
	}

	if (item->values == NULL)
	{
		 item->values = g_hash_table_new_full (g_str_hash,
						       g_str_equal,
						       g_free,
						       g_free);
	}

	if (value != NULL)
	{
		g_hash_table_insert (item->values,
				     g_strdup (key),
				     g_strdup (value));
	}
	else
	{
		g_hash_table_remove (item->values,
				     key);
	}

	item->atime = g_get_real_time () / 1000;

	g_free (uri);

	gedit_metadata_manager_arm_timeout (self);
}

static void
save_values (const gchar *key, const gchar *value, xmlNodePtr parent)
{
	xmlNodePtr xml_node;

#ifdef GEDIT_METADATA_VERBOSE_DEBUG
	gedit_debug (DEBUG_METADATA);
#endif

	g_return_if_fail (key != NULL);

	if (value == NULL)
		return;

	xml_node = xmlNewChild (parent,
				NULL,
				(const xmlChar *)"entry",
				NULL);

	xmlSetProp (xml_node,
		    (const xmlChar *)"key",
		    (const xmlChar *)key);
	xmlSetProp (xml_node,
		    (const xmlChar *)"value",
		    (const xmlChar *)value);

#ifdef GEDIT_METADATA_VERBOSE_DEBUG
	gedit_debug_message (DEBUG_METADATA, "entry: %s = %s", key, value);
#endif
}

static void
save_item (const gchar *key, const gpointer *data, xmlNodePtr parent)
{
	xmlNodePtr xml_node;
	const Item *item = (const Item *)data;
	gchar *atime;

#ifdef GEDIT_METADATA_VERBOSE_DEBUG
	gedit_debug (DEBUG_METADATA);
#endif

	g_return_if_fail (key != NULL);

	if (item == NULL)
		return;

	xml_node = xmlNewChild (parent, NULL, (const xmlChar *)"document", NULL);

	xmlSetProp (xml_node, (const xmlChar *)"uri", (const xmlChar *)key);

#ifdef GEDIT_METADATA_VERBOSE_DEBUG
	gedit_debug_message (DEBUG_METADATA, "uri: %s", key);
#endif

	atime = g_strdup_printf ("%" G_GINT64_FORMAT, item->atime);
	xmlSetProp (xml_node, (const xmlChar *)"atime", (const xmlChar *)atime);

#ifdef GEDIT_METADATA_VERBOSE_DEBUG
	gedit_debug_message (DEBUG_METADATA, "atime: %s", atime);
#endif

	g_free (atime);

	g_hash_table_foreach (item->values,
			      (GHFunc)save_values,
			      xml_node);
}

static const gchar *
gedit_metadata_manager_get_oldest (GeditMetadataManager *self)
{
	GHashTableIter iter;
	gpointer key, value, key_to_remove = NULL;
	const Item *item_to_remove = NULL;

	g_hash_table_iter_init (&iter, self->items);
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		const Item *item = (const Item *) value;

		if (key_to_remove == NULL)
		{
			key_to_remove = key;
			item_to_remove = item;
		}
		else
		{
			g_return_val_if_fail (item_to_remove != NULL, NULL);

			if (item->atime < item_to_remove->atime)
				key_to_remove = key;
		}
	}

	return key_to_remove;
}

static void
gedit_metadata_manager_resize_items (GeditMetadataManager *self)
{
	while (g_hash_table_size (self->items) > MAX_ITEMS)
	{
		const gchar *key_to_remove;

		key_to_remove = gedit_metadata_manager_get_oldest (self);
		g_return_if_fail (key_to_remove != NULL);
		g_hash_table_remove (self->items,
				     key_to_remove);
	}
}

static gboolean
gedit_metadata_manager_save (GeditMetadataManager *self)
{
	xmlDocPtr  doc;
	xmlNodePtr root;

	gedit_debug (DEBUG_METADATA);

	self->timeout_id = 0;

	gedit_metadata_manager_resize_items (self);

	xmlIndentTreeOutput = TRUE;

	doc = xmlNewDoc ((const xmlChar *)"1.0");
	if (doc == NULL)
		return TRUE;

	/* Create metadata root */
	root = xmlNewDocNode (doc, NULL, (const xmlChar *)"metadata", NULL);
	xmlDocSetRootElement (doc, root);

	g_hash_table_foreach (self->items,
			      (GHFunc)save_item,
			      root);

	/* FIXME: lock file - Paolo */
	if (self->metadata_filename != NULL)
	{
		gchar *cache_dir;
		int res;

		/* make sure the cache dir exists */
		cache_dir = g_path_get_dirname (self->metadata_filename);
		res = g_mkdir_with_parents (cache_dir, 0755);
		if (res != -1)
		{
			xmlSaveFormatFile (self->metadata_filename,
			                   doc,
			                   1);
		}

		g_free (cache_dir);
	}

	xmlFreeDoc (doc);

	gedit_debug_message (DEBUG_METADATA, "DONE");

	return FALSE;
}

static void
gedit_metadata_manager_get_property (GObject    *object,
				     guint       prop_id,
				     GValue     *value,
				     GParamSpec *pspec)
{
	GeditMetadataManager *self = GEDIT_METADATA_MANAGER (object);

	switch (prop_id)
	{
	case PROP_METADATA_FILENAME:
		g_value_set_string (value, self->metadata_filename);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
gedit_metadata_manager_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
	GeditMetadataManager *self = GEDIT_METADATA_MANAGER (object);

	switch (prop_id)
	{
	case PROP_METADATA_FILENAME:
		self->metadata_filename = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gedit_metadata_manager_init (GeditMetadataManager *self)
{
	gedit_debug (DEBUG_METADATA);

	self->values_loaded = FALSE;

	self->items =
		g_hash_table_new_full (g_str_hash,
				       g_str_equal,
				       g_free,
				       item_free);
}

static void
gedit_metadata_manager_dispose (GObject *object)
{
	GeditMetadataManager *self = GEDIT_METADATA_MANAGER (object);

	gedit_debug (DEBUG_METADATA);

	if (self->timeout_id)
	{
		g_source_remove (self->timeout_id);
		self->timeout_id = 0;
		gedit_metadata_manager_save (self);
	}

	if (self->items != NULL)
		g_hash_table_destroy (self->items);

	g_free (self->metadata_filename);
}

static void
gedit_metadata_manager_class_init (GeditMetadataManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_metadata_manager_dispose;
	object_class->get_property = gedit_metadata_manager_get_property;
	object_class->set_property = gedit_metadata_manager_set_property;

	/**
	 * GeditMetadataManager:metadata-filename:
	 *
	 * The filename where the metadata is stored.
	 */
	properties[PROP_METADATA_FILENAME] =
		g_param_spec_string ("metadata-filename",
		                      "Metadata filename",
		                      "The filename where the metadata is stored",
		                      NULL,
		                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, LAST_PROP, properties);
}

GeditMetadataManager *
gedit_metadata_manager_new (const gchar *metadata_filename)
{
	gedit_debug (DEBUG_METADATA);

	return g_object_new (GEDIT_TYPE_METADATA_MANAGER,
			     "metadata-filename", metadata_filename,
			     NULL);
}

/* ex:set ts=8 noet: */
