/* gedit-pango.h
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#ifndef GEDIT_PANGO_H
#define GEDIT_PANGO_H

#include <pango/pango.h>

G_BEGIN_DECLS

gchar *gedit_pango_font_description_to_css (const PangoFontDescription *font_desc);

G_END_DECLS

#endif /* GEDIT_PANGO_H */
