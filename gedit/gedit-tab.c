/*
 * gedit-tab.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
 * Copyright (C) 2014, 2015 - Sébastien Wilmet
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

#include "gedit-tab.h"
#include "gedit-tab-private.h"

#include <stdlib.h>
#include <glib/gi18n.h>
#include <tepl/tepl.h>

#include "gedit-app.h"
#include "gedit-app-private.h"
#include "gedit-recent.h"
#include "gedit-utils.h"
#include "gedit-io-error-info-bar.h"
#include "gedit-print-job.h"
#include "gedit-print-preview.h"
#include "gedit-debug.h"
#include "gedit-document.h"
#include "gedit-document-private.h"
#include "gedit-enum-types.h"
#include "gedit-settings.h"
#include "gedit-view-frame.h"

/**
 * SECTION:gedit-tab
 * @Title: GeditTab
 * @Short_description: A document tab
 *
 * #GeditTab is a #GtkContainer that contains a #GeditView and can show other
 * widgets such as a #TeplInfoBar.
 */

#define GEDIT_TAB_KEY "GEDIT_TAB_KEY"

struct _GeditTab
{
	GtkBox parent_instance;

	GeditTabState state;

	GSettings *editor_settings;

	GeditViewFrame *frame;

	GtkWidget *info_bar;

	GeditPrintJob *print_job;
	GtkWidget *print_preview;

	GtkSourceFileSaverFlags save_flags;

	guint scroll_timeout;
	guint scroll_idle;

	gint auto_save_interval;
	guint auto_save_timeout;

	GCancellable *cancellable;

	guint editable : 1;
	guint auto_save : 1;

	guint ask_if_externally_modified : 1;
};

typedef struct _SaverData SaverData;
typedef struct _LoaderData LoaderData;

struct _SaverData
{
	GtkSourceFileSaver *saver;

	GTimer *timer;

	/* Notes about the create_backup saver flag:
	 * - At the beginning of a new file saving, force_no_backup is FALSE.
	 *   The create_backup flag is set to the saver if it is enabled in
	 *   GSettings and if it isn't an auto-save.
	 * - If creating the backup gives an error, and if the user wants to
	 *   save the file without the backup, force_no_backup is set to TRUE
	 *   and the create_backup flag is removed from the saver.
	 *   force_no_backup as TRUE means that the create_backup flag should
	 *   never be added again to the saver (for the current file saving).
	 * - When another error occurs and if the user explicitly retry again
	 *   the file saving, the create_backup flag is added to the saver if
	 *   (1) it is enabled in GSettings, (2) if force_no_backup is FALSE.
	 * - The create_backup flag is added when the user expressed his or her
	 *   willing to save the file, by pressing a button for example. For an
	 *   auto-save, the create_backup flag is thus not added initially, but
	 *   can be added later when an error occurs and the user clicks on a
	 *   button in the info bar to retry the file saving.
	 */
	guint force_no_backup : 1;
};

struct _LoaderData
{
	GeditTab *tab;
	GtkSourceFileLoader *loader;
	GTimer *timer;
	gint line_pos;
	gint column_pos;
	guint user_requested_encoding : 1;
};

enum
{
	PROP_0,
	PROP_NAME,
	PROP_STATE,
	PROP_AUTO_SAVE,
	PROP_AUTO_SAVE_INTERVAL,
	PROP_CAN_CLOSE,
	N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

G_DEFINE_TYPE (GeditTab, gedit_tab, GTK_TYPE_BOX)

/* Prototypes */
static gboolean gedit_tab_auto_save (GeditTab *tab);

static void launch_loader (GTask                   *loading_task,
			   const GtkSourceEncoding *encoding);

static void launch_saver (GTask *saving_task);

static SaverData *
saver_data_new (void)
{
	return g_new0 (SaverData, 1);
}

static void
saver_data_free (SaverData *data)
{
	if (data != NULL)
	{
		if (data->saver != NULL)
		{
			g_object_unref (data->saver);
		}

		if (data->timer != NULL)
		{
			g_timer_destroy (data->timer);
		}

		g_free (data);
	}
}

static LoaderData *
loader_data_new (void)
{
	return g_new0 (LoaderData, 1);
}

static void
loader_data_free (LoaderData *data)
{
	if (data != NULL)
	{
		if (data->loader != NULL)
		{
			g_object_unref (data->loader);
		}

		if (data->timer != NULL)
		{
			g_timer_destroy (data->timer);
		}

		g_free (data);
	}
}

static void
set_editable (GeditTab *tab,
	      gboolean  editable)
{
	GeditView *view;
	gboolean val;

	tab->editable = editable != FALSE;

	view = gedit_tab_get_view (tab);

	val = (tab->state == GEDIT_TAB_STATE_NORMAL &&
	       tab->editable);

	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), val);
}

static void
install_auto_save_timeout (GeditTab *tab)
{
	if (tab->auto_save_timeout == 0)
	{
		g_return_if_fail (tab->auto_save_interval > 0);

		tab->auto_save_timeout = g_timeout_add_seconds (tab->auto_save_interval * 60,
		                                                (GSourceFunc) gedit_tab_auto_save,
		                                                tab);
	}
}

static void
remove_auto_save_timeout (GeditTab *tab)
{
	gedit_debug (DEBUG_TAB);

	if (tab->auto_save_timeout > 0)
	{
		g_source_remove (tab->auto_save_timeout);
		tab->auto_save_timeout = 0;
	}
}

static void
update_auto_save_timeout (GeditTab *tab)
{
	GeditDocument *doc;
	GtkSourceFile *file;

	gedit_debug (DEBUG_TAB);

	doc = gedit_tab_get_document (tab);
	file = gedit_document_get_file (doc);

	if (tab->state == GEDIT_TAB_STATE_NORMAL &&
	    tab->auto_save &&
	    !_gedit_document_is_untitled (doc) &&
	    !gtk_source_file_is_readonly (file))
	{
		install_auto_save_timeout (tab);
	}
	else
	{
		remove_auto_save_timeout (tab);
	}
}

static void
gedit_tab_get_property (GObject    *object,
		        guint       prop_id,
		        GValue     *value,
		        GParamSpec *pspec)
{
	GeditTab *tab = GEDIT_TAB (object);

	switch (prop_id)
	{
		case PROP_NAME:
			g_value_take_string (value, _gedit_tab_get_name (tab));
			break;

		case PROP_STATE:
			g_value_set_enum (value, gedit_tab_get_state (tab));
			break;

		case PROP_AUTO_SAVE:
			g_value_set_boolean (value, gedit_tab_get_auto_save_enabled (tab));
			break;

		case PROP_AUTO_SAVE_INTERVAL:
			g_value_set_int (value, gedit_tab_get_auto_save_interval (tab));
			break;

		case PROP_CAN_CLOSE:
			g_value_set_boolean (value, _gedit_tab_get_can_close (tab));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_tab_set_property (GObject      *object,
		        guint         prop_id,
		        const GValue *value,
		        GParamSpec   *pspec)
{
	GeditTab *tab = GEDIT_TAB (object);

	switch (prop_id)
	{
		case PROP_AUTO_SAVE:
			gedit_tab_set_auto_save_enabled (tab, g_value_get_boolean (value));
			break;

		case PROP_AUTO_SAVE_INTERVAL:
			gedit_tab_set_auto_save_interval (tab, g_value_get_int (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_tab_dispose (GObject *object)
{
	GeditTab *tab = GEDIT_TAB (object);

	g_clear_object (&tab->editor_settings);
	g_clear_object (&tab->print_job);
	g_clear_object (&tab->print_preview);

	remove_auto_save_timeout (tab);

	if (tab->scroll_timeout != 0)
	{
		g_source_remove (tab->scroll_timeout);
		tab->scroll_timeout = 0;
	}

	if (tab->scroll_idle != 0)
	{
		g_source_remove (tab->scroll_idle);
		tab->scroll_idle = 0;
	}

	if (tab->cancellable != NULL)
	{
		g_cancellable_cancel (tab->cancellable);
		g_clear_object (&tab->cancellable);
	}

	G_OBJECT_CLASS (gedit_tab_parent_class)->dispose (object);
}

static void
gedit_tab_grab_focus (GtkWidget *widget)
{
	GeditTab *tab = GEDIT_TAB (widget);

	GTK_WIDGET_CLASS (gedit_tab_parent_class)->grab_focus (widget);

	if (tab->info_bar != NULL)
	{
		gtk_widget_grab_focus (tab->info_bar);
	}
	else
	{
		GeditView *view = gedit_tab_get_view (tab);
		gtk_widget_grab_focus (GTK_WIDGET (view));
	}
}

static void
gedit_tab_class_init (GeditTabClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);

	object_class->dispose = gedit_tab_dispose;
	object_class->get_property = gedit_tab_get_property;
	object_class->set_property = gedit_tab_set_property;

	gtkwidget_class->grab_focus = gedit_tab_grab_focus;

	/**
	 * GeditTab:name:
	 *
	 * The tab's name.
	 *
	 * Deprecated: 47: Use the #TeplBuffer:tepl-short-title property
	 *   instead.
	 */
	properties[PROP_NAME] =
		g_param_spec_string ("name",
		                     "name",
		                     "",
		                     NULL,
		                     G_PARAM_READABLE |
				     G_PARAM_STATIC_STRINGS |
				     G_PARAM_DEPRECATED);

	/**
	 * GeditTab:state:
	 *
	 * The state of the #GeditTab.
	 */
	properties[PROP_STATE] =
		g_param_spec_enum ("state",
		                   "state",
		                   "",
		                   GEDIT_TYPE_TAB_STATE,
		                   GEDIT_TAB_STATE_NORMAL,
		                   G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * GeditTab:autosave:
	 *
	 * Whether the autosave feature is enabled.
	 */
	properties[PROP_AUTO_SAVE] =
		g_param_spec_boolean ("autosave",
		                      "autosave",
		                      "",
		                      TRUE,
		                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * GeditTab:autosave-interval:
	 *
	 * Time in minutes between two autosaves.
	 */
	properties[PROP_AUTO_SAVE_INTERVAL] =
		g_param_spec_int ("autosave-interval",
		                  "autosave-interval",
		                  "",
		                  0,
		                  G_MAXINT,
		                  0,
		                  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * GeditTab:can-close:
	 *
	 * Whether the tab can be closed.
	 */
	properties[PROP_CAN_CLOSE] =
		g_param_spec_boolean ("can-close",
		                      "can-close",
		                      "",
		                      TRUE,
		                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

/**
 * gedit_tab_get_state:
 * @tab: a #GeditTab.
 *
 * Returns: the current #GeditTabState of @tab.
 */
GeditTabState
gedit_tab_get_state (GeditTab *tab)
{
	g_return_val_if_fail (GEDIT_IS_TAB (tab), GEDIT_TAB_STATE_NORMAL);

	return tab->state;
}

static void
set_cursor_according_to_state (GtkTextView   *view,
			       GeditTabState  state)
{
	GdkDisplay *display;
	GdkCursor *cursor;
	GdkWindow *text_window;
	GdkWindow *left_window;

	display = gtk_widget_get_display (GTK_WIDGET (view));

	text_window = gtk_text_view_get_window (view, GTK_TEXT_WINDOW_TEXT);
	left_window = gtk_text_view_get_window (view, GTK_TEXT_WINDOW_LEFT);

	if ((state == GEDIT_TAB_STATE_LOADING)          ||
	    (state == GEDIT_TAB_STATE_REVERTING)        ||
	    (state == GEDIT_TAB_STATE_SAVING)           ||
	    (state == GEDIT_TAB_STATE_PRINTING)         ||
	    (state == GEDIT_TAB_STATE_CLOSING))
	{
		cursor = gdk_cursor_new_from_name (display, "progress");

		if (text_window != NULL)
			gdk_window_set_cursor (text_window, cursor);
		if (left_window != NULL)
			gdk_window_set_cursor (left_window, cursor);

		g_clear_object (&cursor);
	}
	else
	{
		cursor = gdk_cursor_new_from_name (display, "text");

		if (text_window != NULL)
			gdk_window_set_cursor (text_window, cursor);
		if (left_window != NULL)
			gdk_window_set_cursor (left_window, NULL);

		g_clear_object (&cursor);
	}
}

static void
view_realized (GtkTextView *view,
	       GeditTab    *tab)
{
	set_cursor_according_to_state (view, tab->state);
}

static void
set_view_properties_according_to_state (GeditTab      *tab,
					GeditTabState  state)
{
	GeditView *view;
	gboolean val;
	gboolean hl_current_line;

	hl_current_line = g_settings_get_boolean (tab->editor_settings,
						  GEDIT_SETTINGS_HIGHLIGHT_CURRENT_LINE);

	view = gedit_tab_get_view (tab);

	val = ((state == GEDIT_TAB_STATE_NORMAL) &&
	       tab->editable);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), val);

	val = ((state != GEDIT_TAB_STATE_LOADING) &&
	       (state != GEDIT_TAB_STATE_CLOSING));
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), val);

	val = ((state != GEDIT_TAB_STATE_LOADING) &&
	       (state != GEDIT_TAB_STATE_CLOSING) &&
	       (hl_current_line));
	gtk_source_view_set_highlight_current_line (GTK_SOURCE_VIEW (view), val);
}

static void
gedit_tab_set_state (GeditTab      *tab,
		     GeditTabState  state)
{
	g_return_if_fail ((state >= 0) && (state < GEDIT_TAB_NUM_OF_STATES));

	if (tab->state == state)
	{
		return;
	}

	tab->state = state;

	set_view_properties_according_to_state (tab, state);

	/* Hide or show the document.
	 * For GEDIT_TAB_STATE_LOADING_ERROR, tab->frame is either shown or
	 * hidden, depending on the error.
	 */
	if (state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW)
	{
		gtk_widget_hide (GTK_WIDGET (tab->frame));
	}
	else if (state != GEDIT_TAB_STATE_LOADING_ERROR)
	{
		gtk_widget_show (GTK_WIDGET (tab->frame));
	}

	set_cursor_according_to_state (GTK_TEXT_VIEW (gedit_tab_get_view (tab)),
				       state);

	update_auto_save_timeout (tab);

	g_object_notify_by_pspec (G_OBJECT (tab), properties[PROP_STATE]);
	g_object_notify_by_pspec (G_OBJECT (tab), properties[PROP_CAN_CLOSE]);
}

static void
document_modified_changed_cb (GtkTextBuffer *document,
			      GeditTab      *tab)
{
	g_object_notify_by_pspec (G_OBJECT (tab), properties[PROP_CAN_CLOSE]);
}

static void
buffer_short_title_notify_cb (TeplBuffer *buffer,
			      GParamSpec *pspec,
			      GeditTab   *tab)
{
	g_object_notify_by_pspec (G_OBJECT (tab), properties[PROP_NAME]);
}

/* This function must be used carefully, and should be replaced by
 * tepl_tab_add_info_bar() (note the *add*, not *set*).
 * When certain infobars are set, it also configures GeditTab to be in a certain
 * state (e.g. non-editable) and it waits a response from the infobar to restore
 * the GeditTab state. If another infobar is set in the meantime, there will be
 * a bug.
 */
static void
set_info_bar (GeditTab  *tab,
	      GtkWidget *info_bar)
{
	if (tab->info_bar == info_bar)
	{
		return;
	}

	if (tab->info_bar != NULL)
	{
		gtk_widget_destroy (tab->info_bar);
		tab->info_bar = NULL;
	}

	tab->info_bar = info_bar;

	if (info_bar != NULL)
	{
		gtk_box_pack_start (GTK_BOX (tab), info_bar, FALSE, FALSE, 0);
		gtk_widget_show (info_bar);
	}
}

static void
remove_tab (GeditTab *tab)
{
	GtkWidget *notebook;

	notebook = gtk_widget_get_parent (GTK_WIDGET (tab));
	gtk_container_remove (GTK_CONTAINER (notebook), GTK_WIDGET (tab));
}

static void
io_loading_error_info_bar_response (GtkWidget *info_bar,
				    gint       response_id,
				    GTask     *loading_task)
{
	LoaderData *data = g_task_get_task_data (loading_task);
	GFile *location;
	const GtkSourceEncoding *encoding;

	location = gtk_source_file_loader_get_location (data->loader);

	switch (response_id)
	{
		case GTK_RESPONSE_OK:
			encoding = gedit_conversion_error_info_bar_get_encoding (GTK_WIDGET (info_bar));

			set_info_bar (data->tab, NULL);
			gedit_tab_set_state (data->tab, GEDIT_TAB_STATE_LOADING);

			launch_loader (loading_task, encoding);
			break;

		case GTK_RESPONSE_YES:
			/* This means that we want to edit the document anyway */
			set_editable (data->tab, TRUE);
			set_info_bar (data->tab, NULL);
			gedit_tab_set_state (data->tab, GEDIT_TAB_STATE_NORMAL);

			g_task_return_boolean (loading_task, TRUE);
			g_object_unref (loading_task);
			break;

		default:
			if (location != NULL)
			{
				gedit_recent_remove_if_local (location);
			}

			remove_tab (data->tab);

			g_task_return_boolean (loading_task, FALSE);
			g_object_unref (loading_task);
			break;
	}
}

static void
file_already_open_warning_info_bar_response (GtkWidget *info_bar,
					     gint       response_id,
					     GeditTab  *tab)
{
	GeditView *view = gedit_tab_get_view (tab);

	if (response_id == GTK_RESPONSE_YES)
	{
		set_editable (tab, TRUE);
	}

	set_info_bar (tab, NULL);

	gtk_widget_grab_focus (GTK_WIDGET (view));
}

static void
load_cancelled (GtkWidget *bar,
		gint       response_id,
		GTask     *loading_task)
{
	LoaderData *data = g_task_get_task_data (loading_task);

	g_return_if_fail (TEPL_IS_PROGRESS_INFO_BAR (data->tab->info_bar));

	g_cancellable_cancel (g_task_get_cancellable (loading_task));
	remove_tab (data->tab);
}

static void
unrecoverable_reverting_error_info_bar_response (GtkWidget *info_bar,
						 gint       response_id,
						 GTask     *loading_task)
{
	LoaderData *data = g_task_get_task_data (loading_task);
	GeditView *view;

	gedit_tab_set_state (data->tab, GEDIT_TAB_STATE_NORMAL);

	set_info_bar (data->tab, NULL);

	view = gedit_tab_get_view (data->tab);
	gtk_widget_grab_focus (GTK_WIDGET (view));

	g_task_return_boolean (loading_task, FALSE);
	g_object_unref (loading_task);
}

#define MAX_MSG_LENGTH 100

static void
show_loading_info_bar (GTask *loading_task)
{
	LoaderData *data = g_task_get_task_data (loading_task);
	TeplProgressInfoBar *bar;
	GeditDocument *doc;
	gchar *name;
	gchar *dirname = NULL;
	gchar *msg = NULL;
	gchar *name_markup;
	gchar *dirname_markup;
	gint len;

	if (data->tab->info_bar != NULL)
	{
		return;
	}

	gedit_debug (DEBUG_TAB);

	doc = gedit_tab_get_document (data->tab);

	name = tepl_file_get_short_name (tepl_buffer_get_file (TEPL_BUFFER (doc)));
	len = g_utf8_strlen (name, -1);

	/* if the name is awfully long, truncate it and be done with it,
	 * otherwise also show the directory (ellipsized if needed)
	 */
	if (len > MAX_MSG_LENGTH)
	{
		gchar *str;

		str = tepl_utils_str_middle_truncate (name, MAX_MSG_LENGTH);
		g_free (name);
		name = str;
	}
	else
	{
		GtkSourceFile *file = gedit_document_get_file (doc);
		GFile *location = gtk_source_file_get_location (file);

		if (location != NULL)
		{
			gchar *str = _gedit_utils_location_get_dirname_for_display (location);

			/* use the remaining space for the dir, but use a min of 20 chars
			 * so that we do not end up with a dirname like "(a...b)".
			 * This means that in the worst case when the filename is long 99
			 * we have a title long 99 + 20, but I think it's a rare enough
			 * case to be acceptable. It's justa darn title afterall :)
			 */
			dirname = tepl_utils_str_middle_truncate (str,
								  MAX (20, MAX_MSG_LENGTH - len));
			g_free (str);
		}
	}

	name_markup = g_markup_printf_escaped ("<b>%s</b>", name);

	if (data->tab->state == GEDIT_TAB_STATE_REVERTING)
	{
		if (dirname != NULL)
		{
			dirname_markup = g_markup_printf_escaped ("<b>%s</b>", dirname);

			/* Translators: the first %s is a file name (e.g. test.txt) the second one
			   is a directory (e.g. ssh://master.gnome.org/home/users/paolo) */
			msg = g_strdup_printf (_("Reverting %s from %s"),
					       name_markup,
					       dirname_markup);
			g_free (dirname_markup);
		}
		else
		{
			msg = g_strdup_printf (_("Reverting %s"), name_markup);
		}

		bar = tepl_progress_info_bar_new ("document-revert", msg, TRUE);
	}
	else
	{
		if (dirname != NULL)
		{
			dirname_markup = g_markup_printf_escaped ("<b>%s</b>", dirname);

			/* Translators: the first %s is a file name (e.g. test.txt) the second one
			   is a directory (e.g. ssh://master.gnome.org/home/users/paolo) */
			msg = g_strdup_printf (_("Loading %s from %s"),
					       name_markup,
					       dirname_markup);
			g_free (dirname_markup);
		}
		else
		{
			msg = g_strdup_printf (_("Loading %s"), name_markup);
		}

		bar = tepl_progress_info_bar_new ("document-open", msg, TRUE);
	}

	g_signal_connect_object (bar,
				 "response",
				 G_CALLBACK (load_cancelled),
				 loading_task,
				 0);

	set_info_bar (data->tab, GTK_WIDGET (bar));

	g_free (msg);
	g_free (name);
	g_free (name_markup);
	g_free (dirname);
}

static void
show_saving_info_bar (GTask *saving_task)
{
	GeditTab *tab = g_task_get_source_object (saving_task);
	TeplProgressInfoBar *bar;
	GeditDocument *doc;
	gchar *short_name;
	gchar *from;
	gchar *to = NULL;
	gchar *from_markup;
	gchar *to_markup;
	gchar *msg = NULL;
	gint len;

	if (tab->info_bar != NULL)
	{
		return;
	}

	gedit_debug (DEBUG_TAB);

	doc = gedit_tab_get_document (tab);
	short_name = tepl_file_get_short_name (tepl_buffer_get_file (TEPL_BUFFER (doc)));
	len = g_utf8_strlen (short_name, -1);

	/* if the name is awfully long, truncate it and be done with it,
	 * otherwise also show the directory (ellipsized if needed)
	 */
	if (len > MAX_MSG_LENGTH)
	{
		from = tepl_utils_str_middle_truncate (short_name, MAX_MSG_LENGTH);
		g_free (short_name);
	}
	else
	{
		gchar *str;
		SaverData *data;
		GFile *location;

		data = g_task_get_task_data (saving_task);
		location = gtk_source_file_saver_get_location (data->saver);

		from = short_name;
		to = g_file_get_parse_name (location);
		str = tepl_utils_str_middle_truncate (to, MAX (20, MAX_MSG_LENGTH - len));
		g_free (to);

		to = str;
	}

	from_markup = g_markup_printf_escaped ("<b>%s</b>", from);

	if (to != NULL)
	{
		to_markup = g_markup_printf_escaped ("<b>%s</b>", to);

		/* Translators: the first %s is a file name (e.g. test.txt) the second one
		   is a directory (e.g. ssh://master.gnome.org/home/users/paolo) */
		msg = g_strdup_printf (_("Saving %s to %s"), from_markup, to_markup);
		g_free (to_markup);
	}
	else
	{
		msg = g_strdup_printf (_("Saving %s"), from_markup);
	}

	bar = tepl_progress_info_bar_new ("document-save", msg, FALSE);

	set_info_bar (tab, GTK_WIDGET (bar));

	g_free (msg);
	g_free (to);
	g_free (from);
	g_free (from_markup);
}

static void
info_bar_set_progress (GeditTab *tab,
		       goffset   size,
		       goffset   total_size)
{
	TeplProgressInfoBar *progress_info_bar;

	if (tab->info_bar == NULL)
	{
		return;
	}

	gedit_debug_message (DEBUG_TAB, "%" G_GOFFSET_FORMAT "/%" G_GOFFSET_FORMAT, size, total_size);

	g_return_if_fail (TEPL_IS_PROGRESS_INFO_BAR (tab->info_bar));

	progress_info_bar = TEPL_PROGRESS_INFO_BAR (tab->info_bar);

	if (total_size != 0)
	{
		gdouble frac = (gdouble)size / (gdouble)total_size;

		tepl_progress_info_bar_set_fraction (progress_info_bar, frac);
	}
	else if (size != 0)
	{
		tepl_progress_info_bar_pulse (progress_info_bar);
	}
	else
	{
		tepl_progress_info_bar_set_fraction (progress_info_bar, 0);
	}
}

/* Returns whether progress info should be shown. */
static gboolean
should_show_progress_info (GTimer  **timer,
			   goffset   size,
			   goffset   total_size)
{
	gdouble elapsed_time;
	gdouble total_time;
	gdouble remaining_time;

	g_assert (timer != NULL);

	if (*timer == NULL)
	{
		return TRUE;
	}

	elapsed_time = g_timer_elapsed (*timer, NULL);

	/* Wait a little, because at the very beginning it's maybe not very
	 * accurate (it takes initially more time for the first bytes, the
	 * following chunks should arrive more quickly, as a rough guess).
	 */
	if (elapsed_time < 0.5)
	{
		return FALSE;
	}

	/* elapsed_time / total_time = size / total_size */
	total_time = (elapsed_time * total_size) / size;

	remaining_time = total_time - elapsed_time;

	/* Approximately more than 3 seconds remaining. */
	if (remaining_time > 3.0)
	{
		/* Once the progress info bar is shown, it must remain
		 * shown until the end, so we don't need the timer
		 * anymore.
		 */
		g_timer_destroy (*timer);
		*timer = NULL;

		return TRUE;
	}

	return FALSE;
}

static gboolean
scroll_timeout_cb (GeditTab *tab)
{
	GeditView *view;

	view = gedit_tab_get_view (tab);
	tepl_view_scroll_to_cursor (TEPL_VIEW (view));

	tab->scroll_timeout = 0;
	return G_SOURCE_REMOVE;
}

static gboolean
scroll_idle_cb (GeditTab *tab)
{
	/* The idle is not enough, for a detailed analysis of this, see:
	 * https://wiki.gnome.org/Apps/Gedit/FixingTextCutOffBug
	 * or the commit message that changed this.
	 * (here it's a hack, a proper solution in GTK/GtkTextView should be
	 * found).
	 */
	if (tab->scroll_timeout == 0)
	{
		/* Same number of ms as GtkSearchEntry::search-changed delay.
		 * Small enough to not be noticeable, but needs to be at least a
		 * few frames from the GdkFrameClock (during app startup).
		 */
		tab->scroll_timeout = g_timeout_add (150, (GSourceFunc)scroll_timeout_cb, tab);
	}

	tab->scroll_idle = 0;
	return G_SOURCE_REMOVE;
}

static void
unrecoverable_saving_error_info_bar_response (GtkWidget *info_bar,
					      gint       response_id,
					      GTask     *saving_task)
{
	GeditTab *tab = g_task_get_source_object (saving_task);
	GeditView *view;

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);

	set_info_bar (tab, NULL);

	view = gedit_tab_get_view (tab);
	gtk_widget_grab_focus (GTK_WIDGET (view));

	g_task_return_boolean (saving_task, FALSE);
	g_object_unref (saving_task);
}

/* Sets the save flags after an info bar response. */
static void
response_set_save_flags (GTask                   *saving_task,
			 GtkSourceFileSaverFlags  save_flags)
{
	GeditTab *tab = g_task_get_source_object (saving_task);
	SaverData *data = g_task_get_task_data (saving_task);
	gboolean create_backup;

	create_backup = g_settings_get_boolean (tab->editor_settings,
						GEDIT_SETTINGS_CREATE_BACKUP_COPY);

	/* If we are here, it means that the user expressed his or her willing
	 * to save the file, by pressing a button in the info bar. So even if
	 * the file saving was initially an auto-save, we set the create_backup
	 * flag (if the conditions are met).
	 */
	if (create_backup && !data->force_no_backup)
	{
		save_flags |= GTK_SOURCE_FILE_SAVER_FLAGS_CREATE_BACKUP;
	}
	else
	{
		save_flags &= ~GTK_SOURCE_FILE_SAVER_FLAGS_CREATE_BACKUP;
	}

	gtk_source_file_saver_set_flags (data->saver, save_flags);
}

static void
invalid_character_info_bar_response (GtkWidget *info_bar,
				     gint       response_id,
				     GTask     *saving_task)
{
	if (response_id == GTK_RESPONSE_YES)
	{
		GeditTab *tab = g_task_get_source_object (saving_task);
		SaverData *data = g_task_get_task_data (saving_task);
		GtkSourceFileSaverFlags save_flags;

		set_info_bar (tab, NULL);

		/* Don't bug the user again with this... */
		tab->save_flags |= GTK_SOURCE_FILE_SAVER_FLAGS_IGNORE_INVALID_CHARS;

		save_flags = gtk_source_file_saver_get_flags (data->saver);
		save_flags |= GTK_SOURCE_FILE_SAVER_FLAGS_IGNORE_INVALID_CHARS;
		response_set_save_flags (saving_task, save_flags);

		/* Force saving */
		launch_saver (saving_task);
	}
	else
	{
		unrecoverable_saving_error_info_bar_response (info_bar, response_id, saving_task);
	}
}

static void
cant_create_backup_error_info_bar_response (GtkWidget *info_bar,
					    gint       response_id,
					    GTask     *saving_task)
{
	if (response_id == GTK_RESPONSE_YES)
	{
		GeditTab *tab = g_task_get_source_object (saving_task);
		SaverData *data = g_task_get_task_data (saving_task);
		GtkSourceFileSaverFlags save_flags;

		set_info_bar (tab, NULL);

		data->force_no_backup = TRUE;
		save_flags = gtk_source_file_saver_get_flags (data->saver);
		response_set_save_flags (saving_task, save_flags);

		/* Force saving */
		launch_saver (saving_task);
	}
	else
	{
		unrecoverable_saving_error_info_bar_response (info_bar, response_id, saving_task);
	}
}

static void
externally_modified_error_info_bar_response (GtkWidget *info_bar,
					     gint       response_id,
					     GTask     *saving_task)
{
	if (response_id == GTK_RESPONSE_YES)
	{
		GeditTab *tab = g_task_get_source_object (saving_task);
		SaverData *data = g_task_get_task_data (saving_task);
		GtkSourceFileSaverFlags save_flags;

		set_info_bar (tab, NULL);

		/* ignore_modification_time should not be persisted in save
		 * flags across saves (i.e. tab->save_flags is not modified).
		 */
		save_flags = gtk_source_file_saver_get_flags (data->saver);
		save_flags |= GTK_SOURCE_FILE_SAVER_FLAGS_IGNORE_MODIFICATION_TIME;
		response_set_save_flags (saving_task, save_flags);

		/* Force saving */
		launch_saver (saving_task);
	}
	else
	{
		unrecoverable_saving_error_info_bar_response (info_bar, response_id, saving_task);
	}
}

static void
recoverable_saving_error_info_bar_response (GtkWidget *info_bar,
					    gint       response_id,
					    GTask     *saving_task)
{
	if (response_id == GTK_RESPONSE_OK)
	{
		GeditTab *tab = g_task_get_source_object (saving_task);
		SaverData *data = g_task_get_task_data (saving_task);
		const GtkSourceEncoding *encoding;

		set_info_bar (tab, NULL);

		encoding = gedit_conversion_error_info_bar_get_encoding (GTK_WIDGET (info_bar));
		g_return_if_fail (encoding != NULL);

		gtk_source_file_saver_set_encoding (data->saver, encoding);
		launch_saver (saving_task);
	}
	else
	{
		unrecoverable_saving_error_info_bar_response (info_bar, response_id, saving_task);
	}
}

static void
externally_modified_notification_info_bar_response (GtkWidget *info_bar,
						    gint       response_id,
						    GeditTab  *tab)
{
	GeditView *view;

	set_info_bar (tab, NULL);

	view = gedit_tab_get_view (tab);

	if (response_id == GTK_RESPONSE_OK)
	{
		_gedit_tab_revert (tab);
	}
	else
	{
		tab->ask_if_externally_modified = FALSE;

		/* go back to normal state */
		gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
	}

	gtk_widget_grab_focus (GTK_WIDGET (view));
}

static void
display_externally_modified_notification (GeditTab *tab)
{
	TeplInfoBar *info_bar;
	GeditDocument *doc;
	GtkSourceFile *file;
	GFile *location;
	gboolean document_modified;

	doc = gedit_tab_get_document (tab);
	file = gedit_document_get_file (doc);

	/* we're here because the file we're editing changed on disk */
	location = gtk_source_file_get_location (file);
	g_return_if_fail (location != NULL);

	document_modified = gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc));
	info_bar = tepl_io_error_info_bar_externally_modified (location, document_modified);

	set_info_bar (tab, GTK_WIDGET (info_bar));

	g_signal_connect (info_bar,
			  "response",
			  G_CALLBACK (externally_modified_notification_info_bar_response),
			  tab);
}

static gboolean
view_focused_in (GtkWidget     *widget,
                 GdkEventFocus *event,
                 GeditTab      *tab)
{
	GeditDocument *doc;
	GtkSourceFile *file;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), GDK_EVENT_PROPAGATE);

	/* we try to detect file changes only in the normal state */
	if (tab->state != GEDIT_TAB_STATE_NORMAL)
	{
		return GDK_EVENT_PROPAGATE;
	}

	/* we already asked, don't bug the user again */
	if (!tab->ask_if_externally_modified)
	{
		return GDK_EVENT_PROPAGATE;
	}

	doc = gedit_tab_get_document (tab);
	file = gedit_document_get_file (doc);

	/* If file was never saved or is remote we do not check */
	if (gtk_source_file_is_local (file))
	{
		gtk_source_file_check_file_on_disk (file);

		if (gtk_source_file_is_externally_modified (file))
		{
			gedit_tab_set_state (tab, GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION);

			display_externally_modified_notification (tab);
		}
	}

	return GDK_EVENT_PROPAGATE;
}

static void
gedit_tab_init (GeditTab *tab)
{
	gboolean auto_save;
	gint auto_save_interval;
	GeditDocument *doc;
	GeditView *view;

	tab->state = GEDIT_TAB_STATE_NORMAL;

	tab->editor_settings = g_settings_new ("org.gnome.gedit.preferences.editor");

	tab->editable = TRUE;

	tab->ask_if_externally_modified = TRUE;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (tab),
	                                GTK_ORIENTATION_VERTICAL);

	/* Manage auto save data */
	auto_save = g_settings_get_boolean (tab->editor_settings,
					    GEDIT_SETTINGS_AUTO_SAVE);
	g_settings_get (tab->editor_settings, GEDIT_SETTINGS_AUTO_SAVE_INTERVAL,
			"u", &auto_save_interval);
	tab->auto_save = auto_save != FALSE;
	tab->auto_save_interval = auto_save_interval;

	/* Create the frame */
	tab->frame = gedit_view_frame_new ();
	gtk_widget_show (GTK_WIDGET (tab->frame));

	gtk_box_pack_end (GTK_BOX (tab), GTK_WIDGET (tab->frame), TRUE, TRUE, 0);

	doc = gedit_tab_get_document (tab);
	g_object_set_data (G_OBJECT (doc), GEDIT_TAB_KEY, tab);

	g_signal_connect_object (doc,
				 "modified-changed",
				 G_CALLBACK (document_modified_changed_cb),
				 tab,
				 G_CONNECT_DEFAULT);

	g_signal_connect_object (doc,
				 "notify::tepl-short-title",
				 G_CALLBACK (buffer_short_title_notify_cb),
				 tab,
				 G_CONNECT_DEFAULT);


	view = gedit_tab_get_view (tab);

	g_signal_connect_after (view,
				"focus-in-event",
				G_CALLBACK (view_focused_in),
				tab);

	g_signal_connect_after (view,
				"realize",
				G_CALLBACK (view_realized),
				tab);
}

