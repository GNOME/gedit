libopenlinks_sources = files(
  'gedit-open-links-plugin.c',
)

libopenlinks_deps = [
  libgedit_dep,
]

libopenlinks_sha = shared_module(
  'openlinks',
  sources: libopenlinks_sources,
  include_directories: root_include_dir,
  dependencies: libopenlinks_deps,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  ),
  name_suffix: module_suffix,
)

custom_target(
  'openlinks.plugin',
  input: 'openlinks.plugin.desktop.in',
  output: 'openlinks.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)

subdir('unit-tests')