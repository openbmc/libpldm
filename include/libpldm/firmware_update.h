/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef FW_UPDATE_H
#define FW_UPDATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "compiler.h"

#include <libpldm/api.h>
#include <libpldm/base.h>
#include <libpldm/pldm_types.h>
#include <libpldm/utils.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PLDM_FWUP_COMPONENT_BITMAP_MULTIPLE		 8
#define PLDM_FWUP_INVALID_COMPONENT_COMPARISON_TIMESTAMP 0xffffffff
#define PLDM_QUERY_DEVICE_IDENTIFIERS_REQ_BYTES		 0

/** @brief Length of QueryDownstreamDevices response defined in DSP0267_1.1.0
 *  Table 15 - QueryDownstreamDevices command format.
 *
 *  1 byte for completion code
 *  1 byte for downstream device update supported
 *  2 bytes for number of downstream devices
 *  2 bytes for max number of downstream devices
 *  4 bytes for capabilities
 */
#define PLDM_QUERY_DOWNSTREAM_DEVICES_RESP_BYTES 10

/** @brief Length of QueryDownstreamIdentifiers request defined in DSP0267_1.1.0
 * 	Table 16 - QueryDownstreamIdentifiers command format.
 *
 *  4 bytes for data transfer handle
 *  1 byte for transfer operation flag
*/
#define PLDM_QUERY_DOWNSTREAM_IDENTIFIERS_REQ_BYTES 5

/** @brief Minimum length of QueryDownstreamIdentifiers response from DSP0267_1.1.0
 *  if the complement code is success.
 *
 *  1 byte for completion code
 *  4 bytes for next data transfer handle
 *  1 byte for transfer flag
 *  4 bytes for downstream devices length
 *  2 bytes for number of downstream devices
 */
#define PLDM_QUERY_DOWNSTREAM_IDENTIFIERS_RESP_MIN_LEN 12

/** @brief Minimum length of device descriptor, 2 bytes for descriptor type,
 *         2 bytes for descriptor length and at least 1 byte of descriptor data
 */
#define PLDM_FWUP_DEVICE_DESCRIPTOR_MIN_LEN 5

/** @brief Length of GetDownstreamFirmwareParameters request defined in DSP0267_1.1.0
 *
 * 4 bytes for Data Transfer Handle
 * 1 byte for Transfer Operation Flag
 */
#define PLDM_GET_DOWNSTREAM_FIRMWARE_PARAMETERS_REQ_BYTES 5

/** @brief Minimum length of GetDownstreamFirmwareParameters response from
 * DSP0267_1.1.0 if the completion code is success.
 *
 * 1 byte for completion code
 * 4 bytes for next data transfer handle
 * 1 byte for transfer flag
 * 4 bytes for FDP capabilities during update
 * 2 bytes for downstream device count
 */
#define PLDM_GET_DOWNSTREAM_FIRMWARE_PARAMETERS_RESP_MIN_LEN 12

/** @brief Minimum length of DownstreamDeviceParameterTable entry from
 * DSP0267_1.1.0 table 21 - DownstreamDeviceParameterTable
 *
 * 2 bytes for Downstream Device Index
 * 4 bytes for Active Component Comparison Stamp
 * 1 byte for Active Component Version String Type
 * 1 byte for Active Component Version String Length
 * 8 bytes for Active Component Release Date
 * 4 bytes for Pending Component Comparison Stamp
 * 1 byte for Pending Component Version String Type
 * 1 byte for Pending Component Version String Length
 * 8 bytes for Pending Component Release Date
 * 2 bytes for Component Activation Methods
 * 4 bytes for Capabilities During Update
 */
#define PLDM_DOWNSTREAM_DEVICE_PARAMETERS_ENTRY_MIN_LEN 36

#define PLDM_GET_FIRMWARE_PARAMETERS_REQ_BYTES 0
#define PLDM_FWUP_BASELINE_TRANSFER_SIZE       32
#define PLDM_FWUP_MIN_OUTSTANDING_REQ	       1
#define PLDM_GET_STATUS_REQ_BYTES	       0
/* Maximum progress percentage value*/
#define PLDM_FWUP_MAX_PROGRESS_PERCENT	       0x65
#define PLDM_CANCEL_UPDATE_COMPONENT_REQ_BYTES 0
#define PLDM_CANCEL_UPDATE_REQ_BYTES	       0

/** @brief PLDM component release data size in bytes defined in DSP0267_1.1.0
 * Table 14 - ComponentParameterTable and Table 21 - ComponentParameterTable
 *
 * The size can be used in `ASCII[8] - ActiveComponentReleaseDate` and
 * `ASCII[8] - PendingComponentReleaseDate` fields in the tables above.
 */
#define PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN 8

/** @brief PLDM Firmware update commands
 */
enum pldm_firmware_update_commands {
	PLDM_QUERY_DEVICE_IDENTIFIERS = 0x01,
	PLDM_GET_FIRMWARE_PARAMETERS = 0x02,
	PLDM_QUERY_DOWNSTREAM_DEVICES = 0x03,
	PLDM_QUERY_DOWNSTREAM_IDENTIFIERS = 0x04,
	PLDM_QUERY_DOWNSTREAM_FIRMWARE_PARAMETERS = 0x05,
	PLDM_REQUEST_UPDATE = 0x10,
	PLDM_GET_PACKAGE_DATA = 0x11,
	PLDM_GET_DEVICE_META_DATA = 0x12,
	PLDM_PASS_COMPONENT_TABLE = 0x13,
	PLDM_UPDATE_COMPONENT = 0x14,
	PLDM_REQUEST_FIRMWARE_DATA = 0x15,
	PLDM_TRANSFER_COMPLETE = 0x16,
	PLDM_VERIFY_COMPLETE = 0x17,
	PLDM_APPLY_COMPLETE = 0x18,
	PLDM_GET_META_DATA = 0x19,
	PLDM_ACTIVATE_FIRMWARE = 0x1a,
	PLDM_GET_STATUS = 0x1b,
	PLDM_CANCEL_UPDATE_COMPONENT = 0x1c,
	PLDM_CANCEL_UPDATE = 0x1d,
	PLDM_ACTIVATE_PENDING_COMPONENT_IMAGE_SET = 0x1e,
	PLDM_ACTIVATE_PENDING_COMPONENT_IMAGE = 0x1f,
	PLDM_REQUEST_DOWNSTREAM_DEVICE_UPDATE = 0x20,
	PLDM_GET_COMPONENT_OPAQUE_DATA = 0x21,
	PLDM_UPTATE_SECURITY_REVISION = 0x22
};

/** @brief PLDM Firmware update completion codes
 */
enum pldm_firmware_update_completion_codes {
	PLDM_FWUP_NOT_IN_UPDATE_MODE = 0x80,
	PLDM_FWUP_ALREADY_IN_UPDATE_MODE = 0x81,
	PLDM_FWUP_DATA_OUT_OF_RANGE = 0x82,
	PLDM_FWUP_INVALID_TRANSFER_LENGTH = 0x83,
	PLDM_FWUP_INVALID_STATE_FOR_COMMAND = 0x84,
	PLDM_FWUP_INCOMPLETE_UPDATE = 0x85,
	PLDM_FWUP_BUSY_IN_BACKGROUND = 0x86,
	PLDM_FWUP_CANCEL_PENDING = 0x87,
	PLDM_FWUP_COMMAND_NOT_EXPECTED = 0x88,
	PLDM_FWUP_RETRY_REQUEST_FW_DATA = 0x89,
	PLDM_FWUP_UNABLE_TO_INITIATE_UPDATE = 0x8a,
	PLDM_FWUP_ACTIVATION_NOT_REQUIRED = 0x8b,
	PLDM_FWUP_SELF_CONTAINED_ACTIVATION_NOT_PERMITTED = 0x8c,
	PLDM_FWUP_NO_DEVICE_METADATA = 0x8d,
	PLDM_FWUP_RETRY_REQUEST_UPDATE = 0x8e,
	PLDM_FWUP_NO_PACKAGE_DATA = 0x8f,
	PLDM_FWUP_INVALID_TRANSFER_HANDLE = 0x90,
	PLDM_FWUP_INVALID_TRANSFER_OPERATION_FLAG = 0x91,
	PLDM_FWUP_ACTIVATE_PENDING_IMAGE_NOT_PERMITTED = 0x92,
	PLDM_FWUP_PACKAGE_DATA_ERROR = 0x93,
	PLDM_FWUP_NO_OPAQUE_DATA = 0x94,
	PLDM_FWUP_UPDATE_SECURITY_REVISION_NOT_PERMITTED = 0x95,
	PLDM_FWUP_DOWNSTREAM_DEVICE_LIST_CHANGED = 0x96
};

/** @brief String type values defined in the PLDM firmware update specification
 */
enum pldm_firmware_update_string_type {
	PLDM_STR_TYPE_UNKNOWN = 0,
	PLDM_STR_TYPE_ASCII = 1,
	PLDM_STR_TYPE_UTF_8 = 2,
	PLDM_STR_TYPE_UTF_16 = 3,
	PLDM_STR_TYPE_UTF_16LE = 4,
	PLDM_STR_TYPE_UTF_16BE = 5
};

/** @brief Descriptor types defined in PLDM firmware update specification
 */
enum pldm_firmware_update_descriptor_types {
	PLDM_FWUP_PCI_VENDOR_ID = 0x0000,
	PLDM_FWUP_IANA_ENTERPRISE_ID = 0x0001,
	PLDM_FWUP_UUID = 0x0002,
	PLDM_FWUP_PNP_VENDOR_ID = 0x0003,
	PLDM_FWUP_ACPI_VENDOR_ID = 0x0004,
	PLDM_FWUP_IEEE_ASSIGNED_COMPANY_ID = 0x0005,
	PLDM_FWUP_SCSI_VENDOR_ID = 0x0006,
	PLDM_FWUP_PCI_DEVICE_ID = 0x0100,
	PLDM_FWUP_PCI_SUBSYSTEM_VENDOR_ID = 0x0101,
	PLDM_FWUP_PCI_SUBSYSTEM_ID = 0x0102,
	PLDM_FWUP_PCI_REVISION_ID = 0x0103,
	PLDM_FWUP_PNP_PRODUCT_IDENTIFIER = 0x0104,
	PLDM_FWUP_ACPI_PRODUCT_IDENTIFIER = 0x0105,
	PLDM_FWUP_ASCII_MODEL_NUMBER_LONG_STRING = 0x0106,
	PLDM_FWUP_ASCII_MODEL_NUMBER_SHORT_STRING = 0x0107,
	PLDM_FWUP_SCSI_PRODUCT_ID = 0x0108,
	PLDM_FWUP_UBM_CONTROLLER_DEVICE_CODE = 0x0109,
	PLDM_FWUP_IEEE_EUI_64_ID = 0x010a,
	PLDM_FWUP_PCI_REVISION_ID_RANGE = 0x010b,
	PLDM_FWUP_VENDOR_DEFINED = 0xffff
};

/** @brief Descriptor types length defined in PLDM firmware update specification
 */
enum pldm_firmware_update_descriptor_types_length {
	PLDM_FWUP_PCI_VENDOR_ID_LENGTH = 2,
	PLDM_FWUP_IANA_ENTERPRISE_ID_LENGTH = 4,
	PLDM_FWUP_UUID_LENGTH = 16,
	PLDM_FWUP_PNP_VENDOR_ID_LENGTH = 3,
	PLDM_FWUP_ACPI_VENDOR_ID_LENGTH = 4,
	PLDM_FWUP_IEEE_ASSIGNED_COMPANY_ID_LENGTH = 3,
	PLDM_FWUP_SCSI_VENDOR_ID_LENGTH = 8,
	PLDM_FWUP_PCI_DEVICE_ID_LENGTH = 2,
	PLDM_FWUP_PCI_SUBSYSTEM_VENDOR_ID_LENGTH = 2,
	PLDM_FWUP_PCI_SUBSYSTEM_ID_LENGTH = 2,
	PLDM_FWUP_PCI_REVISION_ID_LENGTH = 1,
	PLDM_FWUP_PNP_PRODUCT_IDENTIFIER_LENGTH = 4,
	PLDM_FWUP_ACPI_PRODUCT_IDENTIFIER_LENGTH = 4,
	PLDM_FWUP_ASCII_MODEL_NUMBER_LONG_STRING_LENGTH = 40,
	PLDM_FWUP_ASCII_MODEL_NUMBER_SHORT_STRING_LENGTH = 10,
	PLDM_FWUP_SCSI_PRODUCT_ID_LENGTH = 16,
	PLDM_FWUP_UBM_CONTROLLER_DEVICE_CODE_LENGTH = 4,
	PLDM_FWUP_IEEE_EUI_64_ID_LENGTH = 8,
	PLDM_FWUP_PCI_REVISION_ID_RANGE_LENGTH = 2
};

/** @brief ComponentClassification values defined in firmware update
 *         specification
 */
enum pldm_component_classification_values {
	PLDM_COMP_UNKNOWN = 0x0000,
	PLDM_COMP_OTHER = 0x0001,
	PLDM_COMP_DRIVER = 0x0002,
	PLDM_COMP_CONFIGURATION_SOFTWARE = 0x0003,
	PLDM_COMP_APPLICATION_SOFTWARE = 0x0004,
	PLDM_COMP_INSTRUMENTATION = 0x0005,
	PLDM_COMP_FIRMWARE_OR_BIOS = 0x0006,
	PLDM_COMP_DIAGNOSTIC_SOFTWARE = 0x0007,
	PLDM_COMP_OPERATING_SYSTEM = 0x0008,
	PLDM_COMP_MIDDLEWARE = 0x0009,
	PLDM_COMP_FIRMWARE = 0x000a,
	PLDM_COMP_BIOS_OR_FCODE = 0x000b,
	PLDM_COMP_SUPPORT_OR_SERVICEPACK = 0x000c,
	PLDM_COMP_SOFTWARE_BUNDLE = 0x000d,
	PLDM_COMP_DOWNSTREAM_DEVICE = 0xffff
};

