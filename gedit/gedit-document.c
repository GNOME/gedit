/*
 * gedit-document.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2014-2020 Sébastien Wilmet
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

#include "config.h"
#include "gedit-document.h"
#include "gedit-document-private.h"
#include <string.h>
#include <glib/gi18n.h>
#include "gedit-settings.h"
#include "gedit-debug.h"
#include "gedit-utils.h"

#define NO_LANGUAGE_NAME "_NORMAL_"

static void	gedit_document_loaded_real	(GeditDocument *doc);

static void	gedit_document_saved_real	(GeditDocument *doc);

static void	set_content_type		(GeditDocument *doc,
						 const gchar   *content_type);

typedef struct
{
	GtkSourceFile *file;

	TeplMetadata *metadata;

	gchar *content_type;

	GDateTime *time_of_last_save_or_load;

	/* The search context for the incremental search, or the search and
	 * replace. They are mutually exclusive.
	 */
	GtkSourceSearchContext *search_context;

	guint language_set_by_user : 1;

	/* The search is empty if there is no search context, or if the
	 * search text is empty. It is used for the sensitivity of some menu
	 * actions.
	 */
	guint empty_search : 1;

	/* Create file if location points to a non existing file (for example
	 * when opened from the command line).
	 */
	guint create : 1;
} GeditDocumentPrivate;

enum
{
	PROP_0,
	PROP_CONTENT_TYPE,
	PROP_MIME_TYPE,
	PROP_EMPTY_SEARCH,
	N_PROPERTIES
};

enum
{
	SIGNAL_LOAD,
	SIGNAL_LOADED,
	SIGNAL_SAVE,
	SIGNAL_SAVED,
	N_SIGNALS
};

static GParamSpec *properties[N_PROPERTIES];
static guint document_signals[N_SIGNALS];

G_DEFINE_TYPE_WITH_PRIVATE (GeditDocument, gedit_document, TEPL_TYPE_BUFFER)

static void
load_metadata_from_metadata_manager (GeditDocument *doc)
{
	GeditDocumentPrivate *priv = gedit_document_get_instance_private (doc);
	GFile *location;

	location = gtk_source_file_get_location (priv->file);

	if (location != NULL)
	{
		TeplMetadataManager *manager;

		manager = tepl_metadata_manager_get_singleton ();
		tepl_metadata_manager_copy_from (manager, location, priv->metadata);
	}
}

static void
save_metadata_into_metadata_manager (GeditDocument *doc)
{
	GeditDocumentPrivate *priv = gedit_document_get_instance_private (doc);
	GFile *location;

	location = gtk_source_file_get_location (priv->file);

	if (location != NULL)
	{
		TeplMetadataManager *manager;

		manager = tepl_metadata_manager_get_singleton ();
		tepl_metadata_manager_merge_into (manager, location, priv->metadata);
	}
}

static void
update_time_of_last_save_or_load (GeditDocument *doc)
{
	GeditDocumentPrivate *priv = gedit_document_get_instance_private (doc);

	if (priv->time_of_last_save_or_load != NULL)
	{
		g_date_time_unref (priv->time_of_last_save_or_load);
	}

	priv->time_of_last_save_or_load = g_date_time_new_now_utc ();
}

static const gchar *
get_language_string (GeditDocument *doc)
{
	GtkSourceLanguage *lang = gtk_source_buffer_get_language (GTK_SOURCE_BUFFER (doc));

	return lang != NULL ? gtk_source_language_get_id (lang) : NO_LANGUAGE_NAME;
}

static void
save_metadata (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;
	const gchar *language = NULL;
	GtkTextIter iter;
	gchar *position;

	priv = gedit_document_get_instance_private (doc);
	if (priv->language_set_by_user)
	{
		language = get_language_string (doc);
	}

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc),
					  &iter,
					  gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (doc)));

	position = g_strdup_printf ("%d", gtk_text_iter_get_offset (&iter));

	if (language == NULL)
	{
		gedit_document_set_metadata (doc,
					     GEDIT_METADATA_ATTRIBUTE_POSITION, position,
					     NULL);
	}
	else
	{
		gedit_document_set_metadata (doc,
					     GEDIT_METADATA_ATTRIBUTE_POSITION, position,
					     GEDIT_METADATA_ATTRIBUTE_LANGUAGE, language,
					     NULL);
	}

	g_free (position);
}

