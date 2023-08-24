#ifndef LIBPLDM_RDE_H
#define LIBPLDM_RDE_H

#include "base.h"
#include "pldm_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Data sizes for fixed size RDE commands.
 */
#define PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_SIZE 3

enum pldm_rde_commands {
	PLDM_NEGOTIATE_REDFISH_PARAMETERS = 0x01,
};

enum pldm_rde_varstring_format {
	PLDM_RDE_VARSTRING_UNKNOWN = 0,
	PLDM_RDE_VARSTRING_ASCII = 1,
	PLDM_RDE_VARSTRING_UTF_8 = 2,
	PLDM_RDE_VARSTRING_UTF_16 = 3,
	PLDM_RDE_VARSTRING_UTF_16LE = 4,
	PLDM_RDE_VARSTRING_UTF_16BE = 5,
};

enum pldm_rde_completion_codes {
	PLDM_RDE_ERROR_BAD_CHECKSUM = 0x80,
	PLDM_RDE_ERROR_CANNOT_CREATE_OPERATION = 0x81,
	PLDM_RDE_ERROR_NOT_ALLOWED = 0x82,
	PLDM_RDE_ERROR_WRONG_LOCATION_TYPE = 0x83,
	PLDM_RDE_ERROR_OPERATION_ABANDONED = 0x84,
	PLDM_RDE_ERROR_OPERATION_UNKILLABLE = 0x85,
	PLDM_RDE_ERROR_OPERATION_EXISTS = 0x86,
	PLDM_RDE_ERROR_OPERATION_FAILED = 0x87,
	PLDM_RDE_ERROR_UNEXPECTED = 0x88,
	PLDM_RDE_ERROR_UNSUPPORTED = 0x89,
	PLDM_RDE_ERROR_UNRECOGNIZED_CUSTOM_HEADER = 0x90,
	PLDM_RDE_ERROR_ETAG_MATCH = 0x91,
	PLDM_RDE_ERROR_NO_SUCH_RESOURCE = 0x92,
	PLDM_RDE_ETAG_CALCULATION_ONGOING = 0x93,
};

/**
 * @brief MC feature support.
 *
 * The flags can be OR'd together to build the feature support for a MC.
 */
enum pldm_rde_mc_feature {
	PLDM_RDE_MC_HEAD_SUPPORTED = 1,
	PLDM_RDE_MC_READ_SUPPORTED = 2,
	PLDM_RDE_MC_CREATE_SUPPORTED = 4,
	PLDM_RDE_MC_DELETE_SUPPORTED = 8,
	PLDM_RDE_MC_UPDATE_SUPPORTED = 16,
	PLDM_RDE_MC_REPLACE_SUPPORTED = 32,
	PLDM_RDE_MC_ACTION_SUPPORTED = 64,
	PLDM_RDE_MC_EVENTS_SUPPORTED = 128,
	PLDM_RDE_MC_BEJ_1_1_SUPPORTED = 256,
};

/**
 * @brief Device capability flags.
 *
 * The flags can be OR'd together to build capabilities of a device.
 */
enum pldm_rde_device_capability {
	PLDM_RDE_DEVICE_ATOMIC_RESOURCE_READ_SUPPORT = 1,
	PLDM_RDE_DEVICE_EXPAND_SUPPORT = 2,
	PLDM_RDE_DEVICE_BEJ_1_1_SUPPORT = 4,
};

/**
 * @brief Device feature support.
 *
 * The flags can be OR'd together to build features of a RDE device.
 */
enum pldm_rde_device_feature {
	PLDM_RDE_DEVICE_HEAD_SUPPORTED = 1,
	PLDM_RDE_DEVICE_READ_SUPPORTED = 2,
	PLDM_RDE_DEVICE_CREATE_SUPPORTED = 4,
	PLDM_RDE_DEVICE_DELETE_SUPPORTED = 8,
	PLDM_RDE_DEVICE_UPDATE_SUPPORTED = 16,
	PLDM_RDE_DEVICE_REPLACE_SUPPORTED = 32,
	PLDM_RDE_DEVICE_ACTION_SUPPORTED = 64,
	PLDM_RDE_DEVICE_EVENTS_SUPPORTED = 128,
};

/**
 * @brief NegotiateRedfishParameters request data structure.
 */
struct pldm_rde_negotiate_redfish_parameters_req {
	uint8_t mc_concurrency_support;
	bitfield16_t mc_feature_support;
};

/**
 * @brief varstring PLDM data type.
 */
struct pldm_rde_varstring {
	uint8_t string_format;
	// Includes NULL terminator.
	uint8_t string_length_bytes;
	// String data should be NULL terminated.
	char* string_data;
};

/**
 * @brief Encode NegotiateRedfishParameters request.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] concurrency_support - MC concurrency support.
 * @param[in] feature_support - MC feature support flags.
 * @param[in] payload_length - Length of the encoded payload segment.
 * @param[out] msg - Request message.
 * @return pldm_completion_codes.
 */
int encode_rde_negotiate_redfish_parameters_req(uint8_t instance_id,
						uint8_t concurrency_support,
						bitfield16_t *feature_support,
						size_t payload_length,
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
int decode_rde_negotiate_redfish_parameters_req(
	const struct pldm_msg *msg, size_t payload_length,
	uint8_t *mc_concurrency_support, bitfield16_t *mc_feature_support);

#ifdef __cplusplus
}
#endif

#endif /* LIBPLDM_RDE_H */
