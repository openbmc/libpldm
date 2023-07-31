#ifndef PLDM_RDE_H
#define PLDM_RDE_H

#include "base.h"
#include "pldm_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Minimum transfer size allowed is 64 bytes.
#define PLDM_RDE_MIN_TRANSFER_SIZE_BYTES 64

enum pldm_rde_commands {
	PLDM_NEGOTIATE_REDFISH_PARAMETERS = 0x01,
	PLDM_NEGOTIATE_MEDIUM_PARAMETERS = 0x02,
	PLDM_GET_SCHEMA_DICTIONARY = 0x03,
};

enum pldm_rde_varstring_format {
	PLDM_RDE_VARSTRING_UNKNOWN = 0,
	PLDM_RDE_VARSTRING_ASCII = 1,
	PLDM_RDE_VARSTRING_UTF_8 = 2,
	PLDM_RDE_VARSTRING_UTF_16 = 3,
	PLDM_RDE_VARSTRING_UTF_16LE = 4,
	PLDM_RDE_VARSTRING_UTF_16BE = 5,
};

enum pldm_rde_schema_type {
	PLDM_RDE_SCHEMA_MAJOR = 0,
	PLDM_RDE_SCHEMA_EVENT = 1,
	PLDM_RDE_SCHEMA_ANNOTATION = 2,
	PLDM_RDE_SCHEMA_COLLECTION_MEMBER_TYPE = 3,
	PLDM_RDE_SCHEMA_ERROR = 4,
	PLDM_RDE_SCHEMA_REGISTRY = 5,
};

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
} __attribute__((packed));

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
 * @brief NegotiateMediumParameters request data structure.
 */
struct pldm_rde_negotiate_medium_parameters_req {
	uint32_t mc_maximum_transfer_chunk_size_bytes;
} __attribute__((packed));

/**
 * @brief NegotiateMediumParameters response data structure.
 */
struct pldm_rde_negotiate_medium_parameters_resp {
	uint8_t completion_code;
	uint32_t device_maximum_transfer_chunk_size_bytes;
} __attribute__((packed));

/**
 * @brief GetSchemaDictionary request data structure.
 */
struct pldm_rde_get_schema_dictionary_req {
	uint32_t resource_id;
	uint8_t requested_schema_class;
} __attribute__((packed));

/**
 * @brief GetSchemaDictionary response data structure.
 */
struct pldm_rde_get_schema_dictionary_resp {
	uint8_t completion_code;
	uint8_t dictionary_format;
	uint32_t transfer_handle;
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

/**
 * @brief Create a PLDM response message for NegotiateRedfishParameters.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] completion_code - PLDM completion code.
 * @param[in] device_concurrency_support - Concurrency support.
 * @param[in] device_capabilities_flags - Capabilities flags.
 * @param[in] device_feature_support - Feature support flags.
 * @param[in] device_configuration_signature - RDE device signature.
 * @param[in] device_provider_name - Null terminated device provider name.
 * @param[in] name_format - String format of the device_provider_name.
 * @param[out] msg - Response message will be written to this.
 * @return pldm_completion_codes.
 */
int encode_negotiate_redfish_parameters_resp(
	uint8_t instance_id, uint8_t completion_code,
	uint8_t device_concurrency_support,
	bitfield8_t device_capabilities_flags,
	bitfield16_t device_feature_support,
	uint32_t device_configuration_signature,
	const char *device_provider_name,
	enum pldm_rde_varstring_format name_format, struct pldm_msg *msg);

/**
 * @brief Encode NegotiateMediumParameters request.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] maximum_transfer_size - Maximum amount of data the MC can
 * support for a single message transfer.
 * @param[out] msg - Request message.
 * @return pldm_completion_codes.
 */
int encode_negotiate_medium_parameters_req(uint8_t instance_id,
					   uint32_t maximum_transfer_size,
					   struct pldm_msg *msg);

/**
 * @brief Decode NegotiateMediumParameters request.
 *
 * @param[in] msg - Request message.
 * @param[in] payload_length - Length of request message payload.
 * @param[out] mc_maximum_transfer_size - Pointer to a uint32_t variable.
 * @return pldm_completion_codes.
 */
int decode_negotiate_medium_parameters_req(const struct pldm_msg *msg,
					   size_t payload_length,
					   uint32_t *mc_maximum_transfer_size);

/**
 * @brief Create a PLDM response message for NegotiateMediumParameters.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] completion_code - PLDM completion code.
 * @param[in] device_maximum_transfer_bytes - Device maximum transfer byte
 * support.
 * @param[out] msg - Response message will be written to this.
 * @return pldm_completion_codes.
 */
int encode_negotiate_medium_parameters_resp(
	uint8_t instance_id, uint8_t completion_code,
	uint32_t device_maximum_transfer_bytes, struct pldm_msg *msg);

/**
 * @brief Encode GetSchemaDictionary request.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] resource_id - The ResourceID of any resource in the Redfish
 * Resource PDR.
 * @param[in] schema_class - The class of schema being requested.
 * @param[out] msg - Request message.
 * @return pldm_completion_codes.
 */
int encode_get_schema_dictionary_req(uint8_t instance_id, uint32_t resource_id,
				     uint8_t schema_class,
				     struct pldm_msg *msg);

/**
 * @brief Decode GetSchemaDictionary request.
 *
 * @param[in] msg - Request message.
 * @param[in] payload_length - Length of request message payload.
 * @param[out] resource_id - Pointer to a uint32_t variable.
 * @param[out] requested_schema_class - Pointer to a uint8_t variable.
 * @return pldm_completion_codes.
 */
int decode_get_schema_dictionary_req(const struct pldm_msg *msg,
				     size_t payload_length,
				     uint32_t *resource_id,
				     uint8_t *requested_schema_class);

/**
 * @brief Encode GetSchemaDictionary response.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] completion_code - PLDM completion code.
 * @param[in] dictionary_format - The format of the dictionary.
 * @param[in] transfer_handle - A data transfer handle that the MC shall
 * use.
 * @param[out] msg - Response message will be written to this.
 * @return pldm_completion_codes.
 */
int encode_get_schema_dictionary_resp(uint8_t instance_id,
				      uint8_t completion_code,
				      uint8_t dictionary_format,
				      uint32_t transfer_handle,
				      struct pldm_msg *msg);

#ifdef __cplusplus
}
#endif

#endif /* PLDM_RDE_H */
