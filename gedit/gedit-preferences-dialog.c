/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-preferences-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2001-2005 Paolo Maggi
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

#include "gedit-preferences-dialog.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtksourceview/gtksource.h>
#include <libpeas-gtk/peas-gtk.h>

#include "gedit-utils.h"
#include "gedit-debug.h"
#include "gedit-document.h"
#include "gedit-dirs.h"
#include "gedit-settings.h"
#include "gedit-utils.h"
#include "gedit-file-chooser-dialog.h"

/*
 * gedit-preferences dialog is a singleton since we don't
 * want two dialogs showing an inconsistent state of the
 * preferences.
 * When gedit_show_preferences_dialog is called and there
 * is already a prefs dialog dialog open, it is reparented
 * and shown.
 */

static GtkWidget *preferences_dialog = NULL;

#define GEDIT_SCHEME_ROW_ID_KEY "gedit-scheme-row-id"

#define GEDIT_TYPE_PREFERENCES_DIALOG (gedit_preferences_dialog_get_type())

G_DECLARE_FINAL_TYPE (GeditPreferencesDialog, gedit_preferences_dialog, GEDIT, PREFERENCES_DIALOG, GtkWindow)

enum
{
	ID_COLUMN = 0,
	NAME_COLUMN,
	DESC_COLUMN,
	NUM_COLUMNS
};

enum
{
	CLOSE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

struct _GeditPreferencesDialog
{
	GtkWindow	parent_instance;

	GSettings	*editor;
	GSettings	*uisettings; /* unfortunately our settings are split for historical reasons */

	GtkWidget	*notebook;

	/* Font */
	GtkWidget	*default_font_checkbutton;
	GtkWidget	*font_button;
	GtkWidget	*font_grid;

	/* Style Scheme */
	GtkWidget	*schemes_list;
	GtkWidget	*install_scheme_button;
	GtkWidget	*uninstall_scheme_button;
	GtkWidget	*schemes_scrolled_window;
	GtkWidget	*schemes_toolbar;

	GeditFileChooserDialog *
			 install_scheme_file_schooser;

	/* Tabs */
	GtkWidget	*tabs_width_spinbutton;
	GtkWidget	*insert_spaces_checkbutton;

	/* Auto indentation */
	GtkWidget	*auto_indent_checkbutton;

	/* Text Wrapping */
	GtkWidget	*wrap_text_checkbutton;
	GtkWidget	*split_checkbutton;

	/* File Saving */
	GtkWidget	*backup_copy_checkbutton;
	GtkWidget	*auto_save_checkbutton;
	GtkWidget	*auto_save_spinbutton;

	GtkWidget	*display_line_numbers_checkbutton;
	GtkWidget	*display_statusbar_checkbutton;
	GtkWidget	*display_overview_map_checkbutton;
	GtkWidget	*display_grid_checkbutton;

	/* Right margin */
	GtkWidget	*right_margin_checkbutton;
	GtkWidget       *right_margin_position_grid;
	GtkWidget	*right_margin_position_spinbutton;

	/* Highlighting */
	GtkWidget	*highlight_current_line_checkbutton;
	GtkWidget	*bracket_matching_checkbutton;

	/* Plugin manager */
	GtkWidget	*plugin_manager;
};

G_DEFINE_TYPE (GeditPreferencesDialog, gedit_preferences_dialog, GTK_TYPE_WINDOW)

static void
gedit_preferences_dialog_dispose (GObject *object)
{
	GeditPreferencesDialog *dlg = GEDIT_PREFERENCES_DIALOG (object);

	g_clear_object (&dlg->editor);
	g_clear_object (&dlg->uisettings);

	G_OBJECT_CLASS (gedit_preferences_dialog_parent_class)->dispose (object);
}

static void
gedit_preferences_dialog_close (GeditPreferencesDialog *dialog)
{
	gtk_window_close (GTK_WINDOW (dialog));
}

static void
gedit_preferences_dialog_class_init (GeditPreferencesDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkBindingSet *binding_set;

	/* Otherwise libpeas-gtk might not be linked */
	g_type_ensure (PEAS_GTK_TYPE_PLUGIN_MANAGER);

	object_class->dispose = gedit_preferences_dialog_dispose;

	signals[CLOSE] =
		g_signal_new_class_handler ("close",
		                            G_TYPE_FROM_CLASS (klass),
		                            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		                            G_CALLBACK (gedit_preferences_dialog_close),
		                            NULL, NULL, NULL,
		                            G_TYPE_NONE,
		                            0);

	binding_set = gtk_binding_set_by_class (klass);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Escape, 0, "close", 0);

