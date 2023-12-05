gedit roadmap
=============

This page contains the plans for major code changes we hope to get done in the
future.

Take care and fix the common bugs
---------------------------------

Status: **in progress**

See the list in the [common-bugs.md](common-bugs.md) file.

Continue to make the gedit source code more re-usable
-----------------------------------------------------

Status: **in progress** (this is an ongoing effort)

Develop and use the [Gedit Technology](https://gedit-technology.github.io/) set
of libraries.

Rework the search & replace UI
------------------------------

Status: **todo**

Have an horizontal bar, to not occlude the text.

Be able to quit the application with all documents saved, and restored on next start
------------------------------------------------------------------------------------

Status: **todo**

Even for unsaved and untitled files, be able to quit gedit, restart it later and
come back to the previous state with all tabs restored.

Improve the workflow for printing to paper
------------------------------------------

Status: **todo**

Show first a preview of the file to print and do the configuration from there.

Use native file chooser dialog windows (GtkFileChooserNative)
-------------------------------------------------------------

Status: **todo**

To have the native file chooser on MS Windows, and use the Flatpak portal.

This task requires to rework the file loading and saving code. To have a
different - but still user-friendly - way to choose the character encoding and
line ending type.

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
