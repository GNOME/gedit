General Information
===================

This is version 3.31.2 of gedit. gedit is a small and lightweight UTF-8 text
editor for the GNOME environment.

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

Although new features are always under development, currently gedit has:

* Complete support for UTF-8 text
* Syntax highlighting
* Support for editing remote files
* Search and replace
* Printing and Print Previewing Support
* File Revert
* A complete preferences interface
* Configurable Plugin system, with optional python support


Some of the plugins, packaged and installed with gedit include, among others:

* Word count
* Spell checker
* File Browser
* Automatic snippet expansion
* Sort
* Insert Date/Time
* External Tools
* Tag list

Other external plugins are also available.


gedit is released under the GNU General Public License (GPL) version 2 or
later, see the file 'COPYING' for more information.


The official web site is:

    https://wiki.gnome.org/Apps/Gedit

You can download the latest gedit tarball from:

    https://download.gnome.org/sources/gedit/

Information about gedit mailing lists can be found at

    https://mail.gnome.org/mailman/listinfo/gedit-list


Installation
============

gedit requires `GTK+ >= 3.22.0`, `GtkSourceView >= 3.22.0` and
`libpeas >= 1.14.1` libraries. It also has a run-time dependency on an
icon theme for its icons. If gedit fails to display icons, installing
GNOME's default adwaita-icon-theme is a simple way of providing them.

`PyGObject 3.0.x` and `gobject-introspection 0.9.0` are required to enable
python plugins support.

`gspell >= 0.2.3` is required to enable the spell checking plugin.

Simple install procedure:

```
% tar Jxf gedit-3.31.2.tar.xz			# unpack the sources
% cd gedit-3.31.2				# change to the toplevel directory
% ./configure					# run the `configure' script
% make					# build gedit
[ Become root if necessary ]
% make install				# install gedit
```

See the file 'INSTALL' for more detailed information.


How to report bugs
==================

Bugs should be reported to the [GNOME bug tracking system](https://wiki.gnome.org/Apps/Gedit/ReportingBugs).


Patches
=======

Patches should also be submitted to bugzilla.gnome.org. If the patch
fixes an existing bug, add the patch as an attachment to that bug
report.

Otherwise, enter a new bug report that describes the patch, and attach
the patch to that bug report.

Please create patches with the git format-patch command.

If you are interested in helping us to develop gedit, please see the 
file 'AUTHOR' for contact information and/or send a message to the gedit
mailing list. See also the file 'HACKING' for more detailed information.


  The gedit team.
