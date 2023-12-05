gedit installation by building the source code
==============================================

Recommendation to install in a separate prefix
----------------------------------------------

Once you have built gedit from source, you cannot run the application from its
build directory: you need to install it with `ninja install`. For this reason it
is highly recommended that you install in a separate prefix instead of
overwriting your system binaries.

Note however that when running gedit from a custom prefix you will need to set
many environment variables accordingly, for instance `PATH` and `XDG_DATA_DIRS`.

There exists several tools that GNOME developers use to take care of all of
this. See the _Tools_ section below.

Installation of the dependencies
--------------------------------

You need to have all gedit dependencies installed, with recent enough versions.
If a dependency is missing or is too old, the build configuration fails (you can
try to run the build configuration command for gedit until it succeeds, see the
procedure below).

You can install the dependencies by installing packages provided by your
operating system, for example on Fedora:
```
# dnf builddep gedit
```

But if your version of gedit provided by the OS differs too much from the
version of gedit you want to build from source, you'll need to install the new
dependencies from source too, and it can become a complicated task if you do it
manually.

Also, during gedit development, gedit may depend on a not-yet-released
development version of a dependency. So certain dependencies may need to be
installed from Git.

That's why if you have difficulties installing recent enough versions of the
dependencies, it is usually easier to use one of the tools explained in the next
section.

Tools
-----

There are several tools available that take care of the following:
- Install in a separate prefix.
- Build or install dependencies.
- Plus, for some tools: run in a container/sandbox.

GNOME developers usually use one of these tools:
- [Toolbx](https://containertoolbx.org/) (probably the easiest)
- Or [Flatpak](https://flatpak.org/)
- Or [JHBuild](https://gitlab.gnome.org/GNOME/jhbuild)

General tips for gedit:
- At the time of writing, no need to build GLib or GTK from source, taking their
  latest stable versions are sufficient.

JHBuild tips:
- Try `ignore_suggests = True` in your jhbuildrc to have fewer dependencies to
  build (see the difference with "jhbuild list gedit"). Another solution is to
  put some modules in the skip variable in jhbuildrc.
- Note that the moduleset for gedit may be out-of-date.

Building the gedit module manually
----------------------------------

If you use one of the above tools, you don't need all the explanations in this
section. But it can be instructive.

gedit uses the [Meson](https://mesonbuild.com/) build system.

Once the dependencies are installed, here are simple procedures to finally build
the gedit module from source.

### Installation onto the system

**Warning**: this procedure doesn't install in a separate prefix, so it may
overwrite your system binaries.

```
$ cd build/
$ meson setup                # Build configuration
$ ninja                      # Build
[ Become root if necessary ]
$ ninja install              # Installation
```

### Installation in a separate prefix

Just change the above `meson setup` command by:
```
$ meson setup --prefix /an/other/path
```
