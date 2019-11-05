libtime_sources = files(
  'gedit-time-plugin.c',
)

libtime_deps = [
  libgedit_dep,
]

libtime_c_args = [
  '-DHAVE_CONFIG_H',
]

gnome.mkenums(
  'org.gnome.gedit.plugins.time.enums.xml',
  sources: libtime_sources,
  comments: '<!-- @comment@ -->',
  fhead: '<schemalist>',
  vhead: '  <@type@ id="org.gnome.gedit.plugins.time.@EnumName@">',
  vprod: '    <value nick="@valuenick@" value="@valuenum@"/>',
  vtail: '  </@type@>',
  ftail: '</schemalist>',
  install_header: true,
  install_dir: join_paths(
    glibdir,
    'schemas',
  )
)

subdir('resources')

libtime_sha = shared_module(
  'time',
  sources: libtime_sources,
  include_directories: rootdir,
  dependencies: libtime_deps,
  c_args: libtime_c_args,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  ),
  name_suffix: module_suffix,
)

configure_file(
  input: 'org.gnome.gedit.plugins.time.gschema.xml.in',
  output: 'org.gnome.gedit.plugins.time.gschema.xml',
  configuration: gschema_xml,
  install_dir: join_paths(
    glibdir,
    'schemas',
  )
)

custom_target(
  'time.plugin',
  input: 'time.plugin.desktop.in',
  output: 'time.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)
