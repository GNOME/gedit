plugin_filebrowser_headers = files(
  'gedit-file-bookmarks-store.h',
  'gedit-file-browser-error.h',
  'gedit-file-browser-store.h',
  'gedit-file-browser-view.h',
  'gedit-file-browser-widget.h',
  'gedit-file-browser-utils.h',
  'gedit-file-browser-plugin.h',
  'gedit-file-browser-messages.h',
)

plugin_filebrowser_c_files = files(
  'gedit-file-bookmarks-store.c',
  'gedit-file-browser-messages.c',
  'gedit-file-browser-plugin.c',
  'gedit-file-browser-store.c',
  'gedit-file-browser-utils.c',
  'gedit-file-browser-view.c',
  'gedit-file-browser-widget.c',
)

subdir('messages')

libfilebrowser_register_enums = gnome.mkenums(
  'gedit-file-browser-enum-register',
  sources: plugin_filebrowser_headers,
  c_template: 'gedit-file-browser-enum-register.c.template',
)

libfilebrowser_type_enums = gnome.mkenums(
  'gedit-file-browser-enum-types',
  depends : [libfilebrowser_register_enums],
  sources: plugin_filebrowser_headers,
  h_template: 'gedit-file-browser-enum-types.h.template',
  c_template: 'gedit-file-browser-enum-types-stage1.c.template',
)

# cat won't work on Windows so this
# will need to be reimplemented as a script
cat = find_program('cat')

# Combine the 2 C mkenums templates together before compiling
libfilebrowser_enums_c = custom_target('libfilebrowser_enums_c',
  input: [libfilebrowser_type_enums.get(0),
          libfilebrowser_register_enums],
  output: 'gedit-file-browser-enum-types.c',
  command: [cat, '@INPUT0@', '@INPUT1@'],
  # redirects the command output since we can't use >> here
  capture: true,
)

plugin_filebrowser_c_files += [
  libfilebrowser_enums_c,
  libfilebrowser_type_enums.get(1),
]

subdir('resources')

libfilebrowser_sha = shared_module(
  'filebrowser',
  sources: plugin_filebrowser_c_files,
  include_directories: root_include_dir,
  dependencies: libgedit_dep,
  install: true,
  install_dir: join_paths(
    pkglibdir,
    'plugins',
  ),
  name_suffix: module_suffix,
)

# FIXME: https://github.com/mesonbuild/meson/issues/1687
custom_target(
  'org.gnome.gedit.plugins.filebrowser.enums.xml',
  input : plugin_filebrowser_c_files + plugin_filebrowser_headers,
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

filebrowser_gschema_file = files('org.gnome.gedit.plugins.filebrowser.gschema.xml')
install_data(
  filebrowser_gschema_file,
  install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'glib-2.0/schemas')
)

if xmllint.found()
  test(
    'validate-filebrowser-gschema',
    xmllint,
    args: [
      '--noout',
      '--dtdvalid', gschema_dtd,
      filebrowser_gschema_file,
    ]
  )
endif

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
