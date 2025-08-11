# libpldm

This is a library which deals with the encoding and decoding of PLDM messages.
It should be possible to use this library by projects other than OpenBMC, and
hence certain constraints apply to it:

- keeping it light weight
- implementation in C
- minimal dynamic memory allocations
- endian-safe
- no OpenBMC specific dependencies

Source files are named according to the PLDM Type, for eg base.[h/c], fru.[h/c],
etc.

Given a PLDM command "foo", the library will provide the following API: For the
Requester function:

```c
encode_foo_req() - encode a foo request
decode_foo_resp() - decode a response to foo
```

For the Responder function:

```c
decode_foo_req() - decode a foo request
encode_foo_resp() - encode a response to foo
```

The library also provides API to pack and unpack PLDM headers.

## To Build

`libpldm` is configured and built using `meson`. Python's `pip` or
[`pipx`][pipx] can be used to install a recent version on your machine:

[pipx]: https://pipx.pypa.io/latest/

```sh
pipx install meson
```

Once `meson` is installed:

```sh
meson setup build && meson compile -C build
```

## To run unit tests

```sh
meson test -C build
```

## Working with `libpldm`

Components of the library ABI[^1] (loosely, functions) are separated into three
categories:

[^1]: ["library API + compiler ABI = library ABI"][libstdc++-library-abi]

[libstdc++-library-abi]:
  https://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html

1. Stable
2. Testing
3. Deprecated

Applications depending on `libpldm` should aim to only use functions from the
stable category. However, this may not always be possible. What to do when
required functions fall into the deprecated or testing categories is discussed
in [CONTRIBUTING](CONTRIBUTING.md#Library-background).

### Upgrading libpldm

libpldm is maintained with the expectation that users move between successive
releases when upgrading. This constraint allows the library to reintroduce types
and functions of the same name in subsequent releases in the knowledge that
there are no remaining users of previous definitions. While strategies are
employed to avoid breaking existing APIs unnecessarily, the library is still to
reach maturity, and we must allow for improvements to be made in the design.
