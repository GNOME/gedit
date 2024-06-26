plugin_pythonconsole_sources = files(
  '__init__.py',
  'config.py',
  'console.py',
)

install_data(
  plugin_pythonconsole_sources,
  install_dir: get_option('libdir') / 'gedit/plugins/pythonconsole',
)

install_data(
  'config.ui',
  install_dir: get_option('datadir') / 'gedit/plugins/pythonconsole/ui',
)
