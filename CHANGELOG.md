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

### Added

1. bios_table: Introduce pldm_bios_table_append_pad_checksum_check()
2. fru: Introduce get_fru_record_by_option_check()

### Changed

1. requester: Mark pldm_close() as LIBPLDM_ABI_TESTING
2. requester: Expose pldm_close() in header
3. bios_table: pldm_bios_table_string_entry_encode_check(): Handle overflow
4. bios_table: pldm_bios_table_iter_create(): Return NULL on failed alloc
5. bios_table: pldm_bios_table_iter_next(): Invalid entry halts iteration
6. pdr: pldm_pdr_init(): Return NULL on allocation failure
7. pdr: pldm_pdr_destroy(): Exit early if repo is NULL
8. pdr: Document preconditions for trivial accessor functions

   A trivial accessor function is one that exposes properties of an object in a
   way can't result in an error, beyond passing an invalid argument to the
   function. For APIs meeting this definition we define a precondition that
   struct pointers must point to valid objects to avoid polluting the function
   prototypes. The following APIs now have this precondition explicitly defined:

   - pldm_entity_extract()
   - pldm_entity_get_parent()
   - pldm_entity_is_exist_parent()
   - pldm_entity_is_node_parent()
   - pldm_is_current_parent_child
   - pldm_is_empty_entity_assoc_tree()
   - pldm_pdr_get_record_count()
   - pldm_pdr_get_record_handle()
   - pldm_pdr_get_repo_size()
   - pldm_pdr_record_is_remote()

9. pdr: pldm_entity_node_get_remote_container_id() is a trivial accessor
10. pdr: pldm_pdr_fru_record_set_find_by_rsi(): Exit early on NULL arguments
11. pdr: pldm_entity_association_tree_init(): Return NULL on failed alloc
12. pdr: pldm_entity_association_tree_visit(): Document preconditions
13. pdr: pldm_entity_association_tree_visit(): Exit early on failure

### Deprecated

1. pldm_bios_table_attr_entry_integer_encode()

   Migrate to pldm_bios_table_attr_entry_integer_encode_check()

2. bios_table: Deprecate pldm_bios_table_attr_value_entry_encode_enum()

   Migrate to pldm_bios_table_attr_value_entry_encode_enum_check()

3. bios_table: Deprecate pldm_bios_table_attr_value_entry_encode_string()

   Migrate to pldm_bios_table_attr_value_entry_encode_string_check()

4. bios_table: Deprecate pldm_bios_table_attr_value_entry_encode_integer()

   Migrate to pldm_bios_table_attr_value_entry_encode_integer_check()

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

### Fixed

1. pdr: Return success for pldm_pdr_find_child_container_id_range_exclude() API
2. pdr: Rework pldm_pdr_find_container_id_range_exclude() API
3. transport: mctp-demux: Don't test socket for non-zero value
4. requester: Return PLDM_REQUESTER_OPEN_FAIL from pldm_open() on error
5. pdr: pldm_pdr_fru_record_set_find_by_rsi(): Document reality of return

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
