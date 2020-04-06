# Copyright (C) 2011  Jesse van den Kieboom <jesse@icecrew.nl>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

from gi.repository import GObject
import inspect

from ..overrides import override
from ..importer import modules

Gedit = modules['Gedit']._introspection_module
__all__ = []

class MessageBus(Gedit.MessageBus):
    def create(self, object_path, method, **kwargs):
        tp = self.lookup(object_path, method)

        if not tp.is_a(Gedit.Message.__gtype__):
            return None

        kwargs['object-path'] = object_path
        kwargs['method'] = method

        return GObject.new(tp, **kwargs)

    def send_sync(self, object_path, method, **kwargs):
        msg = self.create(object_path, method, **kwargs)
        self.send_message_sync(msg)

        return msg

    def send(self, object_path, method, **kwargs):
        msg = self.create(object_path, method, **kwargs)
        self.send_message(msg)

        return msg

MessageBus = override(MessageBus)
__all__.append('MessageBus')

class Message(Gedit.Message):
    def __getattribute__(self, name):
        try:
            return Gedit.Message.__getattribute__(self, name)
        except:
            return getattr(self.props, name)

Message = override(Message)
__all__.append('Message')


def get_trace_info(num_back_frames=0):
    frame = inspect.currentframe().f_back
    try:
        for i in range(num_back_frames):
            back_frame = frame.f_back
            if back_frame is None:
                break
            frame = back_frame

        filename = frame.f_code.co_filename

        # http://code.activestate.com/recipes/145297-grabbing-the-current-line-number-easily/
        lineno = frame.f_lineno

        func_name = frame.f_code.co_name
        try:
            # http://stackoverflow.com/questions/2203424/python-how-to-retrieve-class-information-from-a-frame-object
            cls_name = frame.f_locals["self"].__class__.__name__
        except:
            pass
        else:
            func_name = "%s.%s" % (cls_name, func_name)

        return (filename, lineno, func_name)
    finally:
        frame = None

orig_debug_plugin_message_func = Gedit.debug_plugin_message

@override(Gedit.debug_plugin_message)
def debug_plugin_message(format, *format_args):
    filename, lineno, func_name = get_trace_info(1)
    orig_debug_plugin_message_func(filename, lineno, func_name, format % format_args)
__all__.append(debug_plugin_message)

# vi:ex:ts=4:et