/** @brief ComponentActivationMethods is the bit position in the bitfield that
 *         provides the capability of the FD for firmware activation. Multiple
 *         activation methods can be supported.
 */
enum pldm_comp_activation_methods {
	PLDM_ACTIVATION_AUTOMATIC = 0,
	PLDM_ACTIVATION_SELF_CONTAINED = 1,
	PLDM_ACTIVATION_MEDIUM_SPECIFIC_RESET = 2,
	PLDM_ACTIVATION_SYSTEM_REBOOT = 3,
	PLDM_ACTIVATION_DC_POWER_CYCLE = 4,
	PLDM_ACTIVATION_AC_POWER_CYCLE = 5,
	PLDM_SUPPORTS_ACTIVATE_PENDING_IMAGE = 6,
	PLDM_SUPPORTS_ACTIVATE_PENDING_IMAGE_SET = 7
};

/** @brief ComponentResponse values in the response of PassComponentTable
 */
enum pldm_component_responses {
	PLDM_CR_COMP_CAN_BE_UPDATED = 0,
	PLDM_CR_COMP_MAY_BE_UPDATEABLE = 1
};

/** @brief ComponentResponseCode values in the response of PassComponentTable
 */
enum pldm_component_response_codes {
	PLDM_CRC_COMP_CAN_BE_UPDATED = 0x00,
	PLDM_CRC_COMP_COMPARISON_STAMP_IDENTICAL = 0x01,
	PLDM_CRC_COMP_COMPARISON_STAMP_LOWER = 0x02,
	PLDM_CRC_INVALID_COMP_COMPARISON_STAMP = 0x03,
	PLDM_CRC_COMP_CONFLICT = 0x04,
	PLDM_CRC_COMP_PREREQUISITES_NOT_MET = 0x05,
	PLDM_CRC_COMP_NOT_SUPPORTED = 0x06,
	PLDM_CRC_COMP_SECURITY_RESTRICTIONS = 0x07,
	PLDM_CRC_INCOMPLETE_COMP_IMAGE_SET = 0x08,
	PLDM_CRC_ACTIVE_IMAGE_NOT_UPDATEABLE_SUBSEQUENTLY = 0x09,
	PLDM_CRC_COMP_VER_STR_IDENTICAL = 0x0a,
	PLDM_CRC_COMP_VER_STR_LOWER = 0x0b,
	PLDM_CRC_VENDOR_COMP_RESP_CODE_RANGE_MIN = 0xd0,
	PLDM_CRC_VENDOR_COMP_RESP_CODE_RANGE_MAX = 0xef
};

/** @brief ComponentCompatibilityResponse values in the response of
 *         UpdateComponent
 */
enum pldm_component_compatibility_responses {
	PLDM_CCR_COMP_CAN_BE_UPDATED = 0,
	PLDM_CCR_COMP_CANNOT_BE_UPDATED = 1
};

/** @brief ComponentCompatibilityResponse Code values in the response of
 *         UpdateComponent
 */
enum pldm_component_compatibility_response_codes {
	PLDM_CCRC_NO_RESPONSE_CODE = 0x00,
	PLDM_CCRC_COMP_COMPARISON_STAMP_IDENTICAL = 0x01,
	PLDM_CCRC_COMP_COMPARISON_STAMP_LOWER = 0x02,
	PLDM_CCRC_INVALID_COMP_COMPARISON_STAMP = 0x03,
	PLDM_CCRC_COMP_CONFLICT = 0x04,
	PLDM_CCRC_COMP_PREREQUISITES_NOT_MET = 0x05,
	PLDM_CCRC_COMP_NOT_SUPPORTED = 0x06,
	PLDM_CCRC_COMP_SECURITY_RESTRICTIONS = 0x07,
	PLDM_CCRC_INCOMPLETE_COMP_IMAGE_SET = 0x08,
	PLDM_CCRC_COMP_INFO_NO_MATCH = 0x09,
	PLDM_CCRC_COMP_VER_STR_IDENTICAL = 0x0a,
	PLDM_CCRC_COMP_VER_STR_LOWER = 0x0b,
	PLDM_CCRC_VENDOR_COMP_RESP_CODE_RANGE_MIN = 0xd0,
	PLDM_CCRC_VENDOR_COMP_RESP_CODE_RANGE_MAX = 0xef
};

/** @brief Common error codes in TransferComplete, VerifyComplete and
 *        ApplyComplete request
 */
enum pldm_firmware_update_common_error_codes {
	PLDM_FWUP_TIME_OUT = 0x09,
	PLDM_FWUP_GENERIC_ERROR = 0x0a
};

/** @brief TransferResult values in the request of TransferComplete
 */
enum pldm_firmware_update_transfer_result_values {
	PLDM_FWUP_TRANSFER_SUCCESS = 0x00,
	PLDM_FWUP_TRANSFER_ERROR_IMAGE_CORRUPT = 0x02,
	PLDM_FWUP_TRANSFER_ERROR_VERSION_MISMATCH = 0x02,
	PLDM_FWUP_FD_ABORTED_TRANSFER = 0x03,
	PLDM_FWUP_FD_ABORTED_TRANSFER_LOW_POWER_STATE = 0x0b,
	PLDM_FWUP_FD_ABORTED_TRANSFER_RESET_NEEDED = 0x0c,
	PLDM_FWUP_FD_ABORTED_TRANSFER_STORAGE_ISSUE = 0x0d,
	PLDM_FWUP_FD_ABORTED_TRANSFER_INVALID_COMPONENT_OPAQUE_DATA = 0x0e,
	PLDM_FWUP_FD_ABORTED_TRANSFER_DOWNSTREAM_DEVICE_FAILURE = 0x0f,
	PLDM_FWUP_FD_ABORTED_TRANSFER_SECURITY_REVISION_ERROR = 0x10,
	PLDM_FWUP_VENDOR_TRANSFER_RESULT_RANGE_MIN = 0x70,
	PLDM_FWUP_VENDOR_TRANSFER_RESULT_RANGE_MAX = 0x8f
};

/**@brief VerifyResult values in the request of VerifyComplete
 */
enum pldm_firmware_update_verify_result_values {
	PLDM_FWUP_VERIFY_SUCCESS = 0x00,
	PLDM_FWUP_VERIFY_ERROR_VERIFICATION_FAILURE = 0x01,
	PLDM_FWUP_VERIFY_ERROR_VERSION_MISMATCH = 0x02,
	PLDM_FWUP_VERIFY_FAILED_FD_SECURITY_CHECKS = 0x03,
	PLDM_FWUP_VERIFY_ERROR_IMAGE_INCOMPLETE = 0x04,
	PLDM_FWUP_VERIFY_FAILURE_SECURITY_REVISION_ERROR = 0x10,
	PLDM_FWUP_VENDOR_VERIFY_RESULT_RANGE_MIN = 0x90,
	PLDM_FWUP_VENDOR_VERIFY_RESULT_RANGE_MAX = 0xaf
};

/**@brief ApplyResult values in the request of ApplyComplete
 */
enum pldm_firmware_update_apply_result_values {
	PLDM_FWUP_APPLY_SUCCESS = 0x00,
	PLDM_FWUP_APPLY_SUCCESS_WITH_ACTIVATION_METHOD = 0x01,
	PLDM_FWUP_APPLY_FAILURE_MEMORY_ISSUE = 0x02,
	PLDM_FWUP_APPLY_FAILURE_SECURITY_REVISION_ERROR = 0x10,
	PLDM_FWUP_VENDOR_APPLY_RESULT_RANGE_MIN = 0xb0,
	PLDM_FWUP_VENDOR_APPLY_RESULT_RANGE_MAX = 0xcf
};

/** @brief SelfContainedActivationRequest in the request of ActivateFirmware
 */
enum pldm_self_contained_activation_req {
	PLDM_NOT_ACTIVATE_SELF_CONTAINED_COMPONENTS = false,
	PLDM_ACTIVATE_SELF_CONTAINED_COMPONENTS = true
};

/** @brief Current state/previous state of the FD or FDP returned in GetStatus
 *         response
 */
enum pldm_firmware_device_states {
	PLDM_FD_STATE_IDLE = 0,
	PLDM_FD_STATE_LEARN_COMPONENTS = 1,
	PLDM_FD_STATE_READY_XFER = 2,
	PLDM_FD_STATE_DOWNLOAD = 3,
	PLDM_FD_STATE_VERIFY = 4,
	PLDM_FD_STATE_APPLY = 5,
	PLDM_FD_STATE_ACTIVATE = 6
};

/** @brief Firmware device aux state in GetStatus response
 */
enum pldm_get_status_aux_states {
	PLDM_FD_OPERATION_IN_PROGRESS = 0,
	PLDM_FD_OPERATION_SUCCESSFUL = 1,
	PLDM_FD_OPERATION_FAILED = 2,
	PLDM_FD_IDLE_LEARN_COMPONENTS_READ_XFER = 3,
	PLDM_FD_IDLE_SELF_CONTAINED_ACTIVATION_FAILURE = 4
};

/** @brief Firmware device aux state status in GetStatus response
 */
enum pldm_get_status_aux_state_status_values {
	PLDM_FD_AUX_STATE_IN_PROGRESS_OR_SUCCESS = 0x00,
	PLDM_FD_TIMEOUT = 0x09,
	PLDM_FD_GENERIC_ERROR = 0x0a,
	PLDM_FD_SELF_CONTAINED_ACTIVATION_FAILURE = 0x0b,
	PLDM_FD_VENDOR_DEFINED_STATUS_CODE_START = 0x70,
	PLDM_FD_VENDOR_DEFINED_STATUS_CODE_END = 0xef
};

/** @brief Firmware device reason code in GetStatus response
 */
enum pldm_get_status_reason_code_values {
	PLDM_FD_INITIALIZATION = 0,
	PLDM_FD_ACTIVATE_FW = 1,
	PLDM_FD_CANCEL_UPDATE = 2,
	PLDM_FD_TIMEOUT_LEARN_COMPONENT = 3,
	PLDM_FD_TIMEOUT_READY_XFER = 4,
	PLDM_FD_TIMEOUT_DOWNLOAD = 5,
	PLDM_FD_TIMEOUT_VERIFY = 6,
	PLDM_FD_TIMEOUT_APPLY = 7,
	PLDM_FD_STATUS_VENDOR_DEFINED_MIN = 200,
	PLDM_FD_STATUS_VENDOR_DEFINED_MAX = 255
};

/** @brief Components functional indicator in CancelUpdate response
 */
enum pldm_firmware_update_non_functioning_component_indication {
	PLDM_FWUP_COMPONENTS_FUNCTIONING = 0,
	PLDM_FWUP_COMPONENTS_NOT_FUNCTIONING = 1
};

/** @brief Downstream device update supported in QueryDownstreamDevices response
 *         defined in DSP0267_1.1.0
*/
enum pldm_firmware_update_downstream_device_update_supported {
	PLDM_FWUP_DOWNSTREAM_DEVICE_UPDATE_NOT_SUPPORTED = 0,
	PLDM_FWUP_DOWNSTREAM_DEVICE_UPDATE_SUPPORTED = 1
};

/* An arbitrary limit, for static storage */
#define PLDM_FIRMWARE_MAX_STRING 64

/** @struct pldm_firmware_string
 *
 *  A fixed maximum length PLDM firmware string
*/
struct pldm_firmware_string {
	enum pldm_firmware_update_string_type str_type;
	uint8_t str_len;
	uint8_t str_data[PLDM_FIRMWARE_MAX_STRING];
};

/** @struct pldm_firmware_version
 *
 *  A PLDM component version
*/
struct pldm_firmware_version {
	uint32_t comparison_stamp;
	struct pldm_firmware_string str;
	uint8_t date[PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN];
};

/** @struct pldm_package_header_information
 *
 *  Structure representing fixed part of package header information
 */
struct pldm_package_header_information {
	uint8_t uuid[PLDM_FWUP_UUID_LENGTH];
	uint8_t package_header_format_version;
	uint16_t package_header_size;
	uint8_t package_release_date_time[PLDM_TIMESTAMP104_SIZE];
	uint16_t component_bitmap_bit_length;
	uint8_t package_version_string_type;
	uint8_t package_version_string_length;
} __attribute__((packed));

/** @struct pldm_firmware_device_id_record
 *
 *  Structure representing firmware device ID record
 */
struct pldm_firmware_device_id_record {
	uint16_t record_length;
	uint8_t descriptor_count;
	bitfield32_t device_update_option_flags;
	uint8_t comp_image_set_version_string_type;
	uint8_t comp_image_set_version_string_length;
	uint16_t fw_device_pkg_data_length;
} __attribute__((packed));

/** @struct pldm_descriptor_tlv
 *
 *  Structure representing descriptor type, length and value
 */
struct pldm_descriptor_tlv {
	uint16_t descriptor_type;
	uint16_t descriptor_length;
	uint8_t descriptor_data[1];
} __attribute__((packed));

/** @struct pldm_vendor_defined_descriptor_title_data
 *
 *  Structure representing vendor defined descriptor title sections
 */
struct pldm_vendor_defined_descriptor_title_data {
	uint8_t vendor_defined_descriptor_title_str_type;
	uint8_t vendor_defined_descriptor_title_str_len;
	uint8_t vendor_defined_descriptor_title_str[1];
} __attribute__((packed));

/** @struct pldm_component_image_information
 *
 *  Structure representing fixed part of individual component information in
 *  PLDM firmware update package
 */
struct pldm_component_image_information {
	uint16_t comp_classification;
	uint16_t comp_identifier;
	uint32_t comp_comparison_stamp;
	bitfield16_t comp_options;
	bitfield16_t requested_comp_activation_method;
	uint32_t comp_location_offset;
	uint32_t comp_size;
	uint8_t comp_version_string_type;
	uint8_t comp_version_string_length;
} __attribute__((packed));

/** @struct pldm_query_device_identifiers_resp
 *
 *  Structure representing query device identifiers response.
 */
struct pldm_query_device_identifiers_resp {
	uint8_t completion_code;
	uint32_t device_identifiers_len;
	uint8_t descriptor_count;
} __attribute__((packed));