	/* Bind class to template */
	gtk_widget_class_set_template_from_resource (widget_class,
	                                             "/org/gnome/gedit/ui/gedit-preferences-dialog.ui");
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, notebook);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, display_line_numbers_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, display_statusbar_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, display_grid_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, display_overview_map_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, right_margin_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, right_margin_position_grid);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, right_margin_position_spinbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, highlight_current_line_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, bracket_matching_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, wrap_text_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, split_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, tabs_width_spinbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, insert_spaces_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, auto_indent_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, backup_copy_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, auto_save_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, auto_save_spinbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, default_font_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, font_button);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, font_grid);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, schemes_list);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, schemes_scrolled_window);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, install_scheme_button);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, uninstall_scheme_button);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, schemes_toolbar);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, plugin_manager);
}

static void
setup_editor_page (GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS);

	/* Connect signal */
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_TABS_SIZE,
			 dlg->tabs_width_spinbutton,
			 "value",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_INSERT_SPACES,
			 dlg->insert_spaces_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_AUTO_INDENT,
			 dlg->auto_indent_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_CREATE_BACKUP_COPY,
			 dlg->backup_copy_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_BRACKET_MATCHING,
			 dlg->bracket_matching_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_AUTO_SAVE_INTERVAL,
			 dlg->auto_save_spinbutton,
			 "value",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_AUTO_SAVE,
			 dlg->auto_save_spinbutton,
			 "sensitive",
			 G_SETTINGS_BIND_GET);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_AUTO_SAVE,
			 dlg->auto_save_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
}

static void
wrap_mode_checkbutton_toggled (GtkToggleButton        *button,
			       GeditPreferencesDialog *dlg)
{
	GtkWrapMode mode;

	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->wrap_text_checkbutton)))
	{
		mode = GTK_WRAP_NONE;

		gtk_widget_set_sensitive (dlg->split_checkbutton,
					  FALSE);
		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dlg->split_checkbutton), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (dlg->split_checkbutton,
					  TRUE);

		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dlg->split_checkbutton), FALSE);


		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->split_checkbutton)))
		{
			g_settings_set_enum (dlg->editor,
			                     GEDIT_SETTINGS_WRAP_LAST_SPLIT_MODE,
			                     GTK_WRAP_WORD);

			mode = GTK_WRAP_WORD;
		}
		else
		{
			g_settings_set_enum (dlg->editor,
			                     GEDIT_SETTINGS_WRAP_LAST_SPLIT_MODE,
			                     GTK_WRAP_CHAR);

			mode = GTK_WRAP_CHAR;
		}
	}

	g_settings_set_enum (dlg->editor,
			     GEDIT_SETTINGS_WRAP_MODE,
			     mode);
}

static void
grid_checkbutton_toggled (GtkToggleButton        *button,
                          GeditPreferencesDialog *dlg)
{
	GtkSourceBackgroundPatternType background_type;

	background_type = gtk_toggle_button_get_active (button) ?
	                  GTK_SOURCE_BACKGROUND_PATTERN_TYPE_GRID :
		          GTK_SOURCE_BACKGROUND_PATTERN_TYPE_NONE;
	g_settings_set_enum (dlg->editor,
	                     GEDIT_SETTINGS_BACKGROUND_PATTERN,
	                     background_type);
}

