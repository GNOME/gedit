/*
 * gedit-view-centering.c
 * This file is part of gedit
 *
 * Copyright (C) 2014 - Sébastien Lafargue
 *
 * Gedit is free software; you can redistribute this file and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Based on Christian Hergert's prototype.
 */

#include "gedit-view-centering.h"

#include <gtksourceview/gtksource.h>

#include "gedit-view.h"
#include "gedit-debug.h"

struct _GeditViewCenteringPrivate
{
	GtkWidget *box;
	GtkWidget *scrolled_window;
	GtkWidget *sourceview;
	GtkWidget *spacer;

	GtkStyleContext *view_context;
	GdkRGBA view_background;
	GdkRGBA view_line_margin_fg;
	GdkRGBA view_margin_background;
	guint view_text_width;

	guint centered : 1;
	guint view_background_set : 1;
	guint view_line_margin_fg_set : 1;
	guint view_margin_background_set : 1;
};

G_DEFINE_TYPE_WITH_PRIVATE (GeditViewCentering, gedit_view_centering, GTK_TYPE_BIN)

#define STYLE_TEXT			"text"
#define STYLE_RIGHT_MARGIN		"right-margin"

#define RIGHT_MARGIN_LINE_ALPHA		40
#define RIGHT_MARGIN_OVERLAY_ALPHA	15

static gboolean
get_style (GtkSourceStyleScheme *scheme,
	   const gchar          *style_id,
	   const gchar          *attribute,
	   GdkRGBA              *color)
{
	GtkSourceStyle *style;
	gchar *style_string;

	style = gtk_source_style_scheme_get_style (scheme, style_id);
	if (!style)
	{
		return FALSE;
	}

	g_object_get (style, attribute, &style_string, NULL);
	if (style_string)
	{
		gdk_rgba_parse (color, style_string);
		g_free (style_string);

		return TRUE;
	}

	return FALSE;
}

static void
get_spacer_colors (GeditViewCentering   *container,
		   GtkSourceStyleScheme *scheme)
{
	GeditViewCenteringPrivate *priv = container->priv;

	if (scheme)
	{
		priv->view_background_set = get_style (scheme,
		                                       STYLE_TEXT, "background",
		                                       &priv->view_background);

		priv->view_line_margin_fg_set = get_style (scheme,
		                                           STYLE_RIGHT_MARGIN, "foreground",
		                                           &priv->view_line_margin_fg);
		priv->view_line_margin_fg.alpha = RIGHT_MARGIN_LINE_ALPHA / 255.0;

		priv->view_margin_background_set = get_style (scheme,
		                                              STYLE_RIGHT_MARGIN, "background",
		                                              &priv->view_margin_background);
		priv->view_margin_background.alpha = RIGHT_MARGIN_OVERLAY_ALPHA / 255.0;
	}
}

/* FIXME: when GeditViewCentering will be transfered to GtkSourceView,
 * this method will be replaced by a call to a new method called
 * gtk_source_view_get_right_margin_pixel_position ()
 */
static guint
_gedit_view_centering_get_right_margin_pixel_position (GeditViewCentering *container)
{
	GeditViewCenteringPrivate *priv;
	gchar *str;
	PangoFontDescription *font_desc;
	PangoLayout *layout;
	guint right_margin_position;
	gint width = 0;

	g_return_val_if_fail (GEDIT_IS_VIEW_CENTERING (container), 0);

	priv = container->priv;

	right_margin_position = gtk_source_view_get_right_margin_position (GTK_SOURCE_VIEW (priv->sourceview));

	gtk_style_context_save (priv->view_context);
	gtk_style_context_set_state (priv->view_context, GTK_STATE_FLAG_NORMAL);
	gtk_style_context_get (priv->view_context,
	                       gtk_style_context_get_state (priv->view_context),
	                       GTK_STYLE_PROPERTY_FONT, &font_desc,
	                       NULL);
	gtk_style_context_restore (priv->view_context);

	str = g_strnfill (right_margin_position, '_');
	layout = gtk_widget_create_pango_layout (GTK_WIDGET (priv->sourceview), str);
	g_free (str);

	pango_layout_set_font_description (layout, font_desc);
	pango_font_description_free (font_desc);
	pango_layout_get_pixel_size (layout, &width, NULL);

	g_object_unref (G_OBJECT (layout));

	return width;
}

static void
on_view_right_margin_visibility_changed (GeditView          *view,
					 GParamSpec         *pspec,
					 GeditViewCentering *container)
{
	GeditViewCenteringPrivate *priv = container->priv;
	gboolean visibility;

	visibility = gtk_source_view_get_show_right_margin (GTK_SOURCE_VIEW (priv->sourceview));

	gtk_widget_set_visible (GTK_WIDGET (container->priv->spacer), visibility && priv->centered);
}

static void
on_view_right_margin_position_changed (GeditView          *view,
				       GParamSpec         *pspec,
				       GeditViewCentering *container)
{
	GeditViewCenteringPrivate *priv = container->priv;
	gboolean visibility;

	priv->view_text_width = _gedit_view_centering_get_right_margin_pixel_position (container);

	visibility = gtk_source_view_get_show_right_margin (GTK_SOURCE_VIEW (priv->sourceview));

	if (visibility)
	{
		gtk_widget_queue_resize (priv->spacer);
	}
}

