project('event_loop.h','c',
  license: 'MIT',
  version: '0.6.0',
  default_options: ['warning_level=3'])

# needed for ##__VA_ARGS__
add_project_arguments('-Wno-gnu-zero-variadic-macro-arguments', language: 'c')
# needed to get syntax highlighting and completions in implementation code
add_project_arguments('-DEVENT_LOOP_IMPLEMENTATION', language: 'c')

test_exe = executable('test_exe', 'test.c')
test('basic', test_exe)

