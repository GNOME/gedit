libgedit_public_headers = [
  'gedit-app-activatable.h',
  'gedit-app.h',
  'gedit-commands.h',
  'gedit-debug.h',
  'gedit-document.h',
  'gedit-encodings-combo-box.h',
  'gedit-macros.h',
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
]

libgedit_public_sources = [
  'gedit-app-activatable.c',
  'gedit-app.c',
  'gedit-commands-file.c',
  'gedit-debug.c',
  'gedit-document.c',
  'gedit-encodings-combo-box.c',
  'gedit-menu-extension.c',
  'gedit-message-bus.c',
  'gedit-message.c',
  'gedit-progress-info-bar.c',
  'gedit-statusbar.c',
  'gedit-tab.c',
  'gedit-utils.c',
  'gedit-view-activatable.c',
  'gedit-view.c',
  'gedit-view-centering.c',
  'gedit-window-activatable.c',
  'gedit-window.c',
]

libgedit_private_headers = [
  'gedit-app-osx.h',
  'gedit-app-win32.h',
  'gedit-close-confirmation-dialog.h',
  'gedit-dirs.h',
  'gedit-document-private.h',
  'gedit-documents-panel.h',
  'gedit-encoding-items.h',
  'gedit-encodings-dialog.h',
  'gedit-file-chooser-dialog-gtk.h',
  'gedit-file-chooser-dialog.h',
  'gedit-file-chooser.h',
  'gedit-file-chooser-open-dialog.h',
  'gedit-file-chooser-open.h',
  'gedit-file-chooser-open-native.h',
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
]

libgedit_private_sources = [
  'gedit-close-confirmation-dialog.c',
  'gedit-commands-documents.c',
  'gedit-commands-edit.c',
  'gedit-commands-file-print.c',
  'gedit-commands-help.c',
  'gedit-commands-search.c',
  'gedit-commands-view.c',
  'gedit-dirs.c',
  'gedit-documents-panel.c',
  'gedit-encoding-items.c',
  'gedit-encodings-dialog.c',
  'gedit-file-chooser.c',
  'gedit-file-chooser-dialog.c',
  'gedit-file-chooser-dialog-gtk.c',
  'gedit-file-chooser-open.c',
  'gedit-file-chooser-open-dialog.c',
  'gedit-file-chooser-open-native.c',
  'gedit-highlight-mode-dialog.c',
  'gedit-highlight-mode-selector.c',
  'gedit-history-entry.c',
  'gedit-io-error-info-bar.c',
  'gedit-menu-stack-switcher.c',
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
  'gedit-recent.c',
  'gedit-replace-dialog.c',
  'gedit-settings.c',
  'gedit-status-menu-button.c',
  'gedit-tab-label.c',
  'gedit-view-frame.c',
]

libgedit_c_args = []
libgedit_link_args = []

libgedit_deps = [
  deps_basic_list,
  libgd_dep,
  libxml_dep,
]

if host_machine.system() == 'darwin'
  libgedit_private_sources += [
    'gedit-app-osx.m',
  ]
  libgedit_c_args += [
    '-DOS_OSX=1',
  ]
  libgedit_link_args += [
    '-Wl,-framework', '-Wl,Foundation',
    '-Wl,-framework', '-Wl,AppKit',
  ]
  libgedit_deps += [
    dependency('gtk-mac-integration-gtk3'),
  ]
elif host_machine.system() == 'windows'
  libgedit_private_sources += [
    'gedit-app-win32.c',
  ]
endif

headers_install_dir = get_option('includedir') / 'gedit-@0@/gedit/'.format(api_version)
install_headers(
  libgedit_public_headers,
  install_dir: headers_install_dir,
)

libgedit_public_enum_types = gnome.mkenums_simple(
  'gedit-enum-types',
  sources: libgedit_public_headers,
  install_header: true,
  install_dir: headers_install_dir,
)

libgedit_private_enum_types = gnome.mkenums_simple(
  'gedit-enum-types-private',
  sources: ['gedit-notebook.h'],
)

libgedit_private_headers += 'gedit-enum-types-private.h'

subdir('resources')

libgedit_shared_lib = shared_library(
  'gedit-@0@'.format(api_version),
  [libgedit_public_sources,
   libgedit_private_sources,
   libgedit_public_enum_types,
   libgedit_private_enum_types,
   libgedit_gresources],
  include_directories: root_include_dir,
  dependencies: libgedit_deps,
  c_args: libgedit_c_args,
  link_args: libgedit_link_args,
  install: true,
  install_dir: get_option('libdir') / 'gedit',
)

# GObject Introspection
libgedit_gir = gnome.generate_gir(
  libgedit_shared_lib,
  sources: [
    libgedit_public_headers,
    libgedit_public_sources,
    libgedit_public_enum_types,
  ],
  nsversion: '3.0',
  namespace: 'Gedit',
  symbol_prefix: 'gedit',
  identifier_prefix: 'Gedit',
  export_packages: 'gedit-@0@'.format(api_version),
  includes: ['Gtk-3.0', 'GtkSource-4'],
  link_with: libgedit_shared_lib,
  install: true,
  install_dir_gir: get_option('datadir') / 'gedit/gir-1.0',
  install_dir_typelib: get_option('libdir') / 'gedit/girepository-1.0',
)

if get_option('python')
  python3.install_sources(
    'Gedit.py',
    subdir: 'gi/overrides',
  )
endif

# Vala API
libgedit_vapi = gnome.generate_vapi(
  'gedit',
  sources: libgedit_gir[0],
  packages: ['gio-2.0', 'atk', 'gdk-3.0', 'gtk+-3.0', 'gtksourceview-4'],
  install: true,
  install_dir: get_option('datadir') / 'vala/vapi',
)

libgedit_dep = declare_dependency(
  include_directories: root_include_dir,
  link_with: libgedit_shared_lib,
  sources: [libgedit_public_enum_types[1], libgedit_private_enum_types[1]],
  dependencies: libgedit_deps,
)

pkg_config.generate(
  libgedit_shared_lib,
  filebase: 'gedit',
  name: 'gedit',
  description: 'GNOME text editor',
  subdirs: 'gedit-@0@'.format(api_version),
  libraries: libgedit_public_deps,
  install_dir: get_option('libdir') / 'pkgconfig'
)

# FIXME: https://github.com/mesonbuild/meson/issues/1687
custom_target(
  'org.gnome.gedit.enums.xml',
  input : ['gedit-notebook.h'],
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
  install_dir: get_option('datadir') / 'glib-2.0/schemas'
)

gedit_c_args = []
if host_machine.system() == 'darwin'
  gedit_c_args += '-DOS_OSX=1'
endif

executable(
  'gedit',
  'gedit.c',
  dependencies: libgedit_dep,
  c_args: gedit_c_args,
  install: true,
  install_rpath: get_option('prefix') / get_option('libdir') / 'gedit',
  gui_app: true,
)
