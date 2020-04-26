liburicontextmenu_sources = files(
  'gedit-uri-context-menu-plugin.c',
)

liburicontextmenu_deps = [
  libgedit_dep,
]

liburicontextmenu_sha = shared_module(
  'uricontextmenu',
  sources: liburicontextmenu_sources,
  include_directories: root_include_dir,
  dependencies: liburicontextmenu_deps,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  ),
  name_suffix: module_suffix,
)

custom_target(
  'uricontextmenu.plugin',
  input: 'uricontextmenu.plugin.desktop.in',
  output: 'uricontextmenu.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)
