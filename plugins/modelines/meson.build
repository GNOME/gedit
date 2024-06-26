plugin_modelines_sources = files(
  'gedit-modeline-plugin.c',
  'modeline-parser.c',
)

shared_module(
  'modelines',
  sources: plugin_modelines_sources,
  include_directories: root_include_dir,
  dependencies: libgedit_dep,
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
  name_suffix: module_suffix,
)

custom_target(
  'modelines.plugin',
  input: 'modelines.plugin.desktop.in',
  output: 'modelines.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
)

install_data(
  'language-mappings',
  install_dir: get_option('datadir') / 'gedit/plugins/modelines',
)