static void
on_view_context_changed (GtkStyleContext    *stylecontext,
			 GeditViewCentering *container)
{
	GeditViewCenteringPrivate *priv = container->priv;
	GtkTextBuffer *buffer;
	GtkSourceStyleScheme *scheme;
	gboolean visibility;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->sourceview));
	scheme = gtk_source_buffer_get_style_scheme (GTK_SOURCE_BUFFER (buffer));
	get_spacer_colors (container, scheme);

	priv->view_text_width = _gedit_view_centering_get_right_margin_pixel_position (container);

	visibility = gtk_source_view_get_show_right_margin (GTK_SOURCE_VIEW (priv->sourceview));

	if (visibility)
	{
		gtk_widget_queue_resize (priv->spacer);
	}
}

static gboolean
on_spacer_draw (GeditViewCentering *container,
		cairo_t            *cr,
		GtkDrawingArea     *spacer)
{
	GeditViewCenteringPrivate *priv = container->priv;
	GtkStyleContext *context;
	guint width, height;

	if (!container->priv->sourceview)
	{
		return FALSE;
	}

	width = gtk_widget_get_allocated_width (GTK_WIDGET (spacer));
	height = gtk_widget_get_allocated_height (GTK_WIDGET (spacer));

	context = gtk_widget_get_style_context (GTK_WIDGET (spacer));
	gtk_style_context_save (context);
	gtk_style_context_add_class (context, GTK_STYLE_CLASS_VIEW);
	gtk_render_background (context, cr, 0, 0, width, height);
	gtk_style_context_restore (context);

	cairo_set_line_width (cr, 1.0);

	if (priv->view_background_set)
	{
		gdk_cairo_set_source_rgba (cr, &container->priv->view_background);
		cairo_rectangle (cr, 0, 0, width, height);
		cairo_fill (cr);
	}

	if (priv->view_margin_background_set)
	{
		gdk_cairo_set_source_rgba (cr, &container->priv->view_margin_background);
		cairo_rectangle (cr, 0, 0, width, height);
		cairo_fill (cr);
	}

	if (priv->view_line_margin_fg_set)
	{
		gdk_cairo_set_source_rgba (cr, &container->priv->view_line_margin_fg);
		cairo_move_to (cr, width - 0.5, 0);
		cairo_line_to (cr, width - 0.5, height);
		cairo_stroke (cr);
	}

	return FALSE;
}

static void
gedit_view_centering_remove (GtkContainer *container,
			     GtkWidget    *child)
{
	GeditViewCenteringPrivate *priv;

	g_assert (GEDIT_IS_VIEW_CENTERING (container));

	priv = GEDIT_VIEW_CENTERING (container)->priv;

	if (priv->sourceview == child)
	{
		gtk_container_remove (GTK_CONTAINER (priv->scrolled_window), priv->sourceview);
		g_object_remove_weak_pointer (G_OBJECT (priv->sourceview), (gpointer *)&priv->sourceview);
		priv->sourceview = NULL;
		priv->view_context = NULL;
	}
	else
	{
		GTK_CONTAINER_CLASS (gedit_view_centering_parent_class)->remove (container, child);
	}
}

static void
gedit_view_centering_add (GtkContainer *container,
			  GtkWidget    *child)
{
	GeditViewCenteringPrivate *priv;
	GtkTextBuffer *buffer;
	GtkSourceStyleScheme *scheme;

	g_assert (GEDIT_IS_VIEW_CENTERING (container));

	priv = GEDIT_VIEW_CENTERING (container)->priv;

	if (GEDIT_IS_VIEW (child))
	{
		if (priv->sourceview)
		{
			gedit_view_centering_remove (container, priv->sourceview);
		}

		priv->sourceview = child;
		g_object_add_weak_pointer (G_OBJECT (child), (gpointer *)&priv->sourceview);
		gtk_container_add (GTK_CONTAINER (priv->scrolled_window), child);

		priv->view_context = gtk_widget_get_style_context (child);

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->sourceview));
		scheme = gtk_source_buffer_get_style_scheme (GTK_SOURCE_BUFFER (buffer));
		get_spacer_colors (GEDIT_VIEW_CENTERING (container), scheme);

		g_signal_connect (priv->sourceview,
		                  "notify::right-margin-position",
		                  G_CALLBACK (on_view_right_margin_position_changed),
		                  container);

		g_signal_connect (priv->sourceview,
		                  "notify::show-right-margin",
		                  G_CALLBACK (on_view_right_margin_visibility_changed),
		                  container);

		g_signal_connect (priv->view_context,
		                  "changed",
		                  G_CALLBACK (on_view_context_changed),
		                  container);

		gtk_widget_queue_resize (GTK_WIDGET (container));
	}
	else
	{
		GTK_CONTAINER_CLASS (gedit_view_centering_parent_class)->add (container, child);
	}
}

