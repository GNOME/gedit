glib_docpath = dependency('glib-2.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/glib'
gobject_docpath = dependency('gobject-2.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gobject'
gio_docpath = dependency('gio-2.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gio'
gdk_docpath = dependency('gdk-3.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gdk3'
gtk_docpath = dependency('gtk+-3.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gtk3'
gsv_docpath = dependency('gtksourceview-4').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/gtksourceview-4.0'
amtk_docpath = dependency('amtk-5').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/amtk-5.0'
tepl_docpath = dependency('tepl-5').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/tepl-5'
libpeas_docpath = dependency('libpeas-1.0').get_pkgconfig_variable('prefix') / 'share/gtk-doc/html/libpeas'

gedit_doc_deps = [
  gio_dep,
  gtk_dep,
  gtksourceview_dep,
  tepl_dep,
  libpeas_gtk_dep,
]

gedit_doc_dep = declare_dependency(
  link_with: libgedit_sha,
  include_directories: root_include_dir,
  dependencies: gedit_doc_deps,
  sources: libgedit_dep_sources,
)

libgedit_reference_ignored_h = [
  'gedit-app-osx.h',
  'gedit-app-win32.h',
  'gedit-close-confirmation-dialog.h',
  'gedit-dirs.h',
  'gedit-document-private.h',
  'gedit-documents-panel.h',
  'gedit-encoding-items.h',
  'gedit-encodings-dialog.h',
  'gedit-enum-types.h',
  'gedit-factory.h',
  'gedit-file-chooser-dialog-gtk.h',
  'gedit-file-chooser-dialog-osx.h',
  'gedit-file-chooser-dialog.h',
  'gedit-highlight-mode-dialog.h',
  'gedit-highlight-mode-selector.h',
  'gedit-history-entry.h',
  'gedit-io-error-info-bar.h',
  'gedit-menu-stack-switcher.h',
  'gedit-multi-notebook.h',
  'gedit-notebook-popup-menu.h',
  'gedit-notebook-stack-switcher.h',
  'gedit-notebook.h',
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
  'gedit-view-frame.h',
  'gedit-window-private.h',
]

libgedit_reference_scan_args = [
  '--rebuild-types',
  '--deprecated-guards=GEDIT_DISABLE_DEPRECATED',
  '--ignore-headers=' + ' '.join(libgedit_reference_ignored_h),
]

libgedit_reference_mkdb_args = [
  '--xml-mode',
  '--output-format=xml',
  '--name-space=gedit',
]

gtkdocdir = join_paths(get_option('prefix'), get_option('datadir'), 'gtk-doc')

gnome.gtkdoc(
  meson.project_name(),
  main_sgml: 'gedit-docs.xml',
  src_dir: join_paths(srcdir, 'gedit'),
  dependencies: gedit_doc_dep,
  scan_args: libgedit_reference_scan_args,
  mkdb_args: libgedit_reference_mkdb_args,
  fixxref_args: [
    '--html-dir=@0@'.format(join_paths(gtkdocdir, 'html', package_string)),
    '--extra-dir=@0@'.format(glib_docpath),
    '--extra-dir=@0@'.format(gobject_docpath),
    '--extra-dir=@0@'.format(gio_docpath),
    '--extra-dir=@0@'.format(gdk_docpath),
    '--extra-dir=@0@'.format(gtk_docpath),
    '--extra-dir=@0@'.format(gsv_docpath),
    '--extra-dir=@0@'.format(amtk_docpath),
    '--extra-dir=@0@'.format(tepl_docpath),
    '--extra-dir=@0@'.format(libpeas_docpath),
  ],
  content_files : [
    'api-breaks.xml'
  ],
  install: true,
  install_dir: join_paths(
    gtkdocdir,
    'html',
    meson.project_name(),
  )
)
