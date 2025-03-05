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

- Add Firmware Device side firmware_update encode/decode functions

- Add firmware update FD responder

- Add PLDM control responder. PLDM types and support commands/versions can be
  registered.

- PLDM FD responder accepts a PLDM control handle and will register its version.

- base: Define the minimum request bytes for SetTID command.

- pdr: Add pldm_pdr_delete_by_record_handle() API

- Support for building the documentation with doxygen

### Changed

- dsp: firmware_update: Expand "params" in symbol names

  The change only affects structs and functions relating to ABIs that are marked
  as testing. There should be no impact on users of the stable APIs/ ABIs.

- Reimplement parsing of the firmware update downstream device parameter table
  using an iterator macro

  The change removes redundant APIs in the process.

- Returned error values for the following stable APIs have changed their
  semantics:

  - `decode_descriptor_type_length_value()`
  - `decode_event_message_buffer_size_resp()`
  - `decode_get_numeric_effecter_value_resp()`
  - `decode_get_sensor_reading_resp()`
  - `decode_get_state_sensor_readings_resp()`
  - `decode_numeric_sensor_data()`
  - `decode_sensor_op_data()`

  No new error values will be returned, but existing error values may be
  returned under new conditions.

- pdr: Indicates success or failure depending on the outcome of the entity
  association PDR creation

- Register allocation changed for the following APIs:

  - `encode_get_downstream_firmware_parameters_req()`

### Deprecated

### Removed

### Fixed

- pdr: Remove PDR if the contained entity to be removed is the last one
- meson: sizes.h: add includedir to install path
- pdr: Create entity association PDRs with unique record handle
- requester: add null check for instance db object in pldm_instance_id_alloc()
- requester: add null check for instance db object in pldm_instance_id_free()

### Security

## [0.11.0] 2024-12-12

### Added

- dsp: firmware_update: Iterators for downstream device descriptors
- platform: add PLDM Command numbers
- base: add PLDM Command numbers

### Changed

- Register assignment for parameters of `encode_state_effecter_pdr()`

- dsp: firmware_update: Iterators for downstream device descriptors

  The prototype for `decode_query_downstream_identifiers_resp()` was updated to
  improve ergonomics for the iterator APIs.

- meson: Specify OEM extensions in an array

  `include/libpldm/meson.build` is modified, but not in ways that are
  significant.

- Return `ENOENT` rather than `ENOKEY` from
  `pldm_pdr_find_child_container_id_index_range_exclude()`

- dsp: firmware_update: Change return type of downstream device ABIs to ERRNO

  Those downstream device related ABIs have not been stabilized yet, change
  return type from PLDM Completion Code to ERRNO

- dsp: firmware_update: pack decomposed parameters to struct

  `encode_query_downstream_identifiers_req()` and
  `encode_get_downstream_firmware_params_req()`

### Fixed

- dsp: platform: Fix location of closing paren in overflow detection
- libpldm: Install api header, update changelog

## [0.10.0] 2024-11-01

### Added

- oem: meta: Add decode_oem_meta_file_io_write_req()
- oem: meta: Add decode_oem_meta_file_io_read_req()
- oem: meta: Add encode_oem_meta_file_io_read_resp()
- pdr: Add pldm_entity_association_pdr_remove_contained_entity()
- pdr: Add pldm_pdr_remove_fru_record_set_by_rsi()
- pldm_entity_association_tree_copy_root_check()
- oem: ibm: Add topology related state set and enum

- base: Add size and buffer macros for struct pldm_msg

  Together these macros reduce the need for use of reinterpret_cast<>() in C++.

- entity: Add new entity types from DSP0249 v1.3.0
- stateset: Add new state sets from DSP0249 v1.3.0

### Changed

- dsp: bios_table: Null check for pldm_bios_table_iter_is_end()

  pldm_bios_table_iter_is_end() now returns true if the provided argument is
  NULL.

