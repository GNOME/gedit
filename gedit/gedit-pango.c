/* gedit-pango.c
 *
 * This file is a copy of libdazzle dzl-pango.c
 *
 * Copyright (C) 2014-2017 Christian Hergert <christian@hergert.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define G_LOG_DOMAIN "gedit-pango"

#include "config.h"

#include <gdk/gdk.h>
#include <glib/gstdio.h>
#include <math.h>

#include "gedit-pango.h"

#define FONT_FAMILY  "font-family"
#define FONT_VARIANT "font-variant"
#define FONT_STRETCH "font-stretch"
#define FONT_WEIGHT  "font-weight"
#define FONT_SIZE    "font-size"

/**
 * gedit_pango_font_description_to_css:
 *
 * This function will generate CSS suitable for Gtk's CSS engine
 * based on the properties of the #PangoFontDescription.
 *
 * Returns: (transfer full): A newly allocated string containing the
 *    CSS describing the font description.
 */
gchar *
gedit_pango_font_description_to_css (const PangoFontDescription *font_desc)
{
  PangoFontMask mask;
  GString *str;

#define ADD_KEYVAL(key,fmt) \
  g_string_append(str,key":"fmt";")
#define ADD_KEYVAL_PRINTF(key,fmt,...) \
  g_string_append_printf(str,key":"fmt";", __VA_ARGS__)

  g_return_val_if_fail (font_desc, NULL);

  str = g_string_new (NULL);

  mask = pango_font_description_get_set_fields (font_desc);

  if ((mask & PANGO_FONT_MASK_FAMILY) != 0)
    {
      const gchar *family;

      family = pango_font_description_get_family (font_desc);
      ADD_KEYVAL_PRINTF (FONT_FAMILY, "\"%s\"", family);
    }

  if ((mask & PANGO_FONT_MASK_STYLE) != 0)
    {
      PangoVariant variant;

      variant = pango_font_description_get_variant (font_desc);

      switch (variant)
        {
        case PANGO_VARIANT_NORMAL:
          ADD_KEYVAL (FONT_VARIANT, "normal");
          break;

        case PANGO_VARIANT_SMALL_CAPS:
          ADD_KEYVAL (FONT_VARIANT, "small-caps");
          break;

        default:
          break;
        }
    }

  if ((mask & PANGO_FONT_MASK_WEIGHT))
    {
      gint weight;

      weight = pango_font_description_get_weight (font_desc);

      /*
       * WORKAROUND:
       *
       * font-weight with numbers does not appear to be working as expected
       * right now. So for the common (bold/normal), let's just use the string
       * and let gtk warn for the other values, which shouldn't really be
       * used for this.
       */

      switch (weight)
        {
        case PANGO_WEIGHT_SEMILIGHT:
          /*
           * 350 is not actually a valid css font-weight, so we will just round
           * up to 400.
           */
        case PANGO_WEIGHT_NORMAL:
          ADD_KEYVAL (FONT_WEIGHT, "normal");
          break;

        case PANGO_WEIGHT_BOLD:
          ADD_KEYVAL (FONT_WEIGHT, "bold");
          break;

        case PANGO_WEIGHT_THIN:
        case PANGO_WEIGHT_ULTRALIGHT:
        case PANGO_WEIGHT_LIGHT:
        case PANGO_WEIGHT_BOOK:
        case PANGO_WEIGHT_MEDIUM:
        case PANGO_WEIGHT_SEMIBOLD:
        case PANGO_WEIGHT_ULTRABOLD:
        case PANGO_WEIGHT_HEAVY:
        case PANGO_WEIGHT_ULTRAHEAVY:
        default:
          /* round to nearest hundred */
          weight = round (weight / 100.0) * 100;
          ADD_KEYVAL_PRINTF ("font-weight", "%d", weight);
          break;
        }
    }

#ifndef GDK_WINDOWING_QUARTZ
  /*
   * We seem to get "Condensed" for fonts on the Quartz backend,
   * which is rather annoying as it results in us always hitting
   * fallback (stretch) paths. So let's cheat and just disable
   * stretch support for now on Quartz.
   */
  if ((mask & PANGO_FONT_MASK_STRETCH))
    {
      switch (pango_font_description_get_stretch (font_desc))
        {
        case PANGO_STRETCH_ULTRA_CONDENSED:
          ADD_KEYVAL (FONT_STRETCH, "untra-condensed");
          break;

        case PANGO_STRETCH_EXTRA_CONDENSED:
          ADD_KEYVAL (FONT_STRETCH, "extra-condensed");
          break;

        case PANGO_STRETCH_CONDENSED:
          ADD_KEYVAL (FONT_STRETCH, "condensed");
          break;

        case PANGO_STRETCH_SEMI_CONDENSED:
          ADD_KEYVAL (FONT_STRETCH, "semi-condensed");
          break;

        case PANGO_STRETCH_NORMAL:
          ADD_KEYVAL (FONT_STRETCH, "normal");
          break;

        case PANGO_STRETCH_SEMI_EXPANDED:
          ADD_KEYVAL (FONT_STRETCH, "semi-expanded");
          break;

        case PANGO_STRETCH_EXPANDED:
          ADD_KEYVAL (FONT_STRETCH, "expanded");
          break;

        case PANGO_STRETCH_EXTRA_EXPANDED:
          ADD_KEYVAL (FONT_STRETCH, "extra-expanded");
          break;

        case PANGO_STRETCH_ULTRA_EXPANDED:
          ADD_KEYVAL (FONT_STRETCH, "untra-expanded");
          break;

        default:
          break;
        }
    }
#endif

  if ((mask & PANGO_FONT_MASK_SIZE))
    {
      gint font_size;

      font_size = pango_font_description_get_size (font_desc) / PANGO_SCALE;
      ADD_KEYVAL_PRINTF ("font-size", "%dpt", font_size);
    }

  return g_string_free (str, FALSE);

#undef ADD_KEYVAL
#undef ADD_KEYVAL_PRINTF
}
