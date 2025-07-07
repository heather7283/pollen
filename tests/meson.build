test_sources = [
  '00_basic_fd.c',
  '01_invalid_signal.c',
  '02_preserve_sigmask.c',
  '03_idle_callback.c',
  '04_idle_priority.c',
  '05_closure.c',
  '06_signal.c',
  '07_timer.c',
  '08_more_signals.c',
]

# needed for ##__VA_ARGS__
add_project_arguments('-Wno-gnu-zero-variadic-macro-arguments', language: 'c')

add_project_arguments('-Wno-unused-parameter', language: 'c')

foreach test_source: test_sources
  test_name = test_source.split('.')[0]
  test_exe = executable(test_name, test_source, include_directories: ['..'])
  test(test_name, test_exe)
endforeach

