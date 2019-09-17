libspell_sources = files(
  'gedit-spell-app-activatable.c',
  'gedit-spell-plugin.c',
)

libspell_deps = [
  libgedit_dep,
  gspell_dep,
]

libspell_c_args = [
  '-DHAVE_CONFIG_H',
]

subdir('resources')

libspell_sha = shared_module(
  'spell',
  sources: libspell_sources,
  include_directories: rootdir,
  dependencies: libspell_deps,
  c_args: libspell_c_args,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  ),
  name_suffix: module_suffix,
)

configure_file(
  input: 'org.gnome.gedit.plugins.spell.gschema.xml.in',
  output: 'org.gnome.gedit.plugins.spell.gschema.xml',
  configuration: gschema_xml,
  install_dir: join_paths(
    glibdir,
    'schemas',
  )
)

custom_target(
  'spell.plugin',
  input: 'spell.plugin.desktop.in',
  output: 'spell.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)
