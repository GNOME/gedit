subdir('pythonconsole')

configure_file(
  input: 'org.gnome.gedit.plugins.pythonconsole.gschema.xml.in',
  output: 'org.gnome.gedit.plugins.pythonconsole.gschema.xml',
  configuration: gschema_xml,
  install_dir: join_paths(
    glibdir,
    'schemas',
  )
)

custom_target(
  'pythonconsole.plugin',
  input: 'pythonconsole.plugin.desktop.in',
  output: 'pythonconsole.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)
