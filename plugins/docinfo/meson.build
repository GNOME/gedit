libdocinfo_sources = files(
  'gedit-docinfo-plugin.c',
)

libdocinfo_deps = [
  libgedit_dep,
]

subdir('resources')

libdocinfo_sha = shared_module(
  'docinfo',
  sources: libdocinfo_sources,
  include_directories: root_include_dir,
  dependencies: libdocinfo_deps,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  ),
  name_suffix: module_suffix,
)

custom_target(
  'docinfo.plugin',
  input: 'docinfo.plugin.desktop.in',
  output: 'docinfo.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)