GeditTab *
_gedit_tab_new (void)
{
	return g_object_new (GEDIT_TYPE_TAB, NULL);
}

/**
 * gedit_tab_get_view:
 * @tab: a #GeditTab.
 *
 * Returns: (transfer none): the #GeditView of @tab.
 */
GeditView *
gedit_tab_get_view (GeditTab *tab)
{
	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	return gedit_view_frame_get_view (tab->frame);
}

/**
 * gedit_tab_get_document:
 * @tab: a #GeditTab.
 *
 * Convenience function. It is equivalent to call gedit_tab_get_view() followed
 * by gtk_text_view_get_buffer().
 *
 * Returns: (transfer none): the #GeditDocument associated to @tab.
 */
GeditDocument *
gedit_tab_get_document (GeditTab *tab)
{
	GeditView *view;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	view = gedit_view_frame_get_view (tab->frame);

	return GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
}

gchar *
_gedit_tab_get_name (GeditTab *tab)
{
	GeditDocument *doc;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	doc = gedit_tab_get_document (tab);
	return tepl_buffer_get_short_title (TEPL_BUFFER (doc));
}

gchar *
_gedit_tab_get_tooltip (GeditTab *tab)
{
	GeditDocument *doc;
	gchar *full_name;
	gchar *full_name_markup;
	gchar *tip;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	doc = gedit_tab_get_document (tab);
	full_name = tepl_file_get_full_name (tepl_buffer_get_file (TEPL_BUFFER (doc)));
	full_name_markup = g_markup_printf_escaped ("<i>%s</i>", full_name);

	switch (tab->state)
	{
		gchar *content_type;
		gchar *mime_type;
		gchar *content_description;
		gchar *content_full_description;
		gchar *encoding;
		GtkSourceFile *file;
		const GtkSourceEncoding *enc;

		case GEDIT_TAB_STATE_LOADING_ERROR:
			tip = g_strdup_printf (_("Error opening file %s"), full_name_markup);
			break;

		case GEDIT_TAB_STATE_REVERTING_ERROR:
			tip = g_strdup_printf (_("Error reverting file %s"), full_name_markup);
			break;

		case GEDIT_TAB_STATE_SAVING_ERROR:
			tip =  g_strdup_printf (_("Error saving file %s"), full_name_markup);
			break;

		default:
			content_type = gedit_document_get_content_type (doc);
			mime_type = gedit_document_get_mime_type (doc);
			content_description = g_content_type_get_description (content_type);

			if (content_description == NULL)
				content_full_description = g_strdup (mime_type);
			else
				content_full_description = g_strdup_printf ("%s (%s)",
						content_description, mime_type);

			g_free (content_type);
			g_free (mime_type);
			g_free (content_description);

			file = gedit_document_get_file (doc);
			enc = gtk_source_file_get_encoding (file);

			if (enc == NULL)
			{
				enc = gtk_source_encoding_get_utf8 ();
			}

			encoding = gtk_source_encoding_to_string (enc);

			tip =  g_markup_printf_escaped ("<b>%s</b> %s\n\n"
						        "<b>%s</b> %s\n"
						        "<b>%s</b> %s",
						        _("Name:"), full_name,
						        _("MIME Type:"), content_full_description,
						        _("Encoding:"), encoding);

			g_free (encoding);
			g_free (content_full_description);
			break;
	}

	g_free (full_name);
	g_free (full_name_markup);
	return tip;
}

