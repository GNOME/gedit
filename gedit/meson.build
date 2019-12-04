libgedit_public_h = files(
  'gedit-app-activatable.h',
  'gedit-app.h',
  'gedit-commands.h',
  'gedit-debug.h',
  'gedit-document.h',
  'gedit-encodings-combo-box.h',
  'gedit-menu-extension.h',
  'gedit-message-bus.h',
  'gedit-message.h',
  'gedit-progress-info-bar.h',
  'gedit-statusbar.h',
  'gedit-tab.h',
  'gedit-utils.h',
  'gedit-view-activatable.h',
  'gedit-view.h',
  'gedit-window-activatable.h',
  'gedit-window.h',
)

libgedit_private_h = files(
  'gedit-app-private.h',
  'gedit-close-confirmation-dialog.h',
  'gedit-commands-private.h',
  'gedit-dirs.h',
  'gedit-document-private.h',
  'gedit-documents-panel.h',
  'gedit-encoding-items.h',
  'gedit-encodings-dialog.h',
  'gedit-file-chooser-dialog-gtk.h',
  'gedit-file-chooser-dialog.h',
  'gedit-highlight-mode-dialog.h',
  'gedit-highlight-mode-selector.h',
  'gedit-history-entry.h',
  'gedit-io-error-info-bar.h',
  'gedit-menu-stack-switcher.h',
  'gedit-metadata-manager.h',
  'gedit-multi-notebook.h',
  'gedit-notebook.h',
  'gedit-notebook-popup-menu.h',
  'gedit-notebook-stack-switcher.h',
  'gedit-open-document-selector.h',
  'gedit-open-document-selector-helper.h',
  'gedit-open-document-selector-store.h',
  'gedit-pango.h',
  'gedit-plugins-engine.h',
  'gedit-preferences-dialog.h',
  'gedit-print-job.h',
  'gedit-print-preview.h',
  'gedit-recent.h',
  'gedit-replace-dialog.h',
  'gedit-settings.h',
  'gedit-status-menu-button.h',
  'gedit-tab-label.h',
  'gedit-tab-private.h',
  'gedit-view-centering.h',
  'gedit-view-frame.h',
  'gedit-window-private.h',
)

libgedit_sources = files(
  'gedit-app-activatable.c',
  'gedit-app.c',
  'gedit-close-confirmation-dialog.c',
  'gedit-commands-documents.c',
  'gedit-commands-edit.c',
  'gedit-commands-file.c',
  'gedit-commands-file-print.c',
  'gedit-commands-help.c',
  'gedit-commands-search.c',
  'gedit-commands-view.c',
  'gedit-debug.c',
  'gedit-dirs.c',
  'gedit-document.c',
  'gedit-documents-panel.c',
  'gedit-encoding-items.c',
  'gedit-encodings-combo-box.c',
  'gedit-encodings-dialog.c',
  'gedit-file-chooser-dialog.c',
  'gedit-file-chooser-dialog-gtk.c',
  'gedit-highlight-mode-dialog.c',
  'gedit-highlight-mode-selector.c',
  'gedit-history-entry.c',
  'gedit-io-error-info-bar.c',
  'gedit-menu-extension.c',
  'gedit-menu-stack-switcher.c',
  'gedit-message-bus.c',
  'gedit-message.c',
  'gedit-metadata-manager.c',
  'gedit-multi-notebook.c',
  'gedit-notebook.c',
  'gedit-notebook-popup-menu.c',
  'gedit-notebook-stack-switcher.c',
  'gedit-open-document-selector.c',
  'gedit-open-document-selector-helper.c',
  'gedit-open-document-selector-store.c',
  'gedit-pango.c',
  'gedit-plugins-engine.c',
  'gedit-preferences-dialog.c',
  'gedit-print-job.c',
  'gedit-print-preview.c',
  'gedit-progress-info-bar.c',
  'gedit-recent.c',
  'gedit-replace-dialog.c',
  'gedit-settings.c',
  'gedit-statusbar.c',
  'gedit-status-menu-button.c',
  'gedit-tab.c',
  'gedit-tab-label.c',
  'gedit-utils.c',
  'gedit-view-activatable.c',
  'gedit-view.c',
  'gedit-view-centering.c',
  'gedit-view-frame.c',
  'gedit-window-activatable.c',
  'gedit-window.c',
)

libgedit_c_args = [
  '-DHAVE_CONFIG_H',
]

libgedit_link_args = [
]

libgedit_deps = [
  glib_dep,
  gobject_dep,
  gio_dep,
  gtk_dep,
  gtksourceview_dep,
  introspection_dep,
  libpeas_dep,
  libpeas_gtk_dep,
  libxml_dep,
  libgd_dep,
]