static void
setup_view_page (GeditPreferencesDialog *dlg)
{
	GtkWrapMode wrap_mode;
	GtkWrapMode last_split_mode;
	GtkSourceBackgroundPatternType background_pattern;
	gboolean display_right_margin;
	guint right_margin_position;

	gedit_debug (DEBUG_PREFS);

	/* Get values */
	display_right_margin = g_settings_get_boolean (dlg->editor,
						       GEDIT_SETTINGS_DISPLAY_RIGHT_MARGIN);
	g_settings_get (dlg->editor, GEDIT_SETTINGS_RIGHT_MARGIN_POSITION,
			"u", &right_margin_position);
	background_pattern = g_settings_get_enum (dlg->editor,
	                                          GEDIT_SETTINGS_BACKGROUND_PATTERN);

	wrap_mode = g_settings_get_enum (dlg->editor,
					 GEDIT_SETTINGS_WRAP_MODE);

	/* Set initial state */
	switch (wrap_mode)
	{
		case GTK_WRAP_WORD:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->wrap_text_checkbutton), TRUE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->split_checkbutton), TRUE);

			g_settings_set_enum (dlg->editor,
			                     GEDIT_SETTINGS_WRAP_LAST_SPLIT_MODE,
			                     GTK_WRAP_WORD);
			break;
		case GTK_WRAP_CHAR:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->wrap_text_checkbutton), TRUE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->split_checkbutton), FALSE);

			g_settings_set_enum (dlg->editor,
			                     GEDIT_SETTINGS_WRAP_LAST_SPLIT_MODE,
			                     GTK_WRAP_CHAR);
			break;
		default:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->wrap_text_checkbutton), FALSE);

			last_split_mode = g_settings_get_enum (dlg->editor,
			                                       GEDIT_SETTINGS_WRAP_LAST_SPLIT_MODE);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->split_checkbutton),
			                              last_split_mode == GTK_WRAP_WORD);

			gtk_toggle_button_set_inconsistent (
				GTK_TOGGLE_BUTTON (dlg->split_checkbutton), TRUE);
	}

	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (dlg->right_margin_checkbutton),
		display_right_margin);
	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (dlg->display_grid_checkbutton),
		background_pattern == GTK_SOURCE_BACKGROUND_PATTERN_TYPE_GRID);

	/* Set widgets sensitivity */
	gtk_widget_set_sensitive (dlg->split_checkbutton,
				  (wrap_mode != GTK_WRAP_NONE));

	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_DISPLAY_LINE_NUMBERS,
			 dlg->display_line_numbers_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_HIGHLIGHT_CURRENT_LINE,
			 dlg->highlight_current_line_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->uisettings,
	                 GEDIT_SETTINGS_STATUSBAR_VISIBLE,
	                 dlg->display_statusbar_checkbutton,
	                 "active",
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_DISPLAY_OVERVIEW_MAP,
			 dlg->display_overview_map_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
	                 GEDIT_SETTINGS_DISPLAY_RIGHT_MARGIN,
	                 dlg->right_margin_checkbutton,
	                 "active",
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
	                 GEDIT_SETTINGS_DISPLAY_RIGHT_MARGIN,
	                 dlg->right_margin_position_grid,
	                 "sensitive",
	                 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_RIGHT_MARGIN_POSITION,
			 dlg->right_margin_position_spinbutton,
			 "value",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_AUTO_SAVE_INTERVAL,
			 dlg->auto_save_spinbutton,
			 "value",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_signal_connect (dlg->wrap_text_checkbutton,
			  "toggled",
			  G_CALLBACK (wrap_mode_checkbutton_toggled),
			  dlg);
	g_signal_connect (dlg->split_checkbutton,
			  "toggled",
			  G_CALLBACK (wrap_mode_checkbutton_toggled),
			  dlg);
	g_signal_connect (dlg->display_grid_checkbutton,
			  "toggled",
			  G_CALLBACK (grid_checkbutton_toggled),
			  dlg);
}

static void
setup_font_colors_page_font_section (GeditPreferencesDialog *dlg)
{
	GeditSettings *settings;
	gchar *system_font = NULL;
	gchar *label;

	gedit_debug (DEBUG_PREFS);

	gtk_widget_set_tooltip_text (dlg->font_button,
			 _("Click on this button to select the font to be used by the editor"));

	/* Get values */
	settings = _gedit_settings_get_singleton ();
	system_font = gedit_settings_get_system_font (settings);

	label = g_strdup_printf(_("_Use the system fixed width font (%s)"),
				system_font);
	gtk_button_set_label (GTK_BUTTON (dlg->default_font_checkbutton),
			      label);
	g_free (system_font);
	g_free (label);

	/* Bind settings */
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_USE_DEFAULT_FONT,
			 dlg->default_font_checkbutton,
			 "active",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_USE_DEFAULT_FONT,
			 dlg->font_grid,
			 "sensitive",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET | G_SETTINGS_BIND_INVERT_BOOLEAN);
	g_settings_bind (dlg->editor,
			 GEDIT_SETTINGS_EDITOR_FONT,
			 dlg->font_button,
			 "font-name",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
}

static void
set_buttons_sensisitivity_according_to_scheme (GeditPreferencesDialog *dlg,
                                               GtkSourceStyleScheme   *scheme)
{
	gboolean editable = FALSE;

	if (scheme != NULL)
	{
		const gchar *filename;

		filename = gtk_source_style_scheme_get_filename (scheme);
		if (filename != NULL)
		{
			editable = g_str_has_prefix (filename, gedit_dirs_get_user_styles_dir ());
		}
	}

	gtk_widget_set_sensitive (dlg->uninstall_scheme_button,
	                          editable);
}

