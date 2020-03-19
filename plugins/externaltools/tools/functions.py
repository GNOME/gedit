# -*- coding: utf-8 -*-
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

import os
from gi.repository import Gio, Gtk, Gdk, GtkSource, Gedit
from .capture import *

try:
    import gettext
    gettext.bindtextdomain('gedit')
    gettext.textdomain('gedit')
    _ = gettext.gettext
except:
    _ = lambda s: s

def default(val, d):
    if val is not None:
        return val
    else:
        return d


def current_word(document):
    piter = document.get_iter_at_mark(document.get_insert())
    start = piter.copy()

    if not piter.starts_word() and (piter.inside_word() or piter.ends_word()):
        start.backward_word_start()

    if not piter.ends_word() and piter.inside_word():
        piter.forward_word_end()

    return (start, piter)


def file_browser_root(window):
    bus = window.get_message_bus()

    if bus.is_registered('/plugins/filebrowser', 'get_root'):
        msg = bus.send_sync('/plugins/filebrowser', 'get_root')

        if msg:
            browser_root = msg.props.location

            if browser_root and browser_root.is_native():
                return browser_root.get_path()

    return None


# ==== Capture related functions ====
def run_external_tool(window, panel, node):
    # Configure capture environment
    try:
        cwd = os.getcwd()
    except OSError:
        cwd = os.getenv('HOME')

    capture = Capture(node.command, cwd)
    capture.env = os.environ.copy()
    capture.set_env(GEDIT_CWD=cwd)

    view = window.get_active_view()
    document = None

    if view is not None:
        # Environment vars relative to current document
        document = view.get_buffer()
        location = document.get_file().get_location()

        # Current line number
        piter = document.get_iter_at_mark(document.get_insert())
        capture.set_env(GEDIT_CURRENT_LINE_NUMBER=str(piter.get_line() + 1))

        # Current line text
        piter.set_line_offset(0)
        end = piter.copy()

        if not end.ends_line():
            end.forward_to_line_end()

        capture.set_env(GEDIT_CURRENT_LINE=piter.get_text(end))

        if document.get_language() is not None:
            capture.set_env(GEDIT_CURRENT_DOCUMENT_LANGUAGE=document.get_language().get_id())

        # Selected text (only if input is not selection)
        if node.input != 'selection' and node.input != 'selection-document':
            bounds = document.get_selection_bounds()

            if bounds:
                capture.set_env(GEDIT_SELECTED_TEXT=bounds[0].get_text(bounds[1]))

        bounds = current_word(document)
        capture.set_env(GEDIT_CURRENT_WORD=bounds[0].get_text(bounds[1]))

        capture.set_env(GEDIT_CURRENT_DOCUMENT_TYPE=document.get_mime_type())

        if location is not None:
            scheme = location.get_uri_scheme()
            name = location.get_basename()
            capture.set_env(GEDIT_CURRENT_DOCUMENT_URI=location.get_uri(),
                            GEDIT_CURRENT_DOCUMENT_NAME=name,
                            GEDIT_CURRENT_DOCUMENT_SCHEME=scheme)
            if location.has_uri_scheme('file'):
                path = location.get_path()
                cwd = os.path.dirname(path)
                capture.set_cwd(cwd)
                capture.set_env(GEDIT_CURRENT_DOCUMENT_PATH=path,
                                GEDIT_CURRENT_DOCUMENT_DIR=cwd)

        documents_location = [doc.get_file().get_location()
                              for doc in window.get_documents()
                              if doc.get_file().get_location() is not None]
        documents_uri = [location.get_uri()
                         for location in documents_location
                         if location.get_uri() is not None]
        documents_path = [location.get_path()
                          for location in documents_location
                          if location.has_uri_scheme('file')]
        capture.set_env(GEDIT_DOCUMENTS_URI=' '.join(documents_uri),
                        GEDIT_DOCUMENTS_PATH=' '.join(documents_path))

    # set file browser root env var if possible
    browser_root = file_browser_root(window)
    if browser_root:
        capture.set_env(GEDIT_FILE_BROWSER_ROOT=browser_root)

    flags = capture.CAPTURE_BOTH

    if not node.has_hash_bang():
        flags |= capture.CAPTURE_NEEDS_SHELL

    capture.set_flags(flags)

    # Get input text
    input_type = node.input
    output_type = node.output

    # Clear the panel
    panel.clear()

    if output_type == 'output-panel':
        panel.show()

    # Assign the error output to the output panel
    panel.set_process(capture)

    if input_type != 'nothing' and view is not None:
        if input_type == 'document':
            start, end = document.get_bounds()
        elif input_type == 'selection' or input_type == 'selection-document':
            try:
                start, end = document.get_selection_bounds()
            except ValueError:
                if input_type == 'selection-document':
                    start, end = document.get_bounds()

                    if output_type == 'replace-selection':
                        document.select_range(start, end)
                else:
                    start = document.get_iter_at_mark(document.get_insert())
                    end = start.copy()

        elif input_type == 'line':
            start = document.get_iter_at_mark(document.get_insert())
            end = start.copy()
            if not start.starts_line():
                start.set_line_offset(0)
            if not end.ends_line():
                end.forward_to_line_end()
        elif input_type == 'word':
            start = document.get_iter_at_mark(document.get_insert())
            end = start.copy()
            if not start.inside_word():
                panel.write(_('You must be inside a word to run this command'),
                            panel.error_tag)
                return
            if not start.starts_word():
                start.backward_word_start()
            if not end.ends_word():
                end.forward_word_end()

        input_text = document.get_text(start, end, False)
        capture.set_input(input_text)

    # Assign the standard output to the chosen "file"
    if output_type == 'new-document':
        tab = window.create_tab(True)
        view = tab.get_view()
        document = tab.get_document()
        pos = document.get_start_iter()
        capture.connect('stdout-line', capture_stdout_line_document, document, pos)
        document.begin_user_action()
        view.set_editable(False)
        view.set_cursor_visible(False)
    elif output_type != 'output-panel' and output_type != 'nothing' and view is not None:
        document.begin_user_action()
        view.set_editable(False)
        view.set_cursor_visible(False)

        if output_type.startswith('replace-'):
            if output_type == 'replace-selection':
                try:
                    start_iter, end_iter = document.get_selection_bounds()
                except ValueError:
                    start_iter = document.get_iter_at_mark(document.get_insert())
                    end_iter = start_iter.copy()
            elif output_type == 'replace-document':
                start_iter, end_iter = document.get_bounds()
            capture.connect('stdout-line', capture_delayed_replace,
                            document, start_iter, end_iter)
        else:
            if output_type == 'insert':
                pos = document.get_iter_at_mark(document.get_insert())
            else:
                pos = document.get_end_iter()
            capture.connect('stdout-line', capture_stdout_line_document, document, pos)
    elif output_type != 'nothing':
        capture.connect('stdout-line', capture_stdout_line_panel, panel)

        if not document is None:
            document.begin_user_action()

    capture.connect('stderr-line', capture_stderr_line_panel, panel)
    capture.connect('begin-execute', capture_begin_execute_panel, panel, view, node.name)
    capture.connect('end-execute', capture_end_execute_panel, panel, view, output_type)

    # Run the command
    capture.execute()

    if output_type != 'nothing':
        if not document is None:
            document.end_user_action()

