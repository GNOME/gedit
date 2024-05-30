/*
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

#include "gedit-preferences-dialog.h"
#include <tepl/tepl.h>
#include <libpeas-gtk/peas-gtk.h>
#include "gedit-debug.h"
#include "gedit-settings.h"

/* gedit-preferences dialog is a singleton since we don't
 * want two dialogs showing an inconsistent state of the
 * preferences.
 * When gedit_show_preferences_dialog() is called and there
 * is already a prefs dialog dialog open, it is reparented
 * and shown.
 */

static GtkWindow *preferences_dialog = NULL;

#define GEDIT_TYPE_PREFERENCES_DIALOG (gedit_preferences_dialog_get_type())

G_DECLARE_FINAL_TYPE (GeditPreferencesDialog, gedit_preferences_dialog,
		      GEDIT, PREFERENCES_DIALOG,
		      GtkWindow)

enum
{
	SIGNAL_CLOSE,
	N_SIGNALS
};

static guint signals[N_SIGNALS];

struct _GeditPreferencesDialog
{
	GtkWindow parent_instance;

	GSettings *editor;

	/* Tabs */
	GtkWidget *insert_spaces_checkbutton;

	/* Auto indentation */
	GtkWidget *auto_indent_checkbutton;

	/* Text Wrapping */
	GtkWidget *wrap_text_checkbutton;
	GtkWidget *split_checkbutton;

	/* Plugin manager */
	GtkWidget *plugin_manager;

	/* Placeholders */
	GtkGrid *view_placeholder;
	GtkGrid *font_and_colors_placeholder;
	GtkGrid *tab_width_spinbutton_placeholder;
	GtkGrid *highlighting_component_placeholder;
	GtkGrid *files_component_placeholder;
};

G_DEFINE_TYPE (GeditPreferencesDialog, gedit_preferences_dialog, GTK_TYPE_WINDOW)

static void
gedit_preferences_dialog_close (GeditPreferencesDialog *dialog)
{
	gtk_window_close (GTK_WINDOW (dialog));
}

static void
gedit_preferences_dialog_class_init (GeditPreferencesDialogClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkBindingSet *binding_set;

	/* Otherwise libpeas-gtk might not be linked */
	g_type_ensure (PEAS_GTK_TYPE_PLUGIN_MANAGER);

	signals[SIGNAL_CLOSE] =
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
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, wrap_text_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, split_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, insert_spaces_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, auto_indent_checkbutton);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, plugin_manager);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, view_placeholder);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, font_and_colors_placeholder);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, tab_width_spinbutton_placeholder);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, highlighting_component_placeholder);
	gtk_widget_class_bind_template_child (widget_class, GeditPreferencesDialog, files_component_placeholder);
}

