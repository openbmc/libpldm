subdir('fd')

jobs = run_command(
    find_program('nproc', required: true),
    capture: true,
    check: true,
).stdout().strip()

fuzz = executable(
    'fuzz',
    'fuzz.c',
    link_args: ['-fsanitize=fuzzer'],
    implicit_include_directories: false,
    include_directories: test_include_dirs,
    dependencies: [libpldm_dep],
)

test(
    'fuzz',
    fuzz,
    args: [
        '-max_total_time=30',
        '-jobs=@0@'.format(jobs),
        '-workers=@0@'.format(jobs),
    ],
    env: {'LLVM_PROFILE_FILE': 'fuzz.%p.profraw'},
    is_parallel: false,
    timeout: 0,
)

fuzzpp = executable(
    'fuzz++',
    'fuzz++.cpp',
    link_args: ['-fsanitize=fuzzer'],
    implicit_include_directories: false,
    include_directories: test_include_dirs,
    dependencies: [libpldmpp_dep],
)

test(
    'fuzz++',
    fuzzpp,
    args: [
        '-max_total_time=30',
        '-jobs=@0@'.format(jobs),
        '-workers=@0@'.format(jobs),
    ],
    env: {'LLVM_PROFILE_FILE': 'fuzz++.%p.profraw'},
    is_parallel: false,
    timeout: 0,
)
