if get_option('abi').contains('testing')
    subdir('fd')
endif

fuzz = executable(
    'fuzz',
    'fuzz.c',
    link_args: ['-fsanitize=fuzzer'],
    implicit_include_directories: false,
    include_directories: test_include_dirs,
    dependencies: [libpldm_dep],
    build_by_default: get_option('test-methods').contains('fuzz'),
)

if get_option('test-methods').contains('fuzz')
    jobs = run_command(
        find_program('nproc', required: true),
        capture: true,
        check: true,
    ).stdout().strip()
    test(
        'fuzz',
        fuzz,
        args: [
            '-max_total_time=30',
            '-jobs=@0@'.format(jobs),
            '-workers=@0@'.format(jobs),
        ],
        is_parallel: false,
        timeout: 0,
    )
endif
