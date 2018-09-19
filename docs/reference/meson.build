glib_prefix = glib_dep.get_pkgconfig_variable('prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html', 'glib')

gobject_prefix = gobject_dep.get_pkgconfig_variable('prefix')
gobject_docpath = join_paths(gobject_prefix, 'share', 'gtk-doc', 'html', 'gobject')

gtk_prefix = gtk_dep.get_pkgconfig_variable('prefix')
gtk_docpath = join_paths(gobject_prefix, 'share', 'gtk-doc', 'html', 'gtk')

libgedit_reference_ignored_h = [
  'gedit-app-osx.h',
  'gedit-app-win32.h',
  'gedit-app-x11.h',
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

libgedit_reference_fixxref_args = [
  '--html-dir=@0@'.format(join_paths(gtkdocdir, 'html', package_string)),
  '--extra-dir=@0@'.format(glib_docpath),
  '--extra-dir=@0@'.format(gobject_docpath),
  '--extra-dir=@0@'.format(gtk_docpath),
]

gnome.gtkdoc(
  package_name,
  main_sgml: 'gedit-docs.xml',
  src_dir: join_paths(srcdir, 'gedit'),
  dependencies: libgedit_dep,
  scan_args: libgedit_reference_scan_args,
  mkdb_args: libgedit_reference_mkdb_args,
  fixxref_args: libgedit_reference_fixxref_args,
  install: true,
  install_dir: join_paths(
    gtkdocdir,
    'html',
    package_string,
  )
)