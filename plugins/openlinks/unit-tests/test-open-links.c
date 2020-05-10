/*
 * This file is part of Tepl, a text editor library.
 *
 * Copyright 2016-2020 - Sébastien Wilmet <swilmet@gnome.org>
 *
 * Tepl is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * Tepl is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "../gedit-open-links-plugin.c"

static void
test_uri_detection (void)
{
	GtkTextBuffer *buffer;
	GtkTextIter start;
	GString *uri;
	gboolean is_uri;
	GRegex *uri_char_regex;

	uri_char_regex = gedit_open_links_plugin_get_uri_regex();
	buffer = gtk_text_buffer_new (NULL);
	
	/* Invalid URIs */

	gtk_text_buffer_set_text (buffer, "Not a valid URI", -1);
	gtk_text_buffer_get_start_iter (buffer, &start);
	is_uri = gedit_open_links_plugin_get_uri (&start, uri_char_regex, &uri);
	g_assert_false (is_uri);

	gtk_text_buffer_set_text (buffer, "http//foo.bar/", -1);
	gtk_text_buffer_get_start_iter (buffer, &start);
	is_uri = gedit_open_links_plugin_get_uri (&start, uri_char_regex, &uri);
	g_assert_false (is_uri);

	/* Valid URIs */

	gtk_text_buffer_set_text (buffer, "https://wiki.gnome.org/Apps/Gedit", -1);
	gtk_text_buffer_get_start_iter (buffer, &start);
	is_uri = gedit_open_links_plugin_get_uri (&start, uri_char_regex, &uri);
	g_assert_true (is_uri);

	gtk_text_buffer_set_text (buffer, "http://gedit.org/", -1);
	gtk_text_buffer_get_start_iter (buffer, &start);
	is_uri = gedit_open_links_plugin_get_uri (&start, uri_char_regex, &uri);
	g_assert_true (is_uri);

	gtk_text_buffer_set_text (buffer, "www.google.com", -1);
	gtk_text_buffer_get_start_iter (buffer, &start);
	is_uri = gedit_open_links_plugin_get_uri (&start, uri_char_regex, &uri);
	g_assert_true (is_uri);
}


int
main (int    argc,
      char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/openlinks/test_uri_detection", test_uri_detection);

	return g_test_run ();
}
