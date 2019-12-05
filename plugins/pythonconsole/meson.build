subdir('pythonconsole')

pythonconsole_gschema_file = files('org.gnome.gedit.plugins.pythonconsole.gschema.xml')
install_data(
  pythonconsole_gschema_file,
  install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'glib-2.0/schemas')
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
