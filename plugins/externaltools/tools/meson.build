externaltools_sources = files(
  '__init__.py',
  'appactivatable.py',
  'capture.py',
  'filelookup.py',
  'functions.py',
  'library.py',
  'linkparsing.py',
  'manager.py',
  'outputpanel.py',
  'windowactivatable.py',
)

install_data(
  externaltools_sources,
  install_dir: get_option('libdir') / 'gedit/plugins/externaltools',
)

externaltools_data = files(
  'outputpanel.ui',
  'tools.ui',
)

install_data(
  externaltools_data,
  install_dir: get_option('datadir') / 'gedit/plugins/externaltools/ui',
)