static void
gedit_document_dispose (GObject *object)
{
	GeditDocument *doc = GEDIT_DOCUMENT (object);
	GeditDocumentPrivate *priv = gedit_document_get_instance_private (doc);

	gedit_debug (DEBUG_DOCUMENT);

	/* Metadata must be saved here and not in finalize because the language
	 * is gone by the time finalize runs.
	 */
	if (priv->metadata != NULL)
	{
		save_metadata (doc);

		g_object_unref (priv->metadata);
		priv->metadata = NULL;
	}

	g_clear_object (&priv->file);
	g_clear_object (&priv->search_context);

	G_OBJECT_CLASS (gedit_document_parent_class)->dispose (object);
}

static void
gedit_document_finalize (GObject *object)
{
	GeditDocumentPrivate *priv = gedit_document_get_instance_private (GEDIT_DOCUMENT (object));

	gedit_debug (DEBUG_DOCUMENT);

	g_free (priv->content_type);

	if (priv->time_of_last_save_or_load != NULL)
	{
		g_date_time_unref (priv->time_of_last_save_or_load);
	}

	G_OBJECT_CLASS (gedit_document_parent_class)->finalize (object);
}

static void
gedit_document_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	GeditDocument *doc = GEDIT_DOCUMENT (object);
	GeditDocumentPrivate *priv = gedit_document_get_instance_private (doc);

	switch (prop_id)
	{
		case PROP_CONTENT_TYPE:
			g_value_take_string (value, gedit_document_get_content_type (doc));
			break;

		case PROP_MIME_TYPE:
			g_value_take_string (value, gedit_document_get_mime_type (doc));
			break;

		case PROP_EMPTY_SEARCH:
			g_value_set_boolean (value, priv->empty_search);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_document_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	GeditDocument *doc = GEDIT_DOCUMENT (object);

	switch (prop_id)
	{
		case PROP_CONTENT_TYPE:
			set_content_type (doc, g_value_get_string (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_document_constructed (GObject *object)
{
	GeditDocument *doc = GEDIT_DOCUMENT (object);
	GeditSettings *settings;
	GSettings *editor_settings;

	settings = _gedit_settings_get_singleton ();
	editor_settings = _gedit_settings_peek_editor_settings (settings);

	/* Bind construct properties. */
	g_settings_bind (editor_settings, GEDIT_SETTINGS_ENSURE_TRAILING_NEWLINE,
			 doc, "implicit-trailing-newline",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_NO_SENSITIVITY);

	G_OBJECT_CLASS (gedit_document_parent_class)->constructed (object);
}

static void
gedit_document_class_init (GeditDocumentClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_document_dispose;
	object_class->finalize = gedit_document_finalize;
	object_class->get_property = gedit_document_get_property;
	object_class->set_property = gedit_document_set_property;
	object_class->constructed = gedit_document_constructed;

	klass->loaded = gedit_document_loaded_real;
	klass->saved = gedit_document_saved_real;

	/**
	 * GeditDocument:content-type:
	 *
	 * The document's content type.
	 */
	properties[PROP_CONTENT_TYPE] =
		g_param_spec_string ("content-type",
		                     "content-type",
		                     "",
		                     NULL,
		                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * GeditDocument:mime-type:
	 *
	 * The document's MIME type.
	 */
	properties[PROP_MIME_TYPE] =
		g_param_spec_string ("mime-type",
		                     "mime-type",
		                     "",
		                     "text/plain",
		                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * GeditDocument:empty-search:
	 *
	 * <warning>
	 * The property is used internally by gedit. It must not be used in a
	 * gedit plugin. The property can be modified or removed at any time.
	 * </warning>
	 *
	 * Whether the search is empty.
	 */
	properties[PROP_EMPTY_SEARCH] =
		g_param_spec_boolean ("empty-search",
		                      "empty-search",
		                      "",
		                      TRUE,
		                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, N_PROPERTIES, properties);

	/**
	 * GeditDocument::load:
	 * @document: the #GeditDocument.
	 *
	 * The "load" signal is emitted at the beginning of a file loading.
	 *
	 * Before gedit 3.14 this signal contained parameters to configure the
	 * file loading (the location, encoding, etc). Plugins should not need
	 * those parameters.
	 */
	document_signals[SIGNAL_LOAD] =
		g_signal_new ("load",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, load),
			      NULL, NULL, NULL,
			      G_TYPE_NONE, 0);

	/**
	 * GeditDocument::loaded:
	 * @document: the #GeditDocument.
	 *
	 * The "loaded" signal is emitted at the end of a successful file
	 * loading.
	 *
	 * Before gedit 3.14 this signal contained a #GError parameter, and the
	 * signal was also emitted if an error occurred. Plugins should not need
	 * the error parameter.
	 */
	document_signals[SIGNAL_LOADED] =
		g_signal_new ("loaded",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditDocumentClass, loaded),
			      NULL, NULL, NULL,
			      G_TYPE_NONE, 0);

	/**
	 * GeditDocument::save:
	 * @document: the #GeditDocument.
	 *
	 * The "save" signal is emitted at the beginning of a file saving.
	 *
	 * Before gedit 3.14 this signal contained parameters to configure the
	 * file saving (the location, encoding, etc). Plugins should not need
	 * those parameters.
	 */
	document_signals[SIGNAL_SAVE] =
		g_signal_new ("save",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, save),
			      NULL, NULL, NULL,
			      G_TYPE_NONE, 0);

	/**
	 * GeditDocument::saved:
	 * @document: the #GeditDocument.
	 *
	 * The "saved" signal is emitted at the end of a successful file saving.
	 *
	 * Before gedit 3.14 this signal contained a #GError parameter, and the
	 * signal was also emitted if an error occurred. To save a document, a
	 * plugin can use the gedit_commands_save_document_async() function and
	 * get the result of the operation with
	 * gedit_commands_save_document_finish().
	 */
	document_signals[SIGNAL_SAVED] =
		g_signal_new ("saved",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditDocumentClass, saved),
			      NULL, NULL, NULL,
			      G_TYPE_NONE, 0);
}

static void
set_language (GeditDocument     *doc,
              GtkSourceLanguage *lang,
              gboolean           set_by_user)
{
	GeditDocumentPrivate *priv;
	GtkSourceLanguage *old_lang;

	gedit_debug (DEBUG_DOCUMENT);

	priv = gedit_document_get_instance_private (doc);

	old_lang = gtk_source_buffer_get_language (GTK_SOURCE_BUFFER (doc));

	if (old_lang == lang)
	{
		return;
	}

	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (doc), lang);

	if (set_by_user)
	{
		const gchar *language = get_language_string (doc);

		gedit_document_set_metadata (doc,
					     GEDIT_METADATA_ATTRIBUTE_LANGUAGE, language,
					     NULL);
	}

	priv->language_set_by_user = set_by_user;
}

static void
save_encoding_metadata (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;
	const GtkSourceEncoding *encoding;
	const gchar *charset;

	gedit_debug (DEBUG_DOCUMENT);

	priv = gedit_document_get_instance_private (doc);

	encoding = gtk_source_file_get_encoding (priv->file);

	if (encoding == NULL)
	{
		encoding = gtk_source_encoding_get_utf8 ();
	}

	charset = gtk_source_encoding_get_charset (encoding);

	gedit_document_set_metadata (doc,
				     GEDIT_METADATA_ATTRIBUTE_ENCODING, charset,
				     NULL);
}

static GtkSourceLanguage *
guess_language (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;
	gchar *data;
	GtkSourceLanguageManager *manager = gtk_source_language_manager_get_default ();
	GtkSourceLanguage *language = NULL;

	priv = gedit_document_get_instance_private (doc);

	data = gedit_document_get_metadata (doc, GEDIT_METADATA_ATTRIBUTE_LANGUAGE);

	if (data != NULL)
	{
		gedit_debug_message (DEBUG_DOCUMENT, "Language from metadata: %s", data);

		if (!g_str_equal (data, NO_LANGUAGE_NAME))
		{
			language = gtk_source_language_manager_get_language (manager, data);
		}

		g_free (data);
	}
	else
	{
		GFile *location;
		gchar *basename = NULL;

		location = gtk_source_file_get_location (priv->file);
		gedit_debug_message (DEBUG_DOCUMENT, "Sniffing Language");

		if (location != NULL)
		{
			basename = g_file_get_basename (location);
		}

		language = gtk_source_language_manager_guess_language (manager,
								       basename,
								       priv->content_type);

		g_free (basename);
	}

	return language;
}

static void
on_content_type_changed (GeditDocument *doc,
			 GParamSpec    *pspec,
			 gpointer       useless)
{
	GeditDocumentPrivate *priv;

	priv = gedit_document_get_instance_private (doc);

	if (!priv->language_set_by_user)
	{
		GtkSourceLanguage *language = guess_language (doc);

		gedit_debug_message (DEBUG_DOCUMENT, "Language: %s",
				     language != NULL ? gtk_source_language_get_name (language) : "None");

		set_language (doc, language, FALSE);
	}
}

static gchar *
get_default_content_type (void)
{
	return g_content_type_from_mime_type ("text/plain");
}

static void
on_location_changed (GtkSourceFile *file,
		     GParamSpec    *pspec,
		     GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT);
	load_metadata_from_metadata_manager (doc);
}

