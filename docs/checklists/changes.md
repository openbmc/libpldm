# Checklist for making changes to `libpldm`

## Adding a new API

- [ ] My new public `struct` definitions (in any header under `include/libpldm`)
      are _not_ marked `__attribute__((packed))`

- [ ] My new function symbols are marked with `LIBPLDM_ABI_TESTING` in the
      implementation

- [ ] If my function interacts with the PLDM wire format, then I have
      implemented it using the `msgbuf` APIs found in `src/msgbuf.h` (and under
      `src/msgbuf/`) to minimise concerns around spatial memory safety and
      endian-correctness.

- [ ] I've implemented both the encoder and decoder for the message type I'm
      interested in (whether that be a request or a response - however chances
      are you should implement support for both)

- [ ] I've implemented test cases with reasonable branch coverage of each new
      function I've added

- [ ] I've guarded the test cases of functions marked `LIBPLDM_ABI_TESTING` so
      that they are not compiled when the corresponding function symbols aren't
      visible

- [ ] I've added an entry to `CHANGELOG.md` describing my work

## Pre-submission

Each of the following must pass when executed in order:

- [ ] `meson setup -Dabi-compliance-check=disabled build`
- [ ] `meson compile -C build && meson test -C build`

- [ ] `meson configure --buildtype=release build`
- [ ] `meson compile -C build && meson test -C build`

- [ ] `meson configure --buildtype=debug build`
- [ ] `meson configure -Dabi=deprecated,stable build`
- [ ] `meson compile -C build && meson test -C build`
