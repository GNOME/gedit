install_data('org.gnome.gedit.svg',
  install_dir: join_paths(datadir, 'icons', 'hicolor', 'scalable', 'apps'),
)

install_data('org.gnome.gedit-symbolic.svg',
  install_dir: join_paths(datadir, 'icons', 'hicolor', 'symbolic', 'apps'),
)

install_man('gedit.1')

appdata_file = i18n.merge_file(
  'appdata-file',
  input: 'org.gnome.gedit.appdata.xml.in',
  output: 'org.gnome.gedit.appdata.xml',
  type: 'xml',
  po_dir: join_paths(srcdir, 'po'),
  install: true,
  install_dir: join_paths(
    datadir,
    'metainfo',
  )
)

appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
test(
    'validate-appdata', appstream_util,
    args: [
        'validate-relax', '--nonet', appdata_file.full_path()
    ]
)
endif


desktop_in = configuration_data()
desktop_in.set('VERSION', version)
desktop_in.set('FULL_LIBEXECDIR', libexecdir)

desktop_file_configured = configure_file(
  input: 'org.gnome.gedit.desktop.in.in',
  output: 'org.gnome.gedit.desktop.in',
  configuration: desktop_in,
)

desktop_file = i18n.merge_file(
  'desktop-file',
  input: desktop_file_configured,
  output: 'org.gnome.gedit.desktop',
  type: 'desktop',
  po_dir: join_paths(srcdir, 'po'),
  install: true,
  install_dir: join_paths(
    datadir,
    'applications',
  )
)

desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
test(
    'validate-desktop',
    desktop_file_validate,
    args: [
        desktop_file.full_path()
    ]
)
endif

quoted_plugins = []
foreach plugin_name:active_plugins
  quoted_plugins += [
    '\'@0@\''.format(plugin_name)
  ]
endforeach

gschema_xml = configuration_data()
gschema_xml.set('GETTEXT_PACKAGE', package_name)
gschema_xml.set('ACTIVE_PLUGINS', ', '.join(quoted_plugins))

configure_file(
  input: 'org.gnome.gedit.gschema.xml.in',
  output: 'org.gnome.gedit.gschema.xml',
  configuration: gschema_xml,
  install_dir: join_paths(
    glibdir,
    'schemas',
  )
)

service_in = configuration_data()
service_in.set('bindir', bindir)

configure_file(
  input: 'org.gnome.gedit.service.in',
  output: 'org.gnome.gedit.service',
  configuration: service_in,
  install_dir: join_paths(
    dbusdir,
    'services',
  )
)
