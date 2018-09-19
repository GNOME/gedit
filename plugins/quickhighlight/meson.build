libquickhighlight_sources = files(
  'gedit-quick-highlight-plugin.c',
)

libquickhighlight_deps = [
  libgedit_dep,
]

libquickhighlight_c_args = [
  '-DHAVE_CONFIG_H',
]

libquickhighlight_sha = shared_module(
  'quickhighlight',
  sources: libquickhighlight_sources,
  include_directories: rootdir,
  dependencies: libquickhighlight_deps,
  c_args: libquickhighlight_c_args,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
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