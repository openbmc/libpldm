test_include_dirs = [
    include_root,
    include_directories('../src', '../tests'),
]

if get_option('test-methods').contains('unit')
    subdir('unit')
endif

if get_option('test-methods').contains('fuzz')
    if get_option('abi').contains('testing')
        subdir('fuzz')
    else
        error('Configure build with testing ABI to enable fuzz targets')
    endif
endif
