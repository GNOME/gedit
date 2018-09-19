subdir('icons')

install_man('gedit.1')

install_data(
  'gedit.convert',
  install_dir: join_paths(
    gconfdir,
    'gsettings',
  )
)

install_data(
  'gedit-bugreport.sh',
  install_dir: join_paths(
    libexecdir,
    package_name,
  )
)

i18n.merge_file(
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

desktop_in = configuration_data()
desktop_in.set('VERSION', version)
desktop_in.set('FULL_LIBEXECDIR', libexecdir)

desktop_file = configure_file(
  input: 'org.gnome.gedit.desktop.in.in',
  output: 'org.gnome.gedit.desktop.in',
  configuration: desktop_in,
  install: false,
)

i18n.merge_file(
  'desktop-file',
  input: desktop_file,
  output: 'org.gnome.gedit.desktop',
  type: 'desktop',
  po_dir: join_paths(srcdir, 'po'),
  install: true,
  install_dir: join_paths(
    datadir,
    'applications',
  )
)

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
  install: true,
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
  install: true,
  install_dir: join_paths(
    dbusdir,
    'services',
  )
)
