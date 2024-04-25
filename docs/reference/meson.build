html_dir = get_option('prefix') / gnome.gtkdoc_html_dir('gedit')

glib_docpath = dependency('glib-2.0').get_variable(pkgconfig: 'prefix') / 'share/gtk-doc/html/glib'
gobject_docpath = dependency('gobject-2.0').get_variable(pkgconfig: 'prefix') / 'share/gtk-doc/html/gobject'
gio_docpath = dependency('gio-2.0').get_variable(pkgconfig: 'prefix') / 'share/gtk-doc/html/gio'
gdk_docpath = dependency('gdk-3.0').get_variable(pkgconfig: 'prefix') / 'share/gtk-doc/html/gdk3'
gtk_docpath = dependency('gtk+-3.0').get_variable(pkgconfig: 'prefix') / 'share/gtk-doc/html/gtk3'
gsv_docpath = dependency('libgedit-gtksourceview-300').get_variable(pkgconfig: 'prefix') / 'share/gtk-doc/html/libgedit-gtksourceview-300'
amtk_docpath = dependency('libgedit-amtk-5').get_variable(pkgconfig: 'prefix') / 'share/gtk-doc/html/libgedit-amtk-5'
tepl_docpath = dependency('libgedit-tepl-6').get_variable(pkgconfig: 'prefix') / 'share/gtk-doc/html/libgedit-tepl-6'
libpeas_docpath = dependency('libpeas-1.0').get_variable(pkgconfig: 'prefix') / 'share/gtk-doc/html/libpeas'

gedit_doc_dep = declare_dependency(
  link_with: libgedit_shared_lib,
  include_directories: root_include_dir,
  dependencies: deps_basic_list,
)

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
    '--extra-dir=@0@'.format(amtk_docpath),
    '--extra-dir=@0@'.format(tepl_docpath),
    '--extra-dir=@0@'.format(libpeas_docpath),
  ],
  content_files: [
    'api-breaks.xml'
  ],
  ignore_headers: libgedit_private_headers,
  install: true,
)
