libcheckupdate_sources = files(
  'gedit-check-update-plugin.c',
)

libcheckupdate_deps = [
  libgedit_dep,
  libsoup_dep,
]

libcheckupdate_c_args = [
  '-DHAVE_CONFIG_H',
]

libcheckupdate_sha = shared_module(
  'checkupdate',
  sources: libcheckupdate_sources,
  include_directories: rootdir,
  dependencies: libcheckupdate_deps,
  c_args: libcheckupdate_c_args,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  ),
  name_suffix: module_suffix,
)

configure_file(
  input: 'org.gnome.gedit.plugins.checkupdate.gschema.xml.in',
  output: 'org.gnome.gedit.plugins.checkupdate.gschema.xml',
  configuration: gschema_xml,
  install_dir: join_paths(
    glibdir,
    'schemas',
  )
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
