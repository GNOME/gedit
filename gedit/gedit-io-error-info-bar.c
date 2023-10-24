/*
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
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

#include "gedit-io-error-info-bar.h"
#include <tepl/tepl.h>
#include <glib/gi18n.h>
#include "gedit-encodings-combo-box.h"

/* Verbose error reporting for file I/O operations (load, save, revert).
 *
 * For testing, not all I/O errors are easy to reproduce. Here is some
 * documentation:
 *
 * G_IO_ERROR_NOT_REGULAR_FILE:
 * Open for example /dev/null from the command line.
 *
 * G_IO_ERROR_IS_DIRECTORY:
 * The easiest is from the command line.
 *
 * G_IO_ERROR_TIMED_OUT:
 * For example configure your router to add an http(s) domain on the block list
 * in the firewall. Then pass on the command line such an https:// address
 * (normally GIO/GVfs is able to open https:// files) and wait 2 min.
 * There are also probably online services that mock that behavior.
 */

static gboolean
is_recoverable_error (const GError *error)
{
	if (error->domain == G_IO_ERROR)
	{
		switch (error->code)
		{
			case G_IO_ERROR_PERMISSION_DENIED:
			case G_IO_ERROR_NOT_FOUND:
			case G_IO_ERROR_HOST_NOT_FOUND:
			case G_IO_ERROR_TIMED_OUT:
			case G_IO_ERROR_NOT_MOUNTABLE_FILE:
			case G_IO_ERROR_NOT_MOUNTED:
			case G_IO_ERROR_BUSY:
				return TRUE;

			default:
				break;
		}
	}

	return FALSE;
}

static gboolean
is_gio_error (const GError *error,
	      gint          code)
{
	return error->domain == G_IO_ERROR && error->code == code;
}

static GtkWidget *
create_io_loading_error_info_bar (const gchar *primary_msg,
				  const gchar *secondary_msg,
				  gboolean     recoverable_error)
{
	TeplInfoBar *info_bar;

	info_bar = tepl_info_bar_new_simple (GTK_MESSAGE_ERROR,
					     primary_msg,
					     secondary_msg);

	if (recoverable_error)
	{
		/* Since there are several buttons, don't use
		 * gtk_info_bar_set_show_close_button().
		 */
		gtk_info_bar_add_button (GTK_INFO_BAR (info_bar),
					 _("_Retry"),
					 GTK_RESPONSE_OK);

		gtk_info_bar_add_button (GTK_INFO_BAR (info_bar),
					 _("_Cancel"),
					 GTK_RESPONSE_CLOSE);
	}
	else
	{
		gtk_info_bar_set_show_close_button (GTK_INFO_BAR (info_bar), TRUE);
	}

	return GTK_WIDGET (info_bar);
}

static void
get_detailed_error_messages (GFile         *location,
			     const gchar   *uri,
			     const GError  *error,
			     gchar        **primary_msg,
			     gchar        **secondary_msg)
{
	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
	{
		*secondary_msg = g_strdup (_("File not found."));
	}
	else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED))
	{
		gchar *uri_scheme = NULL;

		if (location != NULL)
		{
			uri_scheme = g_file_get_uri_scheme (location);
		}

		if (uri_scheme != NULL &&
		    g_utf8_validate (uri_scheme, -1, NULL))
		{
			/* How to reproduce this case: from the command line,
			 * try to open a URI such as: foo://example.net/file
			 */

			/* Translators: %s is a URI scheme (like for example http:, ftp:, etc.) */
			*secondary_msg = g_strdup_printf (_("“%s:” locations are not supported."),
							  uri_scheme);
		}

		g_free (uri_scheme);
	}
	else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_MOUNTABLE_FILE) ||
		 g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_MOUNTED))
	{
		*secondary_msg = g_strdup (_("The location of the file cannot be accessed."));
	}
	else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME))
	{
		*primary_msg = g_strdup_printf (_("“%s” is not a valid location."), uri);
		*secondary_msg = g_strdup (_("Please check that you typed the "
					     "location correctly and try again."));
	}
	else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_HOST_NOT_FOUND))
	{
		/* How to reproduce this case: from the command line, try to
		 * open a URI such as: ssh://something-that-does-not-exist/file
		 *
		 * But this case is also hit for legitimate network addresses
		 * when the proxy is set up wrong.
		 */
		gchar *file_uri = NULL;
		gchar *hn = NULL;
		gboolean uri_decoded = FALSE;

		if (location != NULL)
		{
			file_uri = g_file_get_uri (location);
		}

		if (file_uri != NULL)
		{
			uri_decoded = g_uri_split_network (file_uri, G_URI_FLAGS_NONE, NULL, &hn, NULL, NULL);
		}

		if (uri_decoded && hn != NULL)
		{
			gchar *host_name;
			gchar *msg1;
			const gchar *msg2;

			host_name = g_utf8_make_valid (hn, -1);

			msg1 = g_strdup_printf (_("Hostname “%s” not known."), host_name);
			msg2 = _("The problem could come from the proxy settings.");

			*secondary_msg = g_strconcat (msg1, "\n", msg2, NULL);

			g_free (host_name);
			g_free (msg1);
		}

		g_free (file_uri);
		g_free (hn);
	}

	if (*primary_msg == NULL && *secondary_msg == NULL)
	{
		*secondary_msg = g_strdup (error->message);
	}
}