const gchar *
_gedit_tab_get_icon_name (GeditTab *tab)
{
	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	switch (tab->state)
	{
		case GEDIT_TAB_STATE_PRINTING:
			return "printer-printing-symbolic";

		case GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW:
			return "printer-symbolic";

		case GEDIT_TAB_STATE_LOADING_ERROR:
		case GEDIT_TAB_STATE_REVERTING_ERROR:
		case GEDIT_TAB_STATE_SAVING_ERROR:
		case GEDIT_TAB_STATE_GENERIC_ERROR:
			return "dialog-error-symbolic";

		case GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION:
			return "dialog-warning-symbolic";

		default:
			break;
	}

	return NULL;
}

/**
 * gedit_tab_get_from_document:
 * @doc: a #GeditDocument.
 *
 * Returns: (transfer none) (nullable): the #GeditTab associated with @doc.
 */
GeditTab *
gedit_tab_get_from_document (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	return g_object_get_data (G_OBJECT (doc), GEDIT_TAB_KEY);
}

static void
loader_progress_cb (goffset  size,
		    goffset  total_size,
		    GTask   *loading_task)
{
	LoaderData *data = g_task_get_task_data (loading_task);

	g_return_if_fail (data->tab->state == GEDIT_TAB_STATE_LOADING ||
			  data->tab->state == GEDIT_TAB_STATE_REVERTING);

	if (should_show_progress_info (&data->timer, size, total_size))
	{
		show_loading_info_bar (loading_task);
		info_bar_set_progress (data->tab, size, total_size);
	}
}

