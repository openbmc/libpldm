gtest_dep = dependency(
    'gtest',
    main: true,
    disabler: true,
    required: false,
    include_type: 'system',
)
gmock_dep = dependency(
    'gmock',
    disabler: true,
    required: false,
    include_type: 'system',
)
if not gtest_dep.found() or not gmock_dep.found()
    gtest_proj = import('cmake').subproject('googletest', required: true)
    gtest_dep = declare_dependency(
        dependencies: [
            dependency('threads', include_type: 'system'),
            gtest_proj.dependency('gtest', include_type: 'system'),
            gtest_proj.dependency('gtest_main', include_type: 'system'),
        ],
    )
    gmock_dep = gtest_proj.dependency('gmock', include_type: 'system')
endif

test_include_dirs = [
    include_root,
    include_directories('../src', '../tests'),
]

tests = ['bcd', 'edac', 'instance-id', 'msgbuf', 'responder', 'utils']

subdir('dsp')

if get_option('abi').contains('testing')
    subdir('fuzz')
endif

# The 'test' transport APIs will never be marked stable as they are just for
# testing.
if get_option('abi').contains('testing')
    subdir('transport')
endif

if get_option('oem').contains('ibm')
    subdir('oem/ibm')
endif

if get_option('oem').contains('meta')
    subdir('oem/meta')
endif

if get_option('bindings').contains('cpp')
    subdir('bindings/cpp')
endif

foreach t : tests
    test(
        t,
        executable(
            t.underscorify(),
            t + '.cpp',
            implicit_include_directories: false,
            include_directories: test_include_dirs,
            dependencies: [libpldm_dep, gtest_dep, gmock_dep],
        ),
        workdir: meson.current_source_dir(),
    )
endforeach

test(
    'msgbuf_generic',
    executable(
        'msgbuf_generic',
        'msgbuf_generic.c',
        implicit_include_directories: false,
        include_directories: test_include_dirs,
    ),
)
