subdir('icons')

# Man page

install_man('gedit.1')

# AppStream metainfo

metainfo = 'org.gnome.gedit.metainfo.xml'
metainfo_file = i18n.merge_file(
  input: metainfo + '.in',
  output: metainfo,
  po_dir: '../po/',
  install: true,
  install_dir: get_option('datadir') / 'metainfo'
)

appstreamcli = find_program('appstreamcli', required: get_option('require_all_tests'))
if appstreamcli.found()
  test(
    'validate-metainfo',
    appstreamcli,
    args: ['validate', '--no-net', metainfo_file]
  )
endif

# Desktop file

desktop_file = 'org.gnome.gedit.desktop'
desktop_output_file = i18n.merge_file(
  type: 'desktop',
  input: desktop_file + '.in',
  output: desktop_file,
  po_dir: '../po/',
  install: true,
  install_dir: get_option('datadir') / 'applications'
)

desktop_file_validate = find_program('desktop-file-validate', required: get_option('require_all_tests'))
if desktop_file_validate.found()
  test(
    'validate-desktop',
    desktop_file_validate,
    args: [desktop_output_file]
  )
endif

# GSettings schema

gschema_config_data = configuration_data()
gschema_config_data.set(
  'ACTIVE_PLUGINS',
  '\'filebrowser\', \'sort\', \'spell\', \'textsize\''
)

gschema_file = configure_file(
  input: 'org.gnome.gedit.gschema.xml.in',
  output: 'org.gnome.gedit.gschema.xml',
  configuration: gschema_config_data,
  install_dir: get_option('datadir') / 'glib-2.0/schemas'
)

gschema_dtd = gio_dep.get_variable(pkgconfig: 'schemasdir') / 'gschema.dtd'

xmllint = find_program('xmllint', required: get_option('require_all_tests'))
if xmllint.found()
  test(
    'validate-gschema',
    xmllint,
    args: [
      '--noout',
      '--dtdvalid', gschema_dtd,
      gschema_file,
    ]
  )
endif

if host_machine.system() == 'windows'
  install_data(
    'org.gnome.gedit-ms-windows.gschema.override',
    install_dir: get_option('datadir') / 'glib-2.0/schemas'
  )
endif

# D-Bus service

service_config_data = configuration_data()
service_config_data.set('bindir', get_option('prefix') / get_option('bindir'))

configure_file(
  input: 'org.gnome.gedit.service.in',
  output: 'org.gnome.gedit.service',
  configuration: service_config_data,
  install_dir: get_option('datadir') / 'dbus-1/services'
)
