msgfmt_externaltools_cmd = [
  find_program('msgfmt'),
  '--desktop',
  '--keyword=Name',
  '--keyword=Comment',
  '--template=@INPUT@',
  '-d', join_paths(srcdir, 'po'),
  '--output=@OUTPUT@'
]

merge_tool_prg = find_program(
  files('gedit-tool-merge.pl'),
)
