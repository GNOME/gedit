project(
  'gedit', 'c',
  version: '46.0',
  meson_version: '>= 0.59',
  default_options: ['warning_level=2']
)

if host_machine.system() == 'darwin'
  add_languages('objc')
endif

gnome = import('gnome')
i18n = import('i18n')
pkg_config = import('pkgconfig')
python = import('python')

api_version = '46'

#####
# CFLAGS
# Some flags are missing when using only the builtin warning_level meson option,
# even at the maximum level.
# The following warning_cflags suppose that warning_level=2.

# Tries to mimic the AX_COMPILER_FLAGS Autotools macro.
warning_cflags = [
  '-fno-strict-aliasing',
  '-Wundef',
  '-Wnested-externs',
  '-Wwrite-strings',
  '-Wpointer-arith',
  '-Wmissing-declarations',
  '-Wmissing-prototypes',
  '-Wstrict-prototypes',
  '-Wredundant-decls',
  '-Wno-unused-parameter',
  '-Wno-missing-field-initializers',
  '-Wdeclaration-after-statement',
  '-Wformat=2',
  '-Wold-style-definition',
  '-Wcast-align',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Wsign-compare',
  '-Wstrict-aliasing',
  '-Wshadow',
  '-Winline',
  '-Wpacked',
  '-Wmissing-format-attribute',
  '-Wmissing-noreturn',
  '-Winit-self',
  '-Wredundant-decls',
  '-Wmissing-include-dirs',
  '-Wunused-but-set-variable',
  '-Warray-bounds',
  '-Wimplicit-function-declaration',
  '-Wreturn-type',
  '-Wswitch-enum',
  '-Wswitch-default',
  '-Wduplicated-cond',
  '-Wduplicated-branches',
  '-Wlogical-op',
  '-Wrestrict',
  '-Wnull-dereference',
  '-Wjump-misses-init',
  '-Wdouble-promotion',
]

c_compiler = meson.get_compiler('c')
supported_warning_cflags = c_compiler.get_supported_arguments(warning_cflags)
# Use add_global_arguments() (not add_project_arguments()) so that it is applied
# to libgd as well.
add_global_arguments(supported_warning_cflags, language: 'c')
##### end CFLAGS

# Paths
root_include_dir = include_directories('.')

srcdir = meson.current_source_dir()

pkglibdir = get_option('prefix') / get_option('libdir') / 'gedit'
pkgdatadir = get_option('prefix') / get_option('datadir') / 'gedit'
glibdir = get_option('prefix') / get_option('datadir') / 'glib-2.0'

# Subprojects
if not get_option('buildtype').contains('plain')
  run_command(
    'git', '-C', meson.project_source_root(), 'submodule', 'update', '--init', '--recursive',
    check: true,
  )
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
  dependency('libgedit-amtk-5'),
  dependency('tepl-6', version: '>= 6.8'),
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
config_h.set10('OS_MACOS', host_machine.system() == 'darwin')

if get_option('headerbar') == 'auto'
  config_h.set10('GEDIT_HAS_HEADERBAR', host_machine.system() != 'darwin')
else
  config_h.set10('GEDIT_HAS_HEADERBAR', get_option('headerbar') == 'yes')
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
)

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true,
)

summary('API version', api_version)
summary('Prefix', get_option('prefix'))
summary('API documentation', get_option('gtk_doc'))
summary('User documentation', get_option('user_documentation'))
summary('Require all tests', get_option('require_all_tests'))
summary('Headerbar', get_option('headerbar'))
