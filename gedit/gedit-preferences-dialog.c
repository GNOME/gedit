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

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <tepl/tepl.h>
#include <libpeas-gtk/peas-gtk.h>

#include "gedit-debug.h"
#include "gedit-dirs.h"
#include "gedit-settings.h"

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

	/* Style Scheme */
	GtkWidget	*schemes_list;
	GtkWidget	*install_scheme_button;
	GtkWidget	*uninstall_scheme_button;
	GtkWidget	*schemes_toolbar;
	GtkFileChooserNative *
			 install_scheme_file_chooser;

	/* Tabs */
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

	GtkWidget	*display_statusbar_checkbutton;
	GtkWidget	*display_grid_checkbutton;

	/* Right margin */
	GtkWidget	*right_margin_checkbutton;
	GtkWidget       *right_margin_position_grid;
	GtkWidget	*right_margin_position_spinbutton;

	/* Plugin manager */
	GtkWidget	*plugin_manager;

	/* Placeholders */
	GtkGrid *font_component_placeholder;
	GtkGrid *display_line_numbers_checkbutton_placeholder;
	GtkGrid *tab_width_spinbutton_placeholder;
	GtkGrid *highlighting_component_placeholder;
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
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, display_statusbar_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, display_grid_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, right_margin_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, right_margin_position_grid);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, right_margin_position_spinbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, wrap_text_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, split_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, insert_spaces_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, auto_indent_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, backup_copy_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, auto_save_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, auto_save_spinbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, schemes_list);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, install_scheme_button);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, uninstall_scheme_button);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, schemes_toolbar);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, plugin_manager);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, font_component_placeholder);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, display_line_numbers_checkbutton_placeholder);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, tab_width_spinbutton_placeholder);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, highlighting_component_placeholder);
}

