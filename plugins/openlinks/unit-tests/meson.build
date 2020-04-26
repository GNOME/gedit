unit_tests = [
  'test-open-links'
]

foreach test_name : unit_tests
  test_exe = executable(
    test_name,
    test_name + '.c',
    dependencies: libgedit_dep,
    c_args: '-DUNIT_TESTS_SOURCE_DIR="@0@"'.format(meson.current_source_dir())
  )
  test(test_name, test_exe)
endforeach
