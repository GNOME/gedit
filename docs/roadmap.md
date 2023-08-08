gedit roadmap
=============

This page contains the plans for major code changes we hope to get done in the
future.

Continue to make the gedit source code more re-usable
-----------------------------------------------------

Status: **in progress** (this is an ongoing effort)

Next steps:
- Use more features from the Tepl library, and develop Tepl alongside gedit.
  The goal is to reduce the amount of code in gedit, by having re-usable code
  in Tepl instead.

Replace search & replace dialog window by an horizontal bar
-----------------------------------------------------------

Status: **todo**

To not occlude the text.

Be able to quit the application with all documents saved, and restored on next start
------------------------------------------------------------------------------------

Status: **todo**

Even for unsaved and untitled files, be able to quit gedit, restart it later and
come back to the state before with all tabs restored.

Improve the workflow for printing to paper
------------------------------------------

Status: **todo**

Show first a preview of the file to print and do the configuration from there.

Handle problem with large files or files containing very long lines
-------------------------------------------------------------------

Status: **started in Tepl**

See the [common-bugs.md](common-bugs.md) file.

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

Avoid the need for gedit forks
------------------------------

Status: **todo**

There are several forks of gedit available: [Pluma](https://github.com/mate-desktop/pluma)
(from the MATE desktop environment) and [xed](https://github.com/linuxmint/xed)
(from the Linux Mint distribution). xed is a fork of Pluma, and Pluma is a fork
of gedit.

The goal is to make gedit suitable for MATE and Linux Mint. This can be
implemented by adding a “gedit-classic” configuration option. Or implement it
similarly to LibreOffice, to give the user a choice between several UI
paradigms.
