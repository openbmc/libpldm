#!/bin/sh

# Builds fuzzing variants. Run this from the toplevel directory.
# Beware this will wipe build directories.

# Requires honggfuzz and afl++ installed

# Builds are:
# * AFL (normal, asan, cmplog)
# * honggfuzz
# * -O0, with coverage
set -v
set -e

# reduce warning level since tests since gtest is noisy
MESONFLAGS="-Dwarning_level=2 -Ddefault_library=static -Doptimization=3 --wipe"
FUZZ_PROGRAMS=tests/fuzz/fd-fuzz

# Meson sets CC="ccache cc" by default, but ccache removes -fprofile-arcs
# so coverage breaks (ccache #1531). Prevent that.
export CC=cc
export CC=c++

( # start AFL
# seems to be required for afl-clang-lto?
export AFL_REAL_LD=ld.lld
export CC=afl-clang-lto
export CXX=afl-clang-lto++

# afl cmplog
(
export AFL_LLVM_CMPLOG=1
BDIR=bcmplog
meson setup $MESONFLAGS $BDIR
ninja -C $BDIR $FUZZ_PROGRAMS
)

# afl normal
(
BDIR=bfuzz
meson setup $MESONFLAGS $BDIR
ninja -C $BDIR $FUZZ_PROGRAMS
)

# afl asan
(
BDIR=bfuzzasan
meson setup $MESONFLAGS -Db_sanitize=address $BDIR
ninja -C $BDIR $FUZZ_PROGRAMS
)
) # end AFL


# No profiling, has coverage
(
BDIR=bnoopt
export CFLAGS='-fprofile-abs-path'
export CXXFLAGS='-fprofile-abs-path'
meson setup --wipe -Db_coverage=true -Doptimization=0 -Ddefault_library=static $BDIR
ninja -C $BDIR $FUZZ_PROGRAMS
)

# honggfuzz
(
BDIR=bhf
export CC=hfuzz-clang
export CXX=hfuzz-clang++
export CFLAGS="-march=native"
export CXXFLAGS="-march=native"
meson setup $MESONFLAGS $BDIR
ninja -C $BDIR $FUZZ_PROGRAMS
)
