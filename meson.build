project(
  'gedit', 'c',
  version: '42.2',
  meson_version: '>= 0.53'
)

if host_machine.system() == 'darwin'
  add_languages('objc')
endif

gnome = import('gnome')
i18n = import('i18n')
pkg_config = import('pkgconfig')
python = import('python')

api_version = '41'

# Paths
root_include_dir = include_directories('.')

srcdir = meson.current_source_dir()

pkglibdir = get_option('prefix') / get_option('libdir') / 'gedit'
pkgdatadir = get_option('prefix') / get_option('datadir') / 'gedit'
glibdir = get_option('prefix') / get_option('datadir') / 'glib-2.0'

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
glib_req = '>= 2.64'
gtk_req = '>= 3.22'
gtksourceview_req =  '>= 4.0'
libpeas_req = '>= 1.14.1'
libxml_req = '>= 2.5.0'
gspell_req = '>= 1.0'
pygobject_req = '>= 3.0.0'

gio_dep = dependency('gio-2.0', version: glib_req)
libxml_dep = dependency('libxml-2.0', version: libxml_req)
gspell_dep = dependency('gspell-1', version: gspell_req, required: get_option('spell'))

if get_option('python')
  python3 = python.find_installation('python3')
endif

libgedit_public_deps = [
  gio_dep,
  dependency('gtk+-3.0', version: gtk_req),
  dependency('gtksourceview-4', version: gtksourceview_req),
  dependency('libpeas-gtk-1.0', version: libpeas_req),
]

deps_basic_list = [
  libgedit_public_deps,
  dependency('gobject-introspection-1.0'),
]

# Configurations
config_h = configuration_data()
config_h.set_quoted('PACKAGE_STRING', 'gedit-@0@'.format(api_version))
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

configure_file(
  output: 'config.h',
  configuration: config_h
)

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
  get_option('prefix') / get_option('libdir'),
  get_option('prefix') / get_option('datadir')
)

summary('API version', api_version)
summary('Prefix', get_option('prefix'))
summary('API documentation', get_option('gtk_doc'))
summary('User documentation', get_option('user_documentation'))
summary('Require all tests', get_option('require_all_tests'))
summary('Spell checker plugin', get_option('spell').enabled().to_string())