static void
goto_line (GTask *loading_task)
{
	LoaderData *data = g_task_get_task_data (loading_task);
	GeditDocument *doc = gedit_tab_get_document (data->tab);
	gboolean check_is_cursor_position = FALSE;
	GtkTextIter iter;

	/* To the top by default. */
	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (doc), &iter);

	/* At the requested line/column if set. */
	if (data->line_pos > 0)
	{
		gtk_text_buffer_get_iter_at_line_offset (GTK_TEXT_BUFFER (doc),
							 &iter,
							 data->line_pos - 1,
							 MAX (0, data->column_pos - 1));
		check_is_cursor_position = TRUE;
	}

	/* From metadata. */
	else if (g_settings_get_boolean (data->tab->editor_settings,
					 GEDIT_SETTINGS_RESTORE_CURSOR_POSITION))
	{
		gchar *position_str;
		guint64 offset = 0;

		position_str = gedit_document_get_metadata (doc, GEDIT_METADATA_ATTRIBUTE_POSITION);

		if (position_str != NULL &&
		    g_ascii_string_to_unsigned (position_str,
						10,
						0,
						G_MAXINT,
						&offset,
						NULL))
		{
			gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc),
							    &iter,
							    (gint) offset);
			check_is_cursor_position = TRUE;
		}

		g_free (position_str);
	}

	/* Make sure it's a valid position, to not end up in the middle of a
	 * utf8 character cluster.
	 */
	if (check_is_cursor_position &&
	    !gtk_text_iter_is_cursor_position (&iter))
	{
		gtk_text_iter_set_line_offset (&iter, 0);
	}

	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc), &iter);

	/* Scroll to the cursor when the document is loaded, we need to do it in
	 * an idle as after the document is loaded the textview is still
	 * redrawing and relocating its internals.
	 */
	if (data->tab->scroll_idle == 0 &&
	    !gtk_text_iter_is_start (&iter))
	{
		data->tab->scroll_idle = g_idle_add ((GSourceFunc)scroll_idle_cb, data->tab);
	}
}

static gboolean
file_already_opened (GeditDocument *doc,
		     GFile         *location)
{
	GList *all_documents;
	GList *l;
	gboolean already_opened = FALSE;

	if (location == NULL)
	{
		return FALSE;
	}

	all_documents = gedit_app_get_documents (GEDIT_APP (g_application_get_default ()));

	for (l = all_documents; l != NULL; l = l->next)
	{
		GeditDocument *cur_doc = l->data;
		GtkSourceFile *cur_file;
		GFile *cur_location;

		if (cur_doc == doc)
		{
			continue;
		}

		cur_file = gedit_document_get_file (cur_doc);
		cur_location = gtk_source_file_get_location (cur_file);

		if (cur_location != NULL &&
		    g_file_equal (location, cur_location))
		{
			already_opened = TRUE;
			break;
		}
	}

	g_list_free (all_documents);

	return already_opened;
}

static void
successful_load (GTask *loading_task)
{
	LoaderData *data = g_task_get_task_data (loading_task);
	GeditDocument *doc = gedit_tab_get_document (data->tab);
	GtkSourceFile *file = gedit_document_get_file (doc);
	GFile *location;

	if (data->user_requested_encoding)
	{
		const GtkSourceEncoding *encoding = gtk_source_file_loader_get_encoding (data->loader);
		const gchar *charset = gtk_source_encoding_get_charset (encoding);

		gedit_document_set_metadata (doc,
					     GEDIT_METADATA_ATTRIBUTE_ENCODING, charset,
					     NULL);
	}

	goto_line (loading_task);

	location = gtk_source_file_loader_get_location (data->loader);

	/* If the document is readonly we don't care how many times the file
	 * is opened.
	 */
	if (!gtk_source_file_is_readonly (file) &&
	    file_already_opened (doc, location))
	{
		TeplInfoBar *info_bar;

		set_editable (data->tab, FALSE);

		info_bar = tepl_io_error_info_bar_file_already_open (location);

		g_signal_connect (info_bar,
				  "response",
				  G_CALLBACK (file_already_open_warning_info_bar_response),
				  data->tab);

		set_info_bar (data->tab, GTK_WIDGET (info_bar));
	}

	/* When loading from stdin, the contents may not be saved, so set the
	 * buffer as modified.
	 */
	if (location == NULL)
	{
		gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (doc), TRUE);
	}

	data->tab->ask_if_externally_modified = TRUE;

	g_signal_emit_by_name (doc, "loaded");
}

static void
load_cb (GtkSourceFileLoader *loader,
	 GAsyncResult        *result,
	 GTask               *loading_task)
{
	LoaderData *data = g_task_get_task_data (loading_task);
	GeditDocument *doc;
	GFile *location = gtk_source_file_loader_get_location (loader);
	gboolean create_named_new_doc;
	GError *error = NULL;

	g_clear_pointer (&data->timer, g_timer_destroy);

	gtk_source_file_loader_load_finish (loader, result, &error);

	if (error != NULL)
	{
		gedit_debug_message (DEBUG_TAB, "File loading error: %s", error->message);

		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
		{
			g_task_return_boolean (loading_task, FALSE);
			g_object_unref (loading_task);

			g_error_free (error);
			return;
		}
	}

	doc = gedit_tab_get_document (data->tab);

	g_return_if_fail (data->tab->state == GEDIT_TAB_STATE_LOADING ||
			  data->tab->state == GEDIT_TAB_STATE_REVERTING);

	set_info_bar (data->tab, NULL);

	/* Special case creating a named new doc. */
	create_named_new_doc = (_gedit_document_get_create (doc) &&
				g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND) &&
				g_file_has_uri_scheme (location, "file"));

	if (create_named_new_doc)
	{
		g_error_free (error);
		error = NULL;
	}

	if (g_error_matches (error,
			     GTK_SOURCE_FILE_LOADER_ERROR,
			     GTK_SOURCE_FILE_LOADER_ERROR_CONVERSION_FALLBACK))
	{
		GtkWidget *info_bar;
		const GtkSourceEncoding *encoding;

		/* Set the tab as not editable as we have an error, the user can
		 * decide to make it editable again.
		 */
		set_editable (data->tab, FALSE);

		encoding = gtk_source_file_loader_get_encoding (loader);

		info_bar = gedit_io_loading_error_info_bar_new (location, encoding, error);

		g_signal_connect (info_bar,
				  "response",
				  G_CALLBACK (io_loading_error_info_bar_response),
				  loading_task);

		set_info_bar (data->tab, info_bar);

		if (data->tab->state == GEDIT_TAB_STATE_LOADING)
		{
			gtk_widget_show (GTK_WIDGET (data->tab->frame));
			gedit_tab_set_state (data->tab, GEDIT_TAB_STATE_LOADING_ERROR);
		}
		else
		{
			gedit_tab_set_state (data->tab, GEDIT_TAB_STATE_REVERTING_ERROR);
		}

		/* The loading was successful, despite some invalid characters. */
		successful_load (loading_task);
		gedit_recent_add_document (doc);

		g_error_free (error);
		return;
	}

	if (error != NULL)
	{
		GtkWidget *info_bar;

		if (data->tab->state == GEDIT_TAB_STATE_LOADING)
		{
			gtk_widget_hide (GTK_WIDGET (data->tab->frame));
			gedit_tab_set_state (data->tab, GEDIT_TAB_STATE_LOADING_ERROR);
		}
		else
		{
			gedit_tab_set_state (data->tab, GEDIT_TAB_STATE_REVERTING_ERROR);
		}

		if (location != NULL)
		{
			gedit_recent_remove_if_local (location);
		}

		if (data->tab->state == GEDIT_TAB_STATE_LOADING_ERROR)
		{
			const GtkSourceEncoding *encoding;

			encoding = gtk_source_file_loader_get_encoding (loader);

			info_bar = gedit_io_loading_error_info_bar_new (location, encoding, error);

			g_signal_connect (info_bar,
					  "response",
					  G_CALLBACK (io_loading_error_info_bar_response),
					  loading_task);
		}
		else
		{
			g_return_if_fail (data->tab->state == GEDIT_TAB_STATE_REVERTING_ERROR);

			info_bar = gedit_unrecoverable_reverting_error_info_bar_new (location, error);

			g_signal_connect (info_bar,
					  "response",
					  G_CALLBACK (unrecoverable_reverting_error_info_bar_response),
					  loading_task);
		}

		set_info_bar (data->tab, info_bar);

		g_error_free (error);
		return;
	}

	g_assert (error == NULL);

	gedit_tab_set_state (data->tab, GEDIT_TAB_STATE_NORMAL);
	successful_load (loading_task);

	if (!create_named_new_doc)
	{
		gedit_recent_add_document (doc);
	}

	g_task_return_boolean (loading_task, TRUE);
	g_object_unref (loading_task);
}

/* The returned list may contain duplicated encodings. Only the first occurrence
 * of a duplicated encoding should be kept, like it is done by
 * gtk_source_file_loader_set_candidate_encodings().
 */
static GSList *
get_candidate_encodings (GeditTab *tab)
{
	GSList *candidates = NULL;
	GeditDocument *doc;
	GtkSourceFile *file;
	gchar *metadata_charset;
	const GtkSourceEncoding *file_encoding;

	candidates = gedit_settings_get_candidate_encodings (NULL);

	/* Prepend the encoding stored in the metadata. */
	doc = gedit_tab_get_document (tab);
	metadata_charset = gedit_document_get_metadata (doc, GEDIT_METADATA_ATTRIBUTE_ENCODING);

	if (metadata_charset != NULL)
	{
		const GtkSourceEncoding *metadata_enc;

		metadata_enc = gtk_source_encoding_get_from_charset (metadata_charset);

		if (metadata_enc != NULL)
		{
			candidates = g_slist_prepend (candidates, (gpointer)metadata_enc);
		}
	}

	/* Finally prepend the GtkSourceFile's encoding, if previously set by a
	 * file loader or file saver.
	 */
	file = gedit_document_get_file (doc);
	file_encoding = gtk_source_file_get_encoding (file);

	if (file_encoding != NULL)
	{
		candidates = g_slist_prepend (candidates, (gpointer)file_encoding);
	}

	g_free (metadata_charset);
	return candidates;
}

static void
launch_loader (GTask                   *loading_task,
	       const GtkSourceEncoding *encoding)
{
	LoaderData *data = g_task_get_task_data (loading_task);
	GSList *candidate_encodings = NULL;
	GeditDocument *doc;

	if (encoding != NULL)
	{
		data->user_requested_encoding = TRUE;
		candidate_encodings = g_slist_append (NULL, (gpointer) encoding);
	}
	else
	{
		data->user_requested_encoding = FALSE;
		candidate_encodings = get_candidate_encodings (data->tab);
	}

	gtk_source_file_loader_set_candidate_encodings (data->loader, candidate_encodings);
	g_slist_free (candidate_encodings);

	doc = gedit_tab_get_document (data->tab);
	g_signal_emit_by_name (doc, "load");

	if (data->timer != NULL)
	{
		g_timer_destroy (data->timer);
	}

	data->timer = g_timer_new ();

	gtk_source_file_loader_load_async (data->loader,
					   G_PRIORITY_DEFAULT,
					   g_task_get_cancellable (loading_task),
					   (GFileProgressCallback) loader_progress_cb,
					   loading_task,
					   NULL,
					   (GAsyncReadyCallback) load_cb,
					   loading_task);
}