static void
setup_editor_page (GeditPreferencesDialog *dlg)
{
	GtkWidget *tab_width_spinbutton_component;
	GtkWidget *files_component;

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

	tab_width_spinbutton_component = tepl_prefs_create_tab_width_spinbutton (dlg->editor,
										 GEDIT_SETTINGS_TABS_SIZE);
	files_component = tepl_prefs_create_files_component (dlg->editor,
							     GEDIT_SETTINGS_CREATE_BACKUP_COPY,
							     GEDIT_SETTINGS_AUTO_SAVE,
							     GEDIT_SETTINGS_AUTO_SAVE_INTERVAL);
	gtk_container_add (GTK_CONTAINER (dlg->tab_width_spinbutton_placeholder),
			   tab_width_spinbutton_component);
	gtk_container_add (GTK_CONTAINER (dlg->files_component_placeholder),
			   files_component);
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
setup_view_page (GeditPreferencesDialog *dlg)
{
	GeditSettings *gedit_settings;
	GSettings *ui_settings;
	GtkWrapMode wrap_mode;
	GtkWrapMode last_split_mode;
	GtkWidget *display_line_numbers_checkbutton;
	GtkWidget *right_margin_component;
	GtkWidget *display_grid_checkbutton;
	GtkWidget *display_statusbar_checkbutton;
	GtkWidget *highlighting_component;

	gedit_settings = _gedit_settings_get_singleton ();
	ui_settings = _gedit_settings_peek_ui_settings (gedit_settings);

	/* Set initial state */
	wrap_mode = g_settings_get_enum (dlg->editor, GEDIT_SETTINGS_WRAP_MODE);

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

	/* Set widgets sensitivity */
	gtk_widget_set_sensitive (dlg->split_checkbutton,
				  (wrap_mode != GTK_WRAP_NONE));

	g_signal_connect (dlg->wrap_text_checkbutton,
			  "toggled",
			  G_CALLBACK (wrap_mode_checkbutton_toggled),
			  dlg);
	g_signal_connect (dlg->split_checkbutton,
			  "toggled",
			  G_CALLBACK (wrap_mode_checkbutton_toggled),
			  dlg);

	display_line_numbers_checkbutton = tepl_prefs_create_display_line_numbers_checkbutton (dlg->editor,
											       GEDIT_SETTINGS_DISPLAY_LINE_NUMBERS);
	right_margin_component = tepl_prefs_create_right_margin_component (dlg->editor,
									   GEDIT_SETTINGS_DISPLAY_RIGHT_MARGIN,
									   GEDIT_SETTINGS_RIGHT_MARGIN_POSITION);
	display_grid_checkbutton = tepl_prefs_create_display_grid_checkbutton (dlg->editor,
									       GEDIT_SETTINGS_BACKGROUND_PATTERN);
	display_statusbar_checkbutton = tepl_prefs_create_display_statusbar_checkbutton (ui_settings,
											 GEDIT_SETTINGS_STATUSBAR_VISIBLE);
	highlighting_component = tepl_prefs_create_highlighting_component (dlg->editor,
									   GEDIT_SETTINGS_HIGHLIGHT_CURRENT_LINE,
									   GEDIT_SETTINGS_BRACKET_MATCHING);

	gtk_orientable_set_orientation (GTK_ORIENTABLE (dlg->view_placeholder),
					GTK_ORIENTATION_VERTICAL);
	gtk_grid_set_row_spacing (dlg->view_placeholder, 6);

	gtk_container_add (GTK_CONTAINER (dlg->view_placeholder),
			   display_line_numbers_checkbutton);
	gtk_container_add (GTK_CONTAINER (dlg->view_placeholder),
			   right_margin_component);
	gtk_container_add (GTK_CONTAINER (dlg->view_placeholder),
			   display_grid_checkbutton);
	gtk_container_add (GTK_CONTAINER (dlg->view_placeholder),
			   display_statusbar_checkbutton);
	gtk_container_add (GTK_CONTAINER (dlg->highlighting_component_placeholder),
			   highlighting_component);
}

static void
setup_font_colors_page (GeditPreferencesDialog *dlg)
{
	GeditSettings *gedit_settings;
	GSettings *editor_settings;
	GSettings *ui_settings;
	GtkWidget *font_component;
	GtkWidget *theme_variant_combo_box;

	gedit_settings = _gedit_settings_get_singleton ();
	editor_settings = _gedit_settings_peek_editor_settings (gedit_settings);
	ui_settings = _gedit_settings_peek_ui_settings (gedit_settings);

	/* Configure GtkGrid placeholder */
	gtk_orientable_set_orientation (GTK_ORIENTABLE (dlg->font_and_colors_placeholder),
					GTK_ORIENTATION_VERTICAL);
	gtk_grid_set_row_spacing (dlg->font_and_colors_placeholder, 18);

	/* Font */
	font_component = tepl_prefs_create_font_component (editor_settings,
							   GEDIT_SETTINGS_USE_DEFAULT_FONT,
							   GEDIT_SETTINGS_EDITOR_FONT);
	gtk_container_add (GTK_CONTAINER (dlg->font_and_colors_placeholder),
			   font_component);

	/* Theme variant */
	theme_variant_combo_box = tepl_prefs_create_theme_variant_combo_box (ui_settings,
									     GEDIT_SETTINGS_THEME_VARIANT);
	gtk_container_add (GTK_CONTAINER (dlg->font_and_colors_placeholder),
			   theme_variant_combo_box);

	/* Color/Style scheme */
	gtk_container_add (GTK_CONTAINER (dlg->font_and_colors_placeholder),
			   GTK_WIDGET (tepl_style_scheme_chooser_widget_new (TRUE)));
}

static void
setup_plugins_page (GeditPreferencesDialog *dlg)
{
	gtk_widget_show_all (dlg->plugin_manager);
}

static void
gedit_preferences_dialog_init (GeditPreferencesDialog *dialog)
{
	GeditSettings *gedit_settings;

	gedit_settings = _gedit_settings_get_singleton ();
	dialog->editor = _gedit_settings_peek_editor_settings (gedit_settings);

	gtk_widget_init_template (GTK_WIDGET (dialog));

	setup_view_page (dialog);
	setup_editor_page (dialog);
	setup_font_colors_page (dialog);
	setup_plugins_page (dialog);
}

void
gedit_show_preferences_dialog (GtkWindow *parent)
{
	g_return_if_fail (GTK_IS_WINDOW (parent));

	if (preferences_dialog == NULL)
	{
		preferences_dialog = g_object_new (GEDIT_TYPE_PREFERENCES_DIALOG,
						   "application", g_application_get_default (),
						   NULL);

		g_signal_connect (preferences_dialog,
				  "destroy",
				  G_CALLBACK (gtk_widget_destroyed),
				  &preferences_dialog);
	}

	if (parent != gtk_window_get_transient_for (preferences_dialog))
	{
		gtk_window_set_transient_for (preferences_dialog, parent);
	}

	gtk_window_present (preferences_dialog);
}
