libpldmpp_sources = files('firmware_update.cpp', 'types.cpp', 'utils.cpp')

libpldmpp = library(
    'pldm++',
    libpldmpp_sources,
    implicit_include_directories: false,
    include_directories: [include_root, include_directories('.')],
    link_args: libpldm_link_args,
    version: meson.project_version(),
    dependencies: [libpldm_dep],
    gnu_symbol_visibility: 'hidden',
    install: true,
)

libpldmpp_dep = declare_dependency(
    include_directories: include_root,
    link_with: libpldmpp,
)

c_suite = meson.get_compiler('c').get_id()
cpp_suite = meson.get_compiler('cpp').get_id()

if get_option('tests') and get_option('abi-compliance-check') and c_suite == cpp_suite and c_suite == 'gcc'

    message('running C++ binding ABI compliance check')

    host = host_machine.cpu_family()
    baseline_dump_cpp = meson.project_source_root() / 'abi' / host / 'libpldm++/@0@.dump'.format(
        cpp_suite,
    )

    current_dump_cpp = custom_target(
        'abi-dump-cpp',
        input: libpldmpp,
        output: 'current.dump',
        command: [
            abi_dumper,
            '-mixed-headers',
            '-include-paths',
            meson.project_source_root() / 'bindings/cpp',
            '-public-headers',
            meson.project_source_root() / 'include/libpldm++',
            '-sort',
            '@INPUT@',
            '-o',
            '@OUTPUT@',
            '-lver',
            meson.project_version(),
        ],
    )
    abi_compliance_cpp = custom_target(
        'abi-compliance-cpp',
        input: [baseline_dump_cpp, current_dump_cpp],
        output: 'abi-compliance-cpp',
        command: [
            abi_compliance_checker,
            '-l',
            meson.project_name() + 'plus',
            '-old',
            '@INPUT0@',
            '-new',
            '@INPUT1@',
        ],
        build_by_default: true,
    )
endif