static void
gedit_document_init (GeditDocument *doc)
{
	GeditDocumentPrivate *priv = gedit_document_get_instance_private (doc);
	TeplFile *tepl_file;
	GeditSettings *settings;
	GSettings *editor_settings;

	gedit_debug (DEBUG_DOCUMENT);

	priv->content_type = get_default_content_type ();
	priv->language_set_by_user = FALSE;
	priv->empty_search = TRUE;

	update_time_of_last_save_or_load (doc);

	priv->file = gtk_source_file_new ();
	tepl_file = tepl_buffer_get_file (TEPL_BUFFER (doc));

	g_object_bind_property (priv->file, "location",
				tepl_file, "location",
				G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

	priv->metadata = tepl_metadata_new ();

	g_signal_connect_object (priv->file,
				 "notify::location",
				 G_CALLBACK (on_location_changed),
				 doc,
				 0);

	settings = _gedit_settings_get_singleton ();
	editor_settings = _gedit_settings_peek_editor_settings (settings);

	g_settings_bind (editor_settings, GEDIT_SETTINGS_MAX_UNDO_ACTIONS,
	                 doc, "max-undo-levels",
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_NO_SENSITIVITY);

	g_settings_bind (editor_settings, GEDIT_SETTINGS_SYNTAX_HIGHLIGHTING,
			 doc, "highlight-syntax",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_NO_SENSITIVITY);

	g_settings_bind (editor_settings, GEDIT_SETTINGS_BRACKET_MATCHING,
	                 doc, "highlight-matching-brackets",
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_NO_SENSITIVITY);

	tepl_buffer_provide_style_scheme_id_gsetting (TEPL_BUFFER (doc),
						      editor_settings, GEDIT_SETTINGS_SCHEME,
						      TRUE);

	g_signal_connect (doc,
			  "notify::content-type",
			  G_CALLBACK (on_content_type_changed),
			  NULL);
}

GeditDocument *
gedit_document_new (void)
{
	return g_object_new (GEDIT_TYPE_DOCUMENT, NULL);
}

static gchar *
get_content_type_from_content (GeditDocument *doc)
{
	gchar *content_type;
	gchar *data;
	GtkTextBuffer *buffer;
	GtkTextIter start;
	GtkTextIter end;

	buffer = GTK_TEXT_BUFFER (doc);

	gtk_text_buffer_get_start_iter (buffer, &start);
	end = start;
	gtk_text_iter_forward_chars (&end, 255);

	data = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

	content_type = g_content_type_guess (NULL,
	                                     (const guchar *)data,
	                                     strlen (data),
	                                     NULL);

	g_free (data);

	return content_type;
}

static void
set_content_type_no_guess (GeditDocument *doc,
			   const gchar   *content_type)
{
	GeditDocumentPrivate *priv;
	gchar *dupped_content_type;

	gedit_debug (DEBUG_DOCUMENT);

	priv = gedit_document_get_instance_private (doc);

	if (priv->content_type != NULL &&
	    content_type != NULL &&
	    g_str_equal (priv->content_type, content_type))
	{
		return;
	}

	g_free (priv->content_type);

	/* For compression types, we try to just guess from the content */
	if (gedit_utils_get_compression_type_from_content_type (content_type) !=
	    GTK_SOURCE_COMPRESSION_TYPE_NONE)
	{
		dupped_content_type = get_content_type_from_content (doc);
	}
	else
	{
		dupped_content_type = g_strdup (content_type);
	}

	if (dupped_content_type == NULL ||
	    g_content_type_is_unknown (dupped_content_type))
	{
		priv->content_type = get_default_content_type ();
		g_free (dupped_content_type);
	}
	else
	{
		priv->content_type = dupped_content_type;
	}

	g_object_notify_by_pspec (G_OBJECT (doc), properties[PROP_CONTENT_TYPE]);
}

static void
set_content_type (GeditDocument *doc,
		  const gchar   *content_type)
{
	GeditDocumentPrivate *priv;

	gedit_debug (DEBUG_DOCUMENT);

	priv = gedit_document_get_instance_private (doc);

	if (content_type == NULL)
	{
		GFile *location;
		gchar *guessed_type = NULL;

		/* If content type is null, we guess from the filename */
		location = gtk_source_file_get_location (priv->file);
		if (location != NULL)
		{
			gchar *basename;

			basename = g_file_get_basename (location);
			guessed_type = g_content_type_guess (basename, NULL, 0, NULL);

			g_free (basename);
		}

		set_content_type_no_guess (doc, guessed_type);
		g_free (guessed_type);
	}
	else
	{
		set_content_type_no_guess (doc, content_type);
	}
}

gchar *
gedit_document_get_content_type (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	priv = gedit_document_get_instance_private (doc);

	return g_strdup (priv->content_type);
}

/**
 * gedit_document_get_mime_type:
 * @doc: a #GeditDocument.
 *
 * Note: this never returns %NULL.
 **/
gchar *
gedit_document_get_mime_type (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), g_strdup ("text/plain"));

	priv = gedit_document_get_instance_private (doc);

	if (priv->content_type != NULL &&
	    !g_content_type_is_unknown (priv->content_type))
	{
		return g_content_type_get_mime_type (priv->content_type);
	}

	return g_strdup ("text/plain");
}

