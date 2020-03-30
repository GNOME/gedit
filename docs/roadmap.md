gedit roadmap
=============

This page contains the plans for major code changes we hope to get done in the
future.

See also the
[GtkSourceView and Tepl roadmap](https://wiki.gnome.org/Projects/GtkSourceView/RoadMap).

See the [NEWS file](../NEWS) for a detailed history.

If you often contribute to gedit, feel free to add your plans here.

Making the gedit source code more re-usable
-------------------------------------------

**Status**: [in progress](https://wiki.gnome.org/Apps/Gedit/ReusableCode) (this
is an ongoing effort)

Recently done:
- gedit 3.36: start to use the [Tepl](https://wiki.gnome.org/Projects/Tepl)
  library.

Next steps:
- Use more features from the [Tepl](https://wiki.gnome.org/Projects/Tepl)
  library, and develop Tepl alongside gedit. The goal is to reduce the amount of
  code from the gedit core, by having re-usable code in Tepl instead.

Making gedit suitable on a smartphone
-------------------------------------

**Status**: [in progress](https://gitlab.gnome.org/GNOME/Initiatives/-/issues/13)

gedit is installed by default with the [Librem 5](https://puri.sm/products/librem-5/)
smartphone.

Replace search and replace dialog window by an horizontal bar below the text
----------------------------------------------------------------------------

**Status**: [todo](https://gitlab.gnome.org/GNOME/gedit/-/issues/288)

To not hide the text.

Changing character encoding and line ending type of opened files
----------------------------------------------------------------

**Status**: started in Tepl

To fully support GtkFileChooserNative and better sandboxing.

Note that the integrated file browser plugin needs access at least to the whole
home directory. But the work on this task (with the code in Tepl) would allow
better sandboxing for other text editors that don't have an integrated file
browser.

Handle problem with large files or files containing very long lines
-------------------------------------------------------------------

**Status**: started in Tepl

As a stopgap measure, prevent those files from being loaded in the first place,
show first an infobar with a warning message.

Longer-term solution: fix the performance problem in GTK for very long lines.

For very big file size (e.g. a 1GB log file or SQL dump), it's more complicated
because the whole file is loaded in memory. It needs another data structure
implementation for the GtkTextView API.

Do not allow incompatible plugins to be loaded
----------------------------------------------

**Status**: todo

There are currently no checks to see if a plugin is compatible with the gedit
version. Currently enabling a plugin can make gedit to crash.

Solution: include the gedit plugin API version in the directory names where
plugins need to be installed. Better solution: see
[this libpeas feature request](https://bugzilla.gnome.org/show_bug.cgi?id=642694#c15).

Be able to quit the application with all documents saved, and restored on next start
------------------------------------------------------------------------------------

**Status**: todo

Even for unsaved and untitled files, be able to quit gedit, restart it later and
come back to the state before with all tabs restored.

Better C language support
-------------------------

**Status**: todo

- Code completion with Clang.
- Align function parameters on the parenthesis (function definition /
  function call).
- Generate and insert GTK-Doc comment header for a function.
- Split/join lines of a C comment with `*` at beginning of each line, ditto when
  pressing Enter (insert `*` at the beginning of the new line).

Improve printing UI workflow
----------------------------

**Status**: todo

Implement it like in Firefox, show first a preview of the file to print.

Avoid the need for gedit forks
------------------------------

**Status**: todo

There are several forks of gedit available: [Pluma](https://github.com/mate-desktop/pluma)
(from the MATE desktop environment) and [xed](https://github.com/linuxmint/xed)
(from the Linux Mint distribution). xed is a fork of Pluma, and Pluma is a fork
of gedit.

The goal is to make gedit suitable for MATE and Linux Mint. This can be
implemented by adding a “gedit-classic” configuration option.

Windows and Mac OS X support
----------------------------

To release new versions. And adapt/port the code if needed. This is an ongoing
effort.

### Windows

**Status**: stalled

Credits: Ignacio Casal Quinteiro

### Mac OS X

**Status**: stalled

Credits: Jesse van den Kieboom
