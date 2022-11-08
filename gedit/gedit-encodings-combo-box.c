/*
 * gedit-encodings-combo-box.c
 * This file is part of gedit
 *
 * Copyright (C) 2003-2005 - Paolo Maggi
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

#include "gedit-encodings-combo-box.h"
#include <glib/gi18n.h>
#include "gedit-encoding-items.h"
#include "gedit-encodings-dialog.h"

struct _GeditEncodingsComboBox
{
	GtkComboBox parent_instance;

	GtkListStore *store;
	glong changed_id;

	guint activated_item;

	guint save_mode : 1;
};

enum
{
	COLUMN_NAME,
	COLUMN_ENCODING,
	COLUMN_CONFIGURE_ROW, /* TRUE for the "Add or Remove..." row. */
	N_COLUMNS
};

enum
{
	PROP_0,
	PROP_SAVE_MODE,
	LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

G_DEFINE_TYPE (GeditEncodingsComboBox, gedit_encodings_combo_box, GTK_TYPE_COMBO_BOX)

static void	update_menu		(GeditEncodingsComboBox       *combo_box);

static void
gedit_encodings_combo_box_set_property (GObject    *object,
					guint       prop_id,
					const       GValue *value,
					GParamSpec *pspec)
{
	GeditEncodingsComboBox *combo;

	combo = GEDIT_ENCODINGS_COMBO_BOX (object);

	switch (prop_id)
	{
		case PROP_SAVE_MODE:
			combo->save_mode = g_value_get_boolean (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_encodings_combo_box_get_property (GObject    *object,
					guint       prop_id,
					GValue 	   *value,
					GParamSpec *pspec)
{
	GeditEncodingsComboBox *combo;

	combo = GEDIT_ENCODINGS_COMBO_BOX (object);

	switch (prop_id)
	{
		case PROP_SAVE_MODE:
			g_value_set_boolean (value, combo->save_mode);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_encodings_combo_box_dispose (GObject *object)
{
	GeditEncodingsComboBox *combo = GEDIT_ENCODINGS_COMBO_BOX (object);

	g_clear_object (&combo->store);

	G_OBJECT_CLASS (gedit_encodings_combo_box_parent_class)->dispose (object);
}

static void
gedit_encodings_combo_box_constructed (GObject *object)
{
	GeditEncodingsComboBox *combo = GEDIT_ENCODINGS_COMBO_BOX (object);
	GtkCellRenderer *text_renderer;

	G_OBJECT_CLASS (gedit_encodings_combo_box_parent_class)->constructed (object);

	/* Setup up the cells */
	text_renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (combo),
				  text_renderer, TRUE);

	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
					text_renderer,
					"text",
					COLUMN_NAME,
					NULL);

	update_menu (combo);
}

static void
gedit_encodings_combo_box_class_init (GeditEncodingsComboBoxClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = gedit_encodings_combo_box_set_property;
	object_class->get_property = gedit_encodings_combo_box_get_property;
	object_class->dispose = gedit_encodings_combo_box_dispose;
	object_class->constructed = gedit_encodings_combo_box_constructed;

	/**
	 * GeditEncodingsComboBox:save-mode:
	 *
	 * Whether the combo box should be used for saving a content. If
	 * %FALSE, the combo box is used for loading a content (e.g. a file)
	 * and the row "Automatically Detected" is added.
	 */
	/* TODO It'd be clearer if "save-mode" is renamed as "mode" with an
	 * enum: loading, saving. Or something like that.
	 */
	properties[PROP_SAVE_MODE] =
		g_param_spec_boolean ("save-mode",
		                      "Save Mode",
		                      "Save Mode",
		                      FALSE,
		                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
dialog_response_cb (GtkDialog              *dialog,
                    gint                    response_id,
                    GeditEncodingsComboBox *menu)
{
	update_menu (menu);
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
configure_encodings (GeditEncodingsComboBox *menu)
{
	GtkWidget *dialog;

	GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (menu));

	if (!gtk_widget_is_toplevel (toplevel))
	{
		toplevel = NULL;
	}

	g_signal_handler_block (menu, menu->changed_id);
	gtk_combo_box_set_active (GTK_COMBO_BOX (menu),
				  menu->activated_item);
	g_signal_handler_unblock (menu, menu->changed_id);

	dialog = gedit_encodings_dialog_new ();

	if (toplevel != NULL)
	{
		GtkWindowGroup *wg;

		gtk_window_set_transient_for (GTK_WINDOW (dialog),
					      GTK_WINDOW (toplevel));

		if (gtk_window_has_group (GTK_WINDOW (toplevel)))
		{
			wg = gtk_window_get_group (GTK_WINDOW (toplevel));
		}
		else
		{
			wg = gtk_window_group_new ();
			gtk_window_group_add_window (wg, GTK_WINDOW (toplevel));
		}

		gtk_window_group_add_window (wg, GTK_WINDOW (dialog));
	}

	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

	g_signal_connect_after (dialog,
				"response",
				G_CALLBACK (dialog_response_cb),
				menu);

	gtk_widget_show (dialog);
}

static void
changed_cb (GeditEncodingsComboBox *menu,
	    GtkTreeModel           *model)
{
	GtkTreeIter iter;
	gboolean configure = FALSE;

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (menu), &iter))
	{
		gtk_tree_model_get (model, &iter,
				    COLUMN_CONFIGURE_ROW, &configure,
				    -1);
	}

	if (configure)
	{
		configure_encodings (menu);
	}
	else
	{
		menu->activated_item = gtk_combo_box_get_active (GTK_COMBO_BOX (menu));
	}
}

static gboolean
separator_func (GtkTreeModel *model,
		GtkTreeIter  *iter,
		gpointer      data)
{
	gchar *str;
	gboolean ret;

	gtk_tree_model_get (model, iter, COLUMN_NAME, &str, -1);
	ret = (str == NULL || str[0] == '\0');
	g_free (str);

	return ret;
}

static void
add_separator (GtkListStore *store)
{
	GtkTreeIter iter;

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    COLUMN_NAME, "",
			    COLUMN_ENCODING, NULL,
			    COLUMN_CONFIGURE_ROW, FALSE,
			    -1);
}

static void
update_menu (GeditEncodingsComboBox *menu)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GSList *encodings;

	store = menu->store;

	/* Unset the previous model */
	g_signal_handler_block (menu, menu->changed_id);
	gtk_list_store_clear (store);
	gtk_combo_box_set_model (GTK_COMBO_BOX (menu), NULL);

	if (!menu->save_mode)
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_NAME, _("Automatically Detected"),
				    COLUMN_ENCODING, NULL,
				    COLUMN_CONFIGURE_ROW, FALSE,
				    -1);

		add_separator (store);
	}