/** @struct pldm_get_firmware_parameters_resp
 *
 *  Structure representing the fixed part of GetFirmwareParameters response
 */
struct pldm_get_firmware_parameters_resp {
	uint8_t completion_code;
	bitfield32_t capabilities_during_update;
	uint16_t comp_count;
	uint8_t active_comp_image_set_ver_str_type;
	uint8_t active_comp_image_set_ver_str_len;
	uint8_t pending_comp_image_set_ver_str_type;
	uint8_t pending_comp_image_set_ver_str_len;
} __attribute__((packed));

/** @struct pldm_get_firmware_parameters_resp_full
 *
 *  Structure representing a full GetFirmwareParameters response
 */
struct pldm_get_firmware_parameters_resp_full {
	uint8_t completion_code;
	bitfield32_t capabilities_during_update;
	uint16_t comp_count;
	struct pldm_firmware_string active_comp_image_set_ver_str;
	struct pldm_firmware_string pending_comp_image_set_ver_str;
};

/** @struct pldm_query_downstream_devices_resp
 *
 *  Structure representing response of QueryDownstreamDevices.
 *  The definition can be found Table 15 - QueryDownstreamDevices command format
 *  in DSP0267_1.1.0
 */
struct pldm_query_downstream_devices_resp {
	uint8_t completion_code;
	uint8_t downstream_device_update_supported;
	uint16_t number_of_downstream_devices;
	uint16_t max_number_of_downstream_devices;
	bitfield32_t capabilities;
};

/** @struct pldm_component_parameter_entry
 *
 *  Structure representing component parameter table entry, as wire format.
 */
struct pldm_component_parameter_entry {
	uint16_t comp_classification;
	uint16_t comp_identifier;
	uint8_t comp_classification_index;
	uint32_t active_comp_comparison_stamp;
	uint8_t active_comp_ver_str_type;
	uint8_t active_comp_ver_str_len;
	uint8_t active_comp_release_date[8];
	uint32_t pending_comp_comparison_stamp;
	uint8_t pending_comp_ver_str_type;
	uint8_t pending_comp_ver_str_len;
	uint8_t pending_comp_release_date[8];
	bitfield16_t comp_activation_methods;
	bitfield32_t capabilities_during_update;
} __attribute__((packed));

/** @struct pldm_component_parameter_entry_full
 *
 *  Structure representing component parameter table entry.
 *  This is non-packed (contrast with struct pldm_component_parameter_entry),
 *  with version strings included.
 */
struct pldm_component_parameter_entry_full {
	uint16_t comp_classification;
	uint16_t comp_identifier;
	uint8_t comp_classification_index;

	struct pldm_firmware_version active_ver;
	struct pldm_firmware_version pending_ver;

	bitfield16_t comp_activation_methods;
	bitfield32_t capabilities_during_update;
};

/** @struct pldm_query_downstream_identifiers_req
 *
 *  Structure for QueryDownstreamIdentifiers request defined in Table 16 -
 *  QueryDownstreamIdentifiers command format in DSP0267_1.1.0
 */
struct pldm_query_downstream_identifiers_req {
	uint32_t data_transfer_handle;
	uint8_t transfer_operation_flag;
};

/** @struct pldm_query_downstream_identifiers_resp
 *
 *  Structure representing the fixed part of QueryDownstreamIdentifiers response
 *  defined in Table 16 - QueryDownstreamIdentifiers command format, and
 *  Table 17 - QueryDownstreamIdentifiers response definition in DSP0267_1.1.0.
 *
 *  Squash the two tables into one since the definition of
 *  Table 17 is `Portion of QueryDownstreamIdentifiers response`
 */
struct pldm_query_downstream_identifiers_resp {
	uint8_t completion_code;
	uint32_t next_data_transfer_handle;
	uint8_t transfer_flag;
	uint32_t downstream_devices_length;
	uint16_t number_of_downstream_devices;
};

/** @struct pldm_downstream_device
 *
 *  Structure representing downstream device information defined in
 *  Table 18 - DownstreamDevice definition in DSP0267_1.1.0
 */
struct pldm_downstream_device {
	uint16_t downstream_device_index;
	uint8_t downstream_descriptor_count;
};
#define PLDM_DOWNSTREAM_DEVICE_BYTES 3

struct pldm_downstream_device_iter {
	struct variable_field field;
	size_t devs;
};

LIBPLDM_ITERATOR
bool pldm_downstream_device_iter_end(
	const struct pldm_downstream_device_iter *iter)
{
	return !iter->devs;
}

LIBPLDM_ITERATOR
bool pldm_downstream_device_iter_next(struct pldm_downstream_device_iter *iter)
{
	if (!iter->devs) {
		return false;
	}

	iter->devs--;
	return true;
}

int decode_pldm_downstream_device_from_iter(
	struct pldm_downstream_device_iter *iter,
	struct pldm_downstream_device *dev);

/** @brief Iterate downstream devices in QueryDownstreamIdentifiers response
 *
 * @param devs The @ref "struct pldm_downstream_device_iter" lvalue used as the
 *                  out-value from the corresponding call to @ref
 *                  decode_query_downstream_identifiers_resp
 * @param dev The @ref "struct pldm_downstream_device" lvalue into which the
 *            next device instance should be decoded.
 * @param rc An lvalue of type int into which the return code from the decoding
 *           will be placed.
 *
 * Example use of the macro is as follows:
 *
 * @code
 * struct pldm_query_downstream_identifiers_resp resp;
 * struct pldm_downstream_device_iter devs;
 * struct pldm_downstream_device dev;
 * int rc;
 *
 * rc = decode_query_downstream_identifiers_resp(..., &resp, &devs);
 * if (rc) {
 *     // Handle any error from decoding fixed-portion of response
 * }
 *
 * foreach_pldm_downstream_device(devs, dev, rc) {
 *     // Do something with each decoded device placed in `dev`
 * }
 *
 * if (rc) {
 *     // Handle any decoding error while iterating variable-length set of
 *     // devices
 * }
 * @endcode
 */
#define foreach_pldm_downstream_device(devs, dev, rc)                          \
	for ((rc) = 0; (!pldm_downstream_device_iter_end(&(devs)) &&           \
			!((rc) = decode_pldm_downstream_device_from_iter(      \
				  &(devs), &(dev))));                          \
	     pldm_downstream_device_iter_next(&(devs)))

/** @struct pldm_descriptor
 *
 * Structure representing a type-length-value descriptor as defined in Table 7 -
 * Descriptor Definition.
 *
 * Member values are always host-endian. When decoding messages, the
 * descriptor_data member points into the message buffer.
 */
struct pldm_descriptor {
	uint16_t descriptor_type;
	uint16_t descriptor_length;
	const void *descriptor_data;
};

struct pldm_descriptor_iter {
	struct variable_field *field;
	size_t count;
};

LIBPLDM_ITERATOR
struct pldm_descriptor_iter pldm_downstream_device_descriptor_iter_init(
	struct pldm_downstream_device_iter *devs,
	const struct pldm_downstream_device *dev)
{
	struct pldm_descriptor_iter iter = { &devs->field,
					     dev->downstream_descriptor_count };
	return iter;
}

LIBPLDM_ITERATOR
bool pldm_descriptor_iter_end(const struct pldm_descriptor_iter *iter)
{
	return !iter->count;
}

LIBPLDM_ITERATOR
bool pldm_descriptor_iter_next(struct pldm_descriptor_iter *iter)
{
	if (!iter->count) {
		return false;
	}

	iter->count--;
	return true;
}

int decode_pldm_descriptor_from_iter(struct pldm_descriptor_iter *iter,
				     struct pldm_descriptor *desc);

/** @brief Iterate a downstream device's descriptors in a
 *         QueryDownstreamIdentifiers response
 *
 * @param devs The @ref "struct pldm_downstream_device_iter" lvalue used as the
 *                  out-value from the corresponding call to @ref
 *                  decode_query_downstream_identifiers_resp
 * @param dev The @ref "struct pldm_downstream_device" lvalue over whose
 *            descriptors to iterate
 * @param desc The @ref "struct pldm_descriptor" lvalue into which the next
 *             descriptor instance should be decoded
 * @param rc An lvalue of type int into which the return code from the decoding
 *           will be placed
 *
 * Example use of the macro is as follows:
 *
 * @code
 * struct pldm_query_downstream_identifiers_resp resp;
 * struct pldm_downstream_device_iter devs;
 * struct pldm_downstream_device dev;
 * int rc;
 *
 * rc = decode_query_downstream_identifiers_resp(..., &resp, &devs);
 * if (rc) {
 *     // Handle any error from decoding fixed-portion of response
 * }
 *
 * foreach_pldm_downstream_device(devs, dev, rc) {
 *     struct pldm_descriptor desc;
 *
 *     foreach_pldm_downstream_device_descriptor(devs, dev, desc, rc) {
 *         // Do something with each decoded descriptor placed in `desc`
 *     }
 *
 *     if (rc) {
 *         // Handle any decoding error while iterating on the variable-length
 *         // set of descriptors
 *     }
 * }
 *
 * if (rc) {
 *     // Handle any decoding error while iterating variable-length set of
 *     // devices
 * }
 * @endcode
 */
