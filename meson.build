project(
  'gedit', 'c',
  version: '3.31.1',
  meson_version: '>=0.46.0',
  license: 'GPL2'
)

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')
python = import('python')

# Versioning
version = meson.project_version()
version_arr = version.split('.')
version_major = version_arr[0]
version_minor = version_arr[1]
version_micro = version_arr[2]

api_version = '3.14'

package_name = meson.project_name()
package_string = '@0@-@1@'.format(package_name, api_version)

# Paths
rootdir = include_directories('.')

srcdir = meson.current_source_dir()
builddir = meson.current_build_dir()

prefix = get_option('prefix')

bindir = join_paths(prefix, get_option('bindir'))
includedir = join_paths(prefix, get_option('includedir'))
libdir = join_paths(prefix, get_option('libdir'))
libexecdir = join_paths(prefix, get_option('libexecdir'))
datadir = join_paths(prefix, get_option('datadir'))
pkgincludedir = join_paths(includedir, package_string)
pkgconfigdir = join_paths(libdir, 'pkgconfig')
pkglibdir = join_paths(libdir, package_name)
pkgdatadir = join_paths(datadir, package_name)

dbusdir = join_paths(datadir, 'dbus-1')
gconfdir = join_paths(datadir, 'GConf')
glibdir = join_paths(datadir, 'glib-2.0')
gtkdocdir = join_paths(datadir, 'gtk-doc')
localdir = join_paths(datadir, 'local')
vapidir = join_paths(datadir, 'vala', 'vapi')

# Subprojects
libgd_sp = subproject(
  'libgd',
  default_options: [
    'with-tagged-entry=true',
    'static=true',
  ]
)

# Dependencies
glib_req = '>= 2.44.0'
gtk_req = '>= 3.22.0'
gtksourceview_req = '>= 4.0.2'
libpeas_req = '>= 1.14.1'
libxml_req = '>= 2.5.0'
libsoup_req = '>= 2.60.0'
gspell_req = '>= 0.2.5'
pygobject_req = '>= 3.0.0'
introspection_req = '>= 0.9.3'
vala_req = '>= 0.25.1'
gtk_doc_req = '>= 1.0'

glib_dep = dependency('glib-2.0', version: glib_req)
gobject_dep = dependency('gobject-2.0', version: glib_req)
gmodule_dep = dependency('gmodule-2.0', version: glib_req)
gio_dep = dependency('gio-2.0', version: glib_req)
gdk_dep = dependency('gdk-3.0', version: gtk_req)
gtk_dep = dependency('gtk+-3.0', version: gtk_req)
gtksourceview_dep = dependency('gtksourceview-4', version: gtksourceview_req)
libpeas_dep = dependency('libpeas-1.0', version: libpeas_req)
libpeas_gtk_dep = dependency('libpeas-gtk-1.0', version: libpeas_req)
libxml_dep = dependency('libxml-2.0', version: libxml_req)

libgd_dep = libgd_sp.get_variable('libgd_dep')

libsoup_dep = dependency('libsoup-2.4', version: libsoup_req, required: true)
gspell_dep = dependency('gspell-1', version: gspell_req, required: true)
x11_dep = dependency('x11', required: false)

introspection_dep = dependency('gobject-introspection-1.0', version: introspection_req, required: false)
vapigen_dep = dependency('vapigen', version: vala_req, required: false)
gtk_doc_dep = dependency('gtk-doc', version: gtk_doc_req, required: false)

python3 = python.find_installation('python3')

# Configurations
windowing_targets = gdk_dep.get_pkgconfig_variable('targets').split()
if windowing_targets.contains('quartz')
  windowing_target = 'quartz'
elif windowing_targets.contains('win32')
  windowing_target = 'win32'
else
  if not x11_dep.found()
    error('Native dependency \'x11\' not found')
  else
    windowing_target = 'x11'
  endif
endif

config_h = configuration_data()
config_h.set_quoted('PACKAGE_NAME', package_name)
config_h.set_quoted('PACKAGE_STRING', package_string)
config_h.set_quoted('PACKAGE_VERSION', version)
config_h.set_quoted('GETTEXT_PACKAGE', package_name)
config_h.set_quoted('LOCALEDIR', localdir)
config_h.set_quoted('LIBDIR', libdir)
config_h.set_quoted('DATADIR', datadir)
config_h.set_quoted('VERSION', version)

config_h.set('GEDIT_MAJOR_VERSION', version_major)
config_h.set('GEDIT_MINOR_VERSION', version_minor)
config_h.set('GEDIT_MICRO_VERSION', version_micro)

configure_file(
  output: 'config.h',
  configuration: config_h
)

# Options
build_plugins = get_option('plugins')

generate_gir = get_option('gir')
if generate_gir and not introspection_dep.found()
  generate_gir = false
endif

generate_vapi = generate_gir and get_option('vapi')
if generate_vapi and not vapigen_dep.found()
  generate_vapi = false
endif

build_gtk_doc = get_option('gtk_doc')
if build_gtk_doc and not gtk_doc_dep.found()
  build_gtk_doc = false
endif

build_yelp_manual = get_option('yelp_manual')

active_plugins = [
  'docinfo',
  'modelines',
  'filebrowser',
  'spell',
  'time',
]

if windowing_target == 'win32'
  active_plugins += 'checkupdate'
endif

subdir('data')
subdir('po')

subdir('gedit')
if build_plugins == true
  subdir('plugins')
endif

subdir('docs')
if build_yelp_manual == true
  subdir('help')
endif

summary = [
  '',
  '------',
  'gedit @0@ (@1@)'.format(version, api_version),
  '',
  '  Documentation: @0@'.format(build_gtk_doc),
  '  Introspection: @0@'.format(generate_gir),
  '         Manual: @0@'.format(build_yelp_manual),
  '        Plugins: @0@'.format(build_plugins),
  '       Vala API: @0@'.format(generate_vapi),
  '',
  'Directories:',
  '         prefix: @0@'.format(prefix),
  '     includedir: @0@'.format(includedir),
  '         libdir: @0@'.format(libdir),
  '        datadir: @0@'.format(datadir),
  '',
  'Plugins:',
  '',
  '------',
]

meson.add_install_script(
  'build-aux/meson/post_install.py',
  libdir, datadir,
)

message('\n'.join(summary))