static void
load_async (GeditTab                *tab,
	    GFile                   *location,
	    const GtkSourceEncoding *encoding,
	    gint                     line_pos,
	    gint                     column_pos,
	    gboolean                 create,
	    GCancellable            *cancellable,
	    GAsyncReadyCallback      callback,
	    gpointer                 user_data)
{
	GeditDocument *doc;
	GtkSourceFile *file;
	GTask *loading_task;
	LoaderData *data;

	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (G_IS_FILE (location));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (tab->state == GEDIT_TAB_STATE_NORMAL);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_LOADING);

	doc = gedit_tab_get_document (tab);
	file = gedit_document_get_file (doc);
	gtk_source_file_set_location (file, location);

	loading_task = g_task_new (NULL, cancellable, callback, user_data);

	data = loader_data_new ();
	g_task_set_task_data (loading_task, data, (GDestroyNotify) loader_data_free);

	data->tab = tab;
	data->loader = gtk_source_file_loader_new (GTK_SOURCE_BUFFER (doc), file);
	data->line_pos = line_pos;
	data->column_pos = column_pos;

	_gedit_document_set_create (doc, create);

	launch_loader (loading_task, encoding);
}

static gboolean
load_finish (GeditTab     *tab,
	     GAsyncResult *result)
{
	g_return_val_if_fail (g_task_is_valid (result, tab), FALSE);

	return g_task_propagate_boolean (G_TASK (result), NULL);
}

static void
tab_load_cb (GObject      *source_object,
	     GAsyncResult *result,
	     gpointer      user_data)
{
	GeditTab *tab = GEDIT_TAB (source_object);
	load_finish (tab, result);
}

/**
 * gedit_tab_load_file:
 * @tab: a #GeditTab.
 * @location: the #GFile to load.
 * @encoding: (nullable): a #GtkSourceEncoding, or %NULL.
 * @line_pos: the line position to visualize.
 * @column_pos: the column position to visualize.
 * @create: %TRUE to show no errors if @location doesn't exist.
 *
 * This function tries to load @location into @tab. It is usually called only on
 * a newly-created tab.
 *
 * If @location doesn't exist, the behavior depends on @create:
 * - If @create is %FALSE, an error is shown.
 * - If @create is %TRUE, an empty #GeditDocument is created without error (but
 *   the file is not yet created on disk).
 *
 * The @tab needs to be in %GEDIT_TAB_STATE_NORMAL. The previous
 * #GtkTextBuffer's content is lost.
 *
 * Since: 45
 */
void
gedit_tab_load_file (GeditTab                *tab,
		     GFile                   *location,
		     const GtkSourceEncoding *encoding,
		     gint                     line_pos,
		     gint                     column_pos,
		     gboolean                 create)
{
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (G_IS_FILE (location));
	g_return_if_fail (tab->state == GEDIT_TAB_STATE_NORMAL);

	if (tab->cancellable != NULL)
	{
		g_cancellable_cancel (tab->cancellable);
		g_object_unref (tab->cancellable);
	}

	tab->cancellable = g_cancellable_new ();

	load_async (tab,
		    location,
		    encoding,
		    line_pos,
		    column_pos,
		    create,
		    tab->cancellable,
		    tab_load_cb,
		    NULL);
}

static void
load_stream_async (GeditTab                *tab,
		   GInputStream            *stream,
		   const GtkSourceEncoding *encoding,
		   gint                     line_pos,
		   gint                     column_pos,
		   GCancellable            *cancellable,
		   GAsyncReadyCallback      callback,
		   gpointer                 user_data)
{
	GeditDocument *doc;
	GtkSourceFile *file;
	GTask *loading_task;
	LoaderData *data;

	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (G_IS_INPUT_STREAM (stream));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (tab->state == GEDIT_TAB_STATE_NORMAL);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_LOADING);

	doc = gedit_tab_get_document (tab);
	file = gedit_document_get_file (doc);

	gtk_source_file_set_location (file, NULL);

	loading_task = g_task_new (NULL, cancellable, callback, user_data);

	data = loader_data_new ();
	g_task_set_task_data (loading_task, data, (GDestroyNotify) loader_data_free);

	data->tab = tab;
	data->loader = gtk_source_file_loader_new_from_stream (GTK_SOURCE_BUFFER (doc),
							       file,
							       stream);
	data->line_pos = line_pos;
	data->column_pos = column_pos;

	_gedit_document_set_create (doc, FALSE);

	launch_loader (loading_task, encoding);
}

/**
 * gedit_tab_load_stream:
 * @tab: a #GeditTab.
 * @stream: the #GInputStream to load, e.g. stdin.
 * @encoding: (nullable): a #GtkSourceEncoding, or %NULL.
 * @line_pos: the line position to visualize.
 * @column_pos: the column position to visualize.
 *
 * Loads @stream into @tab. This function is usually called only on a
 * newly-created tab.
 *
 * The @tab needs to be in %GEDIT_TAB_STATE_NORMAL. The previous
 * #GtkTextBuffer's content is lost.
 *
 * Since: 45
 */
void
gedit_tab_load_stream (GeditTab                *tab,
		       GInputStream            *stream,
		       const GtkSourceEncoding *encoding,
		       gint                     line_pos,
		       gint                     column_pos)
{
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (G_IS_INPUT_STREAM (stream));
	g_return_if_fail (tab->state == GEDIT_TAB_STATE_NORMAL);

	if (tab->cancellable != NULL)
	{
		g_cancellable_cancel (tab->cancellable);
		g_object_unref (tab->cancellable);
	}

	tab->cancellable = g_cancellable_new ();

	load_stream_async (tab,
			   stream,
			   encoding,
			   line_pos,
			   column_pos,
			   tab->cancellable,
			   tab_load_cb,
			   NULL);
}

static void
revert_async (GeditTab            *tab,
	      GCancellable        *cancellable,
	      GAsyncReadyCallback  callback,
	      gpointer             user_data)
{
	GeditDocument *doc;
	GtkSourceFile *file;
	GFile *location;
	GTask *loading_task;
	LoaderData *data;

	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (tab->state == GEDIT_TAB_STATE_NORMAL ||
	                  tab->state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION);

	if (tab->state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)
	{
		set_info_bar (tab, NULL);
	}

	doc = gedit_tab_get_document (tab);
	file = gedit_document_get_file (doc);
	location = gtk_source_file_get_location (file);
	g_return_if_fail (location != NULL);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_REVERTING);

	loading_task = g_task_new (NULL, cancellable, callback, user_data);

	data = loader_data_new ();
	g_task_set_task_data (loading_task, data, (GDestroyNotify) loader_data_free);

	data->tab = tab;
	data->loader = gtk_source_file_loader_new (GTK_SOURCE_BUFFER (doc), file);
	data->line_pos = 0;
	data->column_pos = 0;

	launch_loader (loading_task, NULL);
}

void
_gedit_tab_revert (GeditTab *tab)
{
	if (tab->cancellable != NULL)
	{
		g_cancellable_cancel (tab->cancellable);
		g_object_unref (tab->cancellable);
	}

	tab->cancellable = g_cancellable_new ();

	revert_async (tab,
		      tab->cancellable,
		      tab_load_cb,
		      NULL);
}

static void
close_printing (GeditTab *tab)
{
	if (tab->print_preview != NULL)
	{
		gtk_widget_destroy (tab->print_preview);
	}

	g_clear_object (&tab->print_job);
	g_clear_object (&tab->print_preview);

	/* destroy the info bar */
	set_info_bar (tab, NULL);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
}

static void
saver_progress_cb (goffset  size,
		   goffset  total_size,
		   GTask   *saving_task)
{
	GeditTab *tab = g_task_get_source_object (saving_task);
	SaverData *data = g_task_get_task_data (saving_task);

	g_return_if_fail (tab->state == GEDIT_TAB_STATE_SAVING);

	if (should_show_progress_info (&data->timer, size, total_size))
	{
		show_saving_info_bar (saving_task);
		info_bar_set_progress (tab, size, total_size);
	}
}

static void
save_cb (GtkSourceFileSaver *saver,
	 GAsyncResult       *result,
	 GTask              *saving_task)
{
	GeditTab *tab = g_task_get_source_object (saving_task);
	SaverData *data = g_task_get_task_data (saving_task);
	GeditDocument *doc = gedit_tab_get_document (tab);
	GFile *location = gtk_source_file_saver_get_location (saver);
	GError *error = NULL;

	g_return_if_fail (tab->state == GEDIT_TAB_STATE_SAVING);

	gtk_source_file_saver_save_finish (saver, result, &error);

	if (error != NULL)
	{
		gedit_debug_message (DEBUG_TAB, "File saving error: %s", error->message);
	}

	if (data->timer != NULL)
	{
		g_timer_destroy (data->timer);
		data->timer = NULL;
	}

	set_info_bar (tab, NULL);

	if (error != NULL)
	{
		GtkWidget *info_bar;

		gedit_tab_set_state (tab, GEDIT_TAB_STATE_SAVING_ERROR);

		if (error->domain == GTK_SOURCE_FILE_SAVER_ERROR &&
		    error->code == GTK_SOURCE_FILE_SAVER_ERROR_EXTERNALLY_MODIFIED)
		{
			/* This error is recoverable */
			info_bar = GTK_WIDGET (tepl_io_error_info_bar_saving_externally_modified (location));

			g_signal_connect (info_bar,
					  "response",
					  G_CALLBACK (externally_modified_error_info_bar_response),
					  saving_task);
		}
		else if (error->domain == G_IO_ERROR &&
			 error->code == G_IO_ERROR_CANT_CREATE_BACKUP)
		{
			/* This error is recoverable */
			info_bar = GTK_WIDGET (tepl_io_error_info_bar_cant_create_backup (location, error));
			g_return_if_fail (info_bar != NULL);

			g_signal_connect (info_bar,
					  "response",
					  G_CALLBACK (cant_create_backup_error_info_bar_response),
					  saving_task);
		}
		else if (error->domain == GTK_SOURCE_FILE_SAVER_ERROR &&
			 error->code == GTK_SOURCE_FILE_SAVER_ERROR_INVALID_CHARS)
		{
			/* If we have any invalid char in the document we must warn the user
			 * as it can make the document useless if it is saved.
			 */
			info_bar = GTK_WIDGET (tepl_io_error_info_bar_invalid_characters (location));
			g_return_if_fail (info_bar != NULL);

			g_signal_connect (info_bar,
			                  "response",
			                  G_CALLBACK (invalid_character_info_bar_response),
			                  saving_task);
		}
		else if (error->domain == GTK_SOURCE_FILE_SAVER_ERROR ||
			 (error->domain == G_IO_ERROR &&
			  error->code != G_IO_ERROR_INVALID_DATA &&
			  error->code != G_IO_ERROR_PARTIAL_INPUT))
		{
			/* These errors are _NOT_ recoverable */
			gedit_recent_remove_if_local (location);

			info_bar = gedit_unrecoverable_saving_error_info_bar_new (location, error);
			g_return_if_fail (info_bar != NULL);

			g_signal_connect (info_bar,
					  "response",
					  G_CALLBACK (unrecoverable_saving_error_info_bar_response),
					  saving_task);
		}
		else
		{
			const GtkSourceEncoding *encoding;

			/* This error is recoverable */
			g_return_if_fail (error->domain == G_CONVERT_ERROR ||
			                  error->domain == G_IO_ERROR);

			encoding = gtk_source_file_saver_get_encoding (saver);

			info_bar = gedit_conversion_error_while_saving_info_bar_new (location, encoding);
			g_return_if_fail (info_bar != NULL);

			g_signal_connect (info_bar,
					  "response",
					  G_CALLBACK (recoverable_saving_error_info_bar_response),
					  saving_task);
		}

		set_info_bar (tab, info_bar);
	}
	else
	{
		gedit_recent_add_document (doc);

		gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);

		tab->ask_if_externally_modified = TRUE;

		g_signal_emit_by_name (doc, "saved");
		g_task_return_boolean (saving_task, TRUE);
		g_object_unref (saving_task);
	}

	if (error != NULL)
	{
		g_error_free (error);
	}
}

