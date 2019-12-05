msgfmt_plugin_cmd = [
  find_program('msgfmt'),
  '--desktop',
  '--keyword=Name',
  '--keyword=Description',
  '--template=@INPUT@',
  '-d', join_paths(srcdir, 'po'),
  '--output=@OUTPUT@'
]

if host_machine.system() == 'windows'
  subdir('checkupdate')
endif

subdir('docinfo')
subdir('externaltools')
subdir('filebrowser')
subdir('modelines')
subdir('pythonconsole')
subdir('quickhighlight')
subdir('quickopen')
subdir('snippets')
subdir('sort')
subdir('spell')
subdir('time')