static void
style_scheme_changed (GtkSourceStyleSchemeChooser *chooser,
                      GParamSpec                  *pspec,
                      GeditPreferencesDialog      *dlg)
{
	GtkSourceStyleScheme *scheme;
	const gchar *id;

	scheme = gtk_source_style_scheme_chooser_get_style_scheme (chooser);
	id = gtk_source_style_scheme_get_id (scheme);

	g_settings_set_string (dlg->editor, GEDIT_SETTINGS_SCHEME, id);
	set_buttons_sensisitivity_according_to_scheme (dlg, scheme);
}

static GtkSourceStyleScheme *
get_default_color_scheme (GeditPreferencesDialog *dlg)
{
	GtkSourceStyleSchemeManager *manager;
	GtkSourceStyleScheme *scheme = NULL;
	gchar *pref_id;

	manager = gtk_source_style_scheme_manager_get_default ();

	pref_id = g_settings_get_string (dlg->editor,
	                                 GEDIT_SETTINGS_SCHEME);

	scheme = gtk_source_style_scheme_manager_get_scheme (manager,
	                                                     pref_id);
	g_free (pref_id);

	if (scheme == NULL)
	{
		/* Fall-back to classic style scheme */
		scheme = gtk_source_style_scheme_manager_get_scheme (manager,
		                                                     "classic");
	}

	return scheme;
}

