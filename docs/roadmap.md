gedit roadmap
=============

This page contains the plans for major code changes we hope to get done in the
future.

See also the
[GtkSourceView and Tepl roadmap](https://wiki.gnome.org/Projects/GtkSourceView/RoadMap).

See the [NEWS file](../NEWS) for a detailed history.

See also the
[Tepl roadmap](https://gitlab.gnome.org/GNOME/tepl/blob/master/docs/roadmap.md).

Continue to make the gedit source code more re-usable
-----------------------------------------------------

Status: **in progress** (this is an ongoing effort)

Next steps:
- Use more features from the Tepl library, and develop Tepl alongside gedit.
  The goal is to reduce the amount of code in gedit, by having re-usable code
  in Tepl instead.

Links:
- https://wiki.gnome.org/Apps/Gedit/ReusableCode
- https://wiki.gnome.org/Projects/Tepl

Improve gedit on Windows
------------------------

Status: **in progress**

[gedit is now available on the Microsoft Store](https://www.microsoft.com/store/apps/9PL1J21XF0PT).
The integration with Windows is not perfect, but it works. It is planned to
improve gedit for Windows over time.

Replace search and replace dialog window by an horizontal bar below the text
----------------------------------------------------------------------------

Status: **todo**

To not hide the text.

Be able to quit the application with all documents saved, and restored on next start
------------------------------------------------------------------------------------

Status: **todo**

Even for unsaved and untitled files, be able to quit gedit, restart it later and
come back to the state before with all tabs restored.

Improve the workflow for printing to paper
------------------------------------------

Status: **todo**

Implement it like in Firefox, show first a preview of the file to print.

Handle problem with large files or files containing very long lines
-------------------------------------------------------------------

Status: **started in Tepl**

As a stopgap measure, prevent those files from being loaded in the first place,
show first an infobar with a warning message.

Longer-term solution: fix the performance problem in GTK for very long lines.

For very big file size (e.g. a 1GB log file or SQL dump), it's more complicated
because the whole file is loaded in memory. It needs another data structure
implementation for the GtkTextView API.

Use native file chooser dialog windows (GtkFileChooserNative)
-------------------------------------------------------------

Status: **in progress**

To have the native file chooser on MS Windows, and use the Flatpak portal.

Do not allow incompatible plugins to be loaded
----------------------------------------------

Status: **todo**

There are currently no checks to see if a plugin is compatible with the gedit
version. Currently enabling a plugin can make gedit to crash.

Solution: include the gedit plugin API version in the directory names where
plugins need to be installed. Better solution: see
[this libpeas feature request](https://bugzilla.gnome.org/show_bug.cgi?id=642694#c15).

Making gedit suitable on a smartphone
-------------------------------------

Status: **in progress**

gedit is installed by default with the [Librem 5](https://puri.sm/products/librem-5/)
smartphone.

Avoid the need for gedit forks
------------------------------

Status: **todo**

There are several forks of gedit available: [Pluma](https://github.com/mate-desktop/pluma)
(from the MATE desktop environment) and [xed](https://github.com/linuxmint/xed)
(from the Linux Mint distribution). xed is a fork of Pluma, and Pluma is a fork
of gedit.

The goal is to make gedit suitable for MATE and Linux Mint. This can be
implemented by adding a “gedit-classic” configuration option.

Better C language support
-------------------------

Status: **todo**

- Code completion with Clang.
- Align function parameters on the parenthesis (function definition /
  function call).
- Generate and insert GTK-Doc comment header for a function.
- Split/join lines of a C comment with `*` at beginning of each line, ditto when
  pressing Enter (insert `*` at the beginning of the new line).
