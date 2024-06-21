# -*- coding: UTF-8 -*-
#    Gedit External Tools plugin
#    Copyright (C) 2005-2006  Steve Frécinaux <steve@istique.net>
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

__all__ = ('ExternalToolsPlugin', 'OutputPanel', 'Capture', 'UniqueById')

from gi.repository import GLib, Gio, GObject, Gtk, Gedit, Tepl
from .library import ToolLibrary
from .outputpanel import OutputPanel
from .capture import Capture
from .functions import *

try:
    import gettext
    gettext.bindtextdomain('gedit')
    gettext.textdomain('gedit')
    _ = gettext.gettext
except:
    _ = lambda s: s

class ToolActions(object):
    def __init__(self, library, window, panel):
        super(ToolActions, self).__init__()
        self._library = library
        self._window = window
        self._panel = panel
        self._action_tools = {}

        self.update()

    def deactivate(self):
        self.remove()

    def remove(self):
        for name, tool in self._action_tools.items():
            self._window.remove_action(name)
        self._action_tools = {}

    def _insert_directory(self, directory):
        for tool in sorted(directory.tools, key=lambda x: x.name.lower()):
            # FIXME: find a better way to share the action name
            action_name = 'external-tool-%X-%X' % (id(tool), id(tool.name))
            self._action_tools[action_name] = tool

            action = Gio.SimpleAction(name=action_name)
            action.connect('activate', capture_menu_action, self._window, self._panel, tool)
            self._window.add_action(action)

    def update(self):
        self.remove()
        self._insert_directory(self._library.tree)
        self.filter(self._window.get_active_document())

    def filter_language(self, language, item):
        if not item.languages:
            return True

        if not language and 'plain' in item.languages:
            return True

        if language and (language.get_id() in item.languages):
            return True
        else:
            return False

    def filter(self, document):
        if document is None:
            titled = False
            remote = False
            language = None
        else:
            titled = document.get_file().get_location() is not None
            remote = not document.get_file().is_local()
            language = document.get_language()

        states = {
            'always': True,
            'all': document is not None,
            'local': titled and not remote,
            'remote': titled and remote,
            'titled': titled,
            'untitled': not titled,
        }

        for name, tool in self._action_tools.items():
            action = self._window.lookup_action(name)
            if action:
                action.set_enabled(states[tool.applicability] and
                                   self.filter_language(language, tool))


class WindowActivatable(GObject.Object, Gedit.WindowActivatable):
    __gtype_name__ = "ExternalToolsWindowActivatable"

    window = GObject.Property(type=Gedit.Window)

    def __init__(self):
        GObject.Object.__init__(self)
        self.actions = None

    def do_activate(self):
        self.window.external_tools_window_activatable = self

        self._library = ToolLibrary()

        # Create output console
        self._output_buffer = OutputPanel(self.plugin_info.get_data_dir(), self.window)

        self.actions = ToolActions(self._library, self.window, self._output_buffer)

        self.panel_item = Tepl.PanelItem.new(self._output_buffer.panel,
            "GeditExternalToolsShellOutput", _("Tool Output"), None, 0)
        bottom = self.window.get_bottom_panel()
        bottom.add(self.panel_item)

    def do_update_state(self):
        if self.actions is not None:
            self.actions.filter(self.window.get_active_document())

    def do_deactivate(self):
        self.actions.deactivate()
        bottom = self.window.get_bottom_panel()
        bottom.remove(self.panel_item)
        self.panel_item = None
        self.window.external_tools_window_activatable = None

    def update_actions(self):
        self.actions.update()

# ex:ts=4:et:
