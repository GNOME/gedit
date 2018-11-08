# gedit

gedit is the [GNOME](https://www.gnome.org) text editor.  
While aiming at simplicity and ease of use, gedit is a powerful general purpose text editor.

## Helpful Links

 * [Read the documention](https://help.gnome.org/users/gedit/stable/)
 * [File a Bug](https://gitlab.gnome.org/GNOME/gedit/issues)
 * [Download a Release Tarball](https://download.gnome.org/sources/gedit/)
 * [Browse source code in Git version control](https://gitlab.gnome.org/GNOME/gedit)
 * [Learn more about gedit](https://wiki.gnome.org/Apps/gedit)
 * [Learn more about writing gedit plugins](https://wiki.gnome.org/Apps/gedit/PluginsHowTos)
 * [Chat with the developers](irc://irc.gnome.org/#gedit) in #gedit on irc.gnome.org

----

gedit is part of GNOME and uses the latest GTK+ and GNOME libraries.
Complete GNOME integration is featured, with support for Drag and Drop (DnD) 
from Nautilus (the GNOME file manager), the use of the GNOME help system,
the Virtual File System GVfs and the GTK+ print framework.

gedit uses a Multiple Document Interface (MDI), which lets you edit more than 
one document at the same time.

gedit supports most standard editing features, plus several not found in your 
average text editor (plugins being the most notable of these).

The gedit core is written in the C language. Thanks to GObject introspection,
plugins may also be written in other languages, like Vala and Python.

## Features
 * Full support for internationalized text (UTF-8)
 * Configurable syntax highlighting for various languages (C, C++, Java, HTML, XML,  * Python, Perl and many others)
 * Undo/Redo
 * Editing files from remote locations
 * File reverting
 * Print and print preview support
 * Clipboard support (cut/copy/paste)
 * Search and replace with support of regular expressions
 * Go to specific line
 * Auto indentation
 * Text wrapping
 * Line numbers
 * Right margin
 * Current line highlighting
 * Bracket matching
 * Backup files
 * Configurable fonts and colors
 * A complete online user manual
 * A flexible plugin system which can be used to dynamically add new advanced features

## Plugins

Some of the plugins, packaged and installed with gedit include, among others:

 * Word count
 * Spell checker
 * File Browser
 * Automatic snippet expansion
 * Sort
 * Insert Date/Time
 * External Tools
 * Tag list

Other [external plugins](https://wiki.gnome.org/Apps/Gedit/PluginsLists) are also available, or you can [write your own](https://wiki.gnome.org/Apps/gedit/PluginsHowTos).

## Installation

gedit is developed in conjunction with GNOME releases.
This means that we often contribute to, and rely on, features being developed in other GNOME modules such as Gtk.

### Dependencies

 * libxml-2.0 >= 2.5.0
 * glib-2.0 >= 2.44
 * gio-2.0 >= 2.44
 * gmodule-2.0
 * gtk+-3.0 >= 3.22.0
 * gtksourceview-4 >= 4.0.2
 * libpeas-1.0 >= 1.14.1
 * libpeas-gtk-1.0 >= 1.14.1
 * gsettings-desktop-schemas

#### Optional
 * gspell >= 0.2.5 *(to enable spell checking)*
 * PyGObject >= 3.0.x *(to enable Python plugin support)*
 * gobject-introspection >= 0.9.0 *(to enable Python plugin support)*

*It also has a run-time dependency on an
icon theme for its icons. If gedit fails to display icons, installing
GNOME's default adwaita-icon-theme is a simple way of providing them.*

### Instructions

Download the latest release tarball [here](http://ftp.gnome.org/pub/GNOME/sources/gedit/).

```
  % tar -Jxf gedit-3.x.x.tar.xz	# unpack the sources
  % cd gedit-3.x.x				# change to the toplevel directory
  % ./configure					# run the 'configure' script
  % make					    # build gedit
  [ Become root if necessary ]
  % make install				# install gedit
```

----

gedit is licensed under the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your option)
any later version. Some files are individually licensed under alternative
licenses such as LGPL-2.1+ and LGPL-3.0.

