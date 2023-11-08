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

if get_option('tests').allowed()
  c_suite = meson.get_compiler('c').get_id()
  cpp_suite = meson.get_compiler('cpp').get_id()

  if get_option('abi-compliance-check').allowed() and c_suite == cpp_suite and c_suite == 'gcc'
    host = host_machine.cpu_family()
    baseline_dump = meson.project_source_root() / 'abi' / host / '@0@.dump'.format(c_suite)

    abi_dumper = find_program('abi-dumper',
                              native: true,
                              required: get_option('abi-compliance-check'))
    abi_compliance_checker = find_program('abi-compliance-checker',
                                          native: true,
                                          required: get_option('abi-compliance-check'))

    test_abi_compliance = abi_dumper.found() and \
                          abi_compliance_checker.found() and \
                          import('fs').is_file(baseline_dump)

    if test_abi_compliance
      current_dump = custom_target('abi-dump',
                               input: libpldm,
                               output: 'current.dump',
                               command: [ abi_dumper,
                                          '-public-headers',
                                          meson.project_source_root() / 'include',
                                          '@INPUT@',
                                          '-o',
                                          '@OUTPUT@',
                                          '-lver',
                                          meson.project_version()])
      abi_compliance = custom_target('abi-compliance',
                                     input: [baseline_dump, current_dump],
                                     output: 'abi-compliance',
                                     command: [ abi_compliance_checker,
                                                '-l',
                                                meson.project_name(),
                                                '-old',
                                                '@INPUT0@',
                                                '-new',
                                                '@INPUT1@' ],
                                                build_by_default: true)
    endif
  endif
endif
