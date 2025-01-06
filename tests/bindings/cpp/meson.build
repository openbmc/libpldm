testscxx = ['firmware_update_test']

foreach t : testscxx    test(
        t,
        executable(
            t.underscorify(),
            t + '.cpp',
            implicit_include_directories: false,
            include_directories: ['../../../bindings/cpp/'],
            dependencies: [gtest_dep, gmock_dep, libpldm_dep, libpldmxx_dep],
        ),
    )
endforeach