static void
setup_editor_page (GeditPreferencesDialog *dlg)
{
	GtkWidget *tab_width_spinbutton_component;
	gedit_debug (DEBUG_PREFS);

	/* Connect signal */
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

	tab_width_spinbutton_component = tepl_prefs_create_tab_width_spinbutton (dlg->editor,
										 GEDIT_SETTINGS_TABS_SIZE);
	gtk_container_add (GTK_CONTAINER (dlg->tab_width_spinbutton_placeholder),
			   tab_width_spinbutton_component);
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
	GtkWidget *display_line_numbers_checkbutton;
	GtkWidget *highlighting_component;

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

	g_settings_bind (dlg->uisettings,
	                 GEDIT_SETTINGS_STATUSBAR_VISIBLE,
	                 dlg->display_statusbar_checkbutton,
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

	display_line_numbers_checkbutton = tepl_prefs_create_display_line_numbers_checkbutton (dlg->editor,
											       GEDIT_SETTINGS_DISPLAY_LINE_NUMBERS);
	highlighting_component = tepl_prefs_create_highlighting_component (dlg->editor,
									   GEDIT_SETTINGS_HIGHLIGHT_CURRENT_LINE,
									   GEDIT_SETTINGS_BRACKET_MATCHING);
	gtk_container_add (GTK_CONTAINER (dlg->display_line_numbers_checkbutton_placeholder),
			   display_line_numbers_checkbutton);
	gtk_container_add (GTK_CONTAINER (dlg->highlighting_component_placeholder),
			   highlighting_component);
}

static void
setup_font_colors_page_font_section (GeditPreferencesDialog *dlg)
{
	GtkWidget *font_component;

	font_component = tepl_prefs_create_font_component (dlg->editor,
							   GEDIT_SETTINGS_USE_DEFAULT_FONT,
							   GEDIT_SETTINGS_EDITOR_FONT);

	gtk_container_add (GTK_CONTAINER (dlg->font_component_placeholder),
			   font_component);
}

static void
update_style_scheme_buttons_sensisitivity (GeditPreferencesDialog *dlg)
{
	GtkSourceStyleScheme *selected_style_scheme;
	gboolean editable = FALSE;

	selected_style_scheme = gtk_source_style_scheme_chooser_get_style_scheme (GTK_SOURCE_STYLE_SCHEME_CHOOSER (dlg->schemes_list));

	if (selected_style_scheme != NULL)
	{
		const gchar *filename;

		filename = gtk_source_style_scheme_get_filename (selected_style_scheme);
		if (filename != NULL)
		{
			editable = g_str_has_prefix (filename, gedit_dirs_get_user_styles_dir ());
		}
	}

	gtk_widget_set_sensitive (dlg->uninstall_scheme_button, editable);
}

static void
style_scheme_notify_cb (GtkSourceStyleSchemeChooser *chooser,
			GParamSpec                  *pspec,
			GeditPreferencesDialog      *dlg)
{
	update_style_scheme_buttons_sensisitivity (dlg);
}

static GFile *
get_user_style_scheme_destination_file (GFile *src_file)
{
	gchar *basename;
	const gchar *styles_dir;
	GFile *dest_file;

	basename = g_file_get_basename (src_file);
	g_return_val_if_fail (basename != NULL, NULL);

	styles_dir = gedit_dirs_get_user_styles_dir ();
	dest_file = g_file_new_build_filename (styles_dir, basename, NULL);

	g_free (basename);
	return dest_file;
}

/* Returns: whether @src_file has been correctly copied to @dest_file. */
static gboolean
copy_file (GFile   *src_file,
	   GFile   *dest_file,
	   GError **error)
{
	if (g_file_equal (src_file, dest_file))
	{
		return FALSE;
	}

	if (!tepl_utils_create_parent_directories (dest_file, NULL, error))
	{
		return FALSE;
	}

	return g_file_copy (src_file,
			    dest_file,
			    G_FILE_COPY_OVERWRITE | G_FILE_COPY_TARGET_DEFAULT_PERMS,
			    NULL, /* cancellable */
			    NULL, NULL, /* progress callback */
			    error);
}

/* Get the style scheme ID of @user_style_scheme_file if it has been correctly
 * installed and @user_style_scheme_file is a valid style scheme file.
 */
static const gchar *
get_style_scheme_id_after_installing_user_style_scheme (GFile *user_style_scheme_file)
{
	GtkSourceStyleSchemeManager *manager;
	const gchar * const *scheme_ids;
	gint i;

	manager = gtk_source_style_scheme_manager_get_default ();
	gtk_source_style_scheme_manager_force_rescan (manager);

	scheme_ids = gtk_source_style_scheme_manager_get_scheme_ids (manager);

	for (i = 0; scheme_ids != NULL && scheme_ids[i] != NULL; i++)
	{
		const gchar *cur_scheme_id = scheme_ids[i];
		GtkSourceStyleScheme *scheme;
		const gchar *filename;
		GFile *scheme_file;

		scheme = gtk_source_style_scheme_manager_get_scheme (manager, cur_scheme_id);
		filename = gtk_source_style_scheme_get_filename (scheme);
		if (filename == NULL)
		{
			continue;
		}

		scheme_file = g_file_new_for_path (filename);
		if (g_file_equal (scheme_file, user_style_scheme_file))
		{
			g_object_unref (scheme_file);
			return cur_scheme_id;
		}

		g_object_unref (scheme_file);
	}

	return NULL;
}

/* Returns: (nullable): the installed style scheme ID, or %NULL on failure. */
static const gchar *
install_style_scheme (GFile   *src_file,
		      GError **error)
{
	GFile *dest_file;
	gboolean copied;
	const gchar *installed_style_scheme_id = NULL;
	GError *my_error = NULL;

	g_return_val_if_fail (G_IS_FILE (src_file), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	dest_file = get_user_style_scheme_destination_file (src_file);
	g_return_val_if_fail (dest_file != NULL, NULL);

	copied = copy_file (src_file, dest_file, &my_error);
	if (my_error != NULL)
	{
		g_propagate_error (error, my_error);
		g_object_unref (dest_file);
		return NULL;
	}

	installed_style_scheme_id = get_style_scheme_id_after_installing_user_style_scheme (dest_file);

	if (installed_style_scheme_id == NULL && copied)
	{
		/* The style scheme has not been correctly installed. */
		g_file_delete (dest_file, NULL, &my_error);
		if (my_error != NULL)
		{
			gchar *dest_file_parse_name = g_file_get_parse_name (dest_file);

			g_warning ("Failed to delete the file “%s”: %s",
				   dest_file_parse_name,
				   my_error->message);

			g_free (dest_file_parse_name);
			g_clear_error (&my_error);
		}
	}

	g_object_unref (dest_file);
	return installed_style_scheme_id;
}

/*
 * uninstall_style_scheme:
 * @scheme: a #GtkSourceStyleScheme
 *
 * Uninstall a user scheme.
 *
 * Returns: %TRUE on success, %FALSE otherwise.
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
add_scheme_chooser_response_cb (GtkFileChooserNative   *chooser,
				gint                    response_id,
				GeditPreferencesDialog *dialog)
{
	GFile *file;
	const gchar *scheme_id;
	GeditSettings *settings;
	GSettings *editor_settings;
	GError *error = NULL;

	if (response_id != GTK_RESPONSE_ACCEPT)
	{
		return;
	}

	file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (chooser));
	if (file == NULL)
	{
		return;
	}

	scheme_id = install_style_scheme (file, &error);
	g_object_unref (file);

	if (scheme_id == NULL)
	{
		if (error != NULL)
		{
			tepl_utils_show_warning_dialog (GTK_WINDOW (dialog),
							_("The selected color scheme cannot be installed: %s"),
							error->message);
		}
		else
		{
			tepl_utils_show_warning_dialog (GTK_WINDOW (dialog),
							_("The selected color scheme cannot be installed."));
		}

		g_clear_error (&error);
		return;
	}

	settings = _gedit_settings_get_singleton ();
	editor_settings = _gedit_settings_peek_editor_settings (settings);
	g_settings_set_string (editor_settings, GEDIT_SETTINGS_SCHEME, scheme_id);
}

static void
install_scheme_clicked (GtkButton              *button,
			GeditPreferencesDialog *dialog)
{
	GtkFileChooserNative *chooser;
	GtkFileFilter *scheme_filter;
	GtkFileFilter *all_filter;

	if (dialog->install_scheme_file_chooser != NULL)
	{
		gtk_native_dialog_show (GTK_NATIVE_DIALOG (dialog->install_scheme_file_chooser));
		return;
	}

	chooser = gtk_file_chooser_native_new (_("Add Color Scheme"),
					       GTK_WINDOW (dialog),
					       GTK_FILE_CHOOSER_ACTION_OPEN,
					       _("_Add Scheme"),
					       _("_Cancel"));

	/* Filters */
	scheme_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (scheme_filter, _("Color Scheme Files"));
	gtk_file_filter_add_pattern (scheme_filter, "*.xml");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), scheme_filter);

	all_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (all_filter, _("All Files"));
	gtk_file_filter_add_pattern (all_filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), all_filter);

	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (chooser), scheme_filter);

	g_signal_connect (chooser,
			  "response",
			  G_CALLBACK (add_scheme_chooser_response_cb),
			  dialog);

	g_set_weak_pointer (&dialog->install_scheme_file_chooser, chooser);

	gtk_native_dialog_show (GTK_NATIVE_DIALOG (chooser));
}

static void
uninstall_scheme_clicked (GtkButton              *button,
			  GeditPreferencesDialog *dlg)
{
	GtkSourceStyleScheme *scheme;
	GtkSourceStyleScheme *new_selected_scheme;

	scheme = gtk_source_style_scheme_chooser_get_style_scheme (GTK_SOURCE_STYLE_SCHEME_CHOOSER (dlg->schemes_list));

	if (scheme == NULL)
	{
		return;
	}

	if (!uninstall_style_scheme (scheme))
	{
		tepl_utils_show_warning_dialog (GTK_WINDOW (dlg),
						_("Could not remove color scheme “%s”."),
						gtk_source_style_scheme_get_name (scheme));
		return;
	}

	new_selected_scheme = gtk_source_style_scheme_chooser_get_style_scheme (GTK_SOURCE_STYLE_SCHEME_CHOOSER (dlg->schemes_list));
	if (new_selected_scheme == NULL)
	{
		GeditSettings *settings;
		GSettings *editor_settings;

		settings = _gedit_settings_get_singleton ();
		editor_settings = _gedit_settings_peek_editor_settings (settings);

		g_settings_reset (editor_settings, GEDIT_SETTINGS_SCHEME);
	}
}

static void
setup_font_colors_page_style_scheme_section (GeditPreferencesDialog *dlg)
{
	GtkStyleContext *context;
	GeditSettings *settings;
	GSettings *editor_settings;

	gedit_debug (DEBUG_PREFS);

	/* junction between the schemes list and the toolbar */
	context = gtk_widget_get_style_context (dlg->schemes_list);
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_BOTTOM);
	context = gtk_widget_get_style_context (dlg->schemes_toolbar);
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_TOP);

	/* Connect signals */
	g_signal_connect (dlg->schemes_list,
	                  "notify::style-scheme",
	                  G_CALLBACK (style_scheme_notify_cb),
	                  dlg);
	g_signal_connect (dlg->install_scheme_button,
			  "clicked",
			  G_CALLBACK (install_scheme_clicked),
			  dlg);
	g_signal_connect (dlg->uninstall_scheme_button,
			  "clicked",
			  G_CALLBACK (uninstall_scheme_clicked),
			  dlg);

	settings = _gedit_settings_get_singleton ();
	editor_settings = _gedit_settings_peek_editor_settings (settings);
	g_settings_bind (editor_settings, GEDIT_SETTINGS_SCHEME,
			 dlg->schemes_list, "tepl-style-scheme-id",
			 G_SETTINGS_BIND_DEFAULT);

	update_style_scheme_buttons_sensisitivity (dlg);
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