static void
loaded_query_info_cb (GFile         *location,
		      GAsyncResult  *result,
		      GeditDocument *doc)
{
	GFileInfo *info;
	GError *error = NULL;

	info = g_file_query_info_finish (location, result, &error);

	if (error != NULL)
	{
		/* Ignore not found error as it can happen when opening a
		 * non-existent file from the command line.
		 */
		if (error->domain != G_IO_ERROR ||
		    error->code != G_IO_ERROR_NOT_FOUND)
		{
			g_warning ("Document loading: query info error: %s", error->message);
		}

		g_error_free (error);
		error = NULL;
	}

	if (info != NULL &&
	    g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE))
	{
		const gchar *content_type;

		content_type = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);

		set_content_type (doc, content_type);
	}

	g_clear_object (&info);

	/* Async operation finished. */
	g_object_unref (doc);
}

static void
gedit_document_loaded_real (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;
	GFile *location;

	priv = gedit_document_get_instance_private (doc);

	if (!priv->language_set_by_user)
	{
		GtkSourceLanguage *language = guess_language (doc);

		gedit_debug_message (DEBUG_DOCUMENT, "Language: %s",
				     language != NULL ? gtk_source_language_get_name (language) : "None");

		set_language (doc, language, FALSE);
	}

	update_time_of_last_save_or_load (doc);
	set_content_type (doc, NULL);

	location = gtk_source_file_get_location (priv->file);

	if (location != NULL)
	{
		/* Keep the doc alive during the async operation. */
		g_object_ref (doc);

		g_file_query_info_async (location,
					 G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
					 G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
					 G_FILE_QUERY_INFO_NONE,
					 G_PRIORITY_DEFAULT,
					 NULL,
					 (GAsyncReadyCallback) loaded_query_info_cb,
					 doc);
	}
}

