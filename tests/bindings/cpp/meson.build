testscpp = ['firmware_update_test']

foreach t : testscpp
    test(
        t,
        executable(
            t.underscorify(),
            t + '.cpp',
            implicit_include_directories: false,
            include_directories: ['../../../bindings/cpp/'],
            dependencies: [gtest_dep, gmock_dep, libpldm_dep, libpldmpp_dep],
        ),
    )
endforeach
