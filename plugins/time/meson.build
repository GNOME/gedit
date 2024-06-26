plugin_time_sources = files(
  'gedit-time-plugin.c',
)

gnome.mkenums(
  'org.gnome.gedit.plugins.time.enums.xml',
  sources: plugin_time_sources,
  comments: '<!-- @comment@ -->',
  fhead: '<schemalist>',
  vhead: '  <@type@ id="org.gnome.gedit.plugins.time.@EnumName@">',
  vprod: '    <value nick="@valuenick@" value="@valuenum@"/>',
  vtail: '  </@type@>',
  ftail: '</schemalist>',
  install_header: true,
  install_dir: get_option('datadir') / 'glib-2.0/schemas',
)

subdir('resources')

shared_module(
  'time',
  sources: plugin_time_sources,
  include_directories: root_include_dir,
  dependencies: libgedit_dep,
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
  name_suffix: module_suffix,
)

plugin_time_gschema_file = files('org.gnome.gedit.plugins.time.gschema.xml')
install_data(
  plugin_time_gschema_file,
  install_dir: get_option('datadir') / 'glib-2.0/schemas'
)

if xmllint.found()
  test(
    'validate-time-gschema',
    xmllint,
    args: [
      '--noout',
      '--dtdvalid', gschema_dtd,
      plugin_time_gschema_file,
    ]
  )
endif

custom_target(
  'time.plugin',
  input: 'time.plugin.desktop.in',
  output: 'time.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: get_option('libdir') / 'gedit/plugins',
)