#define foreach_pldm_downstream_device_descriptor(devs, dev, desc, rc)         \
	for (struct pldm_descriptor_iter desc##_iter =                         \
		     ((rc) = 0, pldm_downstream_device_descriptor_iter_init(   \
					&(devs), &(dev)));                     \
	     (!pldm_descriptor_iter_end(&(desc##_iter)) &&                     \
	      !((rc) = decode_pldm_descriptor_from_iter(&(desc##_iter),        \
							&(desc))));            \
	     pldm_descriptor_iter_next(&(desc##_iter)))

/** @struct pldm_query_downstream_firmware_param_req
 *
 *  Structure representing QueryDownstreamFirmwareParameters request
 */
struct pldm_get_downstream_firmware_parameters_req {
	uint32_t data_transfer_handle;
	uint8_t transfer_operation_flag;
};

/** @struct pldm_get_downstream_firmware_parameters_resp
 *
 *  Structure representing the fixed part of QueryDownstreamFirmwareParameters
 *  response in Table 19 - GetDownstreamFirmwareParameters command format, and
 *  Table 20 - QueryDownstreamFirmwareParameters response definition in
 *  DSP0267_1.1.0.
 *
 *  Squash the two tables into one since the definition of Table 20 is `Portion
 *  of GetDownstreamFirmwareParameters response`
 */
struct pldm_get_downstream_firmware_parameters_resp {
	uint8_t completion_code;
	uint32_t next_data_transfer_handle;
	uint8_t transfer_flag;
	bitfield32_t fdp_capabilities_during_update;
	uint16_t downstream_device_count;
};

/** @struct pldm_downstream_device_parameters_entry
 *
 *  Structure representing downstream device parameter table entry defined in
 *  Table 21 - DownstreamDeviceParameterTable in DSP0267_1.1.0
 *
 *  Clients should not allocate memory for this struct to decode the response,
 *  use `pldm_downstream_device_parameter_entry_versions` instead to make sure
 *  that the active and pending component version strings are copied from the
 *  message buffer.
 */
struct pldm_downstream_device_parameters_entry {
	uint16_t downstream_device_index;
	uint32_t active_comp_comparison_stamp;
	uint8_t active_comp_ver_str_type;
	uint8_t active_comp_ver_str_len;
	/* Append 1 bytes for null termination so that it can be used as a
	 * Null-terminated string.
	 */
	char active_comp_release_date[PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN + 1];
	uint32_t pending_comp_comparison_stamp;
	uint8_t pending_comp_ver_str_type;
	uint8_t pending_comp_ver_str_len;
	/* Append 1 bytes for null termination so that it can be used as a
	 * Null-terminated string.
	 */
	char pending_comp_release_date[PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN + 1];
	bitfield16_t comp_activation_methods;
	bitfield32_t capabilities_during_update;
	const void *active_comp_ver_str;
	const void *pending_comp_ver_str;
};

/** @struct pldm_request_update_req
 *
 *  Structure representing fixed part of Request Update request, as wire format.
 */
struct pldm_request_update_req {
	uint32_t max_transfer_size;
	uint16_t num_of_comp;
	uint8_t max_outstanding_transfer_req;
	uint16_t pkg_data_len;
	uint8_t comp_image_set_ver_str_type;
	uint8_t comp_image_set_ver_str_len;
} __attribute__((packed));

/** @struct pldm_request_update_req_full
 *
 *  Structure representing fixed part of Request Update request, including
 *  version string. This is unpacked (contrast to struct pldm_request_update_req).
 */
struct pldm_request_update_req_full {
	uint32_t max_transfer_size;
	uint16_t num_of_comp;
	uint8_t max_outstanding_transfer_req;
	uint16_t pkg_data_len;

	struct pldm_firmware_string image_set_ver;
};

/** @struct pldm_request_update_resp
 *
 *  Structure representing Request Update response
 */
struct pldm_request_update_resp {
	uint8_t completion_code;
	uint16_t fd_meta_data_len;
	uint8_t fd_will_send_pkg_data;
} __attribute__((packed));

/** @struct pldm_request_downstream_dev_update_req
 *
 *  Structure representing Request Downstream Device Update request
 */
struct pldm_request_downstream_device_update_req {
	uint32_t maximum_downstream_device_transfer_size;
	uint8_t maximum_outstanding_transfer_requests;
	uint16_t downstream_device_package_data_length;
};
#define PLDM_DOWNSTREAM_DEVICE_UPDATE_REQUEST_BYTES 7

/** @struct pldm_request_downstream_dev_update_resp
 *
 *  Structure representing Request Downstream Device Update response
 */
struct pldm_request_downstream_device_update_resp {
	uint8_t completion_code;
	uint16_t downstream_device_meta_data_length;
	uint8_t downstream_device_will_send_get_package_data;
	uint16_t get_package_data_maximum_transfer_size;
};
#define PLDM_DOWNSTREAM_DEVICE_UPDATE_RESPONSE_BYTES 6

/** @struct pldm_pass_component_table_req
 *
 *  Structure representing PassComponentTable request, wire format.
 *  Version string data is not included.
 *  Prefer pldm_pass_component_table_req_full for new uses.
 */
struct pldm_pass_component_table_req {
	uint8_t transfer_flag;
	uint16_t comp_classification;
	uint16_t comp_identifier;
	uint8_t comp_classification_index;
	uint32_t comp_comparison_stamp;
	uint8_t comp_ver_str_type;
	uint8_t comp_ver_str_len;
} __attribute__((packed));

/** @struct pldm_pass_component_table_req_full
 *
 *  Structure representing PassComponentTable request, including
 *  version string storage.
 */
struct pldm_pass_component_table_req_full {
	uint8_t transfer_flag;
	uint16_t comp_classification;
	uint16_t comp_identifier;
	uint8_t comp_classification_index;
	uint32_t comp_comparison_stamp;
	struct pldm_firmware_string version;
};

/** @struct pldm_pass_component_table_resp
 *
 *  Structure representing PassComponentTable response
 */
struct pldm_pass_component_table_resp {
	uint8_t completion_code;
	uint8_t comp_resp;
	uint8_t comp_resp_code;
} __attribute__((packed));

/** @struct pldm_update_component_req
 *
 *  Structure representing UpdateComponent request, wire format.
 *  Version string data is not included.
 *  Prefer pldm_update_component_req_full for new uses.
 */
struct pldm_update_component_req {
	uint16_t comp_classification;
	uint16_t comp_identifier;
	uint8_t comp_classification_index;
	uint32_t comp_comparison_stamp;
	uint32_t comp_image_size;
	bitfield32_t update_option_flags;
	uint8_t comp_ver_str_type;
	uint8_t comp_ver_str_len;
} __attribute__((packed));

/** @struct pldm_update_component_req_full
 *
 *  Structure representing UpdateComponent request, including
 *  version string storage.
 */
struct pldm_update_component_req_full {
	uint16_t comp_classification;
	uint16_t comp_identifier;
	uint8_t comp_classification_index;

	uint32_t comp_comparison_stamp;
	struct pldm_firmware_string version;

	uint32_t comp_image_size;
	bitfield32_t update_option_flags;
};

/** @struct pldm_update_component_resp
 *
 *  Structure representing UpdateComponent response
 */
struct pldm_update_component_resp {
	uint8_t completion_code;
	uint8_t comp_compatibility_resp;
	uint8_t comp_compatibility_resp_code;
	bitfield32_t update_option_flags_enabled;
	uint16_t time_before_req_fw_data;
} __attribute__((packed));

/** @struct pldm_request_firmware_data_req
 *
 *  Structure representing RequestFirmwareData request.
 */
struct pldm_request_firmware_data_req {
	uint32_t offset;
	uint32_t length;
} __attribute__((packed));

/** @struct pldm_apply_complete_req
 *
 *  Structure representing ApplyComplete request.
 */
struct pldm_apply_complete_req {
	uint8_t apply_result;
	bitfield16_t comp_activation_methods_modification;
} __attribute__((packed));

/** @struct pldm_activate_firmware_req
 *
 *  Structure representing ActivateFirmware request
 */
struct pldm_activate_firmware_req {
	bool8_t self_contained_activation_req;
} __attribute__((packed));

/** @struct activate_firmware_resp
 *
 *  Structure representing Activate Firmware response
 */
struct pldm_activate_firmware_resp {
	uint8_t completion_code;
	uint16_t estimated_time_activation;
} __attribute__((packed));

/** @struct pldm_get_status_resp
 *
 *  Structure representing GetStatus response.
 */
struct pldm_get_status_resp {
	uint8_t completion_code;
	uint8_t current_state;
	uint8_t previous_state;
	uint8_t aux_state;
	uint8_t aux_state_status;
	uint8_t progress_percent;
	uint8_t reason_code;
	bitfield32_t update_option_flags_enabled;
} __attribute__((packed));

/** @struct pldm_cancel_update_resp
 *
 *  Structure representing CancelUpdate response.
 */
struct pldm_cancel_update_resp {
	uint8_t completion_code;
	bool8_t non_functioning_component_indication;
	uint64_t non_functioning_component_bitmap;
} __attribute__((packed));

/** @brief Decode the PLDM package header information
 *
 *  @param[in] data - pointer to package header information
 *  @param[in] length - available length in the firmware update package
 *  @param[out] package_header_info - pointer to fixed part of PLDM package
 *                                    header information
 *  @param[out] package_version_str - pointer to package version string
 *
 *  @return pldm_completion_codes
 */
int decode_pldm_package_header_info(
	const uint8_t *data, size_t length,
	struct pldm_package_header_information *package_header_info,
	struct variable_field *package_version_str);

/** @brief Decode individual firmware device ID record
 *
 *  @param[in] data - pointer to firmware device ID record
 *  @param[in] length - available length in the firmware update package
 *  @param[in] component_bitmap_bit_length - ComponentBitmapBitLengthfield
 *                                           parsed from the package header info
 *  @param[out] fw_device_id_record - pointer to fixed part of firmware device
 *                                    id record
 *  @param[out] applicable_components - pointer to ApplicableComponents
 *  @param[out] comp_image_set_version_str - pointer to
 *                                           ComponentImageSetVersionString
 *  @param[out] record_descriptors - pointer to RecordDescriptors
 *  @param[out] fw_device_pkg_data - pointer to FirmwareDevicePackageData
 *
 *  @return pldm_completion_codes
 */
int decode_firmware_device_id_record(
	const uint8_t *data, size_t length,
	uint16_t component_bitmap_bit_length,
	struct pldm_firmware_device_id_record *fw_device_id_record,
	struct variable_field *applicable_components,
	struct variable_field *comp_image_set_version_str,
	struct variable_field *record_descriptors,
	struct variable_field *fw_device_pkg_data);

/** @brief Decode the record descriptor entries in the firmware update package
 *         and the Descriptors in the QueryDeviceIDentifiers command
 *
 *  @param[in] data - pointer to descriptor entry
 *  @param[in] length - remaining length of the descriptor data
 *  @param[out] descriptor_type - pointer to descriptor type
 *  @param[out] descriptor_data - pointer to descriptor data
 *
 *  @return pldm_completion_codes
 */
int decode_descriptor_type_length_value(const uint8_t *data, size_t length,
					uint16_t *descriptor_type,
					struct variable_field *descriptor_data);

/** @brief Decode the vendor defined descriptor value
 *
 *  @param[in] data - pointer to vendor defined descriptor value
 *  @param[in] length - length of the vendor defined descriptor value
 *  @param[out] descriptor_title_str_type - pointer to vendor defined descriptor
 *                                          title string type
 *  @param[out] descriptor_title_str - pointer to vendor defined descriptor
 *                                     title string
 *  @param[out] descriptor_data - pointer to vendor defined descriptor data
 *
 *  @return pldm_completion_codes
 */
int decode_vendor_defined_descriptor_value(
	const uint8_t *data, size_t length, uint8_t *descriptor_title_str_type,
	struct variable_field *descriptor_title_str,
	struct variable_field *descriptor_data);

/** @brief Decode individual component image information
 *
 *  @param[in] data - pointer to component image information
 *  @param[in] length - available length in the firmware update package
 *  @param[out] pldm_comp_image_info - pointer to fixed part of component image
 *                                     information
 *  @param[out] comp_version_str - pointer to component version string
 *
 *  @return pldm_completion_codes
 */
int decode_pldm_comp_image_info(
	const uint8_t *data, size_t length,
	struct pldm_component_image_information *pldm_comp_image_info,
	struct variable_field *comp_version_str);

/** @brief Create a PLDM request message for QueryDeviceIdentifiers
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] payload_length - Length of the request message payload
 *  @param[in,out] msg - Message will be written to this
 *
 *  @return pldm_completion_codes
 *
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_query_device_identifiers_req(uint8_t instance_id,
					size_t payload_length,
					struct pldm_msg *msg);

/** @brief Create a PLDM response message for QueryDeviceIdentifiers
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] descriptor_count - Number of descriptors
 *  @param[in] descriptor - Array of descriptors
 *  @param[in,out] msg - Message will be written to this
 *  @param[in,out] payload_length - Size of the response message payload, updated
 *				    with used length.
 *
 *  @return 0 on success, a negative errno value on failure.
 *
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_query_device_identifiers_resp(
	uint8_t instance_id, uint8_t descriptor_count,
	const struct pldm_descriptor *descriptors, struct pldm_msg *msg,
	size_t *payload_length);

/** @brief Decode QueryDeviceIdentifiers response message
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] completion_code - Pointer to response msg's PLDM completion code
 *  @param[out] device_identifiers_len - Pointer to device identifiers length
 *  @param[out] descriptor_count - Pointer to descriptor count
 *  @param[out] descriptor_data - Pointer to descriptor data
 *
 *  @return pldm_completion_codes
 */
int decode_query_device_identifiers_resp(const struct pldm_msg *msg,
					 size_t payload_length,
					 uint8_t *completion_code,
					 uint32_t *device_identifiers_len,
					 uint8_t *descriptor_count,
					 uint8_t **descriptor_data);

/** @brief Create a PLDM request message for GetFirmwareParameters
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] payload_length - Length of the request message payload
 *  @param[in,out] msg - Message will be written to this
 *
 *  @return pldm_completion_codes
 *
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_firmware_parameters_req(uint8_t instance_id,
				       size_t payload_length,
				       struct pldm_msg *msg);

/** @brief Decode GetFirmwareParameters response
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] resp_data - Pointer to get firmware parameters response
 *  @param[out] active_comp_image_set_ver_str - Pointer to active component
 *                                              image set version string
 *  @param[out] pending_comp_image_set_ver_str - Pointer to pending component
 *                                               image set version string
 *  @param[out] comp_parameter_table - Pointer to component parameter table
 *
 *  @return pldm_completion_codes
 */
int decode_get_firmware_parameters_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_get_firmware_parameters_resp *resp_data,
	struct variable_field *active_comp_image_set_ver_str,
	struct variable_field *pending_comp_image_set_ver_str,
	struct variable_field *comp_parameter_table);

/** @brief Decode component entries in the component parameter table which is
 *         part of the response of GetFirmwareParameters command
 *
 *  @param[in] data - Component entry
 *  @param[in] length - Length of component entry
 *  @param[out] component_data - Pointer to component parameter table
 *  @param[out] active_comp_ver_str - Pointer to active component version string
 *  @param[out] pending_comp_ver_str - Pointer to pending component version
 *                                     string
 *
 *  @return pldm_completion_codes
 */
int decode_get_firmware_parameters_resp_comp_entry(
	const uint8_t *data, size_t length,
	struct pldm_component_parameter_entry *component_data,
	struct variable_field *active_comp_ver_str,
	struct variable_field *pending_comp_ver_str);

/** @brief Encode a GetFirmwareParameters response
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] resp_data - Parameter data
 *  @param[in,out] msg - Message will be written to this
 *  @param[in,out] payload_length - Size of the response message payload, updated
 *				    with used length.
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_get_firmware_parameters_resp(
	uint8_t instance_id,
	const struct pldm_get_firmware_parameters_resp_full *resp_data,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Encode a ComponentParameterTable entry
 *
 *  @param[in] comp - Component entry
 *  @param[in,out] payload - Message will be written to this
 *  @param[in,out] payload_length - Size of payload, updated
 *				    with used length.
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_get_firmware_parameters_resp_comp_entry(
	const struct pldm_component_parameter_entry_full *comp,
	uint8_t *payload, size_t *payload_length);

/** @brief Create a PLDM request message for QueryDownstreamDevices
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[out] msg - Message will be written to this
 *
 *  @return 0 on success, otherwise -EINVAL if the input parameters' memory
 *          are not allocated, -EOVERFLOW if the payload length is not enough
 *          to encode the message, -EBADMSG if the message is not valid.
 *
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_query_downstream_devices_req(uint8_t instance_id,
					struct pldm_msg *msg);

/**
 * @brief Decodes the response message for Querying Downstream Devices.
 *
 * @param[in] msg The PLDM message to decode.
 * @param[in] payload_length The length of the message payload.
 * @param[out] resp_data Pointer to the structure to store the decoded response data.
 * @return 0 on success, otherwise -EINVAL if the input parameters' memory
 *         are not allocated, -EOVERFLOW if the payload length is not enough
 *         to decode the message, -EBADMSG if the message is not valid.
 *
 * @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int decode_query_downstream_devices_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_query_downstream_devices_resp *resp_data);

/**
 * @brief Encodes a request message for Query Downstream Identifiers.
 *
 * @param[in] instance_id The instance ID of the PLDM entity.
 * @param[in] data_transfer_handle The handle for the data transfer.
 * @param[in] transfer_operation_flag The flag indicating the transfer operation.
 * @param[out] msg Pointer to the PLDM message structure to store the encoded message.
 * @param[in] payload_length The length of the payload.
 * @return 0 on success, otherwise -EINVAL if the input parameters' memory
 *         are not allocated, -EOVERFLOW if the payload length is not enough
 *         to encode the message, -EBADMSG if the message is not valid.
 *
 * @note Caller is responsible for memory alloc and dealloc of param
 *        'msg.payload'
 */
int encode_query_downstream_identifiers_req(
	uint8_t instance_id,
	const struct pldm_query_downstream_identifiers_req *params_req,
	struct pldm_msg *msg, size_t payload_length);

/**
 * @brief Decodes the response message for Querying Downstream Identifiers.
 * @param[in] msg The PLDM message to decode.
 * @param[in] payload_length The length of the message payload.
 * @param[out] resp_data Pointer to the decoded response data.
 * @param[out] iter Pointer to the downstream device iterator structure.
 * @return 0 on success, otherwise -EINVAL if the input parameters' memory
 *         are not allocated, -EOVERFLOW if the payload length is not enough
 *         to decode the message, -EBADMSG if the message is not valid.
 *
 * @note Caller is responsible for memory alloc and dealloc of pointer params
 */
int decode_query_downstream_identifiers_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_query_downstream_identifiers_resp *resp_data,
	struct pldm_downstream_device_iter *iter);

/**
 * @brief Encodes request message for Get Downstream Firmware Parameters.
 *
 * @param[in] instance_id - The instance ID of the PLDM entity.
 * @param[in] data_transfer_handle - The handle for the data transfer.
 * @param[in] transfer_operation_flag - The flag indicating the transfer operation.
 * @param[in,out] msg - A pointer to the PLDM message structure to store the encoded message.
 * @param[in] payload_length - The length of the payload.
 *
 * @return 0 on success, otherwise -EINVAL if the input parameters' memory
 *         are not allocated, -EOVERFLOW if the payload length is not enough
 *         to encode the message, -EBADMSG if the message is not valid.
 *
 * @note Caller is responsible for memory alloc and dealloc of param
 *        'msg.payload'
 */
int encode_get_downstream_firmware_parameters_req(
	uint8_t instance_id,
	const struct pldm_get_downstream_firmware_parameters_req *params_req,
	struct pldm_msg *msg, size_t payload_length);

struct pldm_downstream_device_parameters_iter {
	struct variable_field field;
	size_t entries;
};

/**
 * @brief Decode response message for Get Downstream Firmware Parameters
 *
 * @param[in] msg - The PLDM message to decode
 * @param[in] payload_length - The length of the message payload
 * @param[out] resp_data - Pointer to the structure to store the decoded response data
 * @param[out] downstream_device_param_table - Pointer to the variable field structure
 *                                           to store the decoded downstream device
 *                                           parameter table
 * @return 0 on success, otherwise -EINVAL if the input parameters' memory
 *         are not allocated, -EOVERFLOW if the payload length is not enough
 *         to decode the message, -EBADMSG if the message is not valid.
 *
 * @note Caller is responsible for memory alloc and dealloc of param
 *        'resp_data' and 'downstream_device_param_table'
 */
int decode_get_downstream_firmware_parameters_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_get_downstream_firmware_parameters_resp *resp_data,
	struct pldm_downstream_device_parameters_iter *iter);

/**
 * @brief Decode the next downstream device parameter table entry
 *
 * @param[in,out] data - A variable field covering the table entries in the
 *                       response message data. @p data is updated to point to
 *                       the remaining entries once the current entry has been
 *                       decoded.

 * @param[out] entry - The struct object into which the current table entry will
 *                     be decoded

 * @param[out] versions - A variable field covering the active and
 *                        pending component version strings in the
 *                        response message data. The component version
 *                        strings can be decoded into @p entry using
 *                        decode_downstream_device_parameter_table_entry_versions()
 *
 * @return 0 on success, otherwise -EINVAL if the input parameters' memory
 *         are not allocated, -EOVERFLOW if the payload length is not enough
 *         to decode the entry.
 *
 * @note Caller is responsible for memory alloc and dealloc of param
 * 	  'entry', 'active_comp_ver_str' and 'pending_comp_ver_str'
 */
int decode_pldm_downstream_device_parameters_entry_from_iter(
	struct pldm_downstream_device_parameters_iter *iter,
	struct pldm_downstream_device_parameters_entry *entry);

LIBPLDM_ITERATOR
bool pldm_downstream_device_parameters_iter_end(
	const struct pldm_downstream_device_parameters_iter *iter)
{
	return !iter->entries;
}

LIBPLDM_ITERATOR
bool pldm_downstream_device_parameters_iter_next(
	struct pldm_downstream_device_parameters_iter *iter)
{
	if (!iter->entries) {
		return false;
	}

	iter->entries--;
	return true;
}

/** @brief Iterator downstream device parameter entries in Get Downstream
 *         Firmware Parameters response
 *
 * @param params The @ref "struct pldm_downstream_device_parameters_iter" lvalue
 *               used as the out-value from the corresponding call to @ref
 *               decode_get_downstream_firmware_parameters_resp
 * @param entry The @ref "struct pldm_downstream_device_parameters_entry" lvalue
 *              into which the next parameter table entry should be decoded
 * @param rc An lvalue of type int into which the return code from the decoding
 *           will be placed
 *
 * Example use of the macro is as follows:
 *
 * @code
 * struct pldm_get_downstream_firmware_parameters_resp resp;
 * struct pldm_downstream_device_parameters_iter params;
 * struct pldm_downstream_device_parameters_entry entry;
 * int rc;
 *
 * rc = decode_get_downstream_firmware_parameters_resp(..., &resp, &params);
 * if (rc) {
 *     // Handle any error from decoding the fixed-portion of response
 * }
 *
 * foreach_pldm_downstream_device_parameters_entry(params, entry, rc) {
 *     // Do something with the decoded entry
 * }
 *
 * if (rc) {
 *     // Handle any decoding error while iterating the variable-length set of
 *     //parameter entries
 * }
 * @endcode
 */
#define foreach_pldm_downstream_device_parameters_entry(params, entry, rc)       \
	for ((rc) = 0;                                                           \
	     (!pldm_downstream_device_parameters_iter_end(&(params)) &&          \
	      !((rc) = decode_pldm_downstream_device_parameters_entry_from_iter( \
			&(params), &(entry))));                                  \
	     pldm_downstream_device_parameters_iter_next(&(params)))

/** @brief Create PLDM request message for RequestUpdate
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] max_transfer_size - Maximum size of the variable payload allowed
 *                                 to be requested via RequestFirmwareData
 *                                 command
 *  @param[in] num_of_comp - Total number of components that will be passed to
 *                           the FD during the update
 *  @param[in] max_outstanding_transfer_req - Total number of outstanding
 * 					      RequestFirmwareData
 * commands that can be sent by the FD
 *  @param[in] pkg_data_len - Value of the FirmwareDevicePackageDataLength field
 *                            present in firmware package header
 *  @param[in] comp_image_set_ver_str_type - StringType of
 *                                           ComponentImageSetVersionString
 *  @param[in] comp_image_set_ver_str_len - The length of the
 *                                          ComponentImageSetVersionString
 *  @param[in] comp_img_set_ver_str - Component Image Set version information
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of request message payload
 *
 *  @return pldm_completion_codes
 *
 *  @note Caller is responsible for memory alloc and dealloc of param
 *        'msg.payload'
 */
int encode_request_update_req(uint8_t instance_id, uint32_t max_transfer_size,
			      uint16_t num_of_comp,
			      uint8_t max_outstanding_transfer_req,
			      uint16_t pkg_data_len,
			      uint8_t comp_image_set_ver_str_type,
			      uint8_t comp_image_set_ver_str_len,
			      const struct variable_field *comp_img_set_ver_str,
			      struct pldm_msg *msg, size_t payload_length);

/** @brief Decode PLDM request message for RequestUpdate
 *
 *  @param[in] msg - Message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] req - RequestUpdate request parameters
 *
 *  @return 0 on success, a negative errno value on failure.
 *
 *  @note Caller is responsible for memory alloc and dealloc of param
 *        'msg.payload'
 */
int decode_request_update_req(const struct pldm_msg *msg, size_t payload_length,
			      struct pldm_request_update_req_full *req);

/** @brief Decode a RequestUpdate response message
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] completion_code - Pointer to hold the completion code
 *  @param[out] fd_meta_data_len - Pointer to hold the length of FD metadata
 *  @param[out] fd_will_send_pkg_data - Pointer to hold information whether FD
 *                                      will send GetPackageData command
 *  @return pldm_completion_codes
 */
int decode_request_update_resp(const struct pldm_msg *msg,
			       size_t payload_length, uint8_t *completion_code,
			       uint16_t *fd_meta_data_len,
			       uint8_t *fd_will_send_pkg_data);

/** @brief Create PLDM response message for RequestUpdate
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] resp_data - Response data
 *  @param[out] msg - Message will be written to this
 *  @param[inout] payload_length - Length of response message payload
 *
 *  @return 0 on success, a negative errno value on failure.
 *
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *		   'msg.payload'
 */
int encode_request_update_resp(uint8_t instance_id,
			       const struct pldm_request_update_resp *resp_data,
			       struct pldm_msg *msg, size_t *payload_length);

/** @brief Create PLDM request message for RequestDownstreamDeviceUpdate
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] req_data - Request data.
 *  @param[in,out] msg - Message will be written to this
 *  @param[in,out] payload_length - Length of response message payload
 *
 *  @return 0 on success,
 *		-EINVAL if any argument is invalid,
 *		-ENOMSG if the message type is incorrect,
 *		-EOVERFLOW if the payload length is invalid
 *
 *  @note Caller is responsible for memory alloc and dealloc of param
 *        'msg.payload'
 */
int encode_request_downstream_device_update_req(
	uint8_t instance_id,
	const struct pldm_request_downstream_device_update_req *req_data,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode PLDM request message for RequestDownstreamDeviceUpdate
 *
 *  @param[in] msg - Message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] req_data - RequestDownstreamDeviceUpdate request parameters
 *
 *  @return 0 on success,
 *		-EINVAL if any argument is invalid,
 *		-ENOMSG if the message type is incorrect,
 *		-EOVERFLOW if the payload length is invalid,
 *		-EBADMSG if the message buffer was not fully consumed
 *
 *  @note Caller is responsible for memory alloc and dealloc of param
 *        'msg.payload'
 */
int decode_request_downstream_device_update_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_request_downstream_device_update_req *req_data);

/** @brief Create PLDM response message for RequestDownstreamDeviceUpdate
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] resp_data - Response data
 *  @param[out] msg - Message will be written to this
 *  @param[inout] payload_length - Length of response message payload
 *
 *  @return 0 on success,
 *		-EINVAL if any argument is invalid,
 *		-ENOMSG if the message type is incorrect,
 *		-EOVERFLOW if the payload length is invalid
 *
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *		   'msg.payload'
 */
int encode_request_downstream_device_update_resp(
	uint8_t instance_id,
	const struct pldm_request_downstream_device_update_resp *resp_data,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode a RequestDownstreamDeviceUpdate response message
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] resp_data - RequestDownstreamDeviceUpdate respond parameters
 *
 *  @return 0 on success,
 *		-EINVAL if any argument is invalid,
 *		-ENOMSG if the message type is incorrect,
 *		-EOVERFLOW if the payload length is invalid,
 *		-EBADMSG if the message buffer was not fully consumed
 */
int decode_request_downstream_device_update_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_request_downstream_device_update_resp *resp_data);

/** @brief Create PLDM request message for PassComponentTable
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] transfer_flag - TransferFlag
 *  @param[in] comp_classification - ComponentClassification
 *  @param[in] comp_identifier - ComponentIdentifier
 *  @param[in] comp_classification_index - ComponentClassificationIndex
 *  @param[in] comp_comparison_stamp - ComponentComparisonStamp
 *  @param[in] comp_ver_str_type - ComponentVersionStringType
 *  @param[in] comp_ver_str_len - ComponentVersionStringLength
 *  @param[in] comp_ver_str - ComponentVersionString
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of request message payload
 *                              information
 *
 *  @return pldm_completion_codes
 *
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_pass_component_table_req(
	uint8_t instance_id, uint8_t transfer_flag,
	uint16_t comp_classification, uint16_t comp_identifier,
	uint8_t comp_classification_index, uint32_t comp_comparison_stamp,
	uint8_t comp_ver_str_type, uint8_t comp_ver_str_len,
	const struct variable_field *comp_ver_str, struct pldm_msg *msg,
	size_t payload_length);

/** @brief Decode a PassComponentTable request
 *
 *  @param[in] msg - PLDM Message
 *  @param[in] payload_length
 *  @param[out] pcomp - Pass Component Table Request
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pass_component_table_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_pass_component_table_req_full *pcomp);

/** @brief Decode PassComponentTable response message
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] completion_code - Pointer to hold completion code
 *  @param[out] comp_resp - Pointer to hold component response
 *  @param[out] comp_resp_code - Pointer to hold component response code
 *
 *  @return pldm_completion_codes
 */
int decode_pass_component_table_resp(const struct pldm_msg *msg,
				     size_t payload_length,
				     uint8_t *completion_code,
				     uint8_t *comp_resp,
				     uint8_t *comp_resp_code);

/** @brief Encode PassComponentTable response
 *
 *  @param[in] instance_id - PLDM Instance ID matching the request
 *  @param[in] resp_data - response data
 *  @param[out] msg - Response message
 *  @param[inout] payload_length - Length of msg payload buffer,
 *				   will be updated with the written
 * 				   length on success.
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pass_component_table_resp(
	uint8_t instance_id,
	const struct pldm_pass_component_table_resp *resp_data,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Create PLDM request message for UpdateComponent
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] comp_classification - ComponentClassification
 *  @param[in] comp_identifier - ComponentIdentifier
 *  @param[in] comp_classification_index - ComponentClassificationIndex
 *  @param[in] comp_comparison_stamp - ComponentComparisonStamp
 *  @param[in] comp_image_size - ComponentImageSize
 *  @param[in] update_option_flags - UpdateOptionFlags
 *  @param[in] comp_ver_str_type - ComponentVersionStringType
 *  @param[in] comp_ver_str_len - ComponentVersionStringLength
 *  @param[in] comp_ver_str - ComponentVersionString
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of request message payload
 *                              information
 *
 *  @return pldm_completion_codes
 *
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_update_component_req(
	uint8_t instance_id, uint16_t comp_classification,
	uint16_t comp_identifier, uint8_t comp_classification_index,
	uint32_t comp_comparison_stamp, uint32_t comp_image_size,
	bitfield32_t update_option_flags, uint8_t comp_ver_str_type,
	uint8_t comp_ver_str_len, const struct variable_field *comp_ver_str,
	struct pldm_msg *msg, size_t payload_length);

/** @brief Decode UpdateComponent request message
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] up - UpdateComponent request parameters
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_update_component_req(const struct pldm_msg *msg,
				size_t payload_length,
				struct pldm_update_component_req_full *up);

/** @brief Decode UpdateComponent response message
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] completion_code - Pointer to hold completion code
 *  @param[out] comp_compatibility_resp - Pointer to hold component
 *                                        compatibility response
 *  @param[out] comp_compatibility_resp_code - Pointer to hold component
 *                                             compatibility response code
 *  @param[out] update_option_flags_enabled - Pointer to hold
 *                                            UpdateOptionsFlagEnabled
 *  @param[out] time_before_req_fw_data - Pointer to hold the estimated time
 *                                        before sending RequestFirmwareData
 *
 *  @return pldm_completion_codes
 */
int decode_update_component_resp(const struct pldm_msg *msg,
				 size_t payload_length,
				 uint8_t *completion_code,
				 uint8_t *comp_compatibility_resp,
				 uint8_t *comp_compatibility_resp_code,
				 bitfield32_t *update_option_flags_enabled,
				 uint16_t *time_before_req_fw_data);

/** @brief Encode UpdateComponent response
 *
 *  @param[in] instance_id - PLDM Instance ID matching the request
 *  @param[in] resp_data - Response data
 *  @param[out] msg - Response message
 *  @param[inout] payload_length - Length of msg payload buffer,
 *				   will be updated with the written
 * 				   length on success.
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_update_component_resp(
	uint8_t instance_id, const struct pldm_update_component_resp *resp_data,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode RequestFirmwareData request message
 *
 *	@param[in] msg - Request message
 *	@param[in] payload_length - Length of request message payload
 *	@param[out] offset - Pointer to hold offset
 *	@param[out] length - Pointer to hold the size of the component image
 *                       segment requested by the FD/FDP
 *
 *	@return pldm_completion_codes
 */
int decode_request_firmware_data_req(const struct pldm_msg *msg,
				     size_t payload_length, uint32_t *offset,
				     uint32_t *length);

/** @brief Encode RequestFirmwareData request
 *
 *  @param[in] instance_id - PLDM Instance ID
 *  @param[in] req_params - Request parameters
 *  @param[in] length - firmware data length to request
 *  @param[out] msg - Response message
 *  @param[inout] payload_length - Length of msg payload buffer,
 *				   will be updated with the written
 * 				   length on success.
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_request_firmware_data_req(
	uint8_t instance_id,
	const struct pldm_request_firmware_data_req *req_params,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Create PLDM response message for RequestFirmwareData
 *
 *  The ComponentImagePortion is not encoded in the PLDM response message
 *  by encode_request_firmware_data_resp to avoid an additional copy. Populating
 *  ComponentImagePortion in the PLDM response message is handled by the user
 *  of this API. The payload_length validation considers only the
 *  CompletionCode.
 *
 *	@param[in] instance_id - Message's instance id
 *	@param[in] completion_code - CompletionCode
 *	@param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of response message payload
 *
 *	@return pldm_completion_codes
 *
 *	@note  Caller is responsible for memory alloc and dealloc of param
 *		   'msg.payload'
 */
int encode_request_firmware_data_resp(uint8_t instance_id,
				      uint8_t completion_code,
				      struct pldm_msg *msg,
				      size_t payload_length);

/** @brief Decode TransferComplete request message
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] transfer_result - Pointer to hold TransferResult
 *
 *  @return pldm_completion_codes
 */
int decode_transfer_complete_req(const struct pldm_msg *msg,
				 size_t payload_length,
				 uint8_t *transfer_result);

/** @brief Encode TransferComplete request
 *
 *  @param[in] instance_id - PLDM Instance ID
 *  @param[in] transfer_result
 *  @param[out] msg - Response message
 *  @param[inout] payload_length - Length of msg payload buffer,
 *				   will be updated with the written
 * 				   length on success.
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_transfer_complete_req(uint8_t instance_id, uint8_t transfer_result,
				 struct pldm_msg *msg, size_t *payload_length);

/** @brief Create PLDM response message for TransferComplete
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - CompletionCode
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of response message payload
 *
 *  @return pldm_completion_codes
 *
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_transfer_complete_resp(uint8_t instance_id, uint8_t completion_code,
				  struct pldm_msg *msg, size_t payload_length);

/** @brief Decode VerifyComplete request message
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[in] verify_result - Pointer to hold VerifyResult
 *
 *  @return pldm_completion_codes
 */
int decode_verify_complete_req(const struct pldm_msg *msg,
			       size_t payload_length, uint8_t *verify_result);

/** @brief Encode VerifyComplete request
 *
 *  @param[in] instance_id - PLDM Instance ID
 *  @param[in] verify_result
 *  @param[out] msg - Response message
 *  @param[inout] payload_length - Length of msg payload buffer,
 *				   will be updated with the written
 * 				   length on success.
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_verify_complete_req(uint8_t instance_id, uint8_t verify_result,
			       struct pldm_msg *msg, size_t *payload_length);

/** @brief Create PLDM response message for VerifyComplete
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - CompletionCode
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of response message payload
 *
 *  @return pldm_completion_codes
 *
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_verify_complete_resp(uint8_t instance_id, uint8_t completion_code,
				struct pldm_msg *msg, size_t payload_length);

/** @brief Decode ApplyComplete request message
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[in] apply_result - Pointer to hold ApplyResult
 *  @param[in] comp_activation_methods_modification - Pointer to hold the
 *                                        ComponentActivationMethodsModification
 *
 *  @return pldm_completion_codes
 */
int decode_apply_complete_req(
	const struct pldm_msg *msg, size_t payload_length,
	uint8_t *apply_result,
	bitfield16_t *comp_activation_methods_modification);

/** @brief Encode ApplyComplete request
 *
 *  @param[in] instance_id - PLDM Instance ID
 *  @param[in] req_data - Request data
 *  @param[out] msg - Request message
 *  @param[inout] payload_length - Length of msg payload buffer,
 *				   will be updated with the written
 * 				   length on success.
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_apply_complete_req(uint8_t instance_id,
			      const struct pldm_apply_complete_req *req_data,
			      struct pldm_msg *msg, size_t *payload_length);

/** @brief Create PLDM response message for ApplyComplete
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - CompletionCode
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of response message payload
 *
 *  @return pldm_completion_codes
 *
 *  @note Caller is responsible for memory alloc and dealloc of param
 *        'msg.payload'
 */
int encode_apply_complete_resp(uint8_t instance_id, uint8_t completion_code,
			       struct pldm_msg *msg, size_t payload_length);

/** @brief Create PLDM request message for ActivateFirmware
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] self_contained_activation_req SelfContainedActivationRequest
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of request message payload
 *
 *  @return pldm_completion_codes
 *
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_activate_firmware_req(uint8_t instance_id,
				 bool8_t self_contained_activation_req,
				 struct pldm_msg *msg, size_t payload_length);

/** @brief Decode ActivateFirmware request
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] self_contained
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_activate_firmware_req(const struct pldm_msg *msg,
				 size_t payload_length, bool *self_contained);

/** @brief Decode ActivateFirmware response message
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] completion_code - Pointer to hold CompletionCode
 *  @param[out] estimated_time_activation - Pointer to hold
 *                                       EstimatedTimeForSelfContainedActivation
 *
 *  @return pldm_completion_codes
 */
int decode_activate_firmware_resp(const struct pldm_msg *msg,
				  size_t payload_length,
				  uint8_t *completion_code,
				  uint16_t *estimated_time_activation);

/** @brief Encode ActivateFirmware response
 *
 *  @param[in] instance_id - PLDM Instance ID matching the request
 *  @param[in] resp_data - Response data
 *  @param[out] msg - Response message
 *  @param[inout] payload_length - Length of msg payload buffer,
 *				   will be updated with the written
 * 				   length on success.
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_activate_firmware_resp(
	uint8_t instance_id,
	const struct pldm_activate_firmware_resp *resp_data,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Create PLDM request message for GetStatus
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of request message payload
 *
 *  @return pldm_completion_codes
 *
 *  @note Caller is responsible for memory alloc and dealloc of param
 *        'msg.payload'
 */
int encode_get_status_req(uint8_t instance_id, struct pldm_msg *msg,
			  size_t payload_length);

/** @brief Decode GetStatus response message
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] completion_code - Pointer to completion code
 *  @param[out] current_state - Pointer to current state machine state
 *  @param[out] previous_state - Pointer to previous different state machine
 *                               state
 *  @param[out] aux_state - Pointer to current operation state of FD/FDP
 *  @param[out] aux_state_status - Pointer to aux state status
 *  @param[out] progress_percent - Pointer to progress percentage
 *  @param[out] reason_code - Pointer to reason for entering current state
 *  @param[out] update_option_flags_enabled - Pointer to update option flags
 *                                            enabled
 *
 *  @return pldm_completion_codes
 */
int decode_get_status_resp(const struct pldm_msg *msg, size_t payload_length,
			   uint8_t *completion_code, uint8_t *current_state,
			   uint8_t *previous_state, uint8_t *aux_state,
			   uint8_t *aux_state_status, uint8_t *progress_percent,
			   uint8_t *reason_code,
			   bitfield32_t *update_option_flags_enabled);

/** @brief Encode GetStatus response
 *
 *  @param[in] instance_id - PLDM Instance ID matching the request
 *  @param[in] status - GetStatus response. completion_code must be PLDM_SUCCESS.
 *  @param[out] msg - Response message
 *  @param[inout] payload_length - Length of msg payload buffer,
 *				   will be updated with the written
 * 				   length on success.
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_get_status_resp(uint8_t instance_id,
			   const struct pldm_get_status_resp *status,
			   struct pldm_msg *msg, size_t *payload_length);

/** @brief Create PLDM request message for CancelUpdateComponent
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of request message payload
 *
 *  @return pldm_completion_codes
 *
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_cancel_update_component_req(uint8_t instance_id,
				       struct pldm_msg *msg,
				       size_t payload_length);

/** @brief Decode CancelUpdateComponent response message
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] completion_code - Pointer to the completion code
 *
 *  @return pldm_completion_codes
 */
int decode_cancel_update_component_resp(const struct pldm_msg *msg,
					size_t payload_length,
					uint8_t *completion_code);

/** @brief Create PLDM request message for CancelUpdate
 *
 *	@param[in] instance_id - Message's instance id
 *	@param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of request message payload
 *
 *	@return pldm_completion_codes
 *
 *	@note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_cancel_update_req(uint8_t instance_id, struct pldm_msg *msg,
			     size_t payload_length);

/** @brief Decode CancelUpdate response message
 *
 *	@param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *	@param[out] completion_code - Pointer to completion code
 *	@param[out] non_functioning_component_indication - Pointer to non
						       functioning
 *                                                     component indication
 *	@param[out] non_functioning_component_bitmap - Pointer to non
 functioning
 *                                                 component bitmap
 *
 *	@return pldm_completion_codes
 */
int decode_cancel_update_resp(const struct pldm_msg *msg, size_t payload_length,
			      uint8_t *completion_code,
			      bool8_t *non_functioning_component_indication,
			      bitfield64_t *non_functioning_component_bitmap);

/** @brief Encode CancelUpdate response
 *
 *  @param[in] instance_id - PLDM Instance ID matching the request
 *  @param[in] resp_data - Response data,
 *  @param[out] msg - Response message
 *  @param[inout] payload_length - Length of msg payload buffer,
 *				   will be updated with the written
 * 				   length on success.
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_cancel_update_resp(uint8_t instance_id,
			      const struct pldm_cancel_update_resp *resp_data,
			      struct pldm_msg *msg, size_t *payload_length);

/** @brief Firmware update v1.0 package header identifier */
#define PLDM_PACKAGE_HEADER_IDENTIFIER_V1_0                                    \
	{ 0xF0, 0x18, 0x87, 0x8C, 0xCB, 0x7D, 0x49, 0x43,                      \
	  0x98, 0x00, 0xA0, 0x2F, 0x05, 0x9A, 0xCA, 0x02 }

/** @brief Firmware update v1.0 package header format revision */
#define PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR01H 0x01

/** @brief Firmware update v1.1 package header identifier */
#define PLDM_PACKAGE_HEADER_IDENTIFIER_V1_1                                    \
	{                                                                      \
		0x12, 0x44, 0xd2, 0x64, 0x8d, 0x7d, 0x47, 0x18,                \
		0xa0, 0x30, 0xfc, 0x8a, 0x56, 0x58, 0x7d, 0x5a,                \
	}

/** @brief Firmware update v1.1 package header format revision */
#define PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR02H 0x02

/** @brief Firmware update v1.2 package header identifier */
#define PLDM_PACKAGE_HEADER_IDENTIFIER_V1_2                                    \
	{                                                                      \
		0x31, 0x19, 0xce, 0x2f, 0xe8, 0x0a, 0x4a, 0x99,                \
		0xaf, 0x6d, 0x46, 0xf8, 0xb1, 0x21, 0xf6, 0xbf,                \
	}

/** @brief Firmware update v1.2 package header format revision */
#define PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR03H 0x03

/** @brief Firmware update v1.3 package header identifier */
#define PLDM_PACKAGE_HEADER_IDENTIFIER_V1_3                                    \
	{                                                                      \
		0x7b, 0x29, 0x1c, 0x99, 0x6d, 0xb6, 0x42, 0x08,                \
		0x80, 0x1b, 0x02, 0x02, 0x6e, 0x46, 0x3c, 0x78,                \
	}

/** @brief Firmware update v1.3 package header format revision */
#define PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR04H 0x04

/** @brief Consumer-side version pinning for package format parsing
 *
 * Parsing a firmware update package requires the package to be of a revision
 * defined in the specification, for libpldm to support parsing a package
 * formatted at the specified revision, and for the consumer to support calling
 * libpldm's package-parsing APIs in the manner required for the package format.
 *
 * pldm_package_format_pin communicates to libpldm the maximum package format
 * revision supported by the consumer.
 *
 * The definition of the pldm_package_format_pin object in the consumer
 * application should not be open-coded. Instead, users should call on of the
 * following macros:
 *
 * - @ref DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR01H
 * - @ref DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR02H
 * - @ref DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR03H
 * - @ref DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR04H
 *
 * The package pinning operates by providing versioning over multiple structs
 * required to perform the package parsing. See [Conventions for extensible
 * system calls][lwn-extensible-syscalls] for discussion of related concepts.
 * Like the syscall structs described there, the structs captured here by the
 * pinning concept must only ever be modified by addition of new members, never
 * alteration of existing members.
 *
 * [lwn-extensible-syscalls]: https://lwn.net/Articles/830666/
 */
struct pldm_package_format_pin {
	struct {
		/**
		 * A value that communicates information about object sizes to the implementation.
		 *
		 * For magic version 0, the sum must be calculated using @ref LIBPLDM_SIZEAT for
		 * the final relevant member of each relevant struct for the format revision
		 * represented by the pin.
		 */
		const uint32_t magic;

		/**
		 * Versioning for the derivation of the magic value
		 *
		 * A version value of 0 defines the magic number to be the sum of the relevant
		 * struct sizes for the members required at the format revision specified by
		 * the pin.
		 */
		const uint8_t version;
	} meta;
	struct {
		/** The maximum supported package format UUID */
		const pldm_uuid identifier;

		/** The maximum supported header format revision */
		const uint8_t revision;
	} format;
};

/**
 * @brief Header information as parsed from the provided package
 *
 * See Table 3, DSP0267 v1.3.0.
 *
 * The provided package data must out-live the header struct.
 */
struct pldm__package_header_information {
	pldm_uuid package_header_identifier;
	uint8_t package_header_format_revision;
	uint8_t package_release_date_time[PLDM_TIMESTAMP104_SIZE];
	uint16_t component_bitmap_bit_length;
	uint8_t package_version_string_type;

	/** A field pointing to the package version string in the provided package data */
	struct variable_field package_version_string;

	/* TODO: some metadata for the parsing process is stored here, reconsider */
	struct variable_field areas;
	struct variable_field package;
};
/* TODO: Deprecate the other struct pldm_package_header_information, remove, drop typedef */
typedef struct pldm__package_header_information
	pldm_package_header_information_pad;

/* TODO: Consider providing an API to access valid bits */
struct pldm_package_component_bitmap {
	struct variable_field bitmap;
};

/**
 * @brief A firmware device ID record from the firmware update package
 *
 * See Table 4, DSP0267 v1.3.0.
 *
 * The provided package data must out-live the @ref "struct
 * pldm_package_firmware_device_id_record" instance.
 */
struct pldm_package_firmware_device_id_record {
	uint8_t descriptor_count;
	bitfield32_t device_update_option_flags;
	uint8_t component_image_set_version_string_type;

	/**
	 * A field pointing to the component image set version string in the provided
	 * package data.
	 */
	struct variable_field component_image_set_version_string;

	/**
	 * A field pointing to the to a bitmap of length @ref
	 * component_bitmap_bit_length in the provided package data..
	 */
	struct pldm_package_component_bitmap applicable_components;

	/**
	 * A field pointing to record descriptors for the
	 * firmware device. Iterate over the entries using @ref
	 * foreach_pldm_package_firmware_device_id_record_descriptor
	 *
	 * See Table 7, DSP0267 v1.3.0
	 */
	struct variable_field record_descriptors;
	struct variable_field firmware_device_package_data;

	/**
	 * An optional field that can contain a Reference Manifest for the firmware
	 * update package. If present, this field points to the Reference Manifest
	 * data, which describes the firmware update provided by this package. The
	 * UA (Update Agent) may use this data as a reference for the firmware
	 * version.
	 *
	 * Note that this data shall not be transferred to the firmware device (FD).
	 * The format of the data is either a Standard Body or Vendor-Defined Header,
	 * followed by the Reference Manifest data.
	 *
	 * See Table 7, DSP0267 v1.3.0
	 */
	struct variable_field reference_manifest_data;
};

/**
 * @brief A downstream device ID record from the firmware update package
 *
 * See Table 5, DSP0267 v1.3.0.
 *
 * The provided package data must out-live the @ref "struct
 * pldm_package_downstream_device_id_record" instance.
 */
struct pldm_package_downstream_device_id_record {
	uint8_t descriptor_count;
	bitfield32_t update_option_flags;
	uint8_t self_contained_activation_min_version_string_type;

	/**
	 * A field pointing to the self-contained activation minimum version string in
	 * the provided package data.
	 */
	struct variable_field self_contained_activation_min_version_string;
	uint32_t self_contained_activation_min_version_comparison_stamp;

	/**
	 * A field pointing to a bitmap of length @ref component_bitmap_bit_length in
	 * the provided package data.
	 */
	struct pldm_package_component_bitmap applicable_components;

	/**
	 * A field pointing to record descriptors for the
	 * downstream device. Iterate over the entries using @ref
	 * foreach_pldm_package_downstream_device_id_record_descriptor
	 *
	 * See Table 7, DSP0267 v1.3.0
	 */
	struct variable_field record_descriptors;

	/**
	 * A field that may point to package data to be proxied by the firmware device.
	 * If present, points into the provided package data.
	 */
	struct variable_field package_data;

	/**
	* A field pointing to a Reference Manifest for the downstream device.
	* If present, this field points to the Reference Manifest data, which describes
	*
	* See Table 7, DSP0267 v1.3.0
	*/
	struct variable_field reference_manifest_data;
};

/**
 * @brief Component image information from the firmware update package.
 *
 * See Table 6, DSP0267 v1.3.0
 *
 * The provided package data must out-live the @ref "struct
 * pldm_package_component_image_information" instance.
 */
struct pldm_package_component_image_information {
	uint16_t component_classification;
	uint16_t component_identifier;
	uint32_t component_comparison_stamp;
	bitfield16_t component_options;
	bitfield16_t requested_component_activation_method;

	/**
	 * A field that points to the component image for a device in the provided
	 * package data.
	 */
	struct variable_field component_image;
	uint8_t component_version_string_type;

	/**
	 * A field that points to the component version string for the image in the
	 * provided package data.
	 */
	struct variable_field component_version_string;

	/**
	 * A field that points to the component opaque data in the
	 * provided package data.
	 */
	struct variable_field component_opaque_data;
};

struct pldm_package_firmware_device_id_record_iter {
	struct variable_field field;
	size_t entries;
};

struct pldm_package_downstream_device_id_record_iter {
	struct variable_field field;
	size_t entries;
};

struct pldm_package_component_image_information_iter {
	struct variable_field field;
	size_t entries;
};

/**
 * @brief State tracking for firmware update package iteration
 *
 * Declare an instance on the stack to be initialised by @ref
 * decode_pldm_firmware_update_package
 *
 * The state is consumed by the following macros:
 *
 * - @ref foreach_pldm_package_firmware_device_id_record
 * - @ref foreach_pldm_package_firmware_device_id_record_descriptor
 * - @ref foreach_pldm_package_downstream_device_id_record
 * - @ref foreach_pldm_package_downstream_device_id_record_descriptor
 * - @ref foreach_pldm_package_component_image_information
 */
struct pldm_package_iter {
	const pldm_package_header_information_pad *hdr;

	/* Modified in the course of iteration */
	struct pldm_package_firmware_device_id_record_iter fds;
	struct pldm_package_downstream_device_id_record_iter dds;
	struct pldm_package_component_image_information_iter infos;
};

/**
 * @brief Initialize the firmware update package iterator.
 *
 * @param[in] data The buffer containing the complete firmware update package
 * @param[in] length The length of the buffer pointed at by @p data
 * @param[in] pin The maximum supported package format revision of the caller
 * @param[out] hdr The parsed package header structure
 * @param[out] iter State-tracking for parsing subsequent package records and components
 *
 * Must be called to ensure version requirements for parsing are met by all
 * components, and to initialise @p iter prior to any subsequent extraction of
 * package records and components.
 *
 * @note @p data is stored in @iter for later reference, and therefore must
 *       out-live @p iter
 * @note @p hdr is stored in @iter for later reference, and therefore must
 *       out-live @p iter
 *
 * @return 0 on success. Otherwise, a negative errno value:
 * - -EBADMSG if the package fails to meet minimum required length for a valid
 *   package
 * - -EINVAL if provided parameter values are invalid
 * - -ENOTSUP on unrecognised or unsupported versions for the format pin or
 *   provided package
 * - -EOVERFLOW if the variable length structures extend beyond the package
 *   data buffer
 * - -EPROTO if parsed values violate the package format specification
 * - -EUCLEAN if the package fails embedded integrity checks
 */
int decode_pldm_firmware_update_package(
	const void *data, size_t length,
	const struct pldm_package_format_pin *pin,
	pldm_package_header_information_pad *hdr,
	struct pldm_package_iter *iter);

LIBPLDM_ITERATOR
bool pldm_package_firmware_device_id_record_iter_end(
	const struct pldm_package_firmware_device_id_record_iter *iter)
{
	return iter->entries == 0;
}

LIBPLDM_ITERATOR
bool pldm_package_firmware_device_id_record_iter_next(
	struct pldm_package_firmware_device_id_record_iter *iter)
{
	if (!iter->entries) {
		return false;
	}
	iter->entries--;
	return true;
}

int pldm_package_firmware_device_id_record_iter_init(
	const pldm_package_header_information_pad *hdr,
	struct pldm_package_firmware_device_id_record_iter *iter);

int decode_pldm_package_firmware_device_id_record_from_iter(
	const pldm_package_header_information_pad *hdr,
	struct pldm_package_firmware_device_id_record_iter *iter,
	struct pldm_package_firmware_device_id_record *rec);

/**
 * @brief Iterate over a package's firmware device ID records
 *
 * @param iter[in,out] The lvalue for the instance of @ref "struct pldm_package_iter"
 *             initialised by @ref decode_pldm_firmware_update_package
 * @param rec[out] An lvalue of type @ref "struct pldm_package_firmware_device_id_record"
 * @param rc[out] An lvalue of type int that holds the status result of parsing the
 *                firmware device ID record
 *
 * @p rc is set to 0 on successful decode. Otherwise, on error, @p rc is set to:
 * - -EINVAL if parameters values are invalid
 * - -EOVERFLOW if the package layout exceeds the bounds of the package buffer
 * - -EPROTO if package metadata doesn't conform to specification constraints
 *
 * Example use of the macro is as follows:
 *
 * @code
 * DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR02H(pin);
 *
 * struct pldm_package_firmware_device_id_record fdrec;
 * pldm_package_header_information_pad hdr;
 * struct pldm_package_iter iter;
 * int rc;
 *
 * rc = decode_pldm_firmware_update_package(package, in, &pin, &hdr,
 * 					 &iter);
 * if (rc < 0) {
 * 	   // Handle header parsing failure
 * }
 * foreach_pldm_package_firmware_device_id_record(iter, fdrec, rc) {
 * 	   // Do something with fdrec
 * }
 * if (rc) {
 * 	   // Handle parsing failure for fdrec
 * }
 * @endcode
 */
#define foreach_pldm_package_firmware_device_id_record(iter, rec, rc)          \
	for ((rc) = pldm_package_firmware_device_id_record_iter_init(          \
		     (iter).hdr, &(iter).fds);                                 \
	     !(rc) &&                                                          \
	     !pldm_package_firmware_device_id_record_iter_end(&(iter).fds) &&  \
	     !((rc) = decode_pldm_package_firmware_device_id_record_from_iter( \
		       (iter).hdr, &(iter).fds, &(rec)));                      \
	     pldm_package_firmware_device_id_record_iter_next(&(iter).fds))

LIBPLDM_ITERATOR
struct pldm_descriptor_iter
pldm_package_firmware_device_id_record_descriptor_iter_init(
	struct pldm_package_firmware_device_id_record_iter *iter,
	struct pldm_package_firmware_device_id_record *rec)
{
	(void)iter;
	return (struct pldm_descriptor_iter){ &rec->record_descriptors,
					      rec->descriptor_count };
}

/**
 * @brief Iterate over the descriptors in a package's firmware device ID record
 *
 * @param iter[in,out] The lvalue for the instance of @ref "struct pldm_package_iter"
 *             initialised by @ref decode_pldm_firmware_update_package
 * @param rec[in] An lvalue of type @ref "struct pldm_package_firmware_device_id_record"
 * @param desc[out] An lvalue of type @ref "struct pldm_descriptor" that holds
 *                  the parsed descriptor
 * @param rc[out] An lvalue of type int that holds the status result of parsing the
 *                firmware device ID record
 *
 * @p rc is set to 0 on successful decode. Otherwise, on error, @p rc is set to:
 * - -EINVAL if parameters values are invalid
 * - -EOVERFLOW if the package layout exceeds the bounds of the package buffer
 * - -EPROTO if package metadata doesn't conform to specification constraints
 *
 * Example use of the macro is as follows:
 *
 * @code
 * DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR02H(pin);
 *
 * struct pldm_package_firmware_device_id_record fdrec;
 * pldm_package_header_information_pad hdr;
 * struct pldm_package_iter iter;
 * int rc;
 *
 * rc = decode_pldm_firmware_update_package(package, in, &pin, &hdr,
 * 					 &iter);
 * if (rc < 0) { ... }
 *
 * foreach_pldm_package_firmware_device_id_record(iter, fdrec, rc) {
 *     struct pldm_descriptor desc;
 *
 * 	   ...
 *
 *     foreach_pldm_package_firmware_device_id_record_descriptor(
 *             iter, fdrec, desc, rc) {
 *         // Do something with desc
 *     }
 *     if (rc) {
 *         // Handle failure to parse desc
 *     }
 * }
 * if (rc) { ... }
 * @endcode
 */
#define foreach_pldm_package_firmware_device_id_record_descriptor(iter, rec,      \
								  desc, rc)       \
	for (struct pldm_descriptor_iter desc##_iter =                            \
		     ((rc) = 0,                                                   \
		     pldm_package_firmware_device_id_record_descriptor_iter_init( \
			      &(iter).fds, &(rec)));                              \
	     (!pldm_descriptor_iter_end(&(desc##_iter))) &&                       \
	     !((rc) = decode_pldm_descriptor_from_iter(&(desc##_iter),            \
						       &(desc)));                 \
	     pldm_descriptor_iter_next(&(desc##_iter)))

LIBPLDM_ITERATOR
bool pldm_package_downstream_device_id_record_iter_end(
	const struct pldm_package_downstream_device_id_record_iter *iter)
{
	return iter->entries == 0;
}

LIBPLDM_ITERATOR
bool pldm_package_downstream_device_id_record_iter_next(
	struct pldm_package_downstream_device_id_record_iter *iter)
{
	if (!iter->entries) {
		return false;
	}
	iter->entries--;
	return true;
}

int pldm_package_downstream_device_id_record_iter_init(
	const pldm_package_header_information_pad *hdr,
	struct pldm_package_firmware_device_id_record_iter *fds,
	struct pldm_package_downstream_device_id_record_iter *dds);

int decode_pldm_package_downstream_device_id_record_from_iter(
	const pldm_package_header_information_pad *hdr,
	struct pldm_package_downstream_device_id_record_iter *iter,
	struct pldm_package_downstream_device_id_record *rec);

/**
 * @brief Iterate over a package's downstream device ID records
 *
 * @param iter[in,out] The lvalue for the instance of @ref "struct pldm_package_iter"
 *             initialised by @ref decode_pldm_firmware_update_package
 * @param rec[out] An lvalue of type @ref "struct pldm_package_downstream_device_id_record"
 * @param rc[out] An lvalue of type int that holds the status result of parsing the
 *                firmware device ID record
 *
 * @p rc is set to 0 on successful decode. Otherwise, on error, @p rc is set to:
 * - -EINVAL if parameters values are invalid
 * - -EOVERFLOW if the package layout exceeds the bounds of the package buffer
 * - -EPROTO if package metadata doesn't conform to specification constraints
 *
 * Example use of the macro is as follows:
 *
 * @code
 * DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR02H(pin);
 *
 * struct pldm_package_downstream_device_id_record ddrec;
 * struct pldm_package_firmware_device_id_record fdrec;
 * pldm_package_header_information_pad hdr;
 * struct pldm_package_iter iter;
 * int rc;
 *
 * rc = decode_pldm_firmware_update_package(package, in, &pin, &hdr,
 * 					 &iter);
 * if (rc < 0) { ... }
 *
 * foreach_pldm_package_firmware_device_id_record(iter, fdrec, rc) {
 *     struct pldm_descriptor desc;
 * 	   ...
 *     foreach_pldm_package_firmware_device_id_record_descriptor(
 *             iter, fdrec, desc, rc) {
 *         ...
 *     }
 *     if (rc) { ... }
 * }
 * if (rc) { ... }
 *
 * foreach_pldm_package_downstream_device_id_record(iter, ddrec, rc) {
 * 	   // Do something with ddrec
 * }
 * if (rc) {
 * 	   // Handle parsing failure for ddrec
 * }
 * @endcode
 */
#define foreach_pldm_package_downstream_device_id_record(iter, rec, rc)          \
	for ((rc) = pldm_package_downstream_device_id_record_iter_init(          \
		     (iter).hdr, &(iter).fds, &(iter).dds);                      \
	     !(rc) &&                                                            \
	     !pldm_package_downstream_device_id_record_iter_end(                 \
		     &(iter).dds) &&                                             \
	     !((rc) = decode_pldm_package_downstream_device_id_record_from_iter( \
		       (iter).hdr, &(iter).dds, &(rec)));                        \
	     pldm_package_downstream_device_id_record_iter_next(&(iter).dds))

LIBPLDM_ITERATOR
struct pldm_descriptor_iter
pldm_package_downstream_device_id_record_descriptor_iter_init(
	struct pldm_package_downstream_device_id_record_iter *iter,
	struct pldm_package_downstream_device_id_record *rec)
{
	(void)iter;
	return (struct pldm_descriptor_iter){ &rec->record_descriptors,
					      rec->descriptor_count };
}

/**
 * @brief Iterate over the descriptors in a package's downstream device ID record
 *
 * @param iter[in,out] The lvalue for the instance of @ref "struct pldm_package_iter"
 *             initialised by @ref decode_pldm_firmware_update_package
 * @param rec[in] An lvalue of type @ref "struct pldm_package_downstream_device_id_record"
 * @param desc[out] An lvalue of type @ref "struct pldm_descriptor" that holds
 *                  the parsed descriptor
 * @param rc[out] An lvalue of type int that holds the status result of parsing the
 *                downstream device ID record
 *
 * @p rc is set to 0 on successful decode. Otherwise, on error, @p rc is set to:
 * - -EINVAL if parameters values are invalid
 * - -EOVERFLOW if the package layout exceeds the bounds of the package buffer
 * - -EPROTO if package metadata doesn't conform to specification constraints
 *
 * Example use of the macro is as follows:
 *
 * @code
 * DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR02H(pin);
 *
 * struct pldm_package_downstream_device_id_record ddrec;
 * struct pldm_package_firmware_device_id_record fdrec;
 * pldm_package_header_information_pad hdr;
 * struct pldm_package_iter iter;
 * int rc;
 *
 * rc = decode_pldm_firmware_update_package(package, in, &pin, &hdr,
 * 					 &iter);
 * if (rc < 0) { ... }
 *
 * foreach_pldm_package_firmware_device_id_record(iter, fdrec, rc) {
 *     struct pldm_descriptor desc;
 * 	   ...
 *     foreach_pldm_package_firmware_device_id_record_descriptor(
 *             iter, fdrec, desc, rc) {
 *         ...
 *     }
 *     if (rc) { ... }
 * }
 * if (rc) { ... }
 *
 * foreach_pldm_package_downstream_device_id_record(iter, ddrec, rc) {
 *     struct pldm_descriptor desc;
 * 	   ...
 *     foreach_pldm_package_downstream_device_id_record_descriptor(
 *             iter, ddrec, desc, rc)
 *     {
 *         // Do something with desc
 *     }
 *     if (rc) {
 *         // Handle parsing failure for desc
 *     }
 * }
 * if (rc) { ... }
 * @endcode
 */
#define foreach_pldm_package_downstream_device_id_record_descriptor(iter, rec,      \
								    desc, rc)       \
	for (struct pldm_descriptor_iter desc##_iter =                              \
		     ((rc) = 0,                                                     \
		     pldm_package_downstream_device_id_record_descriptor_iter_init( \
			      &(iter).dds, &(rec)));                                \
	     (!pldm_descriptor_iter_end(&(desc##_iter))) &&                         \
	     !((rc) = decode_pldm_descriptor_from_iter(&(desc##_iter),              \
						       &(desc)));                   \
	     pldm_descriptor_iter_next(&(desc##_iter)))

LIBPLDM_ITERATOR
bool pldm_package_component_image_information_iter_end(
	const struct pldm_package_component_image_information_iter *iter)
{
	return (iter->entries == 0);
}

LIBPLDM_ITERATOR
bool pldm_package_component_image_information_iter_next(
	struct pldm_package_component_image_information_iter *iter)
{
	if (!iter->entries) {
		return false;
	}
	iter->entries--;
	return true;
}

int pldm_package_component_image_information_iter_init(
	const pldm_package_header_information_pad *hdr,
	struct pldm_package_downstream_device_id_record_iter *dds,
	struct pldm_package_component_image_information_iter *infos);

int decode_pldm_package_component_image_information_from_iter(
	const pldm_package_header_information_pad *hdr,
	struct pldm_package_component_image_information_iter *iter,
	struct pldm_package_component_image_information *info);

/**
 * @brief Iterate over the component image information contained in the package
 *
 * @param iter[in,out] The lvalue for the instance of @ref "struct pldm_package_iter"
 *             initialised by @ref decode_pldm_firmware_update_package
 * @param rec[in] An lvalue of type @ref "struct pldm_package_downstream_device_id_record"
 * @param desc[out] An lvalue of type @ref "struct pldm_descriptor" that holds
 *                  the parsed descriptor
 * @param rc[out] An lvalue of type int that holds the status result of parsing the
 *                downstream device ID record
 *
 * @p rc is set to 0 on successful decode. Otherwise, on error, @p rc is set to:
 * - -EINVAL if parameters values are invalid
 * - -EOVERFLOW if the package layout exceeds the bounds of the package buffer
 * - -EPROTO if package metadata doesn't conform to specification constraints
 *
 * Example use of the macro is as follows:
 *
 * @code
 * DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR02H(pin);
 *
 * struct pldm_package_downstream_device_id_record ddrec;
 * struct pldm_package_component_image_information info;
 * struct pldm_package_firmware_device_id_record fdrec;
 * pldm_package_header_information_pad hdr;
 * struct pldm_package_iter iter;
 * int rc;
 *
 * rc = decode_pldm_firmware_update_package(package, in, &pin, &hdr,
 * 					 &iter);
 * if (rc < 0) { ... }
 *
 * foreach_pldm_package_firmware_device_id_record(iter, fdrec, rc) {
 *     struct pldm_descriptor desc;
 * 	   ...
 *     foreach_pldm_package_firmware_device_id_record_descriptor(
 *             iter, fdrec, desc, rc) {
 *         ...
 *     }
 *     if (rc) { ... }
 * }
 * if (rc) { ... }
 *
 * foreach_pldm_package_downstream_device_id_record(iter, ddrec, rc) {
 *     struct pldm_descriptor desc;
 * 	   ...
 *     foreach_pldm_package_downstream_device_id_record_descriptor(
 *             iter, ddrec, desc, rc) {
 *         ...
 *     }
 *     if (rc) { ... }
 * }
 * if (rc) { ... }
 *
 * foreach_pldm_package_component_image_information(iter, info, rc) {
 *     // Do something with info
 * }
 * if (rc) {
 * 	   // Handle parsing failure for info
 * }
 * @endcode
 */
#define foreach_pldm_package_component_image_information(iter, info, rc)         \
	for ((rc) = pldm_package_component_image_information_iter_init(          \
		     (iter).hdr, &(iter).dds, &(iter).infos);                    \
	     !(rc) &&                                                            \
	     !pldm_package_component_image_information_iter_end(                 \
		     &(iter).infos) &&                                           \
	     !((rc) = decode_pldm_package_component_image_information_from_iter( \
		       (iter).hdr, &(iter).infos, &(info)));                     \
	     pldm_package_component_image_information_iter_next(                 \
		     &(iter).infos))

/**
 * Declare consumer support for at most revision 1 of the firmware update
 * package header
 *
 * @param name The name for the pin object
 *
 * The pin object must be provided to @ref decode_pldm_firmware_update_package
 */
#define DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR01H(name)                             \
	struct pldm_package_format_pin name = { \
		.meta = { \
			.magic = ( \
				LIBPLDM_SIZEAT(struct pldm__package_header_information, package) + \
				LIBPLDM_SIZEAT(struct pldm_package_firmware_device_id_record, firmware_device_package_data) + \
				LIBPLDM_SIZEAT(struct pldm_descriptor, descriptor_data) + \
				LIBPLDM_SIZEAT(struct pldm_package_component_image_information, component_version_string) + \
				LIBPLDM_SIZEAT(struct pldm_package_iter, infos) \
			), \
			.version = 0u, \
		}, \
		.format = { \
			.identifier = PLDM_PACKAGE_HEADER_IDENTIFIER_V1_0, \
			.revision = PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR01H, \
		} \
	}

/**
 * Declare consumer support for at most revision 2 of the firmware update
 * package header
 *
 * @param name The name for the pin object
 *
 * The pin object must be provided to @ref decode_pldm_firmware_update_package
 */
#define DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR02H(name)                             \
	struct pldm_package_format_pin name = { \
		.meta = { \
			.magic = ( \
				LIBPLDM_SIZEAT(struct pldm__package_header_information, package) + \
				LIBPLDM_SIZEAT(struct pldm_package_firmware_device_id_record, firmware_device_package_data) + \
				LIBPLDM_SIZEAT(struct pldm_descriptor, descriptor_data) + \
				LIBPLDM_SIZEAT(struct pldm_package_downstream_device_id_record, package_data) + \
				LIBPLDM_SIZEAT(struct pldm_package_component_image_information, component_version_string) + \
				LIBPLDM_SIZEAT(struct pldm_package_iter, infos) \
			), \
			.version = 0u, \
		}, \
		.format = { \
			.identifier = PLDM_PACKAGE_HEADER_IDENTIFIER_V1_1, \
			.revision = PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR02H, \
		} \
	}

/**
 * Declare consumer support for at most revision 3 of the firmware update
 * package header
 *
 * @param name The name for the pin object
 *
 * The pin object must be provided to @ref decode_pldm_firmware_update_package
 */
#define DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR03H(name)                             \
	struct pldm_package_format_pin name = { \
		.meta = { \
			.magic = ( \
				LIBPLDM_SIZEAT(struct pldm__package_header_information, package) + \
				LIBPLDM_SIZEAT(struct pldm_package_firmware_device_id_record, firmware_device_package_data) + \
				LIBPLDM_SIZEAT(struct pldm_descriptor, descriptor_data) + \
				LIBPLDM_SIZEAT(struct pldm_package_downstream_device_id_record, package_data) + \
				LIBPLDM_SIZEAT(struct pldm_package_component_image_information, component_opaque_data) + \
				LIBPLDM_SIZEAT(struct pldm_package_iter, infos) \
			), \
			.version = 0u, \
		}, \
		.format = { \
			.identifier = PLDM_PACKAGE_HEADER_IDENTIFIER_V1_2, \
			.revision = PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR03H, \
		} \
	}

/**
 * Declare consumer support for at most revision 4 of the firmware update
 * package header
 *
 * @param name The name for the pin object
 *
 * The pin object must be provided to @ref decode_pldm_firmware_update_package
 */
#define DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR04H(name)                             \
	struct pldm_package_format_pin name = { \
		.meta = { \
			.magic = ( \
				LIBPLDM_SIZEAT(struct pldm__package_header_information, package) + \
				LIBPLDM_SIZEAT(struct pldm_package_firmware_device_id_record, reference_manifest_data) + \
				LIBPLDM_SIZEAT(struct pldm_descriptor, descriptor_data) + \
				LIBPLDM_SIZEAT(struct pldm_package_downstream_device_id_record, reference_manifest_data) + \
				LIBPLDM_SIZEAT(struct pldm_package_component_image_information, component_opaque_data) + \
				LIBPLDM_SIZEAT(struct pldm_package_iter, infos) \
			), \
			.version = 0u, \
		}, \
		.format = { \
			.identifier = PLDM_PACKAGE_HEADER_IDENTIFIER_V1_3, \
			.revision = PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR04H, \
		} \
	}

#ifdef __cplusplus
}
#endif

#endif // End of FW_UPDATE_H