- Register assignment for parameters of a number of APIs changed with increased
  scrutiny on their implementations.

  - `decode_entity_auxiliary_names_pdr()`
  - `decode_get_state_sensor_readings_resp()`
  - `decode_oem_meta_file_io_req()`
  - `decode_platform_event_message_req()`
  - `decode_platform_event_message_resp()`
  - `decode_sensor_op_data()`
  - `encode_get_state_effecter_states_resp()`
  - `encode_state_effecter_pdr()`
  - `encode_state_sensor_pdr()`
  - `pldm_bios_table_append_pad_checksum()`
  - `pldm_bios_table_attr_value_entry_encode_enum()`
  - `pldm_bios_table_attr_value_entry_encode_string()`
  - `pldm_pdr_find_record()`
  - `pldm_pdr_get_next_record()`

- platform: Support PLDM_CPER_EVENT in encode_platform_event_message_req()

- dsp: firmware_update: Bounds check
  decode_downstream_device_parameter_table_entry_versions()

  The additional bounds-checking required the addition of further length
  parameters.

### Deprecated

- oem: meta: Deprecate `decode_oem_meta_file_io_req()`

  Users should switch to `decode_oem_meta_file_io_write_req()`. Modify this
  function to make it safer.

  Modification:

  - The meaning of the returned result.
  - Change parameters from individual pointers to a struct.
  - Check the length provided in the message won't exceed the buffer.

- pldm_entity_association_tree_copy_root()

  The implementation allocates, but gives no indication to the caller if an
  allocation (and hence the copy) has failed. Users should migrate to
  pldm_entity_association_tree_copy_root_check().

