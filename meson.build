project(
    'libpldm',
    'c',
    default_options: {
        'debug': true,
        'optimization': 'g',
        'warning_level': '3',
        'werror': true,
        'cpp_std': 'c++23',
        'c_std': 'c17',
        'b_ndebug': 'if-release',
        'tests': not meson.is_subproject(),
    },
    version: '0.15.0',
    meson_version: '>=1.6.0',
)

if get_option('bindings').contains('cpp') or get_option('tests')
    add_languages('cpp', native: false)
endif

# For memmem() in src/msgbuf.h
add_project_arguments('-D_GNU_SOURCE', language: ['c'])

cc = meson.get_compiler('c')
if cc.has_argument('-Wvla')
    add_project_arguments('-Wvla', language: ['c'])
endif

if get_option('bindings').contains('cpp') or get_option('tests')
    cpp = meson.get_compiler('cpp')
    if cpp.has_argument('-Wno-psabi')
        add_project_arguments('-Wno-psabi', language: ['cpp'])
    endif
endif

# avoid using different compilers across C/C++
if get_option('bindings').contains('cpp')
    if (cc.get_id() != cpp.get_id())
        error(
            'unsupported combination: C compiler: ' + cc.get_id() + ' C++ compiler: ' + cpp.get_id(),
        )
    endif

    if get_option('tests') and cpp.get_id() == 'gcc'
        # when building tests, also dump class layout for inspection
        add_project_arguments('-fdump-lang-class', language: ['cpp'])
    endif
endif

# Fuzzing support
if get_option('test-methods').contains('fuzz')
    # For clang, -fcoverage-mapping requires -fprofile-instr-generate. That
    # requirement isn't honoured by meson's strategy for feature testing which
    # expects flags to be independent. Assume that if -fprofile-instr-generate
    # is supported then -fcoverage-mapping is as well.
    cc.get_supported_arguments(
        '-fsanitize=fuzzer',
        '-fprofile-instr-generate',
        checked: 'require',
    )
    cpp.get_supported_arguments(
        '-fsanitize=fuzzer',
        '-fprofile-instr-generate',
        checked: 'require',
    )
    add_project_arguments(
        '-fsanitize=fuzzer',
        '-fprofile-instr-generate',
        '-fcoverage-mapping',
        language: ['c', 'cpp'],
    )

    # Avoid adding -fsanitize=fuzzer to the project link arguments. Rather,
    # require that the flag is provided in the link arguments for relevant
    # executables. This avoids link failures due to multiple definitions of
    # main() for executables whose purpose isn't fuzz testing.
    add_project_link_arguments(
        '-fprofile-instr-generate',
        '-fcoverage-mapping',
        language: ['c', 'cpp'],
    )
endif

conf = configuration_data()
conf.set10(
    'HAVE_LIBPLDM_ABI_DEPRECATED',
    get_option('abi').contains('deprecated'),
)
conf.set10('HAVE_LIBPLDM_ABI_TESTING', get_option('abi').contains('testing'))
if cc.has_header('linux/mctp.h')
    conf.set10(
        'HAVE_STRUCT_MCTP_FQ_ADDR',
        cc.has_type(
            'struct mctp_fq_addr',
            prefix: '#include <linux/mctp.h>',
        ),
        description: 'Is struct mctp_fq_addr available?',
    )
endif
config = configure_file(
    input: 'config.h.in',
    output: 'config.h',
    configuration: conf,
)
add_project_arguments('-include', config.full_path(), language: ['c', 'cpp'])

libpldm_deprecated_aliases = []

include_root = include_directories(get_option('api'), is_system: true)

subdir('include')
subdir('src')

if get_option('bindings').contains('cpp')
    subdir('bindings/cpp')
endif

subdir('tools')

doxygen = find_program('doxygen', required: false)
if doxygen.found()
    doxydata = configuration_data()
    doxydata.set('description', meson.project_name())
    doxydata.set('project', meson.project_name())
    doxydata.set('version', meson.project_version())
    doxydata.set(
        'sources',
        '@0@ @1@'.format(
            meson.project_source_root() / 'include',
            meson.project_source_root() / 'src',
        ),
    )
    doxyfile = configure_file(
        input: 'Doxyfile.in',
        output: 'Doxyfile',
        configuration: doxydata,
        install: false,
    )

    docs = custom_target(
        'docs',
        input: doxyfile,
        output: 'doxygen',
        command: [doxygen, doxyfile],
    )
endif

if get_option('tests')
    subdir('tests')
endif

install_subdir(
    'instance-db',
    install_mode: 'r--r--r--',
    install_dir: get_option('datadir') / meson.project_name(),
)
