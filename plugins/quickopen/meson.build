install_subdir(
  'quickopen',
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)

custom_target(
  'quickopen.plugin',
  input: 'quickopen.plugin.desktop.in',
  output: 'quickopen.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)