/*
 * file_copy:
 * @name: a pointer to a %NULL-terminated string, that names
 * the file to be copied, in the GLib file name encoding
 * @dest_name: a pointer to a %NULL-terminated string, that is the
 * name for the destination file, in the GLib file name encoding
 * @error: return location for a #GError, or %NULL
 *
 * Copies file @name to @dest_name.
 *
 * If the call was successful, it returns %TRUE. If the call was not
 * successful, it returns %FALSE and sets @error. The error domain
 * is #G_FILE_ERROR. Possible error
 * codes are those in the #GFileError enumeration.
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 */
static gboolean
file_copy (const gchar  *name,
	   const gchar  *dest_name,
	   GError      **error)
{
	gchar *contents;
	gsize length;
	gchar *dest_dir;

	/* FIXME - Paolo (Aug. 13, 2007):
	 * Since the style scheme files are relatively small, we can implement
	 * file copy getting all the content of the source file in a buffer and
	 * then write the content to the destination file. In this way we
	 * can use the g_file_get_contents and g_file_set_contents and avoid to
	 * write custom code to copy the file (with sane error management).
	 * If needed we can improve this code later. */

	g_return_val_if_fail (name != NULL, FALSE);
	g_return_val_if_fail (dest_name != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* Note: we allow to copy a file to itself since this is not a problem
	 * in our use case */

	/* Ensure the destination directory exists */
	dest_dir = g_path_get_dirname (dest_name);

	errno = 0;
	if (g_mkdir_with_parents (dest_dir, 0755) != 0)
	{
		gint save_errno = errno;
		gchar *display_filename = g_filename_display_name (dest_dir);

		g_set_error (error,
			     G_FILE_ERROR,
			     g_file_error_from_errno (save_errno),
			     _("Directory “%s” could not be created: g_mkdir_with_parents() failed: %s"),
			     display_filename,
			     g_strerror (save_errno));

		g_free (dest_dir);
		g_free (display_filename);

		return FALSE;
	}

	g_free (dest_dir);

	if (!g_file_get_contents (name, &contents, &length, error))
		return FALSE;

	if (!g_file_set_contents (dest_name, contents, length, error))
	{
		g_free (contents);
		return FALSE;
	}

	g_free (contents);

	return TRUE;
}

/*
 * install_style_scheme:
 * @manager: a #GtkSourceStyleSchemeManager
 * @fname: the file name of the style scheme to be installed
 *
 * Install a new user scheme.
 * This function copies @fname in #GEDIT_STYLES_DIR and ask the style manager to
 * recompute the list of available style schemes. It then checks if a style
 * scheme with the right file name exists.
 *
 * If the call was succesful, it returns the id of the installed scheme
 * otherwise %NULL.
 *
 * Return value: the id of the installed scheme, %NULL otherwise.
 */
static GtkSourceStyleScheme *
install_style_scheme (const gchar *fname)
{
	GtkSourceStyleSchemeManager *manager;
	gchar *new_file_name = NULL;
	gchar *dirname;
	const gchar *styles_dir;
	GError *error = NULL;
	gboolean copied = FALSE;
	const gchar * const *ids;

	g_return_val_if_fail (fname != NULL, NULL);

	manager = gtk_source_style_scheme_manager_get_default ();

	dirname = g_path_get_dirname (fname);
	styles_dir = gedit_dirs_get_user_styles_dir ();

	if (strcmp (dirname, styles_dir) != 0)
	{
		gchar *basename;

		basename = g_path_get_basename (fname);
		new_file_name = g_build_filename (styles_dir, basename, NULL);
		g_free (basename);

		/* Copy the style scheme file into GEDIT_STYLES_DIR */
		if (!file_copy (fname, new_file_name, &error))
		{
			g_free (new_file_name);
			g_free (dirname);

			g_message ("Cannot install style scheme:\n%s",
				   error->message);

			g_error_free (error);

			return NULL;
		}

		copied = TRUE;
	}
	else
	{
		new_file_name = g_strdup (fname);
	}

	g_free (dirname);

	/* Reload the available style schemes */
	gtk_source_style_scheme_manager_force_rescan (manager);

	/* Check the new style scheme has been actually installed */
	ids = gtk_source_style_scheme_manager_get_scheme_ids (manager);

	while (*ids != NULL)
	{
		GtkSourceStyleScheme *scheme;
		const gchar *filename;

		scheme = gtk_source_style_scheme_manager_get_scheme (manager, *ids);

		filename = gtk_source_style_scheme_get_filename (scheme);

		if (filename && (strcmp (filename, new_file_name) == 0))
		{
			/* The style scheme has been correctly installed */
			g_free (new_file_name);

			return scheme;
		}
		++ids;
	}

	/* The style scheme has not been correctly installed */
	if (copied)
		g_unlink (new_file_name);

	g_free (new_file_name);

	return NULL;
}

/**
 * uninstall_style_scheme:
 * @manager: a #GtkSourceStyleSchemeManager
 * @scheme: a #GtkSourceStyleScheme
 *
 * Uninstall a user scheme.
 *
 * If the call was succesful, it returns %TRUE
 * otherwise %FALSE.
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 */
static gboolean
uninstall_style_scheme (GtkSourceStyleScheme *scheme)
{
	GtkSourceStyleSchemeManager *manager;
	const gchar *filename;

	g_return_val_if_fail (GTK_SOURCE_IS_STYLE_SCHEME (scheme), FALSE);

	manager = gtk_source_style_scheme_manager_get_default ();

	filename = gtk_source_style_scheme_get_filename (scheme);
	if (filename == NULL)
		return FALSE;

	if (g_unlink (filename) == -1)
		return FALSE;

	/* Reload the available style schemes */
	gtk_source_style_scheme_manager_force_rescan (manager);

	return TRUE;
}

static void
add_scheme_chooser_response_cb (GeditFileChooserDialog *chooser,
				gint                    res_id,
				GeditPreferencesDialog *dlg)
{
	GFile *file;
	gchar *filename;
	GtkSourceStyleScheme *scheme;

	if (res_id != GTK_RESPONSE_ACCEPT)
	{
		gedit_file_chooser_dialog_hide (chooser);
		return;
	}

	file = gedit_file_chooser_dialog_get_file (chooser);

	if (file == NULL)
	{
		return;
	}

	filename = g_file_get_path (file);
	g_object_unref (file);

	if (filename == NULL)
	{
		return;
	}

	gedit_file_chooser_dialog_hide (chooser);

	scheme = install_style_scheme (filename);
	g_free (filename);

	if (scheme == NULL)
	{
		gedit_warning (GTK_WINDOW (dlg),
		               _("The selected color scheme cannot be installed."));

		return;
	}

	g_settings_set_string (dlg->editor, GEDIT_SETTINGS_SCHEME,
	                       gtk_source_style_scheme_get_id (scheme));

	set_buttons_sensisitivity_according_to_scheme (dlg, scheme);
}

static void
install_scheme_clicked (GtkButton              *button,
			GeditPreferencesDialog *dlg)
{
	GeditFileChooserDialog *chooser;

	if (dlg->install_scheme_file_schooser != NULL)
	{
		gedit_file_chooser_dialog_show (dlg->install_scheme_file_schooser);
		return;
	}

	chooser = gedit_file_chooser_dialog_create (_("Add Scheme"),
						    GTK_WINDOW (dlg),
						    GEDIT_FILE_CHOOSER_FLAG_OPEN,
						    _("_Cancel"),
						    _("A_dd Scheme"));

	/* Filters */
	gedit_file_chooser_dialog_add_pattern_filter (chooser,
	                                              _("Color Scheme Files"),
	                                              "*.xml");

	gedit_file_chooser_dialog_add_pattern_filter (chooser,
	                                              _("All Files"),
	                                              "*");

	g_signal_connect (chooser,
			  "response",
			  G_CALLBACK (add_scheme_chooser_response_cb),
			  dlg);

	dlg->install_scheme_file_schooser = chooser;

	g_object_add_weak_pointer (G_OBJECT (chooser),
				   (gpointer) &dlg->install_scheme_file_schooser);

	gedit_file_chooser_dialog_show (chooser);
}

static void
uninstall_scheme_clicked (GtkButton              *button,
			  GeditPreferencesDialog *dlg)
{
	GtkSourceStyleScheme *scheme;

	scheme = gtk_source_style_scheme_chooser_get_style_scheme (GTK_SOURCE_STYLE_SCHEME_CHOOSER (dlg->schemes_list));

	if (!uninstall_style_scheme (scheme))
	{
		gedit_warning (GTK_WINDOW (dlg),
		               _("Could not remove color scheme “%s”."),
		               gtk_source_style_scheme_get_name (scheme));
	}
}

static void
setup_font_colors_page_style_scheme_section (GeditPreferencesDialog *dlg)
{
	GtkStyleContext *context;
	GtkSourceStyleScheme *scheme;

	gedit_debug (DEBUG_PREFS);

	scheme = get_default_color_scheme (dlg);

	/* junction between the scrolled window and the toolbar */
	context = gtk_widget_get_style_context (dlg->schemes_scrolled_window);
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_BOTTOM);
	context = gtk_widget_get_style_context (dlg->schemes_toolbar);
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_TOP);

	/* Connect signals */
	g_signal_connect (dlg->schemes_list,
	                  "notify::style-scheme",
	                  G_CALLBACK (style_scheme_changed),
	                  dlg);
	g_signal_connect (dlg->install_scheme_button,
			  "clicked",
			  G_CALLBACK (install_scheme_clicked),
			  dlg);
	g_signal_connect (dlg->uninstall_scheme_button,
			  "clicked",
			  G_CALLBACK (uninstall_scheme_clicked),
			  dlg);

	gtk_source_style_scheme_chooser_set_style_scheme (GTK_SOURCE_STYLE_SCHEME_CHOOSER (dlg->schemes_list),
	                                                  scheme);

	/* Set initial widget sensitivity */
	set_buttons_sensisitivity_according_to_scheme (dlg, scheme);
}

