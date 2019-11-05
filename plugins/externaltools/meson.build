subdir('scripts')
subdir('tools')
subdir('data')

externaltools_gschema_file = configure_file(
  input: 'org.gnome.gedit.plugins.externaltools.gschema.xml.in',
  output: 'org.gnome.gedit.plugins.externaltools.gschema.xml',
  configuration: gschema_in,
  install_dir: join_paths(
    glibdir,
    'schemas',
  )
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
