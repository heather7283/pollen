project('pollen','c',
        license: 'MIT',
        version: '3.0.0',
        meson_version: '>=1.1.0',
        default_options: ['warning_level=3'])

pollen_dep = declare_dependency(include_directories: include_directories('.'),
                                compile_args: ['-D_GNU_SOURCE'])
meson.override_dependency('pollen', pollen_dep)

if get_option('test')
  subdir('tests')
endif

if not meson.is_subproject()
  install_headers('pollen.h')

  pkg = import('pkgconfig')
  pkg.generate(name: 'pollen',
               filebase: 'pollen',
               description: 'Event loop abstraction library built on epoll.')
endif

