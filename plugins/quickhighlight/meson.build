plugin_quickhighlight_sources = files(
  'gedit-quick-highlight-plugin.c',
)

shared_module(
  'quickhighlight',
  sources: plugin_quickhighlight_sources,
  include_directories: root_include_dir,
  dependencies: libgedit_dep,
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
  name_suffix: module_suffix,
)

custom_target(
  'quickhighlight.plugin',
  input: 'quickhighlight.plugin.desktop.in',
  output: 'quickhighlight.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
)
