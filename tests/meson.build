gtest_dep = dependency('gtest', main: true, disabler: true, required: false)
gmock_dep = dependency('gmock', disabler: true, required: false)
if not gtest_dep.found() or not gmock_dep.found()
    gtest_proj = import('cmake').subproject('googletest', required: true)
    gtest_dep = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest'),
                gtest_proj.dependency('gtest_main'),
            ]
        )
        gmock_dep = gtest_proj.dependency('gmock')
endif


tests = [
  'instance-id',
  'libpldm_base_test',
  'libpldm_bios_table_test',
  'libpldm_bios_test',
  'libpldm_firmware_update_test',
  'libpldm_fru_test',
  'libpldm_pdr_test',
  'libpldm_platform_test',
  'libpldm_utils_test',
  'msgbuf',
  'responder',
]

# The 'test' transport APIs will never be marked stable as they are just for
# testing.
if get_option('abi').contains('testing')
  subdir('transport')
endif

if get_option('oem-ibm').allowed()
  subdir('oem/ibm')
endif

if get_option('oem-meta').allowed()
  subdir('oem/meta')
endif

test_include_dirs = [ libpldm_include_dir, include_directories('../src') ]

foreach t : tests
  test(t, executable(t.underscorify(), t + '.cpp',
                     implicit_include_directories: false,
                     include_directories: test_include_dirs,
                     dependencies: [
                         libpldm_dep,
                         gtest_dep,
                         gmock_dep]),
       workdir: meson.current_source_dir())
endforeach

test('bios_table_iter', executable('bios_table_iter',
                                   'bios_table_iter.c',
                                   implicit_include_directories: false,
                                   include_directories: test_include_dirs))

test('msgbuf_generic', executable('msgbuf_generic',
                                  'msgbuf_generic.c',
                                  implicit_include_directories: false,
                                  include_directories: test_include_dirs))
