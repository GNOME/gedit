# Contributing to GNOME Builder

We would love for you to contribute to GNOME Builder!

GNOME Builder is a new IDE focusing on the GNOME desktop environment. It is
primarily written in C, but allows for plugins in C, C++, Python or Vala.
Additional compiled languages can also be supported with some work.

Remember that the GNOME community is largely made up of part-time contributors
that do this for fun. Be respectful and assume the best of each other.

## Filing a bug

Please file bugs for issues, enhancements and features on
[our bug tracker](https://gitlab.gnome.org/GNOME/gnome-builder/issues).

Create a
[Merge Request](https://gitlab.gnome.org/GNOME/gnome-builder/merge_requests)
and we'd be happy to review your patch and help you get it merged.

## Asking for Help

You can often find Builder contributors on our IRC channel at
[irc://irc.gimp.net/#gnome-builder](irc://irc.gimp.net/#gnome-builder).
If you have any questions, we'd be happy to help you.

If you'd like to start on a new plugin or feature, stop by our IRC channel and we'd be happy get you oriented.

## Portability

Builder is primarily focused on **GNU/Linux** with the **GNOME desktop**.

However, we do often accept patches for various BSD-based operating systems and alternate desktops.
It is important that you help us by keep things working for your platform as the Builder team cannot test all possible configurations.

## Testing

We use meson for our build system.
It provides support for running unit tests with the `ninja test` command.
We run these tests often so it is important that you check nothing was broken by your changes.

## Licensing

Contributions should be licensed under the LGPL-2.1+ or GPL-3.
Permissively licensed contributions will also be accepted, but we prefer that original contributions are either LGPL-2.1+ or GPL-3 licensed.

## Coding Style

Our coding style matches that of Gtk+.
We consider the Gtk+ project an upstream, and often push features into Gtk+.
This might feel unfamiliar at first, but it works well for us.

We use the recent GNU GCC '11 mode, such as -std=gnu11.

```c
static GtkWidget *
example_object_get_widget (ExampleObject  *object,
                           GError        **error)
{
  g_return_val_if_fail (EXAMPLE_IS_OBJECT (object), NULL);

  if (object->widget == NULL)
    {
      object->widget = g_object_new (GTK_TYPE_LABEL,
                                     "visible", TRUE,
                                     NULL);
    }

  return object->widget;
}
```

You may omit curly-braces for single-line conditionals.

Please use new-style Object declarations such as `G_DECLARE_FINAL_TYPE()` or `G_DECLARE_DERIVABLE_TYPE()`.
Unless you intend to subclass the object, make the object final.

```c
#define EXAMPLE_TYPE_OBJECT (example_object_get_type())

G_DECLARE_FINAL_TYPE (ExampleObject, example_object, EXAMPLE, OBJECT, GObject)
```

Builder's default C mode matches our style guide.

### Be explicit about ownership transfers

Since `GLib 2.44`, we've had helpful macros and functions to be explit about ownership transfers.
Please use them as it drastically saves time when tracking down memory leaks.

These include:

 * `g_autoptr()`, `g_auto()`, and `g_autofree`.
 * `g_steal_pointer()`
 * `g_clear_object()` and `g_clear_pointer()`

We prefer that you zero fields in structures when freeing the contents.

## Documentation

Most functions should be obvious what they do from the object type, name, and parameters.
Add additional documentation when it makes sense.

If you find you come across something particularly tricky, or are being clever, please add a comment denoting such.

## Making a Release

 - Update subprojects like libdazzle/etc for recent releases
 - Update NEWS for release notes
 - Update meson.build for the new version number
 - Update doc/conf.py to reflect the updated version number
 - Update any necessary tags in org.gnome.Builder.json for Flatpak
 - Make sure documentation builds, tests pass
 - Make sure flatpak bundle builds
 - Commit release changes, add a signed tag (git tag -s -u $keyid)
 - Configure meson as normal, ensure docs are built
 - From the build directory, run `ninja dist` to generate the tarball
 - Push changes to master (or branch), push tag
 - scp the tarball to master.gnome.org
 - Run ftpadmin install `gnome-builder-*.tar.xz`.

