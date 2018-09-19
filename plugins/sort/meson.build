libsort_sources = files(
  'gedit-sort-plugin.c',
)

libsort_deps = [
  libgedit_dep,
]

libsort_c_args = [
  '-DHAVE_CONFIG_H',
]

subdir('resources')

libsort_sha = shared_module(
  'sort',
  sources: libsort_sources,
  include_directories: rootdir,
  dependencies: libsort_deps,
  c_args: libsort_c_args,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)

custom_target(
  'sort.plugin',
  input: 'sort.plugin.desktop.in',
  output: 'sort.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)
