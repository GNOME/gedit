libdocinfo_sources = files(
  'gedit-docinfo-plugin.c',
)

libdocinfo_deps = [
  libgedit_dep,
]

libdocinfo_c_args = [
  '-DHAVE_CONFIG_H',
]

subdir('resources')

libdocinfo_sha = shared_module(
  'docinfo',
  sources: libdocinfo_sources,
  include_directories: rootdir,
  dependencies: libdocinfo_deps,
  c_args: libdocinfo_c_args,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
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