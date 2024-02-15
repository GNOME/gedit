subdir('icons')

install_man('gedit.1')

metainfo = 'org.gnome.gedit.metainfo.xml'
metainfo_file = i18n.merge_file(
  input: metainfo + '.in',
  output: metainfo,
  po_dir: '../po/',
  install: true,
  install_dir: get_option('datadir') / 'metainfo'
)

appstream_util = find_program('appstream-util', required: get_option('require_all_tests'))
if appstream_util.found()
  test(
    'validate-metainfo',
    appstream_util,
    args: ['validate-relax', '--nonet', metainfo_file]
  )
endif

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

active_plugins = [
  'docinfo',
  'filebrowser',
  'modelines',
  'sort',
  'spell',
]

quoted_plugins = []
foreach plugin_name : active_plugins
  quoted_plugins += '\'@0@\''.format(plugin_name)
endforeach

gschema_in = configuration_data()
gschema_in.set('ACTIVE_PLUGINS', ', '.join(quoted_plugins))

gschema_file = configure_file(
  input: 'org.gnome.gedit.gschema.xml.in',
  output: 'org.gnome.gedit.gschema.xml',
  configuration: gschema_in,
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

service_in = configuration_data()
service_in.set('bindir', get_option('prefix') / get_option('bindir'))

configure_file(
  input: 'org.gnome.gedit.service.in',
  output: 'org.gnome.gedit.service',
  configuration: service_in,
  install_dir: get_option('datadir') / 'dbus-1/services'
)
