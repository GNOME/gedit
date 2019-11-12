/*
 * gedit-menu-stack-switcher.c
 * This file is part of gedit
 *
 * Copyright (C) 2014 - Steve Frécinaux
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

#include "gedit-menu-stack-switcher.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

struct _GeditMenuStackSwitcher
{
  GtkMenuButton parent_instance;

  GtkStack *stack;
  GtkWidget *label;
  GtkWidget *button_box;
  GtkWidget *popover;
  GHashTable *buttons;
  gboolean in_child_changed;
};

enum {
  PROP_0,
  PROP_STACK,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

G_DEFINE_TYPE (GeditMenuStackSwitcher, gedit_menu_stack_switcher, GTK_TYPE_MENU_BUTTON)

static void
gedit_menu_stack_switcher_init (GeditMenuStackSwitcher *switcher)
{
  GtkWidget *box;
  GtkWidget *arrow;
  GtkStyleContext *context;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  arrow = gtk_image_new_from_icon_name ("pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_end (GTK_BOX (box), arrow, FALSE, TRUE, 0);
  gtk_widget_set_valign (arrow, GTK_ALIGN_BASELINE);

  switcher->label = gtk_label_new (NULL);
  gtk_widget_set_valign (switcher->label, GTK_ALIGN_BASELINE);
  gtk_box_pack_start (GTK_BOX (box), switcher->label, TRUE, TRUE, 6);

  // FIXME: this is not correct if this widget becomes more generic
  // and used also outside the header bar, but for now we just want
  // the same style as title labels
  context = gtk_widget_get_style_context (switcher->label);
  gtk_style_context_add_class (context, "title");

  gtk_widget_show_all (box);
  gtk_container_add (GTK_CONTAINER (switcher), box);

  switcher->popover = gtk_popover_new (GTK_WIDGET (switcher));
  gtk_popover_set_position (GTK_POPOVER (switcher->popover), GTK_POS_BOTTOM);
  context = gtk_widget_get_style_context (switcher->popover);
  gtk_style_context_add_class (context, "gedit-menu-stack-switcher");

  switcher->button_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

  gtk_widget_show (switcher->button_box);

  gtk_container_add (GTK_CONTAINER (switcher->popover), switcher->button_box);

  gtk_menu_button_set_popover (GTK_MENU_BUTTON (switcher), switcher->popover);

  switcher->buttons = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void
clear_popover (GeditMenuStackSwitcher *switcher)
{
  gtk_container_foreach (GTK_CONTAINER (switcher->button_box), (GtkCallback) gtk_widget_destroy, switcher);
}

static void
on_button_clicked (GtkWidget              *widget,
                   GeditMenuStackSwitcher *switcher)
{
  GtkWidget *child;

  if (!switcher->in_child_changed)
    {
      child = g_object_get_data (G_OBJECT (widget), "stack-child");
      gtk_stack_set_visible_child (switcher->stack, child);
      gtk_widget_hide (switcher->popover);
    }
}

static void
update_button (GeditMenuStackSwitcher *switcher,
               GtkWidget              *widget,
               GtkWidget              *button)
{
  GList *children;

  /* We get spurious notifications while the stack is being
   * destroyed, so for now check the child actually exists
   */
  children = gtk_container_get_children (GTK_CONTAINER (switcher->stack));
  if (g_list_index (children, widget) >= 0)
    {
      gchar *title;

      gtk_container_child_get (GTK_CONTAINER (switcher->stack), widget,
                               "title", &title,
                               NULL);

      gtk_button_set_label (GTK_BUTTON (button), title);
      gtk_widget_set_visible (button, gtk_widget_get_visible (widget) && (title != NULL));
      gtk_widget_set_size_request (button, 100, -1);

      if (widget == gtk_stack_get_visible_child (switcher->stack))
        {
          gtk_label_set_label (GTK_LABEL (switcher->label), title);
        }

      g_free (title);
    }

   g_list_free (children);
}

static void
on_title_icon_visible_updated (GtkWidget              *widget,
                               GParamSpec             *pspec,
                               GeditMenuStackSwitcher *switcher)
{
  GtkWidget *button;

  button = g_hash_table_lookup (switcher->buttons, widget);
  update_button (switcher, widget, button);
}

