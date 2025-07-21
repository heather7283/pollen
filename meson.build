project('pollen','c',
        license: 'MIT',
        version: '2.1.1',
        default_options: ['warning_level=3'])

subdir('tests')

pollen = declare_dependency(include_directories: '.')
meson.override_dependency('pollen', pollen)

if not meson.is_subproject()
  install_headers('pollen.h')

  pkg = import('pkgconfig')
  pkg.generate(name: 'pollen',
               filebase: 'pollen',
               description: 'Event loop abstraction library built on epoll.')
endif

