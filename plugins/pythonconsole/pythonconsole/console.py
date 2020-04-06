# -*- coding: utf-8 -*-

# pythonconsole.py -- Console widget
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

import string
import sys
import re
import traceback

from gi.repository import GLib, Gio, Gtk, Gdk, Pango

__all__ = ('PythonConsole', 'OutFile')

class PythonConsole(Gtk.ScrolledWindow):

    __gsignals__ = {
        'grab-focus' : 'override',
    }

    DEFAULT_FONT = "Monospace 10"

    CONSOLE_KEY_BASE = 'org.gnome.gedit.plugins.pythonconsole'
    SETTINGS_INTERFACE_DIR = "org.gnome.desktop.interface"
    SETTINGS_PROFILE_DIR = "org.gnome.GnomeTerminal.profiles.Default"

    CONSOLE_KEY_COMMAND_COLOR = 'command-color'
    CONSOLE_KEY_ERROR_COLOR = 'error-color'

    def __init__(self, namespace = {}):
        Gtk.ScrolledWindow.__init__(self)

        self._settings = Gio.Settings.new(self.CONSOLE_KEY_BASE)
        self._settings.connect("changed", self.on_color_settings_changed)

        self._interface_settings = Gio.Settings.new(self.SETTINGS_INTERFACE_DIR)
        self._interface_settings.connect("changed", self.on_settings_changed)

        self._profile_settings = self.get_profile_settings()
        self._profile_settings.connect("changed", self.on_settings_changed)

        self.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        self.set_shadow_type(Gtk.ShadowType.NONE)
        self.view = Gtk.TextView()
        self.reconfigure()
        self.view.set_editable(True)
        self.view.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)
        self.add(self.view)
        self.view.show()

        buf = self.view.get_buffer()
        self.normal = buf.create_tag("normal")
        self.error  = buf.create_tag("error")
        self.command = buf.create_tag("command")

        # Load the default settings
        self.on_color_settings_changed(self._settings, None)

        self.__spaces_pattern = re.compile(r'^\s+')
        self.namespace = namespace

        self.block_command = False

        # Init first line
        buf.create_mark("input-line", buf.get_end_iter(), True)
        buf.insert(buf.get_end_iter(), ">>> ")
        buf.create_mark("input", buf.get_end_iter(), True)

        # Init history
        self.history = ['']
        self.history_pos = 0
        self.current_command = ''
        self.namespace['__history__'] = self.history

        # Set up hooks for standard output.
        self.stdout = OutFile(self, sys.stdout.fileno(), self.normal)
        self.stderr = OutFile(self, sys.stderr.fileno(), self.error)

        # Signals
        self.view.connect("key-press-event", self.__key_press_event_cb)
        buf.connect("mark-set", self.__mark_set_cb)

    def get_profile_settings(self):
        #FIXME return either the gnome-terminal settings or the gedit one
        return Gio.Settings.new(self.CONSOLE_KEY_BASE)

    def do_grab_focus(self):
        self.view.grab_focus()

    def reconfigure(self):
        # Font
        font_desc = None
        system_font = self._interface_settings.get_string("monospace-font-name")

        if self._profile_settings.get_boolean("use-system-font"):
            font_name = system_font
        else:
            font_name = self._profile_settings.get_string("font")

        try:
            font_desc = Pango.FontDescription(font_name)
        except:
            if font_name != self.DEFAULT_FONT:
                if font_name != system_font:
                    try:
                        font_desc = Pango.FontDescription(system_font)
                    except:
                        pass

                if font_desc is None:
                    try:
                        font_desc = Pango.FontDescription(self.DEFAULT_FONT)
                    except:
                        pass

        if font_desc != None:
            self.view.modify_font(font_desc)

    def on_settings_changed(self, settings, key):
        self.reconfigure()

    def on_color_settings_changed(self, settings, key):
        self.error.set_property("foreground", settings.get_string(self.CONSOLE_KEY_ERROR_COLOR))
        self.command.set_property("foreground", settings.get_string(self.CONSOLE_KEY_COMMAND_COLOR))

    def stop(self):
        self.namespace = None

    def __key_press_event_cb(self, view, event):
        modifier_mask = Gtk.accelerator_get_default_mod_mask()
        event_state = event.state & modifier_mask

        if event.keyval == Gdk.KEY_D and event_state == Gdk.ModifierType.CONTROL_MASK:
            self.destroy()

        elif event.keyval == Gdk.KEY_Return and event_state == Gdk.ModifierType.CONTROL_MASK:
            # Get the command
            buf = view.get_buffer()
            inp_mark = buf.get_mark("input")
            inp = buf.get_iter_at_mark(inp_mark)
            cur = buf.get_end_iter()
            line = buf.get_text(inp, cur, False)
            self.current_command = self.current_command + line + "\n"
            self.history_add(line)

            # Prepare the new line
            cur = buf.get_end_iter()
            buf.insert(cur, "\n... ")
            cur = buf.get_end_iter()
            buf.move_mark(inp_mark, cur)

            # Keep indentation of precendent line
            spaces = re.match(self.__spaces_pattern, line)
            if spaces is not None:
                buf.insert(cur, line[spaces.start() : spaces.end()])
                cur = buf.get_end_iter()

            buf.place_cursor(cur)
            GLib.idle_add(self.scroll_to_end)
            return True

        elif event.keyval == Gdk.KEY_Return:
            # Get the marks
            buf = view.get_buffer()
            lin_mark = buf.get_mark("input-line")
            inp_mark = buf.get_mark("input")

            # Get the command line
            inp = buf.get_iter_at_mark(inp_mark)
            cur = buf.get_end_iter()
            line = buf.get_text(inp, cur, False)
            self.current_command = self.current_command + line + "\n"
            self.history_add(line)

            # Make the line blue
            lin = buf.get_iter_at_mark(lin_mark)
            buf.apply_tag(self.command, lin, cur)
            buf.insert(cur, "\n")

            cur_strip = self.current_command.rstrip()

            if cur_strip.endswith(":") \
            or (self.current_command[-2:] != "\n\n" and self.block_command):
                # Unfinished block command
                self.block_command = True
                com_mark = "... "
            elif cur_strip.endswith("\\"):
                com_mark = "... "
            else:
                # Eval the command
                self.__run(self.current_command)
                self.current_command = ''
                self.block_command = False
                com_mark = ">>> "

            # Prepare the new line
            cur = buf.get_end_iter()
            buf.move_mark(lin_mark, cur)
            buf.insert(cur, com_mark)
            cur = buf.get_end_iter()
            buf.move_mark(inp_mark, cur)
            buf.place_cursor(cur)
            GLib.idle_add(self.scroll_to_end)
            return True

        elif event.keyval == Gdk.KEY_KP_Down or event.keyval == Gdk.KEY_Down:
            # Next entry from history
            view.stop_emission_by_name("key_press_event")
            self.history_down()
            GLib.idle_add(self.scroll_to_end)
            return True

        elif event.keyval == Gdk.KEY_KP_Up or event.keyval == Gdk.KEY_Up:
            # Previous entry from history
            view.stop_emission_by_name("key_press_event")
            self.history_up()
            GLib.idle_add(self.scroll_to_end)
            return True

        elif event.keyval == Gdk.KEY_KP_Left or event.keyval == Gdk.KEY_Left or \
             event.keyval == Gdk.KEY_BackSpace:
            buf = view.get_buffer()
            inp = buf.get_iter_at_mark(buf.get_mark("input"))
            cur = buf.get_iter_at_mark(buf.get_insert())
            if inp.compare(cur) == 0:
                if not event_state:
                    buf.place_cursor(inp)
                return True
            return False

        # For the console we enable smart/home end behavior incoditionally
        # since it is useful when editing python

        elif (event.keyval == Gdk.KEY_KP_Home or event.keyval == Gdk.KEY_Home) and \
             event_state == event_state & (Gdk.ModifierType.SHIFT_MASK|Gdk.ModifierType.CONTROL_MASK):
            # Go to the begin of the command instead of the begin of the line
            buf = view.get_buffer()
            it = buf.get_iter_at_mark(buf.get_mark("input"))
            ins = buf.get_iter_at_mark(buf.get_insert())

            while it.get_char().isspace():
                it.forward_char()

            if it.equal(ins):
                it = buf.get_iter_at_mark(buf.get_mark("input"))

            if event_state & Gdk.ModifierType.SHIFT_MASK:
                buf.move_mark_by_name("insert", it)
            else:
                buf.place_cursor(it)
            return True

        elif (event.keyval == Gdk.KEY_KP_End or event.keyval == Gdk.KEY_End) and \
             event_state == event_state & (Gdk.ModifierType.SHIFT_MASK|Gdk.ModifierType.CONTROL_MASK):

            buf = view.get_buffer()
            it = buf.get_end_iter()
            ins = buf.get_iter_at_mark(buf.get_insert())

            it.backward_char()

            while it.get_char().isspace():
                it.backward_char()

            it.forward_char()

            if it.equal(ins):
                it = buf.get_end_iter()

            if event_state & Gdk.ModifierType.SHIFT_MASK:
                buf.move_mark_by_name("insert", it)
            else:
                buf.place_cursor(it)
            return True

    def __mark_set_cb(self, buf, it, name):
        input = buf.get_iter_at_mark(buf.get_mark("input"))
        pos   = buf.get_iter_at_mark(buf.get_insert())
        self.view.set_editable(pos.compare(input) != -1)

    def get_command_line(self):
        buf = self.view.get_buffer()
        inp = buf.get_iter_at_mark(buf.get_mark("input"))
        cur = buf.get_end_iter()
        return buf.get_text(inp, cur, False)

    def set_command_line(self, command):
        buf = self.view.get_buffer()
        mark = buf.get_mark("input")
        inp = buf.get_iter_at_mark(mark)
        cur = buf.get_end_iter()
        buf.delete(inp, cur)
        buf.insert(inp, command)
        self.view.grab_focus()

    def history_add(self, line):
        if line.strip() != '':
            self.history_pos = len(self.history)
            self.history[self.history_pos - 1] = line
            self.history.append('')

    def history_up(self):
        if self.history_pos > 0:
            self.history[self.history_pos] = self.get_command_line()
            self.history_pos = self.history_pos - 1
            self.set_command_line(self.history[self.history_pos])

    def history_down(self):
        if self.history_pos < len(self.history) - 1:
            self.history[self.history_pos] = self.get_command_line()
            self.history_pos = self.history_pos + 1
            self.set_command_line(self.history[self.history_pos])

    def scroll_to_end(self):
        i = self.view.get_buffer().get_end_iter()
        self.view.scroll_to_iter(i, 0.0, False, 0.5, 0.5)
        return False

    def write(self, text, tag = None):
        buf = self.view.get_buffer()
        if tag is None:
            buf.insert(buf.get_end_iter(), text)
        else:
            buf.insert_with_tags(buf.get_end_iter(), text, tag)

        GLib.idle_add(self.scroll_to_end)

    def eval(self, command, display_command = False):
        buf = self.view.get_buffer()
        lin = buf.get_mark("input-line")
        buf.delete(buf.get_iter_at_mark(lin),
                   buf.get_end_iter())

        if isinstance(command, list) or isinstance(command, tuple):
            for c in command:
                if display_command:
                    self.write(">>> " + c + "\n", self.command)
                self.__run(c)
        else:
            if display_command:
                self.write(">>> " + c + "\n", self.command)
            self.__run(command)

        cur = buf.get_end_iter()
        buf.move_mark_by_name("input-line", cur)
        buf.insert(cur, ">>> ")
        cur = buf.get_end_iter()
        buf.move_mark_by_name("input", cur)
        self.view.scroll_to_iter(buf.get_end_iter(), 0.0, False, 0.5, 0.5)

    def __run(self, command):
        sys.stdout, self.stdout = self.stdout, sys.stdout
        sys.stderr, self.stderr = self.stderr, sys.stderr

        try:
            try:
                r = eval(command, self.namespace, self.namespace)
                if r is not None:
                    print(r)
            except SyntaxError:
                exec(command, self.namespace)
        except:
            if hasattr(sys, 'last_type') and sys.last_type == SystemExit:
                self.destroy()
            else:
                traceback.print_exc()

        sys.stdout, self.stdout = self.stdout, sys.stdout
        sys.stderr, self.stderr = self.stderr, sys.stderr

    def destroy(self):
        pass
        #gtk.ScrolledWindow.destroy(self)

class OutFile:
    """A fake output file object. It sends output to a TK test widget,
    and if asked for a file number, returns one set on instance creation"""
    def __init__(self, console, fn, tag):
        self.fn = fn
        self.console = console
        self.tag = tag
    def close(self):         pass
    def flush(self):         pass
    def fileno(self):        return self.fn
    def isatty(self):        return 0
    def read(self, a):       return ''
    def readline(self):      return ''
    def readlines(self):     return []
    def write(self, s):      self.console.write(s, self.tag)
    def writelines(self, l): self.console.write(l, self.tag)
    def seek(self, a):       raise IOError((29, 'Illegal seek'))
    def tell(self):          raise IOError((29, 'Illegal seek'))
    truncate = tell

# ex:et:ts=4:
