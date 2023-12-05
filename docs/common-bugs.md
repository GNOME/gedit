Common gedit bugs
=================

This page documents common bugs in gedit. If you find your problem in this page,
_please do not report a new bug for it_.

Problem with very long lines
----------------------------

Very long lines (e.g. a wrapped line that takes the whole screen) are not well
supported by gedit, there can be performance problems or freezes.

[GitLab issue](https://gitlab.gnome.org/GNOME/gedit/-/issues/513).

Problem with very large files
-----------------------------

Large files are not well supported, gedit should ask for confirmation when
opening such files, providing a solution.

[GitLab issue](https://gitlab.gnome.org/GNOME/gedit/-/issues/11).

Hyphen/dash inserted for text wrapping
--------------------------------------

It should be disabled in a text editor like gedit, the hyphen is not part of
the content.

[GitLab issue](https://gitlab.gnome.org/GNOME/gedit/issues/365).

Untranslated text (English sentences)
-------------------------------------

gedit relies on other modules for which translation contribution is not yet set
up, so in recent gedit versions you sometimes see English text mixed with
translated text.

We understand that this is important to fix, localization (l10n) is important to
reach a wider audience.

In the meantime, if you are comfortable in English, we recommend that you setup
your environment to use gedit in English. See the
[GNU gettext](https://www.gnu.org/software/gettext/) documentation.

Recently fixed
--------------

### View does not scroll to the end of the text in some cases (text cut off)

Fixed in 40.2, 41.1, 42.2 and >= 43.

[GitLab issue](https://gitlab.gnome.org/GNOME/gedit/issues/42).