if windowing_target == 'quartz'
  libgedit_sources += files(
    'gedit-app-osx.m',
    'gedit-file-chooser-dialog-osx.m',
  )
  libgedit_c_args += [
    '-DOS_OSX=1',
  ]
  libgedit_link_args += [
    '-Wl,-framework', '-Wl,Foundation',
    '-Wl,-framework', '-Wl,AppKit',
  ]
  gtk_mac_integration_dep = dependency('gtk-mac-integration-gtk3')
  libgedit_deps += [
    gtk_mac_integration_dep,
  ]
elif windowing_target == 'win32'
  libgedit_sources += files(
    'gedit-app-win32.c',
  )
else
  libgedit_sources += files(
    'gedit-app-x11.c',
  )

  libgedit_deps += [
    x11_dep,
  ]
endif

libgedit_enums = gnome.mkenums(
  'gedit-enum-types',
  sources: libgedit_public_h + ['gedit-notebook.h'],
  h_template: 'gedit-enum-types.h.template',
  c_template: 'gedit-enum-types.c.template',
)

subdir('resources')

libgedit_sources += [
  libgedit_enums.get(0),
  libgedit_res.get(0),
]

libgedit_dep_sources = []

install_headers(
  libgedit_public_h,
  install_dir: join_paths(
    pkgincludedir,
    'gedit',
  )
)

libgedit_sha = shared_library(
  package_string,
  libgedit_sources + [libgedit_enums.get(1), libgedit_res.get(1)],
  include_directories: rootdir,
  dependencies: libgedit_deps,
  c_args: libgedit_c_args,
  link_args: libgedit_link_args,
  install: true,
  install_dir: pkglibdir,
)

if generate_gir == true
  libgedit_public_h += [
    libgedit_enums.get(1),
  ]

  libgedit_gir_includes = [
    'Gtk-3.0',
    'GtkSource-4',
  ]

  libgedit_gir = gnome.generate_gir(
    libgedit_sha,
    sources: libgedit_sources + libgedit_public_h,
    nsversion: '3.0',
    namespace: 'Gedit',
    symbol_prefix: 'gedit',
    identifier_prefix: 'Gedit',
    export_packages: [package_string],
    includes: libgedit_gir_includes,
    link_with: libgedit_sha,
    install: true,
    install_dir_gir: join_paths(pkgdatadir, 'gir-1.0'),
    install_dir_typelib: join_paths(pkglibdir, 'girepository-1.0'),
  )

  python3.install_sources(
    'Gedit.py',
    subdir: join_paths(
      'gi',
      'overrides',
    )
  )

  libgedit_dep_sources += [
    libgedit_gir,
  ]

  if generate_vapi == true
    libgedit_vapi_packages = [
      'gio-2.0',
      'atk',
      'gdk-3.0',
      'gtk+-3.0',
      'gtksourceview-4',
    ]

    libgedit_vapi = gnome.generate_vapi(
      package_name,
      sources: libgedit_gir.get(0),
      packages: libgedit_vapi_packages,
      install: true,
      install_dir: vapidir,
    )
  endif
endif

libgedit_dep = declare_dependency(
  link_with: libgedit_sha,
  include_directories: rootdir,
  dependencies: libgedit_deps,
  sources: libgedit_dep_sources,
)

libgedit_pc = pkg.generate(
  name: package_name,
  description: 'GNOME text editor',
  requires: ['gtksourceview-4', 'libpeas-1.0', 'libpeas-gtk-1.0'],
  version: meson.project_version(),
  subdirs: package_string,
  libraries: libgedit_sha,
  install_dir: pkgconfigdir,
)

# FIXME: https://github.com/mesonbuild/meson/issues/1687
custom_target(
  'org.gnome.gedit.enums.xml',
  input : libgedit_sources + libgedit_public_h + ['gedit-notebook.h'],
  output: 'org.gnome.gedit.enums.xml',
  capture: true,
  command: [
    'glib-mkenums',
    '--comments', '<!-- @comment@ -->',
    '--fhead', '<schemalist>',
    '--vhead', '  <@type@ id="org.gnome.gedit.@EnumName@">',
    '--vprod', '    <value nick="@valuenick@" value="@valuenum@"/>',
    '--vtail', '  </@type@>',
    '--ftail', '</schemalist>',
    '@INPUT@'
  ],
  install: true,
  install_dir: join_paths(
    glibdir,
    'schemas',
  )
)

gedit_sources = [
  'gedit.c',
]

gedit_c_args = [
  '-DHAVE_CONFIG_H',
]

if windowing_target == 'quartz'
  gedit_c_args += [
    '-DOS_OSX=1',
  ]
endif

gedit_deps = [
  libgedit_dep,
]

gedit_exe = executable(
  package_name,
  gedit_sources,
  dependencies: gedit_deps,
  c_args: gedit_c_args,
  install: true,
  install_dir: bindir,
  install_rpath: pkglibdir,
  gui_app: true,
)