static void
setup_font_colors_page (GeditPreferencesDialog *dlg)
{
	setup_font_colors_page_font_section (dlg);
	setup_font_colors_page_style_scheme_section (dlg);
}

static void
setup_plugins_page (GeditPreferencesDialog *dlg)
{
	gtk_widget_show_all (dlg->plugin_manager);
}

static void
gedit_preferences_dialog_init (GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS);

	dlg->editor = g_settings_new ("org.gnome.gedit.preferences.editor");
	dlg->uisettings = g_settings_new ("org.gnome.gedit.preferences.ui");

	gtk_widget_init_template (GTK_WIDGET (dlg));

	setup_editor_page (dlg);
	setup_view_page (dlg);
	setup_font_colors_page (dlg);
	setup_plugins_page (dlg);
}

void
gedit_show_preferences_dialog (GeditWindow *parent)
{
	gedit_debug (DEBUG_PREFS);

	if (preferences_dialog == NULL)
	{
		preferences_dialog = GTK_WIDGET (g_object_new (GEDIT_TYPE_PREFERENCES_DIALOG,
							       "application", g_application_get_default (),
							       NULL));
		g_signal_connect (preferences_dialog,
				  "destroy",
				  G_CALLBACK (gtk_widget_destroyed),
				  &preferences_dialog);
	}

	if (GTK_WINDOW (parent) != gtk_window_get_transient_for (GTK_WINDOW (preferences_dialog)))
	{
		gtk_window_set_transient_for (GTK_WINDOW (preferences_dialog),
					      GTK_WINDOW (parent));
	}

	gtk_window_present (GTK_WINDOW (preferences_dialog));
}

/* ex:set ts=8 noet: */
