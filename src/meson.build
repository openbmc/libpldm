libpldm_sources = files('bcd.c', 'control.c', 'edac.c', 'responder.c', 'utils.c')

subdir('dsp')

if get_option('transport')
    subdir('requester')
    subdir('transport')
endif

if get_option('oem').contains('ibm')
    subdir('oem/ibm')
endif

if get_option('oem').contains('meta')
    subdir('oem/meta')
endif

subdir('firmware_device')
subdir('environ')

libpldm_link_args = []
foreach alias : libpldm_deprecated_aliases
    libpldm_link_args += '-Wl,--defsym=@0@=@1@'.format(alias[0], alias[1])
endforeach

libpldm = library(
    'pldm',
    libpldm_sources,
    implicit_include_directories: false,
    include_directories: [include_root, include_directories('.')],
    link_args: libpldm_link_args,
    version: meson.project_version(),
    gnu_symbol_visibility: 'hidden',
    install: true,
)

libpldm_dep = declare_dependency(
    include_directories: include_root,
    link_with: libpldm,
)

import('pkgconfig').generate(
    name: 'libpldm',
    description: 'PLDM protocol encode/decode C lib',
    version: meson.project_version(),
    libraries: libpldm,
)

if get_option('tests')
    c_suite = meson.get_compiler('c').get_id()
    cpp_suite = meson.get_compiler('cpp').get_id()

    if get_option('abi-compliance-check') and c_suite == cpp_suite and c_suite == 'gcc'
        host = host_machine.cpu_family()
        baseline_dump = meson.project_source_root() / 'abi' / host / '@0@.dump'.format(
            c_suite,
        )

        abi_dumper = find_program(
            'abi-dumper',
            native: true,
            required: get_option('abi-compliance-check'),
        )
        abi_compliance_checker = find_program(
            'abi-compliance-checker',
            native: true,
            required: get_option('abi-compliance-check'),
        )

        test_abi_compliance = abi_dumper.found() and abi_compliance_checker.found() and import(
            'fs',
        ).is_file(
            baseline_dump,
        )

        if test_abi_compliance
            current_dump = custom_target(
                'abi-dump',
                input: libpldm,
                output: 'current.dump',
                command: [
                    abi_dumper,
                    '-mixed-headers',
                    '-include-paths',
                    meson.project_source_root() / 'src',
                    '-public-headers',
                    meson.project_source_root() / 'include',
                    '-sort',
                    '@INPUT@',
                    '-o',
                    '@OUTPUT@',
                    '-lver',
                    meson.project_version(),
                ],
            )
            abi_compliance = custom_target(
                'abi-compliance',
                input: [baseline_dump, current_dump],
                output: 'abi-compliance',
                command: [
                    abi_compliance_checker,
                    '-l',
                    meson.project_name(),
                    '-old',
                    '@INPUT0@',
                    '-new',
                    '@INPUT1@',
                ],
                build_by_default: true,
            )
        endif
    endif
endif
