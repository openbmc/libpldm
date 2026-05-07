test_include_dirs = [
    include_root,
    include_directories('../src', '../tests'),
]

subdir('unit')

if get_option('abi').contains('testing')
    subdir('fuzz')
endif
