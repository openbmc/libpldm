/**
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 * 
 * The following values have been derived from the PLDM RDE spec by DMTF-
 * https://www.dmtf.org/sites/default/files/standards/documents/DSP0218_1.1.1.pdf
*/
#ifndef PLDM_RDE_H
#define PLDM_RDE_H

#include "libpldm/base.h"
#include "libpldm/pldm_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief RDE Supported Commands- More can be added acc to DMTF spec
 */
enum pldm_rde_commands {
	PLDM_NEGOTIATE_REDFISH_PARAMETERS = 0x01,
	PLDM_NEGOTIATE_MEDIUM_PARAMETERS = 0x02,
	PLDM_GET_SCHEMA_DICTIONARY = 0x03,
	PLDM_GET_SCHEMA_FILE = 0x0C,
	PLDM_RDE_OPERATION_INIT = 0x10,
	PLDM_RDE_OPERATION_COMPLETE = 0x13,
	PLDM_RDE_OPERATION_STATUS = 0x14,
	PLDM_RDE_OPERATION_KILL = 0x15,
	PLDM_RDE_MULTIPART_SEND = 0x30,
	PLDM_RDE_MULTIPART_RECEIVE = 0x31,
};

/**
 * @brief RDE varstring representation
*/
typedef enum pldm_rde_varstring_format {
	PLDM_RDE_VARSTRING_UNKNOWN = 0,
	PLDM_RDE_VARSTRING_ASCII = 1,
	PLDM_RDE_VARSTRING_UTF_8 = 2,
	PLDM_RDE_VARSTRING_UTF_16 = 3,
	PLDM_RDE_VARSTRING_UTF_16LE = 4,
	PLDM_RDE_VARSTRING_UTF_16BE = 5,
} pldm_rde_varstring_format;

/**
 * @brief RDE Operation completion codes
*/
enum pldm_rde_completion_codes {
	PLDM_RDE_ERROR_CANNOT_CREATE_OPERATION = 0x81,
	PLDM_RDE_ERROR_NOT_ALLOWED = 0x82,
	PLDM_RDE_ERROR_WRONG_LOCATION_TYPE = 0x83,
	PLDM_RDE_ERROR_OPERATION_ABANDONED = 0x84,
	PLDM_RDE_ERROR_OPERATION_EXISTS = 0x86,
	PLDM_RDE_ERROR_OPERATION_FAILED = 0x87,
	PLDM_RDE_ERROR_UNEXPECTED = 0x88,
	PLDM_RDE_ERROR_UNSUPPORTED = 0x89,
	PLDM_RDE_ERROR_NO_SUCH_RESOURCE = 0x92,
};

/**
 * @brief Transfer operation for RDE.
 */
enum pldm_rde_transfer_operation {
	PLDM_RDE_XFER_FIRST_PART = 0,
	PLDM_RDE_XFER_NEXT_PART = 1,
	PLDM_RDE_XFER_ABORT = 2,
};

/**
 * @brief Transfer flags for RDE.
 */
enum pldm_rde_transfer_flag {
	PLDM_RDE_START = 0,
	PLDM_RDE_MIDDLE = 1,
	PLDM_RDE_END = 2,
	PLDM_RDE_START_AND_END = 3,
};

/**
 * @brief Operation Types for RDE
*/
enum pldm_rde_operation_type {
	PLDM_RDE_OPERATION_HEAD = 0,
	PLDM_RDE_OPERATION_READ = 1,
	PLDM_RDE_OPERATION_CREATE = 2,
	PLDM_RDE_OPERATION_DELETE = 3,
	PLDM_RDE_OPERATION_UPDATE = 4,
	PLDM_RDE_OPERATION_REPLACE = 5,
	PLDM_RDE_OPERATION_ACTION = 6,
};

/**
 * @brief Operation status for RDE
*/
enum pldm_rde_operation_status {
	PLDM_RDE_OPERATION_INACTIVE = 0,
	PLDM_RDE_OPERATION_NEEDS_INPUT = 1,
	PLDM_RDE_OPERATION_TRIGGERED = 2,
	PLDM_RDE_OPERATION_RUNNING = 3,
	PLDM_RDE_OPERATION_HAVE_RESULTS = 4,
	PLDM_RDE_OPERATION_COMPLETED = 5,
	PLDM_RDE_OPERATION_FAILED = 6,
	PLDM_RDE_OPERATION_ABANDONED = 7,
};

/**
 * @brief varstring PLDM data type.
 *
 * sizeof(struct pldm_rde_varstring) will include the space for the NULL
 * character.
 */
struct pldm_rde_varstring {
	uint8_t string_format;
	// Includes NULL terminator.
	uint8_t string_length_bytes;
	// String data should be NULL terminated.
	uint8_t string_data[1];
} __attribute__((packed));

/**
 * @brief NegotiateRedfishParameters request data structure.
 */
struct pldm_rde_negotiate_redfish_parameters_req {
	uint8_t mc_concurrency_support;
	bitfield16_t mc_feature_support;
} __attribute__((packed));

/**
 * @brief NegotiateRedfishParameters response data structure.
 */
struct pldm_rde_negotiate_redfish_parameters_resp {
	uint8_t completion_code;
	uint8_t device_concurrency_support;
	bitfield8_t device_capabilities_flags;
	bitfield16_t device_feature_support;
	uint32_t device_configuration_signature;
	struct pldm_rde_varstring device_provider_name;
} __attribute__((packed));

/**
 * @brief Encode NegotiateRedfishParameters request.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] concurrency_support - MC concurrency support.
 * @param[in] feature_support - MC feature support flags.
 * @param[out] msg - Request message.
 * @return pldm_completion_codes.
 */
int encode_negotiate_redfish_parameters_req(uint8_t instance_id,
					    uint8_t concurrency_support,
					    bitfield16_t *feature_support,
					    struct pldm_msg *msg);

/**
 * @brief Decode NegotiateRedfishParameters request.
 *
 * @param[in] msg - Request message.
 * @param[in] payload_length - Length of request message payload.
 * @param[out] mc_concurrency_support - Pointer to a uint8_t variable.
 * @param[out] mc_feature_support - Pointer to a bitfield16_t variable.
 * @return pldm_completion_codes.
 */
int decode_negotiate_redfish_parameters_req(const struct pldm_msg *msg,
					    size_t payload_length,
					    uint8_t *mc_concurrency_support,
					    bitfield16_t *mc_feature_support);

#ifdef __cplusplus
}
#endif

#endif /* PLDM_RDE_H */
