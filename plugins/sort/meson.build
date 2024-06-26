plugin_sort_sources = files(
  'gedit-sort-plugin.c',
)

subdir('resources')

shared_module(
  'sort',
  sources: plugin_sort_sources,
  include_directories: root_include_dir,
  dependencies: libgedit_dep,
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
  name_suffix: module_suffix,
)

custom_target(
  'sort.plugin',
  input: 'sort.plugin.desktop.in',
  output: 'sort.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
)
