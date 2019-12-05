/*
 * Copyright (C) 2009 - Ignacio Casal Quinteiro <icq@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
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

#include "gedit-check-update-plugin.h"

#include <glib/gi18n.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-utils.h>
#include <libsoup/soup.h>
#include <gtk/gtk.h>
#include <stdlib.h>

#define CHECKUPDATE_BASE_SETTINGS	"org.gnome.gedit.plugins.checkupdate"
#define CHECKUPDATE_KEY_IGNORE_VERSION	"ignore-version"

#define VERSION_PLACE "<a href=\"[0-9]\\.[0-9]+/\">"

#ifdef G_OS_WIN32
#define GEDIT_URL "http://ftp.acc.umu.se/pub/gnome/binaries/win32/gedit/"
#define FILE_REGEX "gedit\\-setup\\-[0-9]+\\.[0-9]+\\.[0-9]+(\\-[0-9]+)?\\.exe"
#else
#define GEDIT_URL "http://ftp.acc.umu.se/pub/gnome/binaries/mac/gedit/"
#define FILE_REGEX "gedit\\-[0-9]+\\.[0-9]+\\.[0-9]+(\\-[0-9]+)?(_.*)?\\.dmg"
#endif

#ifdef OS_OSX
#include "gedit/gedit-app-osx.h"
#endif

static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);


struct _GeditCheckUpdatePluginPrivate
{
	SoupSession *session;

	GSettings *settings;

	GeditWindow *window;

	gchar *url;
	gchar *version;
};

enum
{
	PROP_0,
	PROP_WINDOW
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditCheckUpdatePlugin,
				gedit_check_update_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init)
				G_ADD_PRIVATE_DYNAMIC (GeditCheckUpdatePlugin))

static void
gedit_check_update_plugin_init (GeditCheckUpdatePlugin *plugin)
{
	plugin->priv = gedit_check_update_plugin_get_instance_private (plugin);

	gedit_debug_message (DEBUG_PLUGINS,
			     "GeditCheckUpdatePlugin initializing");

	plugin->priv->session = soup_session_async_new ();

	plugin->priv->settings = g_settings_new (CHECKUPDATE_BASE_SETTINGS);
}

static void
gedit_check_update_plugin_dispose (GObject *object)
{
	GeditCheckUpdatePlugin *plugin = GEDIT_CHECK_UPDATE_PLUGIN (object);

	g_clear_object (&plugin->priv->session);
	g_clear_object (&plugin->priv->settings);
	g_clear_object (&plugin->priv->window);

	gedit_debug_message (DEBUG_PLUGINS,
			     "GeditCheckUpdatePlugin disposing");

	G_OBJECT_CLASS (gedit_check_update_plugin_parent_class)->dispose (object);
}

static void
gedit_check_update_plugin_finalize (GObject *object)
{
	GeditCheckUpdatePlugin *plugin = GEDIT_CHECK_UPDATE_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS,
			     "GeditCheckUpdatePlugin finalizing");

	g_free (plugin->priv->url);
	g_free (plugin->priv->version);

	G_OBJECT_CLASS (gedit_check_update_plugin_parent_class)->finalize (object);
}

static void
gedit_check_update_plugin_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
	GeditCheckUpdatePlugin *plugin = GEDIT_CHECK_UPDATE_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			plugin->priv->window = GEDIT_WINDOW (g_value_dup_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_check_update_plugin_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
	GeditCheckUpdatePlugin *plugin = GEDIT_CHECK_UPDATE_PLUGIN (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_value_set_object (value, plugin->priv->window);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
set_contents (GtkWidget *infobar,
	      GtkWidget *contents)
{
	GtkWidget *content_area;

	content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (infobar));
	gtk_container_add (GTK_CONTAINER (content_area), contents);
}

static void
set_message_area_text_and_icon (GtkWidget        *message_area,
				const gchar      *primary_text,
				const gchar      *secondary_text)
{
	GtkWidget *hbox_content;
	GtkWidget *vbox;
	gchar *primary_markup;
	gchar *secondary_markup;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;

	hbox_content = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_show (hbox_content);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox_content), vbox, TRUE, TRUE, 0);

	primary_markup = g_strdup_printf ("<b>%s</b>", primary_text);
	primary_label = gtk_label_new (primary_markup);
	g_free (primary_markup);
	gtk_widget_show (primary_label);
	gtk_box_pack_start (GTK_BOX (vbox), primary_label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (primary_label), TRUE);
	gtk_widget_set_halign (primary_label, GTK_ALIGN_START);
	gtk_widget_set_can_focus (primary_label, TRUE);
	gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);

	if (secondary_text != NULL)
	{
		secondary_markup = g_strdup_printf ("<small>%s</small>",
						    secondary_text);
		secondary_label = gtk_label_new (secondary_markup);
		g_free (secondary_markup);
		gtk_widget_show (secondary_label);
		gtk_box_pack_start (GTK_BOX (vbox), secondary_label, TRUE, TRUE, 0);
		gtk_widget_set_can_focus (secondary_label, TRUE);
		gtk_label_set_use_markup (GTK_LABEL (secondary_label), TRUE);
		gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
		gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
		gtk_widget_set_halign (secondary_label, GTK_ALIGN_START);
	}

	set_contents (message_area, hbox_content);
}

static void
on_response_cb (GtkWidget              *infobar,
		gint                    response_id,
		GeditCheckUpdatePlugin *plugin)
{
	if (response_id == GTK_RESPONSE_YES)
	{
		GError *error = NULL;

#ifdef OS_OSX
		gedit_app_osx_show_url (GEDIT_APP_OSX (g_application_get_default ()),
		                        plugin->priv->url);
#else
		gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (plugin->priv->window)),
			      plugin->priv->url,
			      GDK_CURRENT_TIME,
			      &error);
#endif
		if (error != NULL)
		{
			GtkWidget *dialog;

			dialog = gtk_message_dialog_new (GTK_WINDOW (plugin->priv->window),
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_CLOSE,
							 _("There was an error displaying the URI."));

			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
								  "%s", error->message);

			g_signal_connect (G_OBJECT (dialog),
					  "response",
					  G_CALLBACK (gtk_widget_destroy),
					  NULL);

			gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

			gtk_widget_show (dialog);

			g_error_free (error);
		}
	}
	else if (response_id == GTK_RESPONSE_NO)
	{
		g_settings_set_string (plugin->priv->settings,
				       CHECKUPDATE_KEY_IGNORE_VERSION,
				       plugin->priv->version);
	}

	gtk_widget_destroy (infobar);
}

static GtkWidget *
create_infobar (GeditWindow *window,
                const gchar *version)
{
	GtkWidget *infobar;
	gchar *message;

	infobar = gtk_info_bar_new ();

	gtk_info_bar_add_buttons (GTK_INFO_BAR (infobar),
				  _("_Download"), GTK_RESPONSE_YES,
				  _("_Ignore Version"), GTK_RESPONSE_NO,
				  NULL);
	gtk_info_bar_set_show_close_button (GTK_INFO_BAR (infobar), TRUE);
	gtk_info_bar_set_message_type (GTK_INFO_BAR (infobar), GTK_MESSAGE_INFO);

	message = g_strdup_printf ("%s (%s)", _("There is a new version of gedit"), version);
	set_message_area_text_and_icon (infobar,
					message,
					_("You can download the new version of gedit"
					  " by clicking on the download button or"
					  " ignore that version and wait for a new one"));

	g_free (message);

	g_signal_connect (infobar, "response",
			  G_CALLBACK (on_response_cb),
			  window);

	return infobar;
}

static void
pack_infobar (GtkWidget *window,
	      GtkWidget *infobar)
{
	GtkWidget *vbox;

	vbox = gtk_bin_get_child (GTK_BIN (window));

	gtk_box_pack_start (GTK_BOX (vbox), infobar, FALSE, FALSE, 0);
	gtk_box_reorder_child (GTK_BOX (vbox), infobar, 2);
}

static gchar *
get_file (const gchar *text,
	  const gchar *regex_place)
{
	GRegex *regex;
	GMatchInfo *match_info;
	gchar *word = NULL;

	regex = g_regex_new (regex_place, 0, 0, NULL);
	g_regex_match (regex, text, 0, &match_info);
	while (g_match_info_matches (match_info))
	{
		g_free (word);

		word = g_match_info_fetch (match_info, 0);

		g_match_info_next (match_info, NULL);
	}
	g_match_info_free (match_info);
	g_regex_unref (regex);

	return word;
}

static void
get_numbers (const gchar *version,
	     gint *major,
	     gint *minor,
	     gint *micro)
{
	gchar **split;
	gint num = 2;

	if (micro != NULL)
		num = 3;

	split = g_strsplit (version, ".", num);
	*major = atoi (split[0]);
	*minor = atoi (split[1]);
	if (micro != NULL)
		*micro = atoi (split[2]);

	g_strfreev (split);
}

static gboolean
newer_version (const gchar *v1,
	       const gchar *v2,
	       gboolean with_micro)
{
	gboolean newer = FALSE;
	gint major1, minor1, micro1;
	gint major2, minor2, micro2;

	if (v1 == NULL || v2 == NULL)
		return FALSE;

	if (with_micro)
	{
		get_numbers (v1, &major1, &minor1, &micro1);
		get_numbers (v2, &major2, &minor2, &micro2);
	}
	else
	{
		get_numbers (v1, &major1, &minor1, NULL);
		get_numbers (v2, &major2, &minor2, NULL);
	}

	if (major1 > major2)
	{
		newer = TRUE;
	}
	else if (minor1 > minor2 && major1 == major2)
	{
		newer = TRUE;
	}
	else if (with_micro && micro1 > micro2 && minor1 == minor2)
	{
		newer = TRUE;
	}

	return newer;
}

static gchar *
parse_file_version (const gchar *file)
{
	gchar *p, *aux;

	p = (gchar *)file;

	while (*p != '\0' && !g_ascii_isdigit (*p))
	{
		p++;
	}

	if (*p == '\0')
		return NULL;

	aux = g_strrstr (p, "-");
	if (aux == NULL)
		aux = g_strrstr (p, ".");

	return g_strndup (p, aux - p);
}

static gchar *
get_ignore_version (GeditCheckUpdatePlugin *plugin)
{
	return g_settings_get_string (plugin->priv->settings,
				      CHECKUPDATE_KEY_IGNORE_VERSION);
}

static void
parse_page_file (SoupSession            *session,
		 SoupMessage            *msg,
		 GeditCheckUpdatePlugin *plugin)
{
	if (msg->status_code == SOUP_STATUS_OK)
	{
		gchar *file;
		gchar *file_version;
		gchar *ignore_version;

		file = get_file (msg->response_body->data, FILE_REGEX);

		if (!file)
		{
            return;
		}

		file_version = parse_file_version (file);
		ignore_version = get_ignore_version (plugin);

		if (newer_version (file_version, VERSION, TRUE) &&
		    (ignore_version == NULL || *ignore_version == '\0' ||
		     newer_version (file_version, ignore_version, TRUE)))
		{
			GtkWidget *infobar;
			gchar *file_url;

			file_url = g_strconcat (plugin->priv->url, file, NULL);

			g_free (plugin->priv->url);
			plugin->priv->url = file_url;
			plugin->priv->version = g_strdup (file_version);

			infobar = create_infobar (plugin->priv->window,
						  file_version);
			pack_infobar (GTK_WIDGET (plugin->priv->window),
						  infobar);
			gtk_widget_show (infobar);
		}

		g_free (ignore_version);
		g_free (file_version);
		g_free (file);
	}
}

static gboolean
is_unstable (const gchar *version)
{
	gchar **split;
	gint minor;
	gboolean unstable = TRUE;;

	split = g_strsplit (version, ".", 2);
	minor = atoi (split[1]);
	g_strfreev (split);

	if ((minor % 2) == 0)
		unstable = FALSE;

	return unstable;
}

static gchar *
get_file_page_version (const gchar *text,
		       const gchar *regex_place)
{
	GRegex *regex;
	GMatchInfo *match_info;
	GString *string = NULL;
	gchar *unstable = NULL;
	gchar *stable = NULL;

	regex = g_regex_new (regex_place, 0, 0, NULL);
	g_regex_match (regex, text, 0, &match_info);
	while (g_match_info_matches (match_info))
	{
		gint end;
		gint i;

		g_match_info_fetch_pos (match_info, 0, NULL, &end);

		string = g_string_new ("");

		i = end;
		while (text[i] != '/')
		{
			string = g_string_append_c (string, text[i]);
			i++;
		}

		if (is_unstable (string->str))
		{
			g_free (unstable);
			unstable = g_string_free (string, FALSE);
		}
		else
		{
			g_free (stable);
			stable = g_string_free (string, FALSE);
		}

		g_match_info_next (match_info, NULL);
	}
	g_match_info_free (match_info);
	g_regex_unref (regex);

	if ((GEDIT_MINOR_VERSION % 2) == 0)
	{
		g_free (unstable);

		return stable;
	}
	else
	{
		/* We need to check that stable isn't newer than unstable */
		if (newer_version (stable, unstable, FALSE))
		{
			g_free (unstable);

			return stable;
		}
		else
		{
			g_free (stable);

			return unstable;
		}
	}
}

