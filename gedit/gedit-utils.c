/*
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi
 * Copyright (C) 2003-2005 Paolo Maggi
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

#include "gedit-utils.h"
#include <string.h>
#include <glib/gi18n.h>
#include <tepl/tepl.h>
#include "gedit-debug.h"

gboolean
gedit_utils_menu_position_under_tree_view (GtkTreeView  *tree_view,
					   GdkRectangle *rect)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	gint count_rows;
	GList *rows;
	gint widget_x, widget_y;

	model = gtk_tree_view_get_model (tree_view);
	g_return_val_if_fail (model != NULL, FALSE);

	selection = gtk_tree_view_get_selection (tree_view);
	g_return_val_if_fail (selection != NULL, FALSE);

	count_rows = gtk_tree_selection_count_selected_rows (selection);
	if (count_rows != 1)
		return FALSE;

	rows = gtk_tree_selection_get_selected_rows (selection, &model);
	gtk_tree_view_get_cell_area (tree_view, (GtkTreePath *)(rows->data),
				     gtk_tree_view_get_column (tree_view, 0),
				     rect);

	gtk_tree_view_convert_bin_window_to_widget_coords (tree_view, rect->x, rect->y, &widget_x, &widget_y);
	rect->x = widget_x;
	rect->y = widget_y;

	g_list_free_full (rows, (GDestroyNotify) gtk_tree_path_free);
	return TRUE;
}

static gchar *
uri_get_dirname (const gchar *uri)
{
	gchar *res;
	gchar *str;

	g_return_val_if_fail (uri != NULL, NULL);

	/* CHECK: does it work with uri chaining? - Paolo */
	str = g_path_get_dirname (uri);
	g_return_val_if_fail (str != NULL, g_strdup ("."));

	if ((strlen (str) == 1) && (*str == '.'))
	{
		g_free (str);

		return NULL;
	}

	res = tepl_utils_replace_home_dir_with_tilde (str);

	g_free (str);

	return res;
}

/*
 * _gedit_utils_location_get_dirname_for_display:
 * @location: the location
 *
 * Returns a string suitable to be displayed in the UI indicating the name of
 * the directory where the file is located. For remote files it may also contain
 * the hostname etc. For local files it tries to replace the home dir with `~`.
 *
 * Returns: (transfer full): a string to display the dirname.
 */
gchar *
_gedit_utils_location_get_dirname_for_display (GFile *location)
{
	gchar *uri;
	gchar *res;
	GMount *mount;

	g_return_val_if_fail (G_IS_FILE (location), NULL);

	/* we use the parse name, that is either the local path
	 * or an uri but which is utf8 safe */
	uri = g_file_get_parse_name (location);

	/* FIXME: this is sync... is it a problem? */
	mount = g_file_find_enclosing_mount (location, NULL, NULL);
	if (mount != NULL)
	{
		gchar *mount_name;
		gchar *path = NULL;
		gchar *dirname;

		mount_name = g_mount_get_name (mount);
		g_object_unref (mount);

		/* Obtain the "path" part of the uri.
		 * Note that g_uri_split() can give a different result for path,
		 * especially when tepl_utils_decode_uri() returns a NULL path.
		 * So it needs further work to get rid of tepl_utils_decode_uri().
		 */
		tepl_utils_decode_uri (uri,
				       NULL, NULL,
				       NULL, NULL,
				       &path);

		if (path == NULL)
		{
			dirname = uri_get_dirname (uri);
		}
		else
		{
			dirname = uri_get_dirname (path);
		}

		if (dirname == NULL || strcmp (dirname, ".") == 0)
		{
			res = mount_name;
		}
		else
		{
			res = g_strdup_printf ("%s %s", mount_name, dirname);
			g_free (mount_name);
		}

		g_free (path);
		g_free (dirname);
	}
	else
	{
		/* fallback for local files or uris without mounts */
		res = uri_get_dirname (uri);
	}

	g_free (uri);

	return res;
}

static gboolean
is_valid_scheme_character (gchar c)
{
	return g_ascii_isalnum (c) || c == '+' || c == '-' || c == '.';
}

static gboolean
has_valid_scheme (const gchar *uri)
{
	const gchar *p;

	p = uri;

	if (!is_valid_scheme_character (*p))
	{
		return FALSE;
	}

	do
	{
		p++;
	} while (is_valid_scheme_character (*p));

	return *p == ':';
}

