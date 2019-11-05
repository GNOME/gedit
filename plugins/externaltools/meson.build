subdir('scripts')
subdir('tools')
subdir('data')

configure_file(
  input: 'org.gnome.gedit.plugins.externaltools.gschema.xml.in',
  output: 'org.gnome.gedit.plugins.externaltools.gschema.xml',
  configuration: gschema_xml,
  install_dir: join_paths(
    glibdir,
    'schemas',
  )
)

custom_target(
  'externaltools.plugin',
  input: 'externaltools.plugin.desktop.in',
  output: 'externaltools.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)

subdir('tests')
