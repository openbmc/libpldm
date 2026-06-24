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

fuzz_cpp = executable(
    'fuzz_cpp',
    'fuzz_cpp.cpp',
    link_args: ['-fsanitize=fuzzer'],
    implicit_include_directories: false,
    include_directories: test_include_dirs,
    dependencies: [libpldm_dep, libpldmpp_dep],
)

test(
    'fuzz_cpp',
    fuzz_cpp,
    args: [
        '-max_total_time=30',
        '-jobs=@0@'.format(jobs),
        '-workers=@0@'.format(jobs),
    ],
    env: {'LLVM_PROFILE_FILE': 'fuzz_cpp.%p.profraw'},
    is_parallel: false,
    timeout: 0,
)
