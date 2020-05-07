html_dir = get_option('prefix') / gnome.gtkdoc_html_dir('gedit')

glib_docpath = dependency('glib-2.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/glib'
gobject_docpath = dependency('gobject-2.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gobject'
gio_docpath = dependency('gio-2.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gio'
gdk_docpath = dependency('gdk-3.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gdk3'
gtk_docpath = dependency('gtk+-3.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gtk3'
gsv_docpath = dependency('gtksourceview-4').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gtksourceview-4.0'
libpeas_docpath = dependency('libpeas-1.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/libpeas'

gedit_doc_deps = [
  gio_dep,
  gtk_dep,
  gtksourceview_dep,
  libpeas_gtk_dep,
  libxml_dep,
]

gedit_doc_dep = declare_dependency(
  link_with: libgedit_sha,
  include_directories: root_include_dir,
  dependencies: gedit_doc_deps,
  sources: libgedit_dep_sources,
)

libgedit_private_headers = [
  'gedit-app-osx.h',
  'gedit-app-win32.h',
  'gedit-close-confirmation-dialog.h',
  'gedit-dirs.h',
  'gedit-document-private.h',
  'gedit-documents-panel.h',
  'gedit-encoding-items.h',
  'gedit-encodings-dialog.h',
  'gedit-enum-types.h',
  'gedit-file-chooser-dialog-gtk.h',
  'gedit-file-chooser-dialog-osx.h',
  'gedit-file-chooser-dialog.h',
  'gedit-highlight-mode-dialog.h',
  'gedit-highlight-mode-selector.h',
  'gedit-history-entry.h',
  'gedit-io-error-info-bar.h',
  'gedit-menu-stack-switcher.h',
  'gedit-metadata-manager.h',
  'gedit-multi-notebook.h',
  'gedit-notebook-popup-menu.h',
  'gedit-notebook-stack-switcher.h',
  'gedit-notebook.h',
  'gedit-open-document-selector-helper.h',
  'gedit-open-document-selector-store.h',
  'gedit-open-document-selector.h',
  'gedit-open-menu-button.h',
  'gedit-pango.h',
  'gedit-plugins-engine.h',
  'gedit-preferences-dialog.h',
  'gedit-print-job.h',
  'gedit-print-preview.h',
  'gedit-recent.h',
  'gedit-replace-dialog.h',
  'gedit-settings.h',
  'gedit-small-button.h',
  'gedit-status-menu-button.h',
  'gedit-tab-label.h',
  'gedit-view-centering.h',
  'gedit-view-frame.h',
  'gedit-window-private.h',
]

gnome.gtkdoc(
  'gedit',
  main_xml: 'gedit-docs.xml',
  src_dir: include_directories('../../gedit/'),
  dependencies: gedit_doc_dep,
  scan_args: ['--rebuild-types'],
  fixxref_args: [
    '--html-dir=@0@'.format(html_dir),
    '--extra-dir=@0@'.format(glib_docpath),
    '--extra-dir=@0@'.format(gobject_docpath),
    '--extra-dir=@0@'.format(gio_docpath),
    '--extra-dir=@0@'.format(gdk_docpath),
    '--extra-dir=@0@'.format(gtk_docpath),
    '--extra-dir=@0@'.format(gsv_docpath),
    '--extra-dir=@0@'.format(libpeas_docpath),
  ],
  content_files: [
    'api-breaks.xml'
  ],
  ignore_headers: libgedit_private_headers,
  install: true,
)