static void
saved_query_info_cb (GFile         *location,
		     GAsyncResult  *result,
		     GeditDocument *doc)
{
	GeditDocumentPrivate *priv;
	GFileInfo *info;
	const gchar *content_type = NULL;
	GError *error = NULL;

	priv = gedit_document_get_instance_private (doc);

	info = g_file_query_info_finish (location, result, &error);

	if (error != NULL)
	{
		g_warning ("Document saving: query info error: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	if (info != NULL &&
	    g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE))
	{
		content_type = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
	}

	set_content_type (doc, content_type);

	if (info != NULL)
	{
		/* content_type (owned by info) is no longer needed. */
		g_object_unref (info);
	}

	update_time_of_last_save_or_load (doc);

	priv->create = FALSE;

	save_encoding_metadata (doc);

	/* Async operation finished. */
	g_object_unref (doc);
}

static void
gedit_document_saved_real (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;
	GFile *location;

	priv = gedit_document_get_instance_private (doc);

	location = gtk_source_file_get_location (priv->file);

	/* Keep the doc alive during the async operation. */
	g_object_ref (doc);

	g_file_query_info_async (location,
				 G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				 G_FILE_QUERY_INFO_NONE,
				 G_PRIORITY_DEFAULT,
				 NULL,
				 (GAsyncReadyCallback) saved_query_info_cb,
				 doc);
}