static void
on_position_updated (GtkWidget        *widget,
                     GParamSpec       *pspec,
                     GeditMenuStackSwitcher *switcher)
{
  GtkWidget *button;
  gint position;

  button = g_hash_table_lookup (switcher->buttons, widget);

  gtk_container_child_get (GTK_CONTAINER (switcher->stack), widget,
                           "position", &position,
                           NULL);

  gtk_box_reorder_child (GTK_BOX (switcher->button_box), button, position);
}

static void
add_child (GeditMenuStackSwitcher *switcher,
           GtkWidget              *widget)
{
  GtkWidget *button;
  GList *group;

  button = gtk_radio_button_new (NULL);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_widget_set_valign (button, GTK_ALIGN_CENTER);

  update_button (switcher, widget, button);

  group = gtk_container_get_children (GTK_CONTAINER (switcher->button_box));
  if (group != NULL)
    {
      gtk_radio_button_join_group (GTK_RADIO_BUTTON (button), GTK_RADIO_BUTTON (group->data));
      g_list_free (group);
    }

  gtk_container_add (GTK_CONTAINER (switcher->button_box), button);

  g_object_set_data (G_OBJECT (button), "stack-child", widget);
  g_signal_connect (button, "clicked", G_CALLBACK (on_button_clicked), switcher);
  g_signal_connect (widget, "notify::visible", G_CALLBACK (on_title_icon_visible_updated), switcher);
  g_signal_connect (widget, "child-notify::title", G_CALLBACK (on_title_icon_visible_updated), switcher);
  g_signal_connect (widget, "child-notify::icon-name", G_CALLBACK (on_title_icon_visible_updated), switcher);
  g_signal_connect (widget, "child-notify::position", G_CALLBACK (on_position_updated), switcher);

  g_hash_table_insert (switcher->buttons, widget, button);
}

static void
foreach_stack (GtkWidget              *widget,
               GeditMenuStackSwitcher *switcher)
{
  add_child (switcher, widget);
}

static void
populate_popover (GeditMenuStackSwitcher *switcher)
{
  gtk_container_foreach (GTK_CONTAINER (switcher->stack), (GtkCallback)foreach_stack, switcher);
}

static void
on_child_changed (GtkWidget              *widget,
                  GParamSpec             *pspec,
                  GeditMenuStackSwitcher *switcher)
{
  GtkWidget *child;
  GtkWidget *button;

  child = gtk_stack_get_visible_child (GTK_STACK (widget));
  if (child)
    {
      gchar *title;

      gtk_container_child_get (GTK_CONTAINER (switcher->stack), child,
                               "title", &title,
                               NULL);

      gtk_label_set_label (GTK_LABEL (switcher->label), title);
      g_free (title);
    }

  button = g_hash_table_lookup (switcher->buttons, child);
  if (button != NULL)
    {
      switcher->in_child_changed = TRUE;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      switcher->in_child_changed = FALSE;
    }
}

static void
on_stack_child_added (GtkStack               *stack,
                      GtkWidget              *widget,
                      GeditMenuStackSwitcher *switcher)
{
  add_child (switcher, widget);
}

static void
on_stack_child_removed (GtkStack               *stack,
                        GtkWidget              *widget,
                        GeditMenuStackSwitcher *switcher)
{
  GtkWidget *button;

  g_signal_handlers_disconnect_by_func (widget, on_title_icon_visible_updated, switcher);
  g_signal_handlers_disconnect_by_func (widget, on_title_icon_visible_updated, switcher);
  g_signal_handlers_disconnect_by_func (widget, on_title_icon_visible_updated, switcher);
  g_signal_handlers_disconnect_by_func (widget, on_position_updated, switcher);

  button = g_hash_table_lookup (switcher->buttons, widget);
  gtk_container_remove (GTK_CONTAINER (switcher->button_box), button);
  g_hash_table_remove (switcher->buttons, widget);
}

