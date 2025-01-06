testscxx = ['firmware_update_test']

foreach t : testscxx    test(
        'firmware_update_test',
        executable(
            t.underscorify(),
            'firmware_update_test.cpp',
            implicit_include_directories: false,
            include_directories: ['../../../bindings/cpp/'],
            dependencies: [gtest_dep, gmock_dep, libpldm_dep, libpldmplus_dep],
        ),
    )
endforeach
