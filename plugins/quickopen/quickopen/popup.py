# -*- coding: utf-8 -*-

#  Copyright (C) 2009 - Jesse van den Kieboom
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>.

import os
import platform
import functools
import fnmatch

from gi.repository import GLib, Gio, GObject, Pango, Gtk, Gdk, Gedit
import xml.sax.saxutils
from .virtualdirs import VirtualDirectory

try:
    import gettext
    gettext.bindtextdomain('gedit')
    gettext.textdomain('gedit')
    _ = gettext.gettext
except:
    _ = lambda s: s

class Popup(Gtk.Dialog):
    __gtype_name__ = "QuickOpenPopup"

    def __init__(self, window, paths, handler):
        Gtk.Dialog.__init__(self,
                            title=_('Quick Open'),
                            transient_for=window,
                            modal=True,
                            destroy_with_parent=True)

        self.add_button(_("_Cancel"), Gtk.ResponseType.CANCEL)
        self._open_button = self.add_button(_("_Open"),
                                            Gtk.ResponseType.ACCEPT)

        self._handler = handler
        self._build_ui()

        self._size = (0, 0)
        self._dirs = []
        self._cache = {}
        self._theme = None
        self._cursor = None
        self._shift_start = None

        self._busy_cursor = Gdk.Cursor(Gdk.CursorType.WATCH)

        accel_group = Gtk.AccelGroup()
        accel_group.connect(Gdk.KEY_l,
                            Gdk.ModifierType.CONTROL_MASK,
                            0,
                            self.on_focus_entry)

        self.add_accel_group(accel_group)

        unique = []

        for path in paths:
            if not path.get_uri() in unique:
                self._dirs.append(path)
                unique.append(path.get_uri())

        self.connect('show', self.on_show)

    def get_final_size(self):
        return self._size

    def _build_ui(self):
        self.set_border_width(5)
        vbox = self.get_content_area()
        vbox.set_spacing(2)
        action_area = self.get_action_area()
        action_area.set_border_width(5)
        action_area.set_spacing(6)

        self._entry = Gtk.SearchEntry()
        self._entry.set_placeholder_text(_('Type to search…'))

        self._entry.connect('changed', self.on_changed)
        self._entry.connect('key-press-event', self.on_key_press_event)

        sw = Gtk.ScrolledWindow()
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        sw.set_shadow_type(Gtk.ShadowType.OUT)

        tv = Gtk.TreeView()
        tv.set_headers_visible(False)

        self._store = Gtk.ListStore(Gio.Icon,
                                    str,
                                    GObject.Object,
                                    Gio.FileType)
        tv.set_model(self._store)

        self._treeview = tv
        tv.connect('row-activated', self.on_row_activated)

        column = Gtk.TreeViewColumn()

        renderer = Gtk.CellRendererPixbuf()
        column.pack_start(renderer, False)
        column.add_attribute(renderer, "gicon", 0)

        renderer = Gtk.CellRendererText()
        column.pack_start(renderer, True)
        column.add_attribute(renderer, "markup", 1)

        column.set_cell_data_func(renderer, self.on_cell_data_cb, None)

        tv.append_column(column)
        sw.add(tv)

        selection = tv.get_selection()
        selection.connect('changed', self.on_selection_changed)
        selection.set_mode(Gtk.SelectionMode.MULTIPLE)

        vbox.pack_start(self._entry, False, False, 0)
        vbox.pack_start(sw, True, True, 0)

        lbl = Gtk.Label()
        lbl.set_halign(Gtk.Align.START)
        lbl.set_ellipsize(Pango.EllipsizeMode.MIDDLE)
        self._info_label = lbl

        vbox.pack_start(lbl, False, False, 0)

        # Initial selection
        self.on_selection_changed(tv.get_selection())
        vbox.show_all()

    def on_cell_data_cb(self, column, cell, model, piter, user_data):
        path = model.get_path(piter)

        if self._cursor and path == self._cursor.get_path():
            style = self._treeview.get_style()
            bg = style.bg[Gtk.StateType.PRELIGHT]

            cell.set_property('cell-background-gdk', bg)
            cell.set_property('style', Pango.Style.ITALIC)
        else:
            cell.set_property('cell-background-set', False)
            cell.set_property('style-set', False)

    def _is_text(self, entry):
        content_type = entry.get_content_type()

        if content_type is None or Gio.content_type_is_unknown(content_type):
            return True

        if platform.system() != 'Windows':
            if (Gio.content_type_is_a(content_type, 'text/plain') or
                Gio.content_type_equals(content_type, 'application/x-zerosize')):
                return True
        else:
            if Gio.content_type_is_a(content_type, 'text'):
                return True

            # This covers a rare case in which on Windows the PerceivedType
            # is not set to "text" but the Content Type is set to text/plain
            if Gio.content_type_get_mime_type(content_type) == 'text/plain':
                return True

        return False

    def _list_dir(self, gfile):
        entries = []

        try:
            ret = gfile.enumerate_children("standard::*",
                                           Gio.FileQueryInfoFlags.NONE,
                                           None)
        except GLib.Error as e:
            pass

        if isinstance(ret, Gio.FileEnumerator):
            while True:
                entry = ret.next_file(None)

                if not entry:
                    break

                if not entry.get_is_backup():
                    entries.append((gfile.get_child(entry.get_name()), entry))
        else:
            entries = ret

        children = []

        for entry in entries:
            file_type = entry[1].get_file_type()

            if file_type == Gio.FileType.REGULAR:
                if not self._is_text(entry[1]):
                    continue

            children.append((entry[0],
                             entry[1].get_name(),
                             file_type,
                             entry[1].get_icon()))

        return children

    def _compare_entries(self, a, b, lpart):
        if lpart in a:
            if lpart in b:
                if a.index(lpart) < b.index(lpart):
                    return -1
                elif a.index(lpart) > b.index(lpart):
                    return 1
                else:
                    return 0
            else:
                return -1
        elif lpart in b:
            return 1
        else:
            return 0

    def _match_glob(self, s, glob):
        if glob:
            glob += '*'

        return fnmatch.fnmatch(s, glob)

    def do_search_dir(self, parts, d):
        if not parts or not d:
            return []

        if d in self._cache:
            entries = self._cache[d]
        else:
            entries = self._list_dir(d)
            entries.sort(key=lambda x: x[1].lower())
            self._cache[d] = entries

        found = []
        newdirs = []

        lpart = parts[0].lower()

        for entry in entries:
            if not entry:
                continue

            lentry = entry[1].lower()

            if not lpart or lpart in lentry or self._match_glob(lentry, lpart):
                if entry[2] == Gio.FileType.DIRECTORY:
                    if len(parts) > 1:
                        newdirs.append(entry[0])
                    else:
                        found.append(entry)
                elif entry[2] == Gio.FileType.REGULAR and \
                        (not lpart or len(parts) == 1):
                    found.append(entry)

        found.sort(key=functools.cmp_to_key(lambda a, b: self._compare_entries(a[1].lower(), b[1].lower(), lpart)))

        if lpart == '..':
            newdirs.append(d.get_parent())

        for dd in newdirs:
            found.extend(self.do_search_dir(parts[1:], dd))

        return found

    def _replace_insensitive(self, s, find, rep):
        out = ''
        l = s.lower()
        find = find.lower()
        last = 0

        if len(find) == 0:
            return xml.sax.saxutils.escape(s)

        while True:
            m = l.find(find, last)

            if m == -1:
                break
            else:
                out += xml.sax.saxutils.escape(s[last:m]) + rep % (xml.sax.saxutils.escape(s[m:m + len(find)]),)
                last = m + len(find)

        return out + xml.sax.saxutils.escape(s[last:])

    def make_markup(self, parts, path):
        out = []

        for i in range(0, len(parts)):
            out.append(self._replace_insensitive(path[i], parts[i], "<b>%s</b>"))

        return os.sep.join(out)

    def _get_icon(self, f):
        query = f.query_info(Gio.FILE_ATTRIBUTE_STANDARD_ICON,
                             Gio.FileQueryInfoFlags.NONE,
                             None)

        if not query:
            return None
        else:
            return query.get_icon()

    def _make_parts(self, parent, child, pp):
        parts = []

        # We went from parent, to child, using pp
        idx = len(pp) - 1

        while idx >= 0:
            if pp[idx] == '..':
                parts.insert(0, '..')
            else:
                parts.insert(0, child.get_basename())
                child = child.get_parent()

            idx -= 1

        return parts

    def normalize_relative(self, parts):
        if not parts:
            return []

        out = self.normalize_relative(parts[:-1])

        if parts[-1] == '..':
            if not out or (out[-1] == '..') or len(out) == 1:
                out.append('..')
            else:
                del out[-1]
        else:
            out.append(parts[-1])

        return out

    def _append_to_store(self, item):
        uri = item[2].get_uri()

        if uri not in self._stored_items:
            self._store.append(item)
            self._stored_items.add(uri)

    def _clear_store(self):
        self._store.clear()
        self._stored_items = set()

    def _show_virtuals(self):
        for d in self._dirs:
            if isinstance(d, VirtualDirectory):
                for entry in d.enumerate_children("standard::*", 0, None):
                    self._append_to_store((entry[1].get_icon(),
                                          xml.sax.saxutils.escape(entry[1].get_name()),
                                          entry[0],
                                          entry[1].get_file_type()))

    def _set_busy(self, busy):
        if busy:
            self.get_window().set_cursor(self._busy_cursor)
        else:
            self.get_window().set_cursor(None)
        Gdk.flush()

    def _remove_cursor(self):
        if self._cursor:
            path = self._cursor.get_path()
            self._cursor = None

            self._store.row_changed(path, self._store.get_iter(path))

    def do_search(self):
        self._set_busy(True)
        self._remove_cursor()

        text = self._entry.get_text().strip()
        self._clear_store()

        if text == '':
            self._show_virtuals()
        else:
            parts = self.normalize_relative(text.split(os.sep))
            files = []

            for d in self._dirs:
                for entry in self.do_search_dir(parts, d):
                    pathparts = self._make_parts(d, entry[0], parts)
                    self._append_to_store((entry[3],
                                          self.make_markup(parts, pathparts),
                                          entry[0],
                                          entry[2]))

        piter = self._store.get_iter_first()
        if piter:
            path = self._store.get_path(piter)
            self._treeview.get_selection().select_path(path)

        self._set_busy(False)

    # FIXME: override doesn't work anymore for some reason, if we override
    # the widget is not realized
    def on_show(self, data=None):
        # Gtk.Window.do_show(self)

        self._entry.grab_focus()
        self._entry.set_text("")

        self.do_search()

    def on_changed(self, editable):
        self.do_search()
        self.on_selection_changed(self._treeview.get_selection())

    def _shift_extend(self, towhere):
        selection = self._treeview.get_selection()

        if not self._shift_start:
            model, rows = selection.get_selected_rows()
            start = rows[0]

            self._shift_start = Gtk.TreeRowReference.new(self._store, start)
        else:
            start = self._shift_start.get_path()

        selection.unselect_all()
        selection.select_range(start, towhere)

    def _select_index(self, idx, hasctrl, hasshift):
        path = (idx,)

        if not (hasctrl or hasshift):
            self._treeview.get_selection().unselect_all()

        if hasshift:
            self._shift_extend(path)
        else:
            self._shift_start = None

            if not hasctrl:
                self._treeview.get_selection().select_path(path)

        self._treeview.scroll_to_cell(path, None, True, 0.5, 0)
        self._remove_cursor()

        if hasctrl or hasshift:
            self._cursor = Gtk.TreeRowReference(self._store, path)

            piter = self._store.get_iter(path)
            self._store.row_changed(path, piter)

    def _move_selection(self, howmany, hasctrl, hasshift):
        num = self._store.iter_n_children(None)

        if num == 0:
            return True

        # Test for cursor
        path = None

        if self._cursor:
            path = self._cursor.get_path()
        else:
            model, rows = self._treeview.get_selection().get_selected_rows()

            if len(rows) == 1:
                path = rows[0]

        if not path:
            if howmany > 0:
                self._select_index(0, hasctrl, hasshift)
            else:
                self._select_index(num - 1, hasctrl, hasshift)
        else:
            idx = path.get_indices()[0]

            if idx + howmany < 0:
                self._select_index(0, hasctrl, hasshift)
            elif idx + howmany >= num:
                self._select_index(num - 1, hasctrl, hasshift)
            else:
                self._select_index(idx + howmany, hasctrl, hasshift)

        return True

    def _direct_file(self):
        uri = self._entry.get_text()
        gfile = Gio.file_new_for_uri(uri)

        if Gedit.utils_is_valid_location(gfile) or \
           (os.path.isabs(uri) and gfile.query_exists()):
            return gfile
        else:
            return None

    def _activate(self):
        model, rows = self._treeview.get_selection().get_selected_rows()
        ret = True

        for row in rows:
            s = model.get_iter(row)
            info = model.get(s, 2, 3)

            if info[1] != Gio.FileType.DIRECTORY:
                ret = ret and self._handler(info[0])
            else:
                text = self._entry.get_text()

                for i in range(len(text) - 1, -1, -1):
                    if text[i] == os.sep:
                        break

                self._entry.set_text(os.path.join(text[:i], os.path.basename(info[0].get_uri())) + os.sep)
                self._entry.set_position(-1)
                self._entry.grab_focus()
                return True

        if rows and ret:
            # We destroy the popup in an idle callback to work around a crash that happens with
            # GTK_IM_MODULE=xim.  See https://bugzilla.gnome.org/show_bug.cgi?id=737711 .
            GLib.idle_add(self.destroy)

        if not rows:
            gfile = self._direct_file()

            if gfile and self._handler(gfile):
                GLib.idle_add(self.destroy)
            else:
                ret = False
        else:
            ret = False

        return ret

    def toggle_cursor(self):
        if not self._cursor:
            return

        path = self._cursor.get_path()
        selection = self._treeview.get_selection()

        if selection.path_is_selected(path):
            selection.unselect_path(path)
        else:
            selection.select_path(path)

    def on_key_press_event(self, widget, event):
        move_mapping = {
            Gdk.KEY_Down: 1,
            Gdk.KEY_Up: -1,
            Gdk.KEY_Page_Down: 5,
            Gdk.KEY_Page_Up: -5
        }

        if event.keyval == Gdk.KEY_Escape:
            self.destroy()
            return True
        elif event.keyval in move_mapping:
            return self._move_selection(move_mapping[event.keyval], event.state & Gdk.ModifierType.CONTROL_MASK, event.state & Gdk.ModifierType.SHIFT_MASK)
        elif event.keyval in [Gdk.KEY_Return, Gdk.KEY_KP_Enter, Gdk.KEY_Tab, Gdk.KEY_ISO_Left_Tab]:
            return self._activate()
        elif event.keyval == Gdk.KEY_space and event.state & Gdk.ModifierType.CONTROL_MASK:
            self.toggle_cursor()

        return False

    def on_row_activated(self, view, path, column):
        self._activate()

    def do_response(self, response):
        if response != Gtk.ResponseType.ACCEPT or not self._activate():
            self.destroy()

    def do_configure_event(self, event):
        if self.get_realized():
            alloc = self.get_allocation()
            self._size = (alloc.width, alloc.height)

        return Gtk.Dialog.do_configure_event(self, event)

    def on_selection_changed(self, selection):
        model, rows = selection.get_selected_rows()

        gfile = None
        fname = None

        if not rows:
            gfile = self._direct_file()
        elif len(rows) == 1:
            gfile = model.get(model.get_iter(rows[0]), 2)[0]
        else:
            fname = ''

        if gfile:
            if gfile.is_native():
                fname = xml.sax.saxutils.escape(gfile.get_path())
            else:
                fname = xml.sax.saxutils.escape(gfile.get_uri())

        self._open_button.set_sensitive(fname is not None)
        self._info_label.set_markup(fname or '')

    def on_focus_entry(self, group, accel, keyval, modifier):
        self._entry.grab_focus()

# ex:ts=4:et:
