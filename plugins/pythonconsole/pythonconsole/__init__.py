# -*- coding: utf-8 -*-

# __init__.py -- plugin object
#
# Copyright (C) 2006 - Steve Frécinaux
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.

# Parts from "Interactive Python-GTK Console" (stolen from epiphany's console.py)
#     Copyright (C), 1998 James Henstridge <james@daa.com.au>
#     Copyright (C), 2005 Adam Hooper <adamh@densi.com>
# Bits from gedit Python Console Plugin
#     Copyrignt (C), 2005 Raphaël Slinckx

import gi
gi.require_version('Gedit', '3.0')
gi.require_version('Peas', '1.0')
gi.require_version('PeasGtk', '1.0')
gi.require_version('Gtk', '3.0')

from gi.repository import GObject, Gtk, Gedit, Peas, PeasGtk, Tepl
from .console import PythonConsole
from .config import PythonConsoleConfigWidget

try:
    import gettext
    gettext.bindtextdomain('gedit')
    gettext.textdomain('gedit')
    _ = gettext.gettext
except:
    _ = lambda s: s

class PythonConsolePlugin(GObject.Object, Gedit.WindowActivatable, PeasGtk.Configurable):
    __gtype_name__ = "PythonConsolePlugin"

    window = GObject.Property(type=Gedit.Window)

    def __init__(self):
        GObject.Object.__init__(self)

    def do_activate(self):
        self._console = PythonConsole(namespace = {'__builtins__' : __builtins__,
                                                   'gedit' : Gedit,
                                                   'window' : self.window})
        self._console.eval('print("You can access the main window through ' \
                           '\'window\' :\\n%s" % window)', False)
        bottom = self.window.get_bottom_panel()
        self._console.show_all()
        self.panel_item = Tepl.PanelItem.new(self._console, "GeditPythonConsolePanel",
            _('Python Console'), None, 0)
        bottom.add(self.panel_item)

    def do_deactivate(self):
        self._console.stop()
        bottom = self.window.get_bottom_panel()
        bottom.remove(self.panel_item)
        self.panel_item = None

    def do_update_state(self):
        pass

    def do_create_configure_widget(self):
        config_widget = PythonConsoleConfigWidget(self.plugin_info.get_data_dir())

        return config_widget.configure_widget()

# ex:et:ts=4:
