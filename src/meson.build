libpldm_sources = files(
  'base.c',
  'bios.c',
  'platform.c',
  'bios_table.c',
  'firmware_update.c',
  'fru.c',
  'pdr.c',
  'responder.c',
  'utils.c'
  )

subdir('requester')
subdir('transport')

if get_option('oem-ibm').allowed()
   subdir('oem/ibm')
endif

libpldm = library(
  'pldm',
   libpldm_sources,
   implicit_include_directories: false,
   include_directories: [
     libpldm_include_dir,
     include_directories('.')
   ],
   version: meson.project_version(),
   gnu_symbol_visibility: 'hidden',
   install: true
   )

libpldm_dep = declare_dependency(
  include_directories: libpldm_include_dir,
  link_with: libpldm)

import('pkgconfig').generate(
  name: 'libpldm',
  description: 'PLDM protocol encode/decode C lib',
  version: meson.project_version(),
  libraries: libpldm)
