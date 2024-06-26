snippets_sources = [
  '__init__.py',
  'appactivatable.py',
  'completion.py',
  'document.py',
  'exporter.py',
  'helper.py',
  'importer.py',
  'languagemanager.py',
  'library.py',
  'manager.py',
  'parser.py',
  'placeholder.py',
  'shareddata.py',
  'signals.py',
  'singleton.py',
  'snippet.py',
  'substitutionparser.py',
  'windowactivatable.py',
]

install_data(
  snippets_sources,
  install_dir: get_option('libdir') / 'gedit/plugins/snippets',
)

install_data(
  'snippets.ui',
  install_dir: get_option('datadir') / 'gedit/plugins/snippets/ui',
)
