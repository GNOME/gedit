plugin_spell_sources = files(
  'gedit-spell-app-activatable.c',
  'gedit-spell-plugin.c',
)

subdir('resources')

shared_module(
  'spell',
  sources: plugin_spell_sources,
  include_directories: root_include_dir,
  dependencies: [libgedit_dep, gspell_dep],
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
  name_suffix: module_suffix,
)

plugin_spell_gschema_file = files('org.gnome.gedit.plugins.spell.gschema.xml')
install_data(
  plugin_spell_gschema_file,
  install_dir: get_option('datadir') / 'glib-2.0/schemas',
)

if xmllint.found()
  test(
    'validate-spell-gschema',
    xmllint,
    args: [
      '--noout',
      '--dtdvalid', gschema_dtd,
      plugin_spell_gschema_file,
    ]
  )
endif

custom_target(
  'spell.plugin',
  input: 'spell.plugin.desktop.in',
  output: 'spell.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
)