gboolean
gedit_document_is_untitled (GeditDocument *doc)
{
	TeplFile *file;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), TRUE);

	file = tepl_buffer_get_file (TEPL_BUFFER (doc));
	return tepl_file_get_location (file) == NULL;
}

/*
 * Deletion and external modification is only checked for local files.
 */
gboolean
_gedit_document_needs_saving (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;
	gboolean externally_modified = FALSE;
	gboolean deleted = FALSE;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	priv = gedit_document_get_instance_private (doc);

	if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)))
	{
		return TRUE;
	}

	if (gtk_source_file_is_local (priv->file))
	{
		gtk_source_file_check_file_on_disk (priv->file);
		externally_modified = gtk_source_file_is_externally_modified (priv->file);
		deleted = gtk_source_file_is_deleted (priv->file);
	}

	return (externally_modified || deleted) && !priv->create;
}

/**
 * gedit_document_set_language:
 * @doc:
 * @lang: (allow-none):
 **/
void
gedit_document_set_language (GeditDocument     *doc,
			     GtkSourceLanguage *lang)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	set_language (doc, lang, TRUE);
}

glong
_gedit_document_get_seconds_since_last_save_or_load (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;
	GDateTime *now;
	GTimeSpan n_microseconds;

	gedit_debug (DEBUG_DOCUMENT);

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), -1);

	priv = gedit_document_get_instance_private (doc);

	if (priv->time_of_last_save_or_load == NULL)
	{
		return -1;
	}

	now = g_date_time_new_now_utc ();
	if (now == NULL)
	{
		return -1;
	}

	n_microseconds = g_date_time_difference (now, priv->time_of_last_save_or_load);
	g_date_time_unref (now);

	return n_microseconds / (1000 * 1000);
}

/**
 * gedit_document_get_metadata:
 * @doc: a #GeditDocument
 * @key: name of the key
 *
 * Gets the metadata assigned to @key.
 *
 * Returns: the value assigned to @key. Free with g_free().
 */
gchar *
gedit_document_get_metadata (GeditDocument *doc,
			     const gchar   *key)
{
	GeditDocumentPrivate *priv;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);
	g_return_val_if_fail (key != NULL, NULL);

	priv = gedit_document_get_instance_private (doc);

	if (priv->metadata == NULL)
	{
		return NULL;
	}

	return tepl_metadata_get (priv->metadata, key);
}

/**
 * gedit_document_set_metadata:
 * @doc: a #GeditDocument
 * @first_key: name of the first key to set
 * @...: (allow-none): value for the first key, followed optionally by more key/value pairs,
 * followed by %NULL.
 *
 * Sets metadata on a document.
 */
void
gedit_document_set_metadata (GeditDocument *doc,
			     const gchar   *first_key,
			     ...)
{
	GeditDocumentPrivate *priv;
	va_list var_args;
	const gchar *key;

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (first_key != NULL);

	priv = gedit_document_get_instance_private (doc);

	if (priv->metadata == NULL)
	{
		return;
	}

	va_start (var_args, first_key);

	for (key = first_key; key != NULL; key = va_arg (var_args, const gchar *))
	{
		const gchar *value = va_arg (var_args, const gchar *);
		tepl_metadata_set (priv->metadata, key, value);
	}

	va_end (var_args);

	save_metadata_into_metadata_manager (doc);
}

static void
update_empty_search (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;
	gboolean new_value;

	priv = gedit_document_get_instance_private (doc);

	if (priv->search_context == NULL)
	{
		new_value = TRUE;
	}
	else
	{
		GtkSourceSearchSettings *search_settings;

		search_settings = gtk_source_search_context_get_settings (priv->search_context);

		new_value = gtk_source_search_settings_get_search_text (search_settings) == NULL;
	}

	if (priv->empty_search != new_value)
	{
		priv->empty_search = new_value;
		g_object_notify_by_pspec (G_OBJECT (doc), properties[PROP_EMPTY_SEARCH]);
	}
}