static void
launch_saver (GTask *saving_task)
{
	GeditTab *tab = g_task_get_source_object (saving_task);
	GeditDocument *doc = gedit_tab_get_document (tab);
	SaverData *data = g_task_get_task_data (saving_task);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_SAVING);

	g_signal_emit_by_name (doc, "save");

	if (data->timer != NULL)
	{
		g_timer_destroy (data->timer);
	}

	data->timer = g_timer_new ();

	gtk_source_file_saver_save_async (data->saver,
					  G_PRIORITY_DEFAULT,
					  g_task_get_cancellable (saving_task),
					  (GFileProgressCallback) saver_progress_cb,
					  saving_task,
					  NULL,
					  (GAsyncReadyCallback) save_cb,
					  saving_task);
}

/* Gets the initial save flags, when launching a new FileSaver. */
static GtkSourceFileSaverFlags
get_initial_save_flags (GeditTab *tab,
			gboolean  auto_save)
{
	GtkSourceFileSaverFlags save_flags;
	gboolean create_backup;

	save_flags = tab->save_flags;

	create_backup = g_settings_get_boolean (tab->editor_settings,
						GEDIT_SETTINGS_CREATE_BACKUP_COPY);

	/* In case of autosaving, we need to preserve the backup that was produced
	 * the last time the user "manually" saved the file. So we don't set the
	 * CREATE_BACKUP flag for an automatic file saving.
	 */
	if (create_backup && !auto_save)
	{
		save_flags |= GTK_SOURCE_FILE_SAVER_FLAGS_CREATE_BACKUP;
	}

	return save_flags;
}

void
_gedit_tab_save_async (GeditTab            *tab,
		       GCancellable        *cancellable,
		       GAsyncReadyCallback  callback,
		       gpointer             user_data)
{
	GTask *saving_task;
	SaverData *data;
	GeditDocument *doc;
	GtkSourceFile *file;
	GtkSourceFileSaverFlags save_flags;

	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (tab->state == GEDIT_TAB_STATE_NORMAL ||
	                  tab->state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION ||
	                  tab->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW);

	/* The Save and Save As window actions are insensitive when the print
	 * preview is shown, but it's still possible to save several documents
	 * at once (with the Save All action or when quitting gedit). In that
	 * case, the print preview is simply closed. Handling correctly the
	 * document saving when the print preview is shown is more complicated
	 * and error-prone, it doesn't worth the effort. (the print preview
	 * would need to be updated when the filename changes, dealing with file
	 * saving errors is also more complicated, etc).
	 */
	if (tab->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW)
	{
		close_printing (tab);
	}

	doc = gedit_tab_get_document (tab);
	g_return_if_fail (!_gedit_document_is_untitled (doc));

	saving_task = g_task_new (tab, cancellable, callback, user_data);

	data = saver_data_new ();
	g_task_set_task_data (saving_task, data, (GDestroyNotify) saver_data_free);

	save_flags = get_initial_save_flags (tab, FALSE);

	if (tab->state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)
	{
		/* We already told the user about the external modification:
		 * hide the message bar and set the save flag.
		 */
		set_info_bar (tab, NULL);
		save_flags |= GTK_SOURCE_FILE_SAVER_FLAGS_IGNORE_MODIFICATION_TIME;
	}

	file = gedit_document_get_file (doc);

	data->saver = gtk_source_file_saver_new (GTK_SOURCE_BUFFER (doc), file);

	gtk_source_file_saver_set_flags (data->saver, save_flags);

	launch_saver (saving_task);
}

gboolean
_gedit_tab_save_finish (GeditTab     *tab,
			GAsyncResult *result)
{
	g_return_val_if_fail (g_task_is_valid (result, tab), FALSE);

	return g_task_propagate_boolean (G_TASK (result), NULL);
}

static void
auto_save_finished_cb (GeditTab     *tab,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	_gedit_tab_save_finish (tab, result);
}

static gboolean
gedit_tab_auto_save (GeditTab *tab)
{
	GTask *saving_task;
	SaverData *data;
	GeditDocument *doc;
	GtkSourceFile *file;
	GtkSourceFileSaverFlags save_flags;

	gedit_debug (DEBUG_TAB);

	doc = gedit_tab_get_document (tab);
	file = gedit_document_get_file (doc);

	g_return_val_if_fail (!_gedit_document_is_untitled (doc), G_SOURCE_REMOVE);
	g_return_val_if_fail (!gtk_source_file_is_readonly (file), G_SOURCE_REMOVE);

	if (!gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)))
	{
		gedit_debug_message (DEBUG_TAB, "Document not modified");

		return G_SOURCE_CONTINUE;
	}

	if (tab->state != GEDIT_TAB_STATE_NORMAL)
	{
		gedit_debug_message (DEBUG_TAB, "Retry after 30 seconds");

		tab->auto_save_timeout = g_timeout_add_seconds (30,
		                                                (GSourceFunc) gedit_tab_auto_save,
		                                                tab);

		/* Destroy the old timeout. */
		return G_SOURCE_REMOVE;
	}

	/* Set auto_save_timeout to 0 since the timeout is going to be destroyed */
	tab->auto_save_timeout = 0;

	saving_task = g_task_new (tab,
				  NULL,
				  (GAsyncReadyCallback) auto_save_finished_cb,
				  NULL);

	data = saver_data_new ();
	g_task_set_task_data (saving_task, data, (GDestroyNotify) saver_data_free);

	data->saver = gtk_source_file_saver_new (GTK_SOURCE_BUFFER (doc), file);

	save_flags = get_initial_save_flags (tab, TRUE);
	gtk_source_file_saver_set_flags (data->saver, save_flags);

	launch_saver (saving_task);

	return G_SOURCE_REMOVE;
}

/* Call _gedit_tab_save_finish() in @callback, there is no
 * _gedit_tab_save_as_finish().
 */
void
_gedit_tab_save_as_async (GeditTab                 *tab,
			  GFile                    *location,
			  const GtkSourceEncoding  *encoding,
			  GtkSourceNewlineType      newline_type,
			  GtkSourceCompressionType  compression_type,
			  GCancellable             *cancellable,
			  GAsyncReadyCallback       callback,
			  gpointer                  user_data)
{
	GTask *saving_task;
	SaverData *data;
	GeditDocument *doc;
	GtkSourceFile *file;
	GtkSourceFileSaverFlags save_flags;

	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (tab->state == GEDIT_TAB_STATE_NORMAL ||
	                  tab->state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION ||
	                  tab->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW);
	g_return_if_fail (G_IS_FILE (location));
	g_return_if_fail (encoding != NULL);

	/* See note at _gedit_tab_save_async(). */
	if (tab->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW)
	{
		close_printing (tab);
	}

	saving_task = g_task_new (tab, cancellable, callback, user_data);

	data = saver_data_new ();
	g_task_set_task_data (saving_task, data, (GDestroyNotify) saver_data_free);

	doc = gedit_tab_get_document (tab);

	/* reset the save flags, when saving as */
	tab->save_flags = GTK_SOURCE_FILE_SAVER_FLAGS_NONE;

	save_flags = get_initial_save_flags (tab, FALSE);

	if (tab->state == GEDIT_TAB_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)
	{
		/* We already told the user about the external modification:
		 * hide the message bar and set the save flag.
		 */
		set_info_bar (tab, NULL);
		save_flags |= GTK_SOURCE_FILE_SAVER_FLAGS_IGNORE_MODIFICATION_TIME;
	}

	file = gedit_document_get_file (doc);

	data->saver = gtk_source_file_saver_new_with_target (GTK_SOURCE_BUFFER (doc),
							     file,
							     location);

	gtk_source_file_saver_set_encoding (data->saver, encoding);
	gtk_source_file_saver_set_newline_type (data->saver, newline_type);
	gtk_source_file_saver_set_compression_type (data->saver, compression_type);
	gtk_source_file_saver_set_flags (data->saver, save_flags);

	launch_saver (saving_task);
}

#define GEDIT_PAGE_SETUP_KEY "gedit-page-setup-key"
#define GEDIT_PRINT_SETTINGS_KEY "gedit-print-settings-key"

static GtkPageSetup *
get_page_setup (GeditTab *tab)
{
	gpointer data;
	GeditDocument *doc;

	doc = gedit_tab_get_document (tab);

	data = g_object_get_data (G_OBJECT (doc),
				  GEDIT_PAGE_SETUP_KEY);

	if (data == NULL)
	{
		return _gedit_app_get_default_page_setup (GEDIT_APP (g_application_get_default ()));
	}
	else
	{
		return gtk_page_setup_copy (GTK_PAGE_SETUP (data));
	}
}

