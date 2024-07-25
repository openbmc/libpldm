project('libpldm', 'c',
    default_options: [
      'debug=true',
      'optimization=g',
      'warning_level=3',
      'werror=true',
      'cpp_std=c++20',
      'c_std=c17',
      'b_ndebug=if-release',
      'tests=' + (meson.is_subproject() ? 'disabled' : 'enabled'),
    ],
    version: '0.8.0',
    meson_version: '>=1.1.0',
)

if get_option('tests').allowed()
  add_languages('cpp')
endif

# For memmem() in src/msgbuf.h
add_project_arguments('-D_GNU_SOURCE', language: ['c'])

compiler = meson.get_compiler('c')
conf = configuration_data()
if compiler.has_header('poll.h')
  conf.set('PLDM_HAS_POLL', 1)
endif

# ABI control
visible =  '__attribute__((visibility("default")))'
libpldm_deprecated_aliases = []
if get_option('abi').contains('deprecated')
  conf.set('LIBPLDM_ABI_DEPRECATED', visible)
  add_project_arguments('-DLIBPLDM_API_DEPRECATED', language: ['c', 'cpp'])
  libpldm_deprecated_aliases += [
    ['get_fru_record_by_option_check', 'get_fru_record_by_option'],
  ]
else
  conf.set('LIBPLDM_ABI_DEPRECATED', '')
endif
conf.set('LIBPLDM_ABI_STABLE', visible) # Always expose the stable symbols
if get_option('abi').contains('testing')
  conf.set('LIBPLDM_ABI_TESTING', visible)
  add_project_arguments('-DLIBPLDM_API_TESTING', language: ['c', 'cpp'])
else
  conf.set('LIBPLDM_ABI_TESTING', '')
endif

config = configure_file(output: 'config.h',
  configuration: conf
)

add_project_arguments('-include', '@0@'.format(config), language: 'c')

libpldm_include_dir = include_directories('include', is_system: true)

subdir('include')
subdir('src')

if get_option('tests').allowed()
  subdir('tests')
endif

install_subdir('instance-db',
               install_mode: 'r--r--r--',
               install_dir: get_option('datadir') / meson.project_name())