gboolean
gedit_utils_is_valid_location (GFile *location)
{
	const guchar *p;
	gchar *uri;
	gboolean is_valid;

	if (location == NULL)
		return FALSE;

	uri = g_file_get_uri (location);

	if (!has_valid_scheme (uri))
	{
		g_free (uri);
		return FALSE;
	}

	is_valid = TRUE;

	/* We expect to have a fully valid set of characters */
	for (p = (const guchar *)uri; *p; p++) {
		if (*p == '%')
		{
			++p;
			if (!g_ascii_isxdigit (*p))
			{
				is_valid = FALSE;
				break;
			}

			++p;
			if (!g_ascii_isxdigit (*p))
			{
				is_valid = FALSE;
				break;
			}
		}
		else
		{
			if (*p <= 32 || *p >= 128)
			{
				is_valid = FALSE;
				break;
			}
		}
	}

	g_free (uri);

	return is_valid;
}


static gchar *
make_canonical_uri_from_shell_arg (const gchar *str)
{
	GFile *gfile;
	gchar *uri;

	g_return_val_if_fail (str != NULL, NULL);
	g_return_val_if_fail (*str != '\0', NULL);

	/* Note for the future:
	 * FIXME: is still still relevant?
	 *
	 * <federico> paolo: and flame whoever tells
	 * you that file:///gnome/test_files/hëllò
	 * doesn't work --- that's not a valid URI
	 *
	 * <paolo> federico: well, another solution that
	 * does not requires patch to _from_shell_args
	 * is to check that the string returned by it
	 * contains only ASCII chars
	 * <federico> paolo: hmmmm, isn't there
	 * gnome_vfs_is_uri_valid() or something?
	 * <paolo>: I will use gedit_utils_is_valid_uri ()
	 *
	 */

	gfile = g_file_new_for_commandline_arg (str);

	if (gedit_utils_is_valid_location (gfile))
	{
		uri = g_file_get_uri (gfile);
		g_object_unref (gfile);
		return uri;
	}

	g_object_unref (gfile);
	return NULL;
}


/**
 * gedit_utils_basename_for_display:
 * @location: location for which the basename should be displayed
 *
 * Returns: (transfer full): the basename of a file suitable for display to users.
 */
gchar *
gedit_utils_basename_for_display (GFile *location)
{
	gchar *name;
	gchar *hn;
	gchar *uri;

	g_return_val_if_fail (G_IS_FILE (location), NULL);

	uri = g_file_get_uri (location);

	/* First, try to query the display name, but only on local files */
	if (g_file_has_uri_scheme (location, "file"))
	{
		GFileInfo *info;

		info = g_file_query_info (location,
					  G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
					  G_FILE_QUERY_INFO_NONE,
					  NULL,
					  NULL);

		if (info)
		{
			/* Simply get the display name to use as the basename */
			name = g_strdup (g_file_info_get_display_name (info));
			g_object_unref (info);
		}
		else
		{
			/* This is a local file, and therefore we will use
			 * g_filename_display_basename on the local path */
			gchar *local_path;

			local_path = g_file_get_path (location);
			name = g_filename_display_basename (local_path);
			g_free (local_path);
		}
	}
	else if (g_file_has_parent (location, NULL) ||
		 !tepl_utils_decode_uri (uri, NULL, NULL, &hn, NULL, NULL))
	{
		/* For remote files with a parent (so not just http://foo.com)
		   or remote file for which the decoding of the host name fails,
		   use the _parse_name and take basename of that */
		gchar *parse_name;
		gchar *base;

		parse_name = g_file_get_parse_name (location);
		base = g_filename_display_basename (parse_name);
		name = g_uri_unescape_string (base, NULL);

		g_free (base);
		g_free (parse_name);
	}
	else
	{
		/* display '/ on <host>' using the decoded host */
		gchar *hn_utf8;

		if  (hn != NULL)
		{
			hn_utf8 = g_utf8_make_valid (hn, -1);
		}
		else
		{
			/* we should never get here */
			hn_utf8 = g_strdup ("?");
		}

		/* Translators: '/ on <remote-share>' */
		name = g_strdup_printf (_("/ on %s"), hn_utf8);

		g_free (hn_utf8);
		g_free (hn);
	}

	g_free (uri);

	return name;
}