- The following APIs are deprecated as unsafe due to various unfixable CWE
  violations:

  - [CWE-129: Improper Validation of Array Index](https://cwe.mitre.org/data/definitions/129.html)

    - `encode_get_bios_current_value_by_handle_resp()`
    - `encode_get_bios_table_resp()`
    - `encode_get_file_table_resp()`
    - `encode_get_version_resp()`
    - `pldm_bios_table_attr_entry_enum_decode_def_indices()`
    - `pldm_bios_table_attr_entry_enum_decode_def_num()`
    - `pldm_bios_table_attr_find_by_handle()`
    - `pldm_bios_table_attr_find_by_string_handle()`
    - `pldm_bios_table_attr_value_find_by_handle()`
    - `pldm_bios_table_iter_create()`
    - `pldm_bios_table_iter_is_end()`
    - `pldm_bios_table_string_find_by_handle()`
    - `pldm_bios_table_string_find_by_string()`

  - [CWE-617: Reachable Assertion](https://cwe.mitre.org/data/definitions/617.html)

    - `pldm_entity_association_tree_copy_root()`

  - [CWE-789: Memory Allocation with Excessive Size Value](https://cwe.mitre.org/data/definitions/789.html)

    - `decode_oem_meta_file_io_req()`

  - [CWE-823: Use of Out-of-range Pointer Offset](https://cwe.mitre.org/data/definitions/823.html)
    - `encode_fru_record()`
    - `encode_get_pdr_resp()`
    - `pldm_bios_table_attr_entry_enum_encode_length()`

### Removed

- Deprecated functions with the `_check` suffix

  - `get_fru_record_by_option_check()`
  - `pldm_bios_table_append_pad_checksum_check()`
  - `pldm_bios_table_attr_entry_enum_decode_def_num_check()`
  - `pldm_bios_table_attr_entry_enum_decode_pv_hdls_check()`
  - `pldm_bios_table_attr_entry_enum_decode_pv_num_check()`
  - `pldm_bios_table_attr_entry_enum_encode_check()`
  - `pldm_bios_table_attr_entry_integer_encode_check()`
  - `pldm_bios_table_attr_entry_string_decode_def_string_length_check()`
  - `pldm_bios_table_attr_entry_string_encode_check()`
  - `pldm_bios_table_attr_value_entry_encode_enum_check()`
  - `pldm_bios_table_attr_value_entry_encode_integer_check()`
  - `pldm_bios_table_attr_value_entry_encode_string_check()`
  - `pldm_bios_table_string_entry_decode_string_check()`
  - `pldm_bios_table_string_entry_encode_check()`
  - `pldm_entity_association_pdr_add_check()`
  - `pldm_entity_association_pdr_add_from_node_check()`
  - `pldm_pdr_add_check()`
  - `pldm_pdr_add_fru_record_set_check()`

### Fixed

- dsp: bios_table: Null check for pldm_bios_table_iter_is_end()

  Avoid a caller-controlled NULL pointer dereference in the library
  implementation.

- platform: fix encode/decode_poll_for_platform_event_message_req

  Update checking of `TransferOperationFlag` and `eventIDToAcknowledge` to
  follow spec.

- platform: Fix checking `eventIDToAcknowledge`

  As the event receiver sends `PollForPlatformEventMessage` with the
  `tranferFlag` is `AcknowledgementOnly`, the value `eventIDToAcknowledge`
  should be the previously retrieved eventID (from the PLDM terminus).

- dsp: platform: Prevent overflow of arithmetic on event_data_length
- dsp: platform: Bounds check encode_sensor_state_pdr()
- dsp: platform: Bounds check encode_state_effecter_pdr()
- dsp: pdr: Bounds check pldm_entity_association_pdr_extract()
- dsp: bios_table: Bounds check pldm_bios_table_append_pad_checksum()
- dsp: bios_table: Bounds check pldm_bios_table_attr_value_entry_encode_string()
- dsp: bios_table: Bounds check pldm_bios_table_attr_value_entry_encode_enum()
- dsp: firmware_update: Bounds check
  decode_downstream_device_parameter_table_entry_versions()
- oem: ibm: platform: Bounds check encode_bios_attribute_update_event_req()
- dsp: fru: Bounds check encode_get_fru_record_by_option_resp()
- dsp: fru: Bounds check encode_fru_record()
- dsp: bios: Bounds check encode_set_bios_table_req()
- dsp: bios: Bounds check encode_set_bios_attribute_current_value_req()
- dsp: bios_table: Bounds check pldm_bios_table_string_entry_encode()
- dsp: pdr: Rework test in pldm_entity_association_pdr_extract()
- dsp: platform: Fix decode_set_event_receiver_req()

## [0.9.1] - 2024-09-07

### Changed

- Moved evolutions intended for v0.9.0 into place

  Evolutions for the release have been moved from `evolutions/current` to
  `evolutions/v0.9.1`. Library users can apply them to migrate off of deprecated
  APIs.

## [0.9.0] - 2024-09-07

### Added

- base: Define macros for reserved TIDs
- pdr: Add pldm_entity_association_pdr_add_contained_entity_to_remote_pdr()
- pdr: Add pldm_entity_association_pdr_create_new()
- platform: Define macros for the responded transferflags
- pdr: Add pldm_pdr_get_terminus_handle() API
- pdr: Add related decode_entity_auxiliary_names_pdr() APIs
- fw_update: Add encode req & decode resp for get_downstream_fw_params
- platform: Add decode_pldm_platform_cper_event() API
- decode_get_pdr_repository_info_resp_safe()

  Replaces decode_get_pdr_repository_info_resp() as discussed in the
  `Deprecated` section below

- decode_get_pdr_resp_safe()

  Replaces decode_get_pdr_resp() as discussed in the `Deprecated` section below

### Changed

- pdr: Stabilise related decode_entity_auxiliary_names_pdr() APIs
- platform: Rework decode/encode_pldm_message_poll_event_data() APIs
- platform: Stabilise decode_pldm_message_poll_event_data() APIs
- ABI break for decode_sensor_op_data()

  Applying LIBPLDM_CC_NONNULL to the internal msgbuf APIs caused
  abi-compliance-checker to flag a change in the register containing the
  parameter `previous_op_state`.

- platform: Stabilise decode_pldm_platform_cper_event() API
- oem: meta: Stabilise decode_oem_meta_file_io_write_req() API
- oem: meta: Stabilise decode_oem_meta_file_io_read_req() API
- oem: meta: Stabilise encode_oem_meta_file_io_read_resp() API

### Deprecated

- Rename and deprecate functions with the `_check` suffix

  All library function return values always need to be checked. The `_check`
  suffix is redundant, so remove it. Migration to the non-deprecated equivalents
  without the `_check` suffix can be performed using `scripts/ apply-renames`
  and the [clang-rename][] configurations under `evolutions/`

  The deprecated functions:

  - `get_fru_record_by_option_check()`
  - `pldm_bios_table_append_pad_checksum_check()`
  - `pldm_bios_table_attr_entry_enum_decode_def_num_check()`
  - `pldm_bios_table_attr_entry_enum_decode_pv_hdls_check()`
  - `pldm_bios_table_attr_entry_enum_decode_pv_num_check()`
  - `pldm_bios_table_attr_entry_enum_encode_check()`
  - `pldm_bios_table_attr_entry_integer_encode_check()`
  - `pldm_bios_table_attr_entry_string_decode_def_string_length_check()`
  - `pldm_bios_table_attr_entry_string_encode_check()`
  - `pldm_bios_table_attr_value_entry_encode_enum_check()`
  - `pldm_bios_table_attr_value_entry_encode_integer_check()`
  - `pldm_bios_table_attr_value_entry_encode_string_check()`
  - `pldm_bios_table_string_entry_decode_string_check()`
  - `pldm_bios_table_string_entry_encode_check()`
  - `pldm_entity_association_pdr_add_check()`
  - `pldm_entity_association_pdr_add_from_node_check()`
  - `pldm_pdr_add_check()`
  - `pldm_pdr_add_fru_record_set_check()`

[clang-rename]: https://clang.llvm.org/extra/clang-rename.html

- `decode_get_pdr_repository_info_resp()`

  Users should move to `decode_get_pdr_repository_info_resp_safe()` which
  eliminates the opportunity for buffer overruns when extracting objects from
  the message.

- `decode_get_pdr_resp()`

  Users should move to `decode_get_pdr_resp_safe()` which reduces the invocation
  tedium and improves memory safety over `decode_get_pdr_resp()`.

### Removed

- IBM OEM header compatibility symlinks.

  Anyone left using the deprecated paths can migrate using the coccinelle patch
  at `evolutions/current/oem-ibm-header-compat.cocci`.

### Fixed

- requester: instance-id: Release read lock on conflict
- pdr: Error propagation for
  pldm_entity_association_pdr_add_from_node_with_record_handle()

## [0.8.0] - 2024-05-23

### Added

- base: Provide pldm_msg_hdr_correlate_response()
- transport: af-mctp: Add pldm_transport_af_mctp_bind()
- oem: ibm: Add chapdata file type support
- base: Added PLDM_SMBIOS & PLDM_RDE message types
- oem: meta: Add decode_oem_meta_file_io_req()
- state-set: Add all state set values to system power state enum as per DSP0249
- platform: Add alias members to the enum
  pldm_pdr_repository_chg_event_change_record_event_data_operation.

  enum constants with inconsistent names are deprecated with this change. remove
  old inconsistent enum members after backward compatibility cleanup is done

- oem-ibm: Alias `pldm_oem_ibm_fru_field_type` members as `PLDM_OEM_IBM_*`
- oem: ibm: Add Firmware Update Access Key(UAK) as a FRU field type
- platform: Add 3 PDR type enum for Redfish Device Enablement per DSP0248_1.2.0
- state_set: Add CONNECTED and DISCONNECTED enum for Link State set
- entity: Add enum for Network Interface Connectors and Network Ports Connection
  Types
- pdr: Add decode_numeric_effecter_pdr_data()
- oem: ibm: Support for the Real SAI entity id
- fw_update: Add encode req & decode resp for query_downstream_devices
- fw_update: Add encode req & decode resp for query_downstream_identifiers
- platform: Add support for GetStateEffecterStates command

### Changed

- base: Stabilise pldm_msg_hdr_correlate_response()
- transport: af-mctp: Stabilise pldm_transport_af_mctp_bind()
- libpldm: Fix header use
- libpldm: More fixes for header use
- pdr: Stabilise pldm_pdr_find_last_in_range() API
- pdr: Stabilise pldm_entity_association_pdr_add_from_node_with_record_handle()
- oem: meta: stabilise decode_oem_meta_file_io_req()
- pdr: pldm_entity_association_tree_copy_root(): Document preconditions

### Deprecated

- Deprecate `pldm_oem_ibm_fru_field_type` members that that are not prefixed
  with `PLDM_OEM_IBM_`

### Fixed

- libpldm: Rationalise the local and installed path of pldm.h
- pdr: Assign record_handle in entity_association_pdr_add_children()
- msgbuf: Require sensor data enum in pldm_msgbuf_extract_sensor_value()
- pdr: Remove redundant constant for minimum numeric sensor PDR length
- tests: oem: meta: Fix fileio use of msgbuf

## [0.7.0] - 2023-08-29

### Added

- state-set: Add new enum for Operational Fault Status enum

### Changed

- transport: Match specified metadata in pldm_transport_send_recv_msg()
- transport: mctp-demux: Drop ABI annotation for internal symbols
- transport: Stabilise core transport and implementation APIs

  This stabilisation covers the following headers and functions:

  - libpldm/transport.h

    - pldm_transport_poll()
    - pldm_transport_send_msg()
    - pldm_transport_recv_msg()
    - pldm_transport_send_recv_msg()

  - libpldm/transport/af-mctp.h

    - pldm_transport_af_mctp_init()
    - pldm_transport_af_mctp_destroy()
    - pldm_transport_af_mctp_core()
    - pldm_transport_af_mctp_init_pollfd()
    - pldm_transport_af_mctp_map_tid()
    - pldm_transport_af_mctp_unmap_tid()

  - libpldm/transport/mctp-demux.h
    - pldm_transport_mctp_demux_init()
    - pldm_transport_mctp_demux_destroy()
    - pldm_transport_mctp_demux_core()
    - pldm_transport_mctp_demux_init_pollfd()
    - pldm_transport_mctp_demux_map_tid()
    - pldm_transport_mctp_demux_unmap_tid()

### Deprecated

- All the existing "requester" APIs from `libpldm/pldm.h` (also known as
  `libpldm/requester/pldm.h`):

  - pldm_open()
  - pldm_send_recv()
  - pldm_send()
  - pldm_recv()
  - pldm_recv_any()
  - pldm_close()

  Users should migrate to the newer "transport" APIs instead.

## Fixed

- tests: Exclude transport tests when build excludes testing ABIs
- abi: Capture deprecation of pldm_close()

## [0.6.0] - 2023-08-22

### Changed

- pdr: Avoid ID overflow in pldm_entity_association_tree_add_entity()
- meson: Apply `b_ndebug=if-release` by default
- pdr : Stabilize pldm_entity_association_tree_add_entity()
- pdr: Stabilise pldm_entity_association_tree_find_with_locality()
- pdr: Stabilize pldm_entity_node_get_remote_container_id()
- transport: af-mctp: Assign out-params on success in \*\_recv()
- transport: Generalise the pldm_transport_recv_msg() API

### Removed

- pdr: Remove pldm_entity_association_pdr_add()
- state-set: Remove enum pldm_state_set_operational_fault_status_values

### Fixed

- transport: register init_pollfd callback for af-mctp
- transport: fix init_pollfd function parameter
- transport: Fix doxygen and variables for send and recv functions
- transport: af-mctp: Ensure malloc() succeeds in \*\_recv()

## [0.5.0] - 2023-08-09

### Added

- pdr: Introduce pldm_entity_association_pdr_add_check()

### Changed

- pdr: Allow record_handle to be NULL for pldm_pdr_add_check()
- transport: pldm_transport_poll(): Adjust return value semantics
- transport: free un-wanted responses in pldm_transport_send_recv_msg()

### Deprecated

- state-set: Enum pldm_state_set_operational_fault_status_values

  The enum operational_fault_status is defined with wrong members and will
  eventually be replaced with the correct members. Any uses of
  pldm_state_set_operational_fault_status_values members should move to
  equivalent pldm_state_set_operational_stress_status_values members if needed.

- platform: Struct field name in fru_record_set PDR

  References to entity_instance_num should be changed to entity_instance

- platform: Struct field name in numeric sensor value PDR

  References to entity_instance_num should be changed to entity_instance

### Removed

- bios_table: Remove pldm_bios_table_attr_entry_integer_encode_length()
- bios_table: Remove pldm_bios_table_attr_value_entry_encode_enum()
- bios_table: Remove pldm_bios_table_attr_value_entry_encode_string()
- bios_table: Remove pldm_bios_table_attr_value_entry_encode_integer()
- bios_table: Remove pldm_bios_table_append_pad_checksum()
- fru: Remove get_fru_record_by_option()
- pdr: Make is_present() static
- pdr: Remove pldm_pdr_add()
- pdr: Remove pldm_pdr_add_fru_record_set()
- pdr: Remove pldm_entity_association_pdr_add_from_node()
- pdr: Make find_entity_ref_in_tree() static
- pdr: Make entity_association_tree_find() static

### Fixed

- requester: Fix response buffer cast in pldm_send_recv()
- pdr: Hoist record handle overflow test to avoid memory leak
- transport: Correct comparison in while loop condition

## [0.4.0] - 2023-07-14

### Added

- bios_table: Introduce pldm_bios_table_append_pad_checksum_check()
- fru: Introduce get_fru_record_by_option_check()
- pdr: Introduce pldm_entity_association_pdr_add_from_node_check()
- pdr: Introduce pldm_pdr_add_check()
- pdr: Introduce pldm_pdr_add_fru_record_set_check()

### Changed

- requester: Mark pldm_close() as LIBPLDM_ABI_TESTING
- requester: Expose pldm_close() in header
- bios_table: pldm_bios_table_string_entry_encode_check(): Handle overflow
- bios_table: pldm_bios_table_iter_create(): Return NULL on failed alloc
- bios_table: pldm_bios_table_iter_next(): Invalid entry halts iteration
- pdr: pldm_pdr_init(): Return NULL on allocation failure
- pdr: pldm_pdr_destroy(): Exit early if repo is NULL
- pdr: Document preconditions for trivial accessor functions

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

- pdr: pldm_entity_node_get_remote_container_id() is a trivial accessor
- pdr: pldm_pdr_fru_record_set_find_by_rsi(): Exit early on NULL arguments
- pdr: pldm_entity_association_tree_init(): Return NULL on failed alloc
- pdr: pldm_entity_association_tree_visit(): Document preconditions
- pdr: pldm_entity_association_tree_visit(): Exit early on failure
- pdr: pldm_entity_association_tree_destroy(): Exit early on bad argument
- pdr: pldm_entity_get_num_children(): Return zero for invalid arguments
- pdr: pldm_is_current_parent_child(): Return false for invalid arguments
- pdr: pldm_entity_association_pdr_add(): Exit early on bad arguments
- pdr: pldm_find_entity_ref_in_tree(): Exit early on bad arguments
- pdr: pldm_entity_association_tree_find(): Early exit on bad arguments
- pdr: pldm_entity_association_tree_destroy_root(): Exit early on bad arg
- pdr: pldm_entity_association_pdr_extract(): Early exit on bad arguments
- pdr: pldm_entity_association_pdr_extract(): Assign out params at exit
- pdr: pldm_entity_get_num_children(): Don't return invalid values
- libpldm: Lift or remove asserts where a subsequent check exists

### Deprecated

- pldm_bios_table_attr_entry_integer_encode()

  Migrate to pldm_bios_table_attr_entry_integer_encode_check()

- bios_table: Deprecate pldm_bios_table_attr_value_entry_encode_enum()

  Migrate to pldm_bios_table_attr_value_entry_encode_enum_check()

- bios_table: Deprecate pldm_bios_table_attr_value_entry_encode_string()

  Migrate to pldm_bios_table_attr_value_entry_encode_string_check()

- bios_table: Deprecate pldm_bios_table_attr_value_entry_encode_integer()

  Migrate to pldm_bios_table_attr_value_entry_encode_integer_check()

- pdr: Deprecate is_present()

  There should be no users of this symbol. If you are a user, you should figure
  out how to stop, or get in touch. This symbol will be marked static the
  release after deprecation.

- pdr: Deprecate find_entity_ref_in_tree()

  There should be no users of this symbol. If you are a user, you should figure
  out how to stop, or get in touch. This symbol will be marked static the
  release after deprecation.

- pdr: Deprecate entity_association_tree_find()

  There should be no users of this symbol. If you are a user, you should figure
  out how to stop, or get in touch. This symbol will be marked static the
  release after deprecation.

- bios_table: Stabilise pldm_bios_table_append_pad_checksum_check()

  pldm_bios_table_append_pad_checksum() is deprecated by this change. Users of
  pldm_bios_table_append_pad_checksum() should migrate to
  pldm_bios_table_append_pad_checksum_check()

- fru: Stabilise get_fru_record_by_option_check()

  get_fru_record_by_option() is deprecated by this change. Users of
  get_fru_record_by_option() should migrate to get_fru_record_by_option_check()

- pdr: Stabilise pldm_entity_association_pdr_add_from_node_check()

  pldm_entity_association_pdr_add_from_node() is deprecated by this change.
  Users of pldm_entity_association_pdr_add_from_node() should migrate to
  pldm_entity_association_pdr_add_from_node_check()

- pdr: Stabilise pldm_pdr_add_check()

  pldm_pdr_add() is deprecated by this change. Users of pldm_pdr_add() should
  migrate to pldm_pdr_add_check()

- pdr: Stabilise pldm_pdr_add_fru_record_set_check()

  pldm_pdr_add_fru_record_set() is deprecated by this change. Users of
  pldm_pdr_add_fru_record_set() should migrate to
  pldm_pdr_add_fru_record_set_check()

### Removed

- bios_table: Remove deprecated APIs sanitized by assert():

  - pldm_bios_table_string_entry_encode()
  - pldm_bios_table_string_entry_decode_string()
  - pldm_bios_table_attr_entry_enum_encode()
  - pldm_bios_table_attr_entry_enum_decode_pv_num()
  - pldm_bios_table_attr_entry_enum_decode_def_num()
  - pldm_bios_table_attr_entry_enum_decode_pv_hdls()
  - pldm_bios_table_attr_entry_string_encode()
  - pldm_bios_table_attr_entry_string_decode_def_string_length()

### Fixed

- pdr: Return success for pldm_pdr_find_child_container_id_range_exclude() API
- pdr: Rework pldm_pdr_find_container_id_range_exclude() API
- transport: mctp-demux: Don't test socket for non-zero value
- requester: Return PLDM_REQUESTER_OPEN_FAIL from pldm_open() on error
- pdr: pldm_pdr_fru_record_set_find_by_rsi(): Document reality of return
- transport: Fix possible NULL ptr deref in pldm_socket_sndbuf_init()
- abi: Update to remove pldm_close() from reference dumps
- bios_table: Annotate pldm_bios_table_attr_value_entry_encode_integer()

## [0.3.0] - 2023-06-23

### Added

- Add encode/decode pldmMessagePollEvent data
- README: Add a section on working with libpldm
- pdr: Introduce remote_container_id and associated APIs
- pdr: Add APIs for creating and locating remote PDRs
- pdr: Add pldm_pdr_find_last_in_range()
- pdr: Add pldm_entity_association_pdr_add_from_node_with_record_handle()
- pdr: Add pldm_pdr_find_container_id_range_exclude()

### Changed

- include: Move installed transport.h under libpldm/
- libpldm: Explicit deprecated, stable and testing ABI classes
- meson: Reduce strength of oem-ibm requirements from enabled to allowed

  The `oem-ibm` feature is now enabled by the default meson configuration, for
  CI purposes. `oem-ibm` is still disabled by default in the `libpldm` bitbake
  recipe:

  <https://github.com/openbmc/openbmc/blob/master/meta-phosphor/recipes-phosphor/libpldm/libpldm_git.bb#L10>

  To disable `oem-ibm` in your development builds, pass `-Doem-ibm=disabled`
  when invoking `meson setup`

- bios_table: Relax pldm_bios_table_string_entry_decode_string_check()
- bios_table: Relax pldm_bios_table_attr_entry_enum_decode_pv_hdls_check()

### Deprecated

- bios_table: Deprecate APIs with arguments sanitized using assert()

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

- libpldm: Remove the requester-api option

### Fixed

- requester: Make pldm_open() return existing fd
- transport: Prevent sticking in waiting for response
- transport: Match on response in pldm_transport_send_recv_msg()
- requester: Add check before accessing hdr in pldm_recv()
- bios_table: pldm_bios_table_attr_entry_string_info_check() NULL deref
