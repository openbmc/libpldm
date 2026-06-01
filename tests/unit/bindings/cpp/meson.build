testscpp = [
    'firmware_update_test',
    'firmware_update_test_v1_1_0',
    'firmware_update_test_v1_3_0',
]

foreach t : testscpp
    test(
        t,
        executable(
            t.underscorify(),
            t + '.cpp',
            implicit_include_directories: false,
            include_directories: ['../../../../bindings/cpp/'],
            dependencies: [gtest_dep, gmock_dep, libpldm_dep, libpldmpp_dep],
        ),
    )
endforeach