static GtkPrintSettings *
get_print_settings (GeditTab *tab)
{
	gpointer data;
	GeditDocument *doc;
	GtkPrintSettings *settings;
	gchar *name;

	doc = gedit_tab_get_document (tab);

	data = g_object_get_data (G_OBJECT (doc),
				  GEDIT_PRINT_SETTINGS_KEY);

	if (data == NULL)
	{
		settings = _gedit_app_get_default_print_settings (GEDIT_APP (g_application_get_default ()));
	}
	else
	{
		settings = gtk_print_settings_copy (GTK_PRINT_SETTINGS (data));
	}

	/* Be sure the OUTPUT_URI is unset, because otherwise the
	 * OUTPUT_BASENAME is not taken into account.
	 */
	gtk_print_settings_set (settings, GTK_PRINT_SETTINGS_OUTPUT_URI, NULL);

	name = tepl_file_get_short_name (tepl_buffer_get_file (TEPL_BUFFER (doc)));
	gtk_print_settings_set (settings, GTK_PRINT_SETTINGS_OUTPUT_BASENAME, name);
	g_free (name);

	return settings;
}

/* FIXME: show the info bar only if the operation will be "long" */
static void
printing_cb (GeditPrintJob       *job,
	     GeditPrintJobStatus  status,
	     GeditTab            *tab)
{
	g_return_if_fail (TEPL_IS_PROGRESS_INFO_BAR (tab->info_bar));

	gtk_widget_show (tab->info_bar);

	tepl_progress_info_bar_set_text (TEPL_PROGRESS_INFO_BAR (tab->info_bar),
					 gedit_print_job_get_status_string (job));

	tepl_progress_info_bar_set_fraction (TEPL_PROGRESS_INFO_BAR (tab->info_bar),
					     gedit_print_job_get_progress (job));
}

static void
store_print_settings (GeditTab      *tab,
		      GeditPrintJob *job)
{
	GeditDocument *doc;
	GtkPrintSettings *settings;
	GtkPageSetup *page_setup;

	doc = gedit_tab_get_document (tab);

	settings = gedit_print_job_get_print_settings (job);

	/* clear n-copies settings since we do not want to
	 * persist that one */
	gtk_print_settings_unset (settings,
				  GTK_PRINT_SETTINGS_N_COPIES);

	/* remember settings for this document */
	g_object_set_data_full (G_OBJECT (doc),
				GEDIT_PRINT_SETTINGS_KEY,
				g_object_ref (settings),
				(GDestroyNotify)g_object_unref);

	/* make them the default */
	_gedit_app_set_default_print_settings (GEDIT_APP (g_application_get_default ()),
					       settings);

	page_setup = gedit_print_job_get_page_setup (job);

	/* remember page setup for this document */
	g_object_set_data_full (G_OBJECT (doc),
				GEDIT_PAGE_SETUP_KEY,
				g_object_ref (page_setup),
				(GDestroyNotify)g_object_unref);

	/* make it the default */
	_gedit_app_set_default_page_setup (GEDIT_APP (g_application_get_default ()),
					   page_setup);
}

static void
done_printing_cb (GeditPrintJob       *job,
		  GeditPrintJobResult  result,
		  GError              *error,
		  GeditTab            *tab)
{
	GeditView *view;

	g_return_if_fail (tab->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW ||
	                  tab->state == GEDIT_TAB_STATE_PRINTING);

	if (result == GEDIT_PRINT_JOB_RESULT_OK)
	{
		store_print_settings (tab, job);
	}

	/* TODO Show the error in an info bar. */
	if (error != NULL)
	{
		g_warning ("Printing error: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	close_printing (tab);

	view = gedit_tab_get_view (tab);
	gtk_widget_grab_focus (GTK_WIDGET (view));
}

static void
show_preview_cb (GeditPrintJob     *job,
		 GeditPrintPreview *preview,
		 GeditTab          *tab)
{
	g_return_if_fail (tab->print_preview == NULL);

	/* destroy the info bar */
	set_info_bar (tab, NULL);

	tab->print_preview = GTK_WIDGET (preview);
	g_object_ref_sink (tab->print_preview);

	gtk_box_pack_end (GTK_BOX (tab), tab->print_preview, TRUE, TRUE, 0);

	gtk_widget_show (tab->print_preview);
	gtk_widget_grab_focus (tab->print_preview);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW);
}

static void
print_cancelled (GtkWidget *bar,
		 gint       response_id,
		 GeditTab  *tab)
{
	gedit_debug (DEBUG_TAB);

	if (tab->print_job != NULL)
	{
		gedit_print_job_cancel (tab->print_job);
	}
}

static void
add_printing_info_bar (GeditTab *tab)
{
	TeplProgressInfoBar *bar;

	bar = tepl_progress_info_bar_new ("document-print", NULL, TRUE);

	g_signal_connect (bar,
			  "response",
			  G_CALLBACK (print_cancelled),
			  tab);

	set_info_bar (tab, GTK_WIDGET (bar));

	/* hide until we start printing */
	gtk_widget_hide (GTK_WIDGET (bar));
}

void
_gedit_tab_print (GeditTab *tab)
{
	GeditView *view;
	GtkPageSetup *setup;
	GtkPrintSettings *settings;
	GtkPrintOperationResult res;
	GError *error = NULL;

	g_return_if_fail (GEDIT_IS_TAB (tab));

	/* FIXME: currently we can have just one printoperation going on at a
	 * given time, so before starting the print we close the preview.
	 * Would be nice to handle it properly though.
	 */
	if (tab->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW)
	{
		close_printing (tab);
	}

	g_return_if_fail (tab->print_job == NULL);
	g_return_if_fail (tab->state == GEDIT_TAB_STATE_NORMAL);

	view = gedit_tab_get_view (tab);

	tab->print_job = gedit_print_job_new (TEPL_VIEW (view));

	add_printing_info_bar (tab);

	g_signal_connect_object (tab->print_job,
				 "printing",
				 G_CALLBACK (printing_cb),
				 tab,
				 0);

	g_signal_connect_object (tab->print_job,
				 "show-preview",
				 G_CALLBACK (show_preview_cb),
				 tab,
				 0);

	g_signal_connect_object (tab->print_job,
				 "done",
				 G_CALLBACK (done_printing_cb),
				 tab,
				 0);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_PRINTING);

	setup = get_page_setup (tab);
	settings = get_print_settings (tab);

	res = gedit_print_job_print (tab->print_job,
				     GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
				     setup,
				     settings,
				     GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (tab))),
				     &error);

	/* TODO: manage res in the correct way */
	if (res == GTK_PRINT_OPERATION_RESULT_ERROR)
	{
		/* FIXME: go in error state */
		g_warning ("Async print preview failed (%s)", error->message);
		g_error_free (error);

		close_printing (tab);
	}

	g_object_unref (setup);
	g_object_unref (settings);
}

void
_gedit_tab_mark_for_closing (GeditTab *tab)
{
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (tab->state == GEDIT_TAB_STATE_NORMAL);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_CLOSING);
}

gboolean
_gedit_tab_get_can_close (GeditTab *tab)
{
	GeditDocument *doc;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), FALSE);

	/* if we are loading or reverting, the tab can be closed */
	if (tab->state == GEDIT_TAB_STATE_LOADING ||
	    tab->state == GEDIT_TAB_STATE_LOADING_ERROR ||
	    tab->state == GEDIT_TAB_STATE_REVERTING ||
	    tab->state == GEDIT_TAB_STATE_REVERTING_ERROR) /* CHECK: I'm not sure this is the right behavior for REVERTING ERROR */
	{
		return TRUE;
	}

	/* Do not close tab with saving errors */
	if (tab->state == GEDIT_TAB_STATE_SAVING_ERROR)
	{
		return FALSE;
	}

	doc = gedit_tab_get_document (tab);

	if (_gedit_document_needs_saving (doc))
	{
		return FALSE;
	}

	return TRUE;
}

/**
 * gedit_tab_get_auto_save_enabled:
 * @tab: a #GeditTab.
 *
 * Returns: the value of the #GeditTab:autosave property.
 */
gboolean
gedit_tab_get_auto_save_enabled	(GeditTab *tab)
{
	gedit_debug (DEBUG_TAB);

	g_return_val_if_fail (GEDIT_IS_TAB (tab), FALSE);

	return tab->auto_save;
}

/**
 * gedit_tab_set_auto_save_enabled:
 * @tab: a #GeditTab.
 * @enable: the new value.
 *
 * Sets the #GeditTab:autosave property.
 *
 * It does not install an autosave timeout if the document is new or is
 * read-only.
 */
void
gedit_tab_set_auto_save_enabled	(GeditTab *tab,
				 gboolean  enable)
{
	gedit_debug (DEBUG_TAB);

	g_return_if_fail (GEDIT_IS_TAB (tab));

	enable = enable != FALSE;

	if (tab->auto_save != enable)
	{
		tab->auto_save = enable;
		update_auto_save_timeout (tab);
		return;
	}
}

/**
 * gedit_tab_get_auto_save_interval:
 * @tab: a #GeditTab.
 *
 * Returns: the value of the #GeditTab:autosave-interval property.
 */
gint
gedit_tab_get_auto_save_interval (GeditTab *tab)
{
	gedit_debug (DEBUG_TAB);

	g_return_val_if_fail (GEDIT_IS_TAB (tab), 0);

	return tab->auto_save_interval;
}

/**
 * gedit_tab_set_auto_save_interval:
 * @tab: a #GeditTab.
 * @interval: the new value.
 *
 * Sets the #GeditTab:autosave-interval property.
 */
void
gedit_tab_set_auto_save_interval (GeditTab *tab,
				  gint      interval)
{
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (interval > 0);

	gedit_debug (DEBUG_TAB);

	if (tab->auto_save_interval != interval)
	{
		tab->auto_save_interval = interval;
		remove_auto_save_timeout (tab);
		update_auto_save_timeout (tab);
	}
}

/**
 * gedit_tab_set_info_bar:
 * @tab: a #GeditTab.
 * @info_bar: a #GtkInfoBar.
 *
 * Sets the #GtkInfoBar of @tab. Note that there can be only one #GtkInfoBar per
 * #GeditTab. If there was already an infobar set, it is destroyed and replaced
 * by the new one.
 *
 * See also #TeplInfoBar, it permits to create a #GtkInfoBar more easily.
 */
void
gedit_tab_set_info_bar (GeditTab  *tab,
                        GtkWidget *info_bar)
{
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (info_bar == NULL || GTK_IS_WIDGET (info_bar));

	/* FIXME: this can cause problems with the tab state machine */
	set_info_bar (tab, info_bar);
}

GeditViewFrame *
_gedit_tab_get_view_frame (GeditTab *tab)
{
	return tab->frame;
}

/* ex:set ts=8 noet: */