GtkWidget *
gedit_unrecoverable_reverting_error_info_bar_new (GFile        *location,
						  const GError *error)
{
	gchar *uri;
	gchar *primary_msg = NULL;
	gchar *secondary_msg = NULL;
	GtkWidget *info_bar;

	g_return_val_if_fail (G_IS_FILE (location), NULL);
	g_return_val_if_fail (error != NULL, NULL);

	uri = g_file_get_parse_name (location);

	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
	{
		secondary_msg = g_strdup (_("File not found. "
					    "Perhaps it has recently been deleted."));
	}
	else
	{
		get_detailed_error_messages (location, uri, error, &primary_msg, &secondary_msg);
	}

	if (primary_msg == NULL)
	{
		primary_msg = g_strdup_printf (_("Could not revert the file “%s”."), uri);
	}

	info_bar = create_io_loading_error_info_bar (primary_msg,
						     secondary_msg,
						     FALSE);

	g_free (uri);
	g_free (primary_msg);
	g_free (secondary_msg);
	return info_bar;
}

static void
add_encodings_combo_box (TeplInfoBar *info_bar)
{
	GtkWidget *hgrid;
	gchar *label_markup;
	GtkWidget *label;
	GtkWidget *combo_box;

	hgrid = gtk_grid_new ();
	gtk_grid_set_column_spacing (GTK_GRID (hgrid), 6);

	label_markup = g_strdup_printf ("<small>%s</small>",
					_("Ch_aracter Encoding:"));
	label = gtk_label_new_with_mnemonic (label_markup);
	g_free (label_markup);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

	combo_box = gedit_encodings_combo_box_new (TRUE);
	g_object_set_data (G_OBJECT (info_bar),
			   "gedit-info-bar-encoding-combo-box",
			   combo_box);

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo_box);

	gtk_container_add (GTK_CONTAINER (hgrid), label);
	gtk_container_add (GTK_CONTAINER (hgrid), combo_box);
	gtk_widget_show_all (hgrid);

	tepl_info_bar_add_content_widget (info_bar, hgrid, TEPL_INFO_BAR_LOCATION_ALONGSIDE_ICON);
}

static GtkWidget *
create_conversion_error_info_bar (const gchar *primary_msg,
				  const gchar *secondary_msg,
				  gboolean     edit_anyway)
{
	GtkMessageType msg_type;
	TeplInfoBar *info_bar;

	msg_type = edit_anyway ? GTK_MESSAGE_WARNING : GTK_MESSAGE_ERROR;

	info_bar = tepl_info_bar_new_simple (msg_type, primary_msg, secondary_msg);

	gtk_info_bar_add_button (GTK_INFO_BAR (info_bar),
				 _("_Retry"),
				 GTK_RESPONSE_OK);

	if (edit_anyway)
	{
		gtk_info_bar_add_button (GTK_INFO_BAR (info_bar),
					 _("_Edit Anyway"),
					 GTK_RESPONSE_YES);
	}

	gtk_info_bar_add_button (GTK_INFO_BAR (info_bar),
				 _("_Cancel"),
				 GTK_RESPONSE_CLOSE);

	add_encodings_combo_box (info_bar);

	return GTK_WIDGET (info_bar);
}

GtkWidget *
gedit_io_loading_error_info_bar_new (GFile                   *location,
				     const GtkSourceEncoding *encoding,
				     const GError            *error)
{
	gchar *uri;
	gchar *primary_msg = NULL;
	gchar *secondary_msg = NULL;
	gboolean edit_anyway = FALSE;
	gboolean convert_error = FALSE;
	GtkWidget *info_bar;

	g_return_val_if_fail (error != NULL, NULL);

	if (location != NULL)
	{
		uri = g_file_get_parse_name (location);
	}
	else
	{
		uri = g_strdup ("stdin");
	}

	if (is_gio_error (error, G_IO_ERROR_TOO_MANY_LINKS))
	{
		secondary_msg = g_strdup (_("The number of followed links is limited and the actual "
					    "file could not be found within this limit."));
	}
	else if (is_gio_error (error, G_IO_ERROR_PERMISSION_DENIED))
	{
		secondary_msg = g_strdup (_("You do not have the permissions necessary to open the file."));
	}
	else if ((is_gio_error (error, G_IO_ERROR_INVALID_DATA) && encoding == NULL) ||
		 (error->domain == GTK_SOURCE_FILE_LOADER_ERROR &&
		  error->code == GTK_SOURCE_FILE_LOADER_ERROR_ENCODING_AUTO_DETECTION_FAILED))
	{
		secondary_msg = g_strconcat (_("Unable to detect the character encoding."), "\n",
					     _("Please check that you are not trying to open a binary file."), "\n",
					     _("Select a character encoding from the menu and try again."), NULL);
		convert_error = TRUE;
	}
	else if (error->domain == GTK_SOURCE_FILE_LOADER_ERROR &&
		 error->code == GTK_SOURCE_FILE_LOADER_ERROR_CONVERSION_FALLBACK)
	{
		primary_msg = g_strdup_printf (_("There was a problem opening the file “%s”."), uri);
		secondary_msg = g_strconcat (_("The file you opened has some invalid characters. "
					       "If you continue editing this file you could corrupt this "
					       "document."), "\n",
					     _("You can also choose another character encoding and try again."),
					     NULL);
		edit_anyway = TRUE;
		convert_error = TRUE;
	}
	else if (is_gio_error (error, G_IO_ERROR_INVALID_DATA) && encoding != NULL)
	{
		gchar *encoding_name = gtk_source_encoding_to_string (encoding);

		primary_msg = g_strdup_printf (_("Could not open the file “%s” using the “%s” character encoding."),
					       uri,
					       encoding_name);
		secondary_msg = g_strconcat (_("Please check that you are not trying to open a binary file."), "\n",
					     _("Select a different character encoding from the menu and try again."), NULL);
		convert_error = TRUE;

		g_free (encoding_name);
	}
	else
	{
		get_detailed_error_messages (location, uri, error, &primary_msg, &secondary_msg);
	}

	if (primary_msg == NULL)
	{
		primary_msg = g_strdup_printf (_("Could not open the file “%s”."), uri);
	}

	if (convert_error)
	{
		info_bar = create_conversion_error_info_bar (primary_msg,
							     secondary_msg,
							     edit_anyway);
	}
	else
	{
		info_bar = create_io_loading_error_info_bar (primary_msg,
							     secondary_msg,
							     is_recoverable_error (error));
	}

	g_free (uri);
	g_free (primary_msg);
	g_free (secondary_msg);
	return info_bar;
}

