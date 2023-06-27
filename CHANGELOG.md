# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Change categories:

- Added
- Changed
- Deprecated
- Removed
- Fixed
- Security

## [Unreleased]

### Changed

1. bios_table: pldm_bios_table_string_entry_encode_check(): Handle overflow

### Deprecated

1. pldm_bios_table_attr_entry_integer_encode()

   Migrate to pldm_bios_table_attr_entry_integer_encode_check()

2. bios_table: Deprecate pldm_bios_table_attr_value_entry_encode_enum()

   Migrate to pldm_bios_table_attr_value_entry_encode_enum_check()

3. bios_table: Deprecate pldm_bios_table_attr_value_entry_encode_string()

   Migrate to pldm_bios_table_attr_value_entry_encode_string_check()

### Removed

1. bios_table: Remove deprecated APIs sanitized by assert():

   - pldm_bios_table_string_entry_encode()
   - pldm_bios_table_string_entry_decode_string()
   - pldm_bios_table_attr_entry_enum_encode()
   - pldm_bios_table_attr_entry_enum_decode_pv_num()
   - pldm_bios_table_attr_entry_enum_decode_def_num()
   - pldm_bios_table_attr_entry_enum_decode_pv_hdls()
   - pldm_bios_table_attr_entry_string_encode()
   - pldm_bios_table_attr_entry_string_decode_def_string_length()

## [0.3.0] - 2023-06-23

### Added

1. Add encode/decode pldmMessagePollEvent data
2. README: Add a section on working with libpldm
3. pdr: Introduce remote_container_id and associated APIs
4. pdr: Add APIs for creating and locating remote PDRs
5. pdr: Add pldm_pdr_find_last_in_range()
6. pdr: Add pldm_entity_association_pdr_add_from_node_with_record_handle()
7. pdr: Add pldm_pdr_find_container_id_range_exclude()

### Changed

1. include: Move installed transport.h under libpldm/
2. libpldm: Explicit deprecated, stable and testing ABI classes
3. meson: Reduce strength of oem-ibm requirements from enabled to allowed

   The `oem-ibm` feature is now enabled by the default meson configuration, for
   CI purposes. `oem-ibm` is still disabled by default in the `libpldm` bitbake
   recipe:

   https://github.com/openbmc/openbmc/blob/master/meta-phosphor/recipes-phosphor/libpldm/libpldm_git.bb#L10

   To disable `oem-ibm` in your development builds, pass `-Doem-ibm=disabled`
   when invoking `meson setup`

4. bios_table: Relax pldm_bios_table_string_entry_decode_string_check()
5. bios_table: Relax pldm_bios_table_attr_entry_enum_decode_pv_hdls_check()

### Deprecated

1. bios_table: Deprecate APIs with arguments sanitized using assert()

   C provides enough foot-guns without us encoding them into library APIs.
   Specifically, deprecate the following in favour of their `*_check()` variants
   which ensure assertions won't fail or otherwise invoke UB:

   - pldm_bios_table_string_entry_encode()
   - pldm_bios_table_string_entry_decode_string()
   - pldm_bios_table_attr_entry_enum_encode()
   - pldm_bios_table_attr_entry_enum_decode_pv_num()
   - pldm_bios_table_attr_entry_enum_decode_def_num()
   - pldm_bios_table_attr_entry_enum_decode_pv_hdls()
   - pldm_bios_table_attr_entry_string_encode()
   - pldm_bios_table_attr_entry_string_decode_def_string_length()

### Removed

1. libpldm: Remove the requester-api option

### Fixed

1. requester: Make pldm_open() return existing fd
2. transport: Prevent sticking in waiting for response
3. transport: Match on response in pldm_transport_send_recv_msg()
4. requester: Add check before accessing hdr in pldm_recv()
5. bios_table: pldm_bios_table_attr_entry_string_info_check() NULL deref
