test_include_dirs = [
    include_root,
    include_directories('../src', '../tests'),
]

if get_option('test-methods').contains('unit')
    subdir('unit')
endif

if get_option('test-methods').contains('fuzz')
    subdir('fuzz')
endif