/**
 * gedit_utils_drop_get_uris:
 * @selection_data: the #GtkSelectionData from drag_data_received
 *
 * Create a list of valid uri's from a uri-list drop.
 *
 * Returns: (transfer full): a string array which will hold the uris or
 *           %NULL if there were no valid uris. g_strfreev should be used when
 *           the string array is no longer used
 */
gchar **
gedit_utils_drop_get_uris (GtkSelectionData *selection_data)
{
	gchar **uris;
	gint i;
	gint p = 0;
	gchar **uri_list;

	uris = g_uri_list_extract_uris ((gchar *) gtk_selection_data_get_data (selection_data));
	uri_list = g_new0(gchar *, g_strv_length (uris) + 1);

	for (i = 0; uris[i] != NULL; i++)
	{
		gchar *uri;

		uri = make_canonical_uri_from_shell_arg (uris[i]);

		/* Silently ignore malformed URI/filename */
		if (uri != NULL)
			uri_list[p++] = uri;
	}

	if (*uri_list == NULL)
	{
		g_free(uri_list);
		g_strfreev (uris);
		return NULL;
	}

	g_strfreev (uris);
	return uri_list;
}

GtkSourceCompressionType
gedit_utils_get_compression_type_from_content_type (const gchar *content_type)
{
	if (content_type == NULL)
	{
		return GTK_SOURCE_COMPRESSION_TYPE_NONE;
	}

	if (g_content_type_is_a (content_type, "application/x-gzip"))
	{
		return GTK_SOURCE_COMPRESSION_TYPE_GZIP;
	}

	return GTK_SOURCE_COMPRESSION_TYPE_NONE;
}

/* Copied from nautilus */
static gchar *
get_direct_save_filename (GdkDragContext *context)
{
	guchar *prop_text;
	gint prop_len;

	if (!gdk_property_get (gdk_drag_context_get_source_window  (context), gdk_atom_intern ("XdndDirectSave0", FALSE),
			       gdk_atom_intern ("text/plain", FALSE), 0, 1024, FALSE, NULL, NULL,
			       &prop_len, &prop_text) && prop_text != NULL) {
		return NULL;
	}

	/* Zero-terminate the string */
	prop_text = g_realloc (prop_text, prop_len + 1);
	prop_text[prop_len] = '\0';

	/* Verify that the file name provided by the source is valid */
	if (*prop_text == '\0' ||
	    strchr ((const gchar *) prop_text, G_DIR_SEPARATOR) != NULL) {
		gedit_debug_message (DEBUG_UTILS, "Invalid filename provided by XDS drag site");
		g_free (prop_text);
		return NULL;
	}

	return (gchar *)prop_text;
}

gchar *
gedit_utils_set_direct_save_filename (GdkDragContext *context)
{
	gchar *uri;
	gchar *filename;

	uri = NULL;
	filename = get_direct_save_filename (context);

	if (filename != NULL)
	{
		gchar *tempdir;
		gchar *path;

		tempdir = g_dir_make_tmp ("gedit-drop-XXXXXX", NULL);
		if (tempdir == NULL)
		{
			tempdir = g_strdup (g_get_tmp_dir ());
		}

		path = g_build_filename (tempdir,
					filename,
					NULL);

		uri = g_filename_to_uri (path, NULL, NULL);

		/* Change the property */
		gdk_property_change (gdk_drag_context_get_source_window (context),
				     gdk_atom_intern ("XdndDirectSave0", FALSE),
				     gdk_atom_intern ("text/plain", FALSE), 8,
				     GDK_PROP_MODE_REPLACE, (const guchar *) uri,
				     strlen (uri));

		g_free (tempdir);
		g_free (path);
		g_free (filename);
	}

	return uri;
}

const gchar *
gedit_utils_newline_type_to_string (GtkSourceNewlineType newline_type)
{
	switch (newline_type)
	{
	case GTK_SOURCE_NEWLINE_TYPE_LF:
		return _("Unix/Linux");
	case GTK_SOURCE_NEWLINE_TYPE_CR:
		return _("Mac OS Classic");
	case GTK_SOURCE_NEWLINE_TYPE_CR_LF:
		return _("Windows");
	}

	return NULL;
}

/* ex:set ts=8 noet: */
