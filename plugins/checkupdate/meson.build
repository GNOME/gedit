libcheckupdate_sources = files(
  'gedit-check-update-plugin.c',
)

libcheckupdate_deps = [
  libgedit_dep,
  libsoup_dep,
]

libcheckupdate_sha = shared_module(
  'checkupdate',
  sources: libcheckupdate_sources,
  include_directories: root_include_dir,
  dependencies: libcheckupdate_deps,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  ),
  name_suffix: module_suffix,
)

install_data(
  'org.gnome.gedit.plugins.checkupdate.gschema.xml',
  install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'glib-2.0/schemas')
)

custom_target(
  'checkupdate.plugin',
  input: 'checkupdate.plugin.desktop.in',
  output: 'checkupdate.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)