static void
parse_page_version (SoupSession            *session,
		    SoupMessage            *msg,
		    GeditCheckUpdatePlugin *plugin)
{
	if (msg->status_code == SOUP_STATUS_OK)
	{
		gchar *version;
		SoupMessage *msg2;

		version = get_file_page_version (msg->response_body->data,
						 VERSION_PLACE);

		plugin->priv->url = g_strconcat (GEDIT_URL, version, "/", NULL);
		g_free (version);
		msg2 = soup_message_new ("GET", plugin->priv->url);

		soup_session_queue_message (session, msg2,
					    (SoupSessionCallback)parse_page_file,
					    plugin);
	}
}

static void
gedit_check_update_plugin_activate (GeditWindowActivatable *activatable)
{
	GeditCheckUpdatePluginPrivate *priv;
	SoupMessage *msg;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_CHECK_UPDATE_PLUGIN (activatable)->priv;

	msg = soup_message_new ("GET", GEDIT_URL);

	soup_session_queue_message (priv->session, msg,
				    (SoupSessionCallback)parse_page_version,
				    activatable);
}

static void
gedit_check_update_plugin_deactivate (GeditWindowActivatable *activatable)
{

	gedit_debug (DEBUG_PLUGINS);

	soup_session_abort (GEDIT_CHECK_UPDATE_PLUGIN (activatable)->priv->session);
}

static void
gedit_check_update_plugin_class_init (GeditCheckUpdatePluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_check_update_plugin_finalize;
	object_class->dispose = gedit_check_update_plugin_dispose;
	object_class->set_property = gedit_check_update_plugin_set_property;
	object_class->get_property = gedit_check_update_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
gedit_check_update_plugin_class_finalize (GeditCheckUpdatePluginClass *klass)
{
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_check_update_plugin_activate;
	iface->deactivate = gedit_check_update_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_check_update_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_CHECK_UPDATE_PLUGIN);
}

/* ex:set ts=8 noet: */
