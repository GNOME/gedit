subdir('snippets')

install_subdir(
  'data',
  strip_directory : true,
  install_dir: join_paths(
    pkgdatadir,
    'plugins',
    'snippets',
  )
)

custom_target(
  'snippets.plugin',
  input: 'snippets.plugin.desktop.in',
  output: 'snippets.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)
