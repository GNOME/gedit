install_subdir(
  'textsize',
  install_dir: get_option('libdir') / 'gedit/plugins',
)

custom_target(
  'textsize.plugin',
  input: 'textsize.plugin.desktop.in',
  output: 'textsize.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
)