class MultipleDocumentsSaver:
    def __init__(self, window, panel, all_docs, node):
        self._window = window
        self._panel = panel
        self._node = node

        if all_docs:
            docs = window.get_documents()
        else:
            docs = [window.get_active_document()]

        self._docs_to_save = [doc for doc in docs if doc.get_modified()]
        self.save_next_document()

    def save_next_document(self):
        if len(self._docs_to_save) == 0:
            # The documents are saved, we can run the tool.
            run_external_tool(self._window, self._panel, self._node)
        else:
            next_doc = self._docs_to_save[0]
            self._docs_to_save.remove(next_doc)

            Gedit.commands_save_document_async(next_doc,
                                               self._window,
                                               None,
                                               self.on_document_saved,
                                               None)

    def on_document_saved(self, doc, result, user_data):
        saved = Gedit.commands_save_document_finish(doc, result)
        if saved:
            self.save_next_document()


def capture_menu_action(action, parameter, window, panel, node):
    if node.save_files == 'document' and window.get_active_document():
        MultipleDocumentsSaver(window, panel, False, node)
        return
    elif node.save_files == 'all':
        MultipleDocumentsSaver(window, panel, True, node)
        return

    run_external_tool(window, panel, node)


def capture_stderr_line_panel(capture, line, panel):
    if not panel.visible():
        panel.show()

    panel.write(line, panel.error_tag)


def capture_begin_execute_panel(capture, panel, view, label):
    if view:
        view.get_window(Gtk.TextWindowType.TEXT).set_cursor(Gdk.Cursor.new(Gdk.CursorType.WATCH))

    panel['stop'].set_sensitive(True)
    panel.clear()
    panel.write(_("Running tool:"), panel.italic_tag)
    panel.write(" %s\n\n" % label, panel.bold_tag)


def capture_end_execute_panel(capture, exit_code, panel, view, output_type):
    panel['stop'].set_sensitive(False)

    if view:
        if output_type in ('new-document', 'replace-document'):
            doc = view.get_buffer()
            start = doc.get_start_iter()
            end = start.copy()
            end.forward_chars(300)
            uri = ''

            mtype, uncertain = Gio.content_type_guess(None, doc.get_text(start, end, False).encode('utf-8'))
            lmanager = GtkSource.LanguageManager.get_default()

            location = doc.get_file().get_location()
            if location:
                uri = location.get_uri()
            language = lmanager.guess_language(uri, mtype)

            if language is not None:
                doc.set_language(language)

        view.get_window(Gtk.TextWindowType.TEXT).set_cursor(Gdk.Cursor.new(Gdk.CursorType.XTERM))
        view.set_cursor_visible(True)
        view.set_editable(True)

    if exit_code == 0:
        panel.write("\n" + _("Done.") + "\n", panel.italic_tag)
    else:
        panel.write("\n" + _("Exited") + ":", panel.italic_tag)
        panel.write(" %d\n" % exit_code, panel.bold_tag)


def capture_stdout_line_panel(capture, line, panel):
    panel.write(line)


def capture_stdout_line_document(capture, line, document, pos):
    document.insert(pos, line)


def capture_delayed_replace(capture, line, document, start_iter, end_iter):
    document.delete(start_iter, end_iter)

    # Must be done after deleting the text
    pos = document.get_iter_at_mark(document.get_insert())

    capture_stdout_line_document(capture, line, document, pos)

    capture.disconnect_by_func(capture_delayed_replace)
    capture.connect('stdout-line', capture_stdout_line_document, document, pos)

# ex:ts=4:et:
