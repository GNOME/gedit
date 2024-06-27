# -*- coding: utf-8 -*-
#
#  __init__.py - Text size plugin
#
#  Copyright (C) 2008 - Konstantin Mikhaylov <jtraub.devel@gmail.com>
#  Copyright (C) 2009 - Wouter Bolsterlee <wbolster@gnome.org>
#  Copyright (C) 2010 - Ignacio Casal Quinteiro <icq@gnome.org>
#  Copyright (C) 2010 - Jesse van den Kieboom <jessevdk@gnome.org>
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
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor,
#  Boston, MA 02110-1301, USA.

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Gedit', '3.0')
from gi.repository import GObject, Gio, Gtk, Gdk, Gedit
from .viewactivatable import TextSizeViewActivatable

try:
    import gettext
    gettext.bindtextdomain('gedit-plugins')
    gettext.textdomain('gedit-plugins')
    _ = gettext.gettext
except:
    _ = lambda s: s


class TextSizeAppActivatable(GObject.Object, Gedit.AppActivatable):

    app = GObject.Property(type=Gedit.App)

    def __init__(self):
        GObject.Object.__init__(self)

    def do_activate(self):
        self.app.set_accels_for_action("win.text-larger", ["<Primary>plus", "<Primary>KP_Add"])
        self.app.set_accels_for_action("win.text-smaller", ["<Primary>minus", "<Primary>KP_Subtract"])
        self.app.set_accels_for_action("win.text-normal", ["<Primary>0", "<Primary>KP_0"])

        self.menu_ext = self.extend_menu("view-section-2")
        item = Gio.MenuItem.new(_("_Normal size"), "win.text-normal")
        self.menu_ext.prepend_menu_item(item)
        item = Gio.MenuItem.new(_("S_maller Text"), "win.text-smaller")
        self.menu_ext.prepend_menu_item(item)
        item = Gio.MenuItem.new(_("_Larger Text"), "win.text-larger")
        self.menu_ext.prepend_menu_item(item)

    def do_deactivate(self):
        self.app.set_accels_for_action("win.text-larger", [])
        self.app.set_accels_for_action("win.text-smaller", [])
        self.app.set_accels_for_action("win.text-normal", [])
        self.menu_ext = None


class TextSizeWindowActivatable(GObject.Object, Gedit.WindowActivatable):

    window = GObject.Property(type=Gedit.Window)

    def __init__(self):
        GObject.Object.__init__(self)

    def do_activate(self):
        action = Gio.SimpleAction(name="text-larger")
        action.connect('activate', self.on_larger_text_activate)
        self.window.add_action(action)

        action = Gio.SimpleAction(name="text-smaller")
        action.connect('activate', self.on_smaller_text_activate)
        self.window.add_action(action)

        action = Gio.SimpleAction(name="text-normal")
        action.connect('activate', self.on_normal_size_activate)
        self.window.add_action(action)

    def do_deactivate(self):
        self.window.remove_action("text-larger")
        self.window.remove_action("text-smaller")
        self.window.remove_action("text-normal")

    def do_update_state(self):
        self.window.lookup_action("text-larger").set_enabled(self.window.get_active_document() != None)
        self.window.lookup_action("text-smaller").set_enabled(self.window.get_active_document() != None)
        self.window.lookup_action("text-normal").set_enabled(self.window.get_active_document() != None)

    def get_view_activatable(self, view):
        if not hasattr(view, "textsize_view_activatable"):
            return None
        return view.textsize_view_activatable

    def call_view_activatable(self, cb):
        view = self.window.get_active_view()

        if view:
            cb(self.get_view_activatable(view))

    # Menu activate handlers
    def on_larger_text_activate(self, action, parameter, user_data=None):
        self.call_view_activatable(lambda va: va.larger_text())

    def on_smaller_text_activate(self, action, parameter, user_data=None):
        self.call_view_activatable(lambda va: va.smaller_text())

    def on_normal_size_activate(self, action, parameter, user_data=None):
        self.call_view_activatable(lambda va: va.normal_size())

# ex:ts=4:et:
