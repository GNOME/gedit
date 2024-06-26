externaltools_tools = [
  'build',
  'remove-trailing-spaces',
  'send-to-fpaste',
]

if host_machine.system() == 'darwin'
  externaltools_tools += [
    'open-terminal-here-osx',
  ]
elif host_machine.system() != 'windows'
  externaltools_tools += [
    'open-terminal-here',
    'run-command',
  ]
endif

foreach tool_name: externaltools_tools
  dektop_file = custom_target(
    '@0@.desktop'.format(tool_name),
    input: '@0@.desktop.in'.format(tool_name),
    output: '@0@.desktop'.format(tool_name),
    command: msgfmt_externaltools_cmd,
    install: false,
  )

  custom_target(
    '@0@.tool'.format(tool_name),
    input: '@0@.tool.in'.format(tool_name),
    output: '@0@'.format(tool_name),
    depends: dektop_file,
    command: [
      merge_tool_prg,
      '@INPUT@',
      dektop_file.full_path(),
    ],
    capture: true,
    install: true,
    install_dir: get_option('datadir') / 'gedit/plugins/externaltools/tools',
  )
endforeach
