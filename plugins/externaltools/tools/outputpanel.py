# -*- coding: utf-8 -*-
#    Gedit External Tools plugin
#    Copyright (C) 2005-2006  Steve Frécinaux <steve@istique.net>
#    Copyright (C) 2010  Per Arneng <per.arneng@anyplanet.com>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

__all__ = ('OutputPanel', 'UniqueById')

import os
from weakref import WeakKeyDictionary
from .capture import *
import re
from . import linkparsing
from . import filelookup
from gi.repository import GLib, Gio, Gdk, Gtk, Pango, Gedit

try:
    import gettext
    gettext.bindtextdomain('gedit')
    gettext.textdomain('gedit')
    _ = gettext.gettext
except:
    _ = lambda s: s

class UniqueById:
    __shared_state = WeakKeyDictionary()

    def __init__(self, i):
        if i in self.__class__.__shared_state:
            self.__dict__ = self.__class__.__shared_state[i]
            return True
        else:
            self.__class__.__shared_state[i] = self.__dict__
            return False

    def states(self):
        return self.__class__.__shared_state


class OutputPanel(UniqueById):
    def __init__(self, datadir, window):
        if UniqueById.__init__(self, window):
            return

        callbacks = {
            'on_stop_clicked': self.on_stop_clicked,
            'on_view_visibility_notify_event': self.on_view_visibility_notify_event,
            'on_view_motion_notify_event': self.on_view_motion_notify_event
        }

        self.profile_settings = self.get_profile_settings()
        self.profile_settings.connect("changed", self.font_changed)
        self.system_settings = Gio.Settings.new("org.gnome.desktop.interface")
        self.system_settings.connect("changed::monospace-font-name", self.font_changed)

        self.window = window
        self.ui = Gtk.Builder()
        self.ui.add_from_file(os.path.join(datadir, 'ui', 'outputpanel.ui'))
        self.ui.connect_signals(callbacks)
        self['view'].connect('button-press-event', self.on_view_button_press_event)

        self.panel = self["output-panel"]
        self.font_changed()

        buffer = self['view'].get_buffer()

        self.normal_tag = buffer.create_tag('normal')

        self.error_tag = buffer.create_tag('error')
        self.error_tag.set_property('foreground', 'red')

        self.italic_tag = buffer.create_tag('italic')
        self.italic_tag.set_property('style', Pango.Style.OBLIQUE)

        self.bold_tag = buffer.create_tag('bold')
        self.bold_tag.set_property('weight', Pango.Weight.BOLD)

        self.invalid_link_tag = buffer.create_tag('invalid_link')

        self.link_tag = buffer.create_tag('link')
        self.link_tag.set_property('underline', Pango.Underline.SINGLE)

        self.link_cursor = Gdk.Cursor.new(Gdk.CursorType.HAND2)
        self.normal_cursor = Gdk.Cursor.new(Gdk.CursorType.XTERM)

        self.process = None

        self.links = []

        self.link_parser = linkparsing.LinkParser()
        self.file_lookup = filelookup.FileLookup(window)

    def get_profile_settings(self):
        #FIXME return either the gnome-terminal settings or the gedit one
        return Gio.Settings.new("org.gnome.gedit.plugins.externaltools")

    def font_changed(self, settings=None, key=None):
        if self.profile_settings.get_boolean("use-system-font"):
            font = self.system_settings.get_string("monospace-font-name")
        else:
            font = self.profile_settings.get_string("font")

        font_desc = Pango.font_description_from_string(font)

        self["view"].override_font(font_desc)

    def set_process(self, process):
        self.process = process

    def __getitem__(self, key):
        # Convenience function to get an object from its name
        return self.ui.get_object(key)

    def on_stop_clicked(self, widget, *args):
        if self.process is not None:
            self.write("\n" + _('Stopped.') + "\n",
                       self.italic_tag)
            self.process.stop(-1)

    def scroll_to_end(self):
        iter = self['view'].get_buffer().get_end_iter()
        self['view'].scroll_to_iter(iter, 0.0, False, 0.5, 0.5)
        return False  # don't requeue this handler

    def clear(self):
        self['view'].get_buffer().set_text("")
        self.links = []

    def write(self, text, tag=None):
        buffer = self['view'].get_buffer()

        end_iter = buffer.get_end_iter()
        insert = buffer.create_mark(None, end_iter, True)

        if tag is None:
            buffer.insert(end_iter, text)
        else:
            buffer.insert_with_tags(end_iter, text, tag)

        # find all links and apply the appropriate tag for them
        links = self.link_parser.parse(text)
        for lnk in links:
            insert_iter = buffer.get_iter_at_mark(insert)
            lnk.start = insert_iter.get_offset() + lnk.start
            lnk.end = insert_iter.get_offset() + lnk.end

            start_iter = buffer.get_iter_at_offset(lnk.start)
            end_iter = buffer.get_iter_at_offset(lnk.end)

            tag = None

            # if the link points to an existing file then it is a valid link
            if self.file_lookup.lookup(lnk.path) is not None:
                self.links.append(lnk)
                tag = self.link_tag
            else:
                tag = self.invalid_link_tag

            buffer.apply_tag(tag, start_iter, end_iter)

        buffer.delete_mark(insert)
        GLib.idle_add(self.scroll_to_end)

    def show(self):
        bottom_panel = self.window.get_bottom_panel()
        #bottom_panel.set_active() # TODO
        bottom_panel.show()

    def update_cursor_style(self, view, x, y):
        if self.get_link_at_location(view, x, y) is not None:
            cursor = self.link_cursor
        else:
            cursor = self.normal_cursor

        view.get_window(Gtk.TextWindowType.TEXT).set_cursor(cursor)

    def on_view_motion_notify_event(self, view, event):
        if event.window == view.get_window(Gtk.TextWindowType.TEXT):
            self.update_cursor_style(view, int(event.x), int(event.y))

        return False

    def on_view_visibility_notify_event(self, view, event):
        if event.window == view.get_window(Gtk.TextWindowType.TEXT):
            win, x, y, flags = event.window.get_pointer()
            self.update_cursor_style(view, x, y)

        return False

    def idle_grab_focus(self):
        self.window.get_active_view().grab_focus()
        return False

    def get_link_at_location(self, view, x, y):
        """
        Get the link under a specified x,y coordinate. If no link exists then
        None is returned.
        """

        # get the offset within the buffer from the x,y coordinates
        buff_x, buff_y = view.window_to_buffer_coords(Gtk.TextWindowType.TEXT, x, y)
        (over_text, iter_at_xy) = view.get_iter_at_location(buff_x, buff_y)
        if not over_text:
            return None
        offset = iter_at_xy.get_offset()

        # find the first link that contains the offset
        for lnk in self.links:
            if offset >= lnk.start and offset <= lnk.end:
                return lnk

        # no link was found at x,y
        return None

    def on_view_button_press_event(self, view, event):
        if event.button != 1 or event.type != Gdk.EventType.BUTTON_PRESS or \
           event.window != view.get_window(Gtk.TextWindowType.TEXT):
            return False

        link = self.get_link_at_location(view, int(event.x), int(event.y))
        if link is None:
            return False

        gfile = self.file_lookup.lookup(link.path)

        if gfile:
            Gedit.commands_load_location(self.window, gfile, None, link.line_nr, link.col_nr)
            GLib.idle_add(self.idle_grab_focus)

# ex:ts=4:et:
