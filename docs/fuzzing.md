# Fuzzing libpldm

## Firmware FD Responder

`tests/fuzz/fd-fuzz.cpp` exercises the FD responder implementation. It can run with
various fuzzing engines - either AFL++, honggfuzz, or libfuzzer.

Each fuzz corpus input is split into two parts. The first 1024 bytes is a "control" stream which
used to randomise certain events in the fuzzer, such as returning failure from callbacks,
or choosing whether to receive a "packet" or run progress.

The remainder of the fuzz input is taken as an stream of `length:data` PLDM packet contents,
as passed to `pldm_fd_handle_msg()`.

## Build

From the top level libpldm directory, run `./tests/fuzz/fuzz-build.py`. That will
produce several build variants required for different fuzz engines/stages.

## Honggfuzz

[Honggfuzz](https://github.com/google/honggfuzz) handles running across 
multiple threads itself with a single corpus directory, 
which is easy to work with. It needs to be built from source.

Run with

```
nice honggfuzz -i corpusdir --linux_perf_branch --dict tests/fuzz/fd.dict  -- ./bhf/tests/fuzz/fd-fuzz
```

The `--linux_perf_branch` switch is optional, it requires permissions for perf counters:

```
echo 0 | sudo tee /proc/sys/kernel/perf_event_paranoid
```

Optionally a thread count can be given, 24 threads on a 12 core system seems to give best utilisation
(`--nthreads 12`).

The corpus directory can be reused between runs with different fuzzers. For a totally fresh start, copy in 
`tests/fuzz/fd-fuzz-input1.dat`, a sample handcrafted input.

## AFL++

AFL++ requires a separate GUI instantiation for each CPU thread. The helper 
[AFL Runner](https://github.com/0xricksanchez/afl_runner) makes that easier.

Running with 20 threads:

```
nice aflr run  -t bfuzz/tests/fuzz/fd-fuzz -i workdir/out5/m_fd-fuzz/queue -o workdir/out6 -c bcmplog/tests/fuzz/fd-fuzz -s bfuzzasan/tests/fuzz/fd-fuzz -n 20 -x tests/fuzz/fd.dict --session-name fuzz
```

Kill it with `aflr kill fuzz`.

`aflr tui workdir/out6` could be used to view progress, though its calculations may be inaccurate
if some runners are idle. Another option is `afl-whatsup workdir/out6`.

## Coverage

The coverage provided by a corpus directory can be reported using `tests/fuzz/fuzz-coverage.py`.

It will:
- Run a binary compiled with `--coverage` against each corpus file
- Use [grcov](https://github.com/mozilla/grcov) to aggregate the coverage traces (much faster than lcov).
- Use `genhtml` to create a report

## Reproducing crashes

When the fuzz run encounters a crash, the testcase can be run against the built target manually,
and stepped through with GDB etc.

```
env TRACEFWFD=1 ./bnoopt/tests/fuzz/fd-fuzz < crashing.bin
```

The `printf`s are disabled by default to improve normal fuzzing speed.
