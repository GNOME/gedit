gedit roadmap
=============

This page contains the plans for major code changes we hope to get done in the
future.

See also the
[GtkSourceView and Tepl roadmap](https://wiki.gnome.org/Projects/GtkSourceView/RoadMap).

See the [NEWS file](../NEWS) for a detailed history.

If you often contribute to gedit, feel free to add your plans here, and add your
name to the tasks that you are interested to solve.

Task structure
--------------

**Status**: todo/in progress/stalled/other text with more details

Description.

Credits/People interested to work on it: list of names. (Credits is for work
already started).

Make the gedit source code more re-usable
-----------------------------------------

**Status**: in progress

https://wiki.gnome.org/Apps/Gedit/ReusableCode

Recently done:
- gedit 3.35: start to use the [Tepl](https://wiki.gnome.org/Projects/Tepl)
  library.

Next steps:
- Use more features from the [Tepl](https://wiki.gnome.org/Projects/Tepl)
  library, and develop Tepl alongside gedit. The goal is to reduce the amount of
  code from the gedit core, by having re-usable code in Tepl instead.

Credits: Sébastien Wilmet

Changing character encoding and line ending type of opened files
----------------------------------------------------------------

**Status**: started in Tepl

To fully support GtkFileChooserNative and better sandboxing.

Note that the integrated file browser plugin needs access at least to the whole
home directory. But the work on this task (with the code in Tepl) would allow
better sandboxing for other text editors that don't have an integrated file
browser.

Credits: Sébastien Wilmet

Handle problem with large files or files containing very long lines
-------------------------------------------------------------------

**Status**: started in Tepl

As a stopgap measure, prevent those files from being loaded in the first place,
show an infobar with an error message.

Longer-term solution: fix the performance problem in GTK for very long lines.

For very big file size (e.g. a 1GB log file or SQL dump), it's more complicated
because the whole file is loaded in memory. It needs another data structure
implementation for the GtkTextView API.

Credits: Sébastien Wilmet

Do not allow incompatible plugins to be loaded
----------------------------------------------

**Status**: todo

There are currently no checks to see if a plugin is compatible with the gedit
version. Currently enabling a plugin can make gedit to crash.

Solution: include the gedit plugin API version in the directory names where
plugins need to be installed. Better solution: see
[this libpeas feature request](https://bugzilla.gnome.org/show_bug.cgi?id=642694#c15).

People interested to work on it: Sébastien Wilmet

Be able to quit the application with all documents saved, and restored on next start
------------------------------------------------------------------------------------

**Status**: todo

Like in Sublime Text. Even for unsaved and untitled files, be able to quit
gedit, restart it later and come back to the state before with all tabs
restored.

People interested to work on it: Sébastien Wilmet

Better C language support
-------------------------

**Status**: todo

Needed for more dog-fooding of gedit.

- Code completion with Clang.
- Align function parameters on the parenthesis (function definition /
  function call).
- Generate and insert GTK-Doc comment header for a function.
- Split/join lines of a C comment with `*` at beginning of each line, ditto when
  pressing Enter (insert `*` at the beginning of the new line).

People interested to work on it: Sébastien Wilmet

Improve printing UI workflow
----------------------------

**Status**: todo

Implement it like in Firefox, show first a preview of the file to print.

People interested to work on it: Sébastien Wilmet

Replace search and replace dialog window by an horizontal bar above or below the text
-------------------------------------------------------------------------------------

**Status**: todo

To not hide the text.

People interested to work on it: Sébastien Wilmet

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
