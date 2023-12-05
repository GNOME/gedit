project(
  'gedit', 'c',
  version: '44.3',
  meson_version: '>= 0.53'
)

if host_machine.system() == 'darwin'
  add_languages('objc')
endif

gnome = import('gnome')
i18n = import('i18n')
pkg_config = import('pkgconfig')
python = import('python')

api_version = '44'

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
gio_dep = dependency('gio-2.0', version: '>= 2.70')

libgedit_public_deps = [
  gio_dep,
  dependency('gtk+-3.0', version: '>= 3.22'),
  dependency('amtk-5', version: '>= 5.6'),
  dependency('tepl-6', version: '>= 6.4'),
  dependency('libpeas-gtk-1.0'),
]

deps_basic_list = [
  libgedit_public_deps,
  dependency('gobject-introspection-1.0'),
]

gspell_dep = dependency('gspell-1', version: '>= 1.0')
python3 = python.find_installation('python3')

# Configurations
config_h = configuration_data()
config_h.set_quoted('PACKAGE_STRING', 'gedit-@0@'.format(api_version))
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())
config_h.set_quoted('LIBDIR', join_paths(get_option('prefix'), get_option('libdir')))
config_h.set_quoted('DATADIR', join_paths(get_option('prefix'), get_option('datadir')))
config_h.set_quoted('VERSION', meson.project_version())

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
