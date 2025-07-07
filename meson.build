project('pollen','c',
  license: 'MIT',
  version: '1.0.0',
  meson_version: '>=1.1',
  default_options: ['warning_level=3'])

# needed for ##__VA_ARGS__
add_project_arguments('-Wno-gnu-zero-variadic-macro-arguments', language: 'c')

subdir('tests')

