testscpp = [
    'firmware-update-test',
    'firmware-update-test-v1-1-0',
    'firmware-update-test-v1-3-0',
    'firmware-update-test-v1-3-0-amend',
]

testpkg_util_lib = static_library(
    'testpkg-util',
    'testpkg-util.cpp',
    include_directories: [include_root],
)

foreach t : testscpp
    test(
        t,
        executable(
            t.underscorify(),
            t + '.cpp',
            implicit_include_directories: false,
            include_directories: ['../../../../bindings/cpp/', include_root],
            dependencies: [gtest_dep, gmock_dep, libpldm_dep, libpldmpp_dep],
            link_with: [testpkg_util_lib],
        ),
    )
endforeach
