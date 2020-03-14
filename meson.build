project(
  'gedit', 'c',
  version: '3.36.0',
  meson_version: '>= 0.49'
)

if host_machine.system() == 'darwin'
  add_languages('objc')
endif

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')
python = import('python')

api_version = '3.36'

package_string = '@0@-@1@'.format(meson.project_name(), api_version)

# Paths
root_include_dir = include_directories('.')

srcdir = meson.current_source_dir()

bindir = join_paths(get_option('prefix'), get_option('bindir'))
libdir = join_paths(get_option('prefix'), get_option('libdir'))
libexecdir = join_paths(get_option('prefix'), get_option('libexecdir'))
datadir = join_paths(get_option('prefix'), get_option('datadir'))
pkgconfigdir = join_paths(libdir, 'pkgconfig')
pkglibdir = join_paths(libdir, meson.project_name())
pkgdatadir = join_paths(datadir, meson.project_name())

glibdir = join_paths(datadir, 'glib-2.0')
vapidir = join_paths(datadir, 'vala', 'vapi')

# Subprojects
if not get_option('buildtype').contains('plain')
  run_command('git', '-C', meson.source_root(), 'submodule', 'update', '--init', '--recursive')
endif

libgd_subproject = subproject(
  'libgd',
  default_options: [
    'with-tagged-entry=true',
    'static=true',
  ]
)

libgd_dep = libgd_subproject.get_variable('libgd_dep')

# Dependencies
libpeas_req = '>= 1.14.1'
libxml_req = '>= 2.5.0'
libsoup_req = '>= 2.60.0'
gspell_req = '>= 1.0'
pygobject_req = '>= 3.0.0'
introspection_req = '>= 0.9.3'
vala_req = '>= 0.25.1'

gio_dep = dependency('gio-2.0', version: glib_req)
gtk_dep = dependency('gtk+-3.0', version: gtk_req)
gtksourceview_dep = dependency('gtksourceview-4', version: gtksourceview_req)
libpeas_gtk_dep = dependency('libpeas-gtk-1.0', version: libpeas_req)
libxml_dep = dependency('libxml-2.0', version: libxml_req)
gspell_dep = dependency('gspell-1', version: gspell_req)
x11_dep = dependency('x11', required: false)

introspection_dep = dependency('gobject-introspection-1.0', version: introspection_req)
vapigen_dep = dependency('vapigen', version: vala_req)

python3 = python.find_installation('python3')

build_checkupdate_plugin = host_machine.system() == 'windows'
if build_checkupdate_plugin
  libsoup_dep = dependency('libsoup-2.4', version: libsoup_req)
endif

# Configurations
config_h = configuration_data()
config_h.set_quoted('PACKAGE_STRING', package_string)
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())
config_h.set_quoted('LIBDIR', join_paths(get_option('prefix'), get_option('libdir')))
config_h.set_quoted('DATADIR', join_paths(get_option('prefix'), get_option('datadir')))
config_h.set_quoted('VERSION', meson.project_version())

enable_gvfs_metadata = get_option('enable-gvfs-metadata')
if enable_gvfs_metadata == 'yes' or (enable_gvfs_metadata == 'auto' and host_machine.system() == 'linux')
  enable_gvfs_metadata = true
else
  enable_gvfs_metadata = false
endif

if enable_gvfs_metadata
  config_h.set('ENABLE_GVFS_METADATA', 1)
endif

version_array = meson.project_version().split('.')
config_h.set('GEDIT_MAJOR_VERSION', version_array[0])
config_h.set('GEDIT_MINOR_VERSION', version_array[1])
config_h.set('GEDIT_MICRO_VERSION', version_array[2])

configure_file(
  output: 'config.h',
  configuration: config_h
)

active_plugins = [
  'docinfo',
  'modelines',
  'filebrowser',
  'spell',
  'time',
]

if build_checkupdate_plugin
  active_plugins += 'checkupdate'
endif

subdir('data')
subdir('po')
subdir('gedit')
subdir('plugins')

if get_option('gtk_doc')
  subdir('docs/reference')
endif

if get_option('user_documentation')
  subdir('help')
endif

meson.add_install_script(
  'build-aux/meson/post_install.py',
  join_paths(get_option('prefix'), get_option('libdir')),
  join_paths(get_option('prefix'), get_option('datadir'))
)

# Summary message

summary = [
  'Configuration:',
  '',
  '        gedit version @0@ (API version @1@)'.format(meson.project_version(), api_version),
  '',
  '        Prefix:                @0@'.format(get_option('prefix')),
  '        API documentation:     @0@'.format(get_option('gtk_doc')),
  '        User documentation:    @0@'.format(get_option('user_documentation')),
  '',
]
message('\n'.join(summary))
