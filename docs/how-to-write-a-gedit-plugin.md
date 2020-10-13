How to write a gedit plugin
===========================

The [gedit-development-getting-started.md](gedit-development-getting-started.md)
documentation is a good start.

Programming language for a gedit plugin
---------------------------------------

The preferred language is the C language, that way the code can be easily
refactored to be included in a library.

Rust, C++ and Vala plugins are possible too since they are equivalent to C.

Python plugins are possible too.

### What if I prefer to write in Ruby/JS/Scheme/Perl/C#/modula-2/Oz/whatever…

While GObject allows you to use many other languages, using more than one
interpreter in the same process is not possible, not only because of bloat and
performance, but also because different interpreted languages cannot manage
garbage collections of GObjects at the same time: see
[this email](https://mail.gnome.org/archives/desktop-devel-list/2010-August/msg00036.html).

The gedit developers have chosen Python as the interpreted language.

API reference
-------------

Build gedit with `-D gtk_doc=true`, you can then browse the API reference in the
[Devhelp](https://wiki.gnome.org/Apps/Devhelp) application.

To know how to write a plugin, refer to the
[libpeas](https://wiki.gnome.org/Projects/Libpeas) documentation as well.

More documentation and tips
---------------------------

More documentation, for example a tutorial, would be useful. In the meantime,
the recommended thing to do is to see how core gedit plugins are implemented.

### Unofficial documentation and tutorials (may be outdated)

- [Writing plugins in Python](https://wiki.gnome.org/Apps/Gedit/PythonPluginHowTo) (a little outdated)
- [Writing plugins in Vala](https://wiki.gnome.org/Projects/Vala/Gedit3PluginSample) (maybe outdated)