GtkWidget *
gedit_conversion_error_while_saving_info_bar_new (GFile                   *location,
						  const GtkSourceEncoding *encoding)
{
	gchar *uri;
	gchar *encoding_name;
	gchar *primary_msg = NULL;
	gchar *secondary_msg = NULL;
	GtkWidget *info_bar;

	g_return_val_if_fail (G_IS_FILE (location), NULL);
	g_return_val_if_fail (encoding != NULL, NULL);

	uri = g_file_get_parse_name (location);
	encoding_name = gtk_source_encoding_to_string (encoding);

	primary_msg = g_strdup_printf (_("Could not save the file “%s” using the “%s” character encoding."),
				       uri,
				       encoding_name);
	secondary_msg = g_strconcat (_("The document contains one or more characters that cannot be encoded "
				       "using the specified character encoding."), "\n",
				     _("Select a different character encoding from the menu and try again."), NULL);

	info_bar = create_conversion_error_info_bar (primary_msg,
						     secondary_msg,
						     FALSE);

	g_free (uri);
	g_free (encoding_name);
	g_free (primary_msg);
	g_free (secondary_msg);
	return info_bar;
}

const GtkSourceEncoding *
gedit_conversion_error_info_bar_get_encoding (GtkWidget *info_bar)
{
	gpointer combo_box;

	g_return_val_if_fail (GTK_IS_INFO_BAR (info_bar), NULL);

	combo_box = g_object_get_data (G_OBJECT (info_bar),
				       "gedit-info-bar-encoding-combo-box");
	if (combo_box != NULL)
	{
		return gedit_encodings_combo_box_get_selected_encoding (GEDIT_ENCODINGS_COMBO_BOX (combo_box));
	}

	return NULL;
}

GtkWidget *
gedit_unrecoverable_saving_error_info_bar_new (GFile        *location,
					       const GError *error)
{
	gchar *uri;
	gchar *primary_msg = NULL;
	gchar *secondary_msg = NULL;
	TeplInfoBar *info_bar;

	g_return_val_if_fail (G_IS_FILE (location), NULL);
	g_return_val_if_fail (error != NULL, NULL);

	uri = g_file_get_parse_name (location);

	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED))
	{
		gchar *uri_scheme = g_file_get_uri_scheme (location);

		if (uri_scheme != NULL &&
		    g_utf8_validate (uri_scheme, -1, NULL))
		{
			/* Translators: %s is a URI scheme (like for example http:, ftp:, etc.) */
			secondary_msg = g_strdup_printf (_("Cannot handle “%s:” locations in write mode. "
							   "Please check that you typed the "
							   "location correctly and try again."),
							 uri_scheme);
		}
		else
		{
			secondary_msg = g_strdup (_("Cannot handle this location in write mode. "
						    "Please check that you typed the "
						    "location correctly and try again."));
		}

		g_free (uri_scheme);
	}
	else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME))
	{
		secondary_msg = g_strdup_printf (_("“%s” is not a valid location. "
						   "Please check that you typed the "
						   "location correctly and try again."),
						 uri);
	}
	else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED))
	{
		secondary_msg = g_strdup (_("You do not have the permissions necessary to save the file. "
					    "Please check that you typed the "
					    "location correctly and try again."));
	}
	else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE))
	{
		secondary_msg = g_strdup (_("There is not enough disk space to save the file. "
					    "Please free some disk space and try again."));
	}
	else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_READ_ONLY))
	{
		secondary_msg = g_strdup (_("You are trying to save the file on a read-only disk. "
					    "Please check that you typed the location "
					    "correctly and try again."));
	}
	else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))
	{
		secondary_msg = g_strdup (_("A file with the same name already exists. "
					    "Please use a different name."));
	}
	else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_FILENAME_TOO_LONG))
	{
		secondary_msg = g_strdup (_("The disk where you are trying to save the file has "
					    "a limitation on length of the file names. "
					    "Please use a shorter name."));
	}
	else
	{
		get_detailed_error_messages (location,
					     uri,
					     error,
					     &primary_msg,
					     &secondary_msg);
	}

	if (primary_msg == NULL)
	{
		primary_msg = g_strdup_printf (_("Could not save the file “%s”."), uri);
	}

	info_bar = tepl_info_bar_new_simple (GTK_MESSAGE_ERROR, primary_msg, secondary_msg);
	gtk_info_bar_set_show_close_button (GTK_INFO_BAR (info_bar), TRUE);

	g_free (uri);
	g_free (primary_msg);
	g_free (secondary_msg);
	return GTK_WIDGET (info_bar);
}

/* ex:set ts=8 noet: */
