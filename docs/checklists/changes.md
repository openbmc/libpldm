# Checklist for making changes to `libpldm`

## Definitions

- Public API: Any definitions and declarations under `include/libpldm`

## Adding a new API

- [ ] My new public `struct` definitions are _not_ marked
      `__attribute__((packed))`

- [ ] If my work interacts with the PLDM wire format, then I have done so using
      the `msgbuf` APIs found in `src/msgbuf.h` (and under `src/msgbuf/`) to
      minimise concerns around spatial memory safety and endian-correctness.

- [ ] If I've added support for a new PLDM message type, then I've implemented
      both the encoder and decoder for that message. Note this applies for both
      request _and_ response message types.

- [ ] My new function symbols are marked with `LIBPLDM_ABI_TESTING` in the
      implementation

- [ ] I've implemented test cases with reasonable branch coverage of each new
      function I've added

- [ ] I've guarded the test cases of functions marked `LIBPLDM_ABI_TESTING` so
      that they are not compiled when the corresponding function symbols aren't
      visible

- [ ] If I've added support for a new message type, then my commit message
      specifies all of:

  - [ ] The relevant DMTF specification by its DSP number and title
  - [ ] The relevant version of the specification
  - [ ] The section of the specification that defines the message type

- [ ] If my work impacts the public API of the library, then I've added an entry
      to `CHANGELOG.md` describing my work

## Stabilising an existing API

- [ ] The API of interest is currently marked `LIBPLDM_ABI_TESTING`

- [ ] My commit message links to a publicly visible patch that makes use of the
      API

- [ ] My commit updates the annotation from `LIBPLDM_ABI_TESTING` to
      `LIBPLDM_ABI_STABLE` only for the function symbols demonstrated by the
      patch linked in the commit message.

- [ ] I've removed guards from the function's tests so they are always compiled

- [ ] If I've updated the ABI dump, then I've used the OpenBMC CI container to
      do so.

## Updating an ABI dump

Each of the following must succeed:

- [ ] Enter the OpenBMC CI Docker container
  - Approximately:
    `docker run --cap-add=sys_admin --rm=true --privileged=true -u $USER -w $(pwd) -v $(pwd):$(pwd) -e MAKEFLAGS= -it openbmc/ubuntu-unit-test:2024-W21-ce361f95ff4fa669`
- [ ] `CC=gcc CXX=g++; [ $(uname -m) = 'x86_64' ] && meson setup -Dabi=deprecated,stable build`
- [ ] `meson compile -C build`
- [ ] `cp build/src/current.dump abi/x86_64/gcc.dump`

## Removing an API

- [ ] If the function is marked `LIBPLDM_ABI_TESTING`, then I have removed it

- [ ] If the function is marked `LIBPLDM_ABI_STABLE`, then I have changed the
      annotation to `LIBPLDM_ABI_DEPRECATED` and left it in-place.

- [ ] If the function is marked `LIBPLDM_ABI_DEPRECATED`, then I have removed it
      only after satisfying myself that each of the following is true:

  - [ ] There are no known users of the function left in the community
  - [ ] There has been at least one tagged release of `libpldm` subsequent to
        the API being marked deprecated

## Testing my changes

Each of the following must succeed when executed in order. Note that to avoid
[googletest bug #4232][googletest-issue-4232] you must avoid using GCC 12
(shipped in Debian Bookworm).

[googletest-issue-4232](https://github.com/google/googletest/issues/4232)

- [ ] `meson setup -Dabi-compliance-check=disabled build`
- [ ] `meson compile -C build && meson test -C build`

- [ ] `meson configure --buildtype=release build`
- [ ] `meson compile -C build && meson test -C build`

- [ ] `meson configure --buildtype=debug build`
- [ ] `meson configure -Dabi=deprecated,stable build`
- [ ] `meson compile -C build && meson test -C build`