static void
disconnect_stack_signals (GeditMenuStackSwitcher *switcher)
{
  g_signal_handlers_disconnect_by_func (switcher->stack, on_stack_child_added, switcher);
  g_signal_handlers_disconnect_by_func (switcher->stack, on_stack_child_removed, switcher);
  g_signal_handlers_disconnect_by_func (switcher->stack, on_child_changed, switcher);
  g_signal_handlers_disconnect_by_func (switcher->stack, disconnect_stack_signals, switcher);
}

static void
connect_stack_signals (GeditMenuStackSwitcher *switcher)
{
  g_signal_connect (switcher->stack, "add",
                    G_CALLBACK (on_stack_child_added), switcher);
  g_signal_connect (switcher->stack, "remove",
                    G_CALLBACK (on_stack_child_removed), switcher);
  g_signal_connect (switcher->stack, "notify::visible-child",
                    G_CALLBACK (on_child_changed), switcher);
  g_signal_connect_swapped (switcher->stack, "destroy",
                            G_CALLBACK (disconnect_stack_signals), switcher);
}

void
gedit_menu_stack_switcher_set_stack (GeditMenuStackSwitcher *switcher,
                                     GtkStack               *stack)
{
  g_return_if_fail (GEDIT_IS_MENU_STACK_SWITCHER (switcher));
  g_return_if_fail (stack == NULL || GTK_IS_STACK (stack));

  if (switcher->stack == stack)
    return;

  if (switcher->stack)
    {
      disconnect_stack_signals (switcher);
      clear_popover (switcher);
      g_clear_object (&switcher->stack);
    }

  if (stack)
    {
      switcher->stack = g_object_ref (stack);
      populate_popover (switcher);
      connect_stack_signals (switcher);
    }

  gtk_widget_queue_resize (GTK_WIDGET (switcher));

  g_object_notify_by_pspec (G_OBJECT (switcher), properties[PROP_STACK]);
}

GtkStack *
gedit_menu_stack_switcher_get_stack (GeditMenuStackSwitcher *switcher)
{
  g_return_val_if_fail (GEDIT_IS_MENU_STACK_SWITCHER (switcher), NULL);

  return switcher->stack;
}

static void
gedit_menu_stack_switcher_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GeditMenuStackSwitcher *switcher = GEDIT_MENU_STACK_SWITCHER (object);

  switch (prop_id)
    {
    case PROP_STACK:
      g_value_set_object (value, switcher->stack);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gedit_menu_stack_switcher_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GeditMenuStackSwitcher *switcher = GEDIT_MENU_STACK_SWITCHER (object);

  switch (prop_id)
    {
    case PROP_STACK:
      gedit_menu_stack_switcher_set_stack (switcher, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gedit_menu_stack_switcher_dispose (GObject *object)
{
  GeditMenuStackSwitcher *switcher = GEDIT_MENU_STACK_SWITCHER (object);

  gedit_menu_stack_switcher_set_stack (switcher, NULL);

  G_OBJECT_CLASS (gedit_menu_stack_switcher_parent_class)->dispose (object);
}

static void
gedit_menu_stack_switcher_finalize (GObject *object)
{
  GeditMenuStackSwitcher *switcher = GEDIT_MENU_STACK_SWITCHER (object);

  g_hash_table_destroy (switcher->buttons);

  G_OBJECT_CLASS (gedit_menu_stack_switcher_parent_class)->finalize (object);
}

static void
gedit_menu_stack_switcher_class_init (GeditMenuStackSwitcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gedit_menu_stack_switcher_get_property;
  object_class->set_property = gedit_menu_stack_switcher_set_property;
  object_class->dispose = gedit_menu_stack_switcher_dispose;
  object_class->finalize = gedit_menu_stack_switcher_finalize;

  properties[PROP_STACK] =
    g_param_spec_object ("stack",
                         "Stack",
                         "Stack",
                         GTK_TYPE_STACK,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

GtkWidget *
gedit_menu_stack_switcher_new (void)
{
  return g_object_new (GEDIT_TYPE_MENU_STACK_SWITCHER, NULL);
}

/* ex:set ts=2 sw=2 et: */
