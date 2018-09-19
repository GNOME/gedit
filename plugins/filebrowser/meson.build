libfilebrowser_public_h = files(
  'gedit-file-bookmarks-store.h',
  'gedit-file-browser-error.h',
  'gedit-file-browser-store.h',
  'gedit-file-browser-view.h',
  'gedit-file-browser-widget.h',
  'gedit-file-browser-utils.h',
  'gedit-file-browser-plugin.h',
  'gedit-file-browser-messages.h',
)

libfilebrowser_sources = files(
  'gedit-file-bookmarks-store.c',
  'gedit-file-browser-messages.c',
  'gedit-file-browser-plugin.c',
  'gedit-file-browser-store.c',
  'gedit-file-browser-utils.c',
  'gedit-file-browser-view.c',
  'gedit-file-browser-widget.c',
)

libfilebrowser_deps = [
  libgedit_dep,
]

libfilebrowser_c_args = [
  '-DHAVE_CONFIG_H',
]

subdir('messages')

libfilebrowser_enums = gnome.mkenums(
  'gedit-file-browser-enum-types',
  sources: libfilebrowser_public_h,
  h_template: 'gedit-file-browser-enum-types.h.template',
  c_template: 'gedit-file-browser-enum-types.c.template',
)

libfilebrowser_sources += [
  libfilebrowser_enums.get(0),
]

subdir('resources')

libfilebrowser_sha = shared_module(
  'filebrowser',
  sources: libfilebrowser_sources,
  include_directories: rootdir,
  dependencies: libfilebrowser_deps,
  c_args: libfilebrowser_c_args,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)

# FIXME: https://github.com/mesonbuild/meson/issues/1687
custom_target(
  'org.gnome.gedit.plugins.filebrowser.enums.xml',
  input : libfilebrowser_sources + libfilebrowser_public_h,
  output: 'org.gnome.gedit.plugins.filebrowser.enums.xml',
  capture: true,
  command: [
    'glib-mkenums',
    '--comments', '<!-- @comment@ -->',
    '--fhead', '<schemalist>',
    '--vhead', '  <@type@ id="org.gnome.gedit.plugins.filebrowser.@EnumName@">',
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

configure_file(
  input: 'org.gnome.gedit.plugins.filebrowser.gschema.xml.in',
  output: 'org.gnome.gedit.plugins.filebrowser.gschema.xml',
  configuration: gschema_xml,
  install: true,
  install_dir: join_paths(
    glibdir,
    'schemas',
  )
)

custom_target(
  'filebrowser.plugin',
  input: 'filebrowser.plugin.desktop.in',
  output: 'filebrowser.plugin',
  command: msgfmt_plugin_cmd,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  )
)