static void
connect_search_settings (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;
	GtkSourceSearchSettings *search_settings;

	priv = gedit_document_get_instance_private (doc);

	search_settings = gtk_source_search_context_get_settings (priv->search_context);

	/* Note: the signal handler is never disconnected. If the search context
	 * changes its search settings, the old search settings will most
	 * probably be destroyed, anyway. So it shouldn't cause performance
	 * problems.
	 */
	g_signal_connect_object (search_settings,
				 "notify::search-text",
				 G_CALLBACK (update_empty_search),
				 doc,
				 G_CONNECT_SWAPPED);
}

/**
 * gedit_document_set_search_context:
 * @doc: a #GeditDocument
 * @search_context: (allow-none): the new #GtkSourceSearchContext
 *
 * Sets the new search context for the document. Use this function only when the
 * search occurrences are highlighted. So this function should not be used for
 * background searches. The purpose is to have only one highlighted search
 * context at a time in the document.
 *
 * After using this function, you should unref the @search_context. The @doc
 * should be the only owner of the @search_context, so that the Clear Highlight
 * action works. If you need the @search_context after calling this function,
 * use gedit_document_get_search_context().
 */
void
gedit_document_set_search_context (GeditDocument          *doc,
				   GtkSourceSearchContext *search_context)
{
	GeditDocumentPrivate *priv;

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	priv = gedit_document_get_instance_private (doc);

	if (priv->search_context != NULL)
	{
		g_signal_handlers_disconnect_by_func (priv->search_context,
						      connect_search_settings,
						      doc);

		g_object_unref (priv->search_context);
	}

	priv->search_context = search_context;

	if (search_context != NULL)
	{
		GeditSettings *settings;
		GSettings *editor_settings;

		g_object_ref (search_context);

		settings = _gedit_settings_get_singleton ();
		editor_settings = _gedit_settings_peek_editor_settings (settings);

		g_settings_bind (editor_settings, GEDIT_SETTINGS_SEARCH_HIGHLIGHTING,
				 search_context, "highlight",
				 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_NO_SENSITIVITY);

		g_signal_connect_object (search_context,
					 "notify::settings",
					 G_CALLBACK (connect_search_settings),
					 doc,
					 G_CONNECT_SWAPPED);

		connect_search_settings (doc);
	}

	update_empty_search (doc);
}

/**
 * gedit_document_get_search_context:
 * @doc: a #GeditDocument
 *
 * Gets the search context. Use this function only if you have used
 * gedit_document_set_search_context() before. You should not alter other search
 * contexts, so you have to verify that the returned search context is yours.
 * One way to verify that is to compare the search settings object, or to mark
 * the search context with g_object_set_data().
 *
 * Returns: (transfer none): the current search context of the document, or NULL
 * if there is no current search context.
 */
GtkSourceSearchContext *
gedit_document_get_search_context (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	priv = gedit_document_get_instance_private (doc);

	return priv->search_context;
}

gboolean
_gedit_document_get_empty_search (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), TRUE);

	priv = gedit_document_get_instance_private (doc);

	return priv->empty_search;
}

/**
 * gedit_document_get_file:
 * @doc: a #GeditDocument.
 *
 * Gets the associated #GtkSourceFile. You should use it only for reading
 * purposes, not for creating a #GtkSourceFileLoader or #GtkSourceFileSaver,
 * because gedit does some extra work when loading or saving a file and
 * maintains an internal state. If you use in a plugin a file loader or saver on
 * the returned #GtkSourceFile, the internal state of gedit won't be updated.
 *
 * If you want to save the #GeditDocument to a secondary file, you can create a
 * new #GtkSourceFile and use a #GtkSourceFileSaver.
 *
 * Returns: (transfer none): the associated #GtkSourceFile.
 * Since: 3.14
 */
GtkSourceFile *
gedit_document_get_file (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	priv = gedit_document_get_instance_private (doc);

	return priv->file;
}

void
_gedit_document_set_create (GeditDocument *doc,
			    gboolean       create)
{
	GeditDocumentPrivate *priv;

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	priv = gedit_document_get_instance_private (doc);

	priv->create = create != FALSE;
}

gboolean
_gedit_document_get_create (GeditDocument *doc)
{
	GeditDocumentPrivate *priv;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	priv = gedit_document_get_instance_private (doc);

	return priv->create;
}

/* ex:set ts=8 noet: */