static gboolean
on_spacer_scroll_event (GtkWidget          *widget,
			GdkEvent           *event,
			GeditViewCentering *container)
{
	GdkEventScroll *new_scroll_event;

	new_scroll_event = (GdkEventScroll *)gdk_event_copy (event);
	g_object_unref (new_scroll_event->window);

	new_scroll_event->window = g_object_ref (gtk_widget_get_window (container->priv->sourceview));
	new_scroll_event->send_event = TRUE;

	new_scroll_event->x = 0;
	new_scroll_event->y = 0;
	new_scroll_event->x_root = 0;
	new_scroll_event->y_root = 0;

	gtk_main_do_event ((GdkEvent *)new_scroll_event);
	gdk_event_free ((GdkEvent *)new_scroll_event);

	return TRUE;
}

static void
gedit_view_centering_size_allocate (GtkWidget     *widget,
				    GtkAllocation *alloc)
{
	GeditViewCenteringPrivate *priv;
	GtkTextView *view;
	gint container_width;
	gint gutter_width;
	gint text_width;
	gint spacer_width;
	gint current_spacer_width;
	GdkWindow *gutter_window;

	g_assert (GEDIT_IS_VIEW_CENTERING (widget));

	priv = GEDIT_VIEW_CENTERING (widget)->priv;

	view = GTK_TEXT_VIEW (priv->sourceview);

	if (view)
	{
		container_width = alloc->width;

		gutter_window = gtk_text_view_get_window (view, GTK_TEXT_WINDOW_LEFT);
		gutter_width = (gutter_window) ? gdk_window_get_width (gutter_window) : 0;

		text_width = priv->view_text_width;
		spacer_width = MAX (0, container_width - text_width - gutter_width) / 2;

		g_object_get(priv->spacer, "width-request", &current_spacer_width, NULL);

		if (current_spacer_width != spacer_width)
		{
			g_object_set(priv->spacer, "width-request", spacer_width, NULL);
		}
	}

	GTK_WIDGET_CLASS (gedit_view_centering_parent_class)->size_allocate (widget, alloc);
}

static void
gedit_view_centering_finalize (GObject *object)
{
	GeditViewCentering *container = GEDIT_VIEW_CENTERING (object);
	GeditViewCenteringPrivate *priv = container->priv;

	if (priv->sourceview)
	{
		gedit_view_centering_remove (GTK_CONTAINER (container), priv->sourceview);
	}

	G_OBJECT_CLASS (gedit_view_centering_parent_class)->finalize (object);
}

static void
gedit_view_centering_class_init (GeditViewCenteringClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

	gobject_class->finalize = gedit_view_centering_finalize;

	widget_class->size_allocate = gedit_view_centering_size_allocate;

	container_class->add = gedit_view_centering_add;
	container_class->remove = gedit_view_centering_remove;
}

static void
gedit_view_centering_init (GeditViewCentering *container)
{
	GeditViewCenteringPrivate *priv;

	container->priv = gedit_view_centering_get_instance_private (container);
	priv = container->priv;
	priv->view_text_width = 0;

	priv->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	priv->spacer = gtk_drawing_area_new ();
	priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);

	gtk_container_add (GTK_CONTAINER (container), priv->box);
	gtk_box_pack_start (GTK_BOX (priv->box), priv->spacer, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->box), priv->scrolled_window, TRUE, TRUE, 0);

	gtk_widget_set_no_show_all (GTK_WIDGET (priv->spacer), TRUE);
	gtk_widget_show_all (GTK_WIDGET (priv->box));

	g_signal_connect_swapped (priv->spacer, "draw",
	                          G_CALLBACK (on_spacer_draw),
	                          container);

	gtk_widget_add_events(GTK_WIDGET(priv->spacer), GDK_SCROLL_MASK);
	g_signal_connect (priv->spacer, "scroll-event",
	                  G_CALLBACK (on_spacer_scroll_event),
	                  container);
}

/**
 * gedit_view_centering_set_centered:
 * @container: a #GeditViewCentering.
 * @centered: whether to center the sourceview child or not.
 *
 * If @centered is %TRUE, the sourceview child is centered
 * horizontally on the #GeditViewCentering container.
 **/
void
gedit_view_centering_set_centered (GeditViewCentering *container,
				   gboolean            centered)
{
	g_return_if_fail (GEDIT_IS_VIEW_CENTERING (container));

	container->priv->centered = centered != FALSE;

	on_view_right_margin_visibility_changed (GEDIT_VIEW (container->priv->sourceview), NULL, container);
}

/**
 * gedit_view_centering_get_centered:
 * @container: a #GeditViewCentering.
 *
 * Return whether the #GtkSourceView child is centered or not.
 *
 * Return value: %TRUE if the #GtkSourceView child is centered
 * horizontally on the #GeditViewCentering container.
 **/
gboolean
gedit_view_centering_get_centered (GeditViewCentering *container)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_CENTERING (container), FALSE);

	return container->priv->centered;
}

GeditViewCentering *
gedit_view_centering_new (void)
{
	return g_object_new (GEDIT_TYPE_VIEW_CENTERING,
	                     NULL);
}

/* ex:set ts=8 noet: */
