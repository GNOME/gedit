libquickhighlight_sources = files(
  'gedit-quick-highlight-plugin.c',
)

libquickhighlight_deps = [
  libgedit_dep,
]

libquickhighlight_sha = shared_module(
  'quickhighlight',
  sources: libquickhighlight_sources,
  include_directories: root_include_dir,
  dependencies: libquickhighlight_deps,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  ),
  name_suffix: module_suffix,
)

custom_target(
  'quickhighlight.plugin',
  input: 'quickhighlight.plugin.desktop.in',
  output: 'quickhighlight.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)
