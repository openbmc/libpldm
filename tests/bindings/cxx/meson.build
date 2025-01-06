
testscxx = ['package_parser_test']

foreach t : testscxx
    test(
        'package_parser_test',
        executable(
            t.underscorify(),
            'package_parser_test.cpp',
            implicit_include_directories: false,
            include_directories: '../../../bindings/cxx/',
            dependencies: [gtest_dep, gmock_dep, libpldm_dep],
        ),
    )
endforeach
