subdir('scripts')
subdir('tools')
subdir('data')

externaltools_gschema_file = files('org.gnome.gedit.plugins.externaltools.gschema.xml')
install_data(
  externaltools_gschema_file,
  install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'glib-2.0/schemas')
)

if xmllint.found()
  test(
    'validate-externaltools-gschema',
    xmllint,
    args: [
      '--noout',
      '--dtdvalid', gschema_dtd,
      externaltools_gschema_file,
    ]
  )
endif

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
