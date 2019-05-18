externaltools_tests = {
  'LinkParser': files('testlinkparsing.py'),
}

externaltools_srcdir = join_paths(
  srcdir,
  'plugins',
  'externaltools',
  'tools',
)

foreach test_name, test_script : externaltools_tests
  test(
    'test-externaltools-@0@'.format(test_name),
    python3,
    args: [test_script],
    env: [
      'PYTHONPATH=@0@'.format(externaltools_srcdir),
    ]
  )
endforeach
