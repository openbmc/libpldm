testscpp = [
    'firmware_update_test',
    'firmware-update-test-v1-1-0',
    'firmware-update-test-v1-3-0',
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
