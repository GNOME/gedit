# Keep the autotools convention for shared module suffix because GModule
# depends on it: https://gitlab.gnome.org/GNOME/glib/issues/520
module_suffix = []
if host_machine.system() == 'darwin'
  module_suffix = 'so'
endif

msgfmt_plugin_cmd = [
  find_program('msgfmt'),
  '--desktop',
  '--keyword=Name',
  '--keyword=Description',
  '--template=@INPUT@',
  '-d', join_paths(srcdir, 'po'),
  '--output=@OUTPUT@'
]

subdir('docinfo')
subdir('filebrowser')
subdir('modelines')
subdir('openlinks')
subdir('quickhighlight')
subdir('sort')
if gspell_dep.found()
  subdir('spell')
endif
subdir('time')

if get_option('python')
  if get_option('plugin_externaltools')
    subdir('externaltools')
  endif

  subdir('pythonconsole')
  subdir('quickopen')
  subdir('snippets')
endif
