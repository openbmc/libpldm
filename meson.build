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
    meson_version: '>=1.4.0',
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

conf = configuration_data()
if cc.has_header('poll.h')
    conf.set('PLDM_HAS_POLL', 1)
endif

# ABI control
cc.has_function_attribute('visibility:default', required: true)
entrypoint = '__attribute__((visibility("default")))'

cpp = meson.get_compiler('cpp')

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


## Compile test until meson supports it via compiler.has_function_attribute()
have_tainted_args_test = '#if !__has_attribute(tainted_args)\n#error\n#endif'
if cc.compiles(
    have_tainted_args_test,
    args: '-E',
    name: 'compiler supports function attribute tainted_args',
)
    if get_option('bindings').contains('cpp')
        if cpp.compiles(
            have_tainted_args_test,
            args: '-E',
            name: 'compiler supports function attribute tainted_args',
        )
            entrypoint += ' __attribute__((tainted_args))'
        endif
    else
        entrypoint += ' __attribute__((tainted_args))'
    endif
endif

libpldm_deprecated_aliases = []
if get_option('abi').contains('deprecated')
    conf.set('LIBPLDM_ABI_DEPRECATED', entrypoint)
    conf.set(
        'LIBPLDM_ABI_DEPRECATED_UNSAFE',
        '__attribute__((visibility("default")))',
    )
    add_project_arguments('-DLIBPLDM_API_DEPRECATED', language: ['c', 'cpp'])
else
    conf.set('LIBPLDM_ABI_DEPRECATED', '')
    conf.set('LIBPLDM_ABI_DEPRECATED_UNSAFE', '')
endif
conf.set('LIBPLDM_ABI_STABLE', entrypoint)  # Always expose the stable symbols
if get_option('abi').contains('testing')
    conf.set('LIBPLDM_ABI_TESTING', entrypoint)
    add_project_arguments('-DLIBPLDM_API_TESTING', language: ['c', 'cpp'])
else
    conf.set('LIBPLDM_ABI_TESTING', '')
endif

config = configure_file(output: 'config.h', configuration: conf)

add_project_arguments('-include', config.full_path(), language: ['c', 'cpp'])

if get_option('tests')
    # we have to declare all project arguments before any build targets are defined
    add_project_arguments('-DLIBPLDM_GTEST_FRIENDS', language: ['cpp'])
endif

libpldm_include_dir = include_directories('include', is_system: true)

subdir('include')
subdir('src')

if get_option('bindings').contains('cpp') or get_option('tests')
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