	encodings = gedit_encoding_items_get ();

	while (encodings)
	{
		GeditEncodingItem *item = encodings->data;

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_NAME, gedit_encoding_item_get_name (item),
				    COLUMN_ENCODING, gedit_encoding_item_get_encoding (item),
				    COLUMN_CONFIGURE_ROW, FALSE,
				    -1);

		gedit_encoding_item_free (item);
		encodings = g_slist_delete_link (encodings, encodings);
	}

	add_separator (store);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    COLUMN_NAME, _("Add or Remove…"),
			    COLUMN_ENCODING, NULL,
			    COLUMN_CONFIGURE_ROW, TRUE,
			    -1);

	/* set the model back */
	gtk_combo_box_set_model (GTK_COMBO_BOX (menu),
				 GTK_TREE_MODEL (menu->store));
	gtk_combo_box_set_active (GTK_COMBO_BOX (menu), 0);

	g_signal_handler_unblock (menu, menu->changed_id);
}

static void
gedit_encodings_combo_box_init (GeditEncodingsComboBox *menu)
{
	menu->store = gtk_list_store_new (N_COLUMNS,
	                                  G_TYPE_STRING,
	                                  G_TYPE_POINTER,
	                                  G_TYPE_BOOLEAN);

	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (menu),
	                                      separator_func, NULL,
	                                      NULL);

	menu->changed_id = g_signal_connect (menu,
	                                     "changed",
	                                     G_CALLBACK (changed_cb),
	                                     menu->store);
}

/**
 * gedit_encodings_combo_box_new:
 * @save_mode: whether the combo box is used for saving a content.
 *
 * Creates a new encodings combo box object. If @save_mode is %FALSE, it means
 * that the combo box is used for loading a content (e.g. a file), so the row
 * "Automatically Detected" is added. For saving a content, the encoding must be
 * provided.
 *
 * Returns: a new #GeditEncodingsComboBox object.
 */
GtkWidget *
gedit_encodings_combo_box_new (gboolean save_mode)
{
	return g_object_new (GEDIT_TYPE_ENCODINGS_COMBO_BOX,
			     "save_mode", save_mode,
			     NULL);
}

/**
 * gedit_encodings_combo_box_get_selected_encoding:
 * @menu: a #GeditEncodingsComboBox.
 *
 * Returns: the selected #GtkSourceEncoding, or %NULL if the encoding should be
 * auto-detected (only for loading mode, not for saving).
 */
const GtkSourceEncoding *
gedit_encodings_combo_box_get_selected_encoding (GeditEncodingsComboBox *menu)
{
	GtkTreeIter iter;

	g_return_val_if_fail (GEDIT_IS_ENCODINGS_COMBO_BOX (menu), NULL);

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (menu), &iter))
	{
		const GtkSourceEncoding *ret;
		GtkTreeModel *model;

		model = gtk_combo_box_get_model (GTK_COMBO_BOX (menu));

		gtk_tree_model_get (model, &iter,
				    COLUMN_ENCODING, &ret,
				    -1);

		return ret;
	}

	return NULL;
}

/**
 * gedit_encodings_combo_box_set_selected_encoding:
 * @menu: a #GeditEncodingsComboBox.
 * @encoding: the #GtkSourceEncoding.
 *
 * Sets the selected encoding.
 */
void
gedit_encodings_combo_box_set_selected_encoding (GeditEncodingsComboBox  *menu,
						 const GtkSourceEncoding *encoding)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean b;

	g_return_if_fail (GEDIT_IS_ENCODINGS_COMBO_BOX (menu));

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (menu));
	b = gtk_tree_model_get_iter_first (model, &iter);

	while (b)
	{
		const GtkSourceEncoding *enc;

		gtk_tree_model_get (model, &iter,
				    COLUMN_ENCODING, &enc,
				    -1);

		if (enc == encoding)
		{
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (menu), &iter);
			return;
		}

		b = gtk_tree_model_iter_next (model, &iter);
	}
}

/* ex:set ts=8 noet: */
