libspell_sources = files(
  'gedit-spell-app-activatable.c',
  'gedit-spell-plugin.c',
)

libspell_deps = [
  libgedit_dep,
  gspell_dep,
]

subdir('resources')

libspell_sha = shared_module(
  'spell',
  sources: libspell_sources,
  include_directories: root_include_dir,
  dependencies: libspell_deps,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  ),
  name_suffix: module_suffix,
)

spell_gschema_file = files('org.gnome.gedit.plugins.spell.gschema.xml')
install_data(
  spell_gschema_file,
  install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'glib-2.0/schemas')
)

if xmllint.found()
  test(
    'validate-spell-gschema',
    xmllint,
    args: [
      '--noout',
      '--dtdvalid', gschema_dtd,
      spell_gschema_file,
    ]
  )
endif

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
