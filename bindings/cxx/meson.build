
libpldmcxx = library('pldmcxx',
  'utils.cpp',
  'package_parser.cpp',
  include_directories: ['.'],
  dependencies: [
    libpldm_dep,
  ],
  install: true,
)

libpldmcxx_headers = files(
  'package_parser.hpp',
  'types.hpp',
)

install_headers(libpldmcxx_headers, subdir: 'libpldmcxx', preserve_path: true)
