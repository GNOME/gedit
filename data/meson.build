subdir('icons')

install_man('gedit.1')

appdata = 'org.gnome.gedit.appdata.xml'
appdata_file = i18n.merge_file(
  appdata,
  input: appdata + '.in',
  output: appdata,
  po_dir: '../po/',
  install: true,
  install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'metainfo')
)

appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test(
    'validate-appdata',
    appstream_util,
    args: [
      'validate-relax',
      '--nonet',
      appdata_file.full_path()
    ]
  )
endif


desktop_in = configuration_data()
desktop_in.set('VERSION', meson.project_version())
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
      desktop_file.full_path(),
    ]
  )
endif

quoted_plugins = []
foreach plugin_name:active_plugins
  quoted_plugins += [
    '\'@0@\''.format(plugin_name)
  ]
endforeach

gschema_in = configuration_data()
gschema_in.set('GETTEXT_PACKAGE', meson.project_name())
gschema_in.set('ACTIVE_PLUGINS', ', '.join(quoted_plugins))

gschema_file = configure_file(
  input: 'org.gnome.gedit.gschema.xml.in',
  output: 'org.gnome.gedit.gschema.xml',
  configuration: gschema_in,
  install_dir: join_paths(
    glibdir,
    'schemas',
  )
)

gschema_dtd = join_paths(
  gio_dep.get_pkgconfig_variable('schemasdir'),
  'gschema.dtd',
)

xmllint = find_program('xmllint', required: false)
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
