subdir('pythonconsole')

pythonconsole_gschema_file = configure_file(
  input: 'org.gnome.gedit.plugins.pythonconsole.gschema.xml.in',
  output: 'org.gnome.gedit.plugins.pythonconsole.gschema.xml',
  configuration: gschema_in,
  install_dir: join_paths(
    glibdir,
    'schemas',
  )
)

if xmllint.found()
  test(
    'validate-pythonconsole-gschema',
    xmllint,
    args: [
      '--noout',
      '--dtdvalid', gschema_dtd,
      pythonconsole_gschema_file,
    ]
  )
endif

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
