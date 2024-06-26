plugin_docinfo_sources = files(
  'gedit-docinfo-plugin.c',
)

subdir('resources')

shared_module(
  'docinfo',
  sources: plugin_docinfo_sources,
  include_directories: root_include_dir,
  dependencies: libgedit_dep,
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
  name_suffix: module_suffix,
)

custom_target(
  'docinfo.plugin',
  input: 'docinfo.plugin.desktop.in',
  output: 'docinfo.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
)
