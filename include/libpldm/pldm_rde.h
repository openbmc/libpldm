#ifndef PLDM_RDE_H
#define PLDM_RDE_H

#include "base.h"
#include "pldm_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Minimum transfer size allowed is 64 bytes.
#define PLDM_RDE_MIN_TRANSFER_SIZE_BYTES 64
// Dictionary VersionTag for DSP0218 v1.0.0, v1.1.0, v1.1.1.
#define PLDM_RDE_DICT_VERSION_TAG 0x00
#define PLDM_RDE_NOT_A_OPERATION 0x00
#define PLD_RDE_NULL_TRANSFER_HANDLE 0
#define PLD_RDE_OP_PENDING_TRANSFER_HANDLE 0xFFFFFFFF
#define PLDM_RDE_COMP_TIME_NOT_SUPPORTED 0xFFFFFFFF
#define PLDM_RDE_COMP_PERCENTAGE_NOT_SUPPORTED 254
// Variable struct header sizes
#define PLDM_RDE_MULTIPART_RECEIVE_RESP_HDR_SIZE 10
#define PLDM_RDE_OPERATION_INIT_REQ_HDR_SIZE 17
#define PLDM_RDE_OPERATION_INIT_RESP_HDR_SIZE 17
#define PLDM_RDE_OPERATION_STATUS_RESP_HDR_SIZE 17

#define PLDM_RDE 0x06 /*Response should support PLDM_PLATFORM and PLDM_RDE*/
#define RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_BYTES 12
#define RDE_NEGOTIATE_MEDIUM_PARAMETERS_RESP_BYTES 5
#define RDE_GET_DICTIONARY_SCHEMA_RESP_BYTES 6
#define RDE_MULTIPART_RECV_MINIMUM_RESP_BYTES 6
#define RDE_READ_OPERATION_INIT_MIN_BYTES 13
#define RESOURCE_ID_ANY 0xFFFFFFFF
#define IGNORE(x) (void)(x)

/** @brief RDE Supported Commands
 */
enum pldm_rde_commands {
	PLDM_NEGOTIATE_REDFISH_PARAMETERS = 0x01,
	PLDM_NEGOTIATE_MEDIUM_PARAMETERS = 0x02,
	PLDM_GET_SCHEMA_DICTIONARY = 0x03,
	PLDM_GET_SCHEMA_FILE = 0x0C,
	PLDM_RDE_OPERATION_INIT = 0x10,
	PLDM_RDE_OPERATION_COMPLETE = 0x13,
	PLDM_RDE_OPERATION_STATUS = 0x14,
	PLDM_RDE_MULTIPART_SEND = 0x30,
	PLDM_RDE_MULTIPART_RECEIVE = 0x31,
};

typedef enum pldm_rde_varstring_format {
	PLDM_RDE_VARSTRING_UNKNOWN = 0,
	PLDM_RDE_VARSTRING_ASCII = 1,
	PLDM_RDE_VARSTRING_UTF_8 = 2,
	PLDM_RDE_VARSTRING_UTF_16 = 3,
	PLDM_RDE_VARSTRING_UTF_16LE = 4,
	PLDM_RDE_VARSTRING_UTF_16BE = 5,
} pldm_rde_varstring_format;

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

// TODO: Do we need this because base.h already has this
/**
 * @brief Transfer operation flags.
 */
enum pldm_rde_transfer_operation {
	PLDM_RDE_XFER_FIRST_PART = 0,
	PLDM_RDE_XFER_NEXT_PART = 1,
	PLDM_RDE_XFER_ABORT = 2,
};
// TODO: Do we need this because base.h already has this
enum pldm_rde_transfer_flag {
	PLDM_RDE_START = 0,
	PLDM_RDE_MIDDLE = 1,
	PLDM_RDE_END = 2,
	PLDM_RDE_START_AND_END = 3,
};

enum pldm_rde_operation_type {
	PLDM_RDE_OPERATION_HEAD = 0,
	PLDM_RDE_OPERATION_READ = 1,
	PLDM_RDE_OPERATION_CREATE = 2,
	PLDM_RDE_OPERATION_DELETE = 3,
	PLDM_RDE_OPERATION_UPDATE = 4,
	PLDM_RDE_OPERATION_REPLACE = 5,
	PLDM_RDE_OPERATION_ACTION = 6,
};

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
 * @brief RDEMultipartReceive request data structure.
 */
struct pldm_rde_multipart_receive_req {
	uint32_t data_transfer_handle;
	uint16_t operation_id;
	uint8_t transfer_operation;
} __attribute__((packed));
/**
 * @brief RDEMultipartReceive response data structure.
 */
struct pldm_rde_multipart_receive_resp {
	uint8_t completion_code;
	uint8_t transfer_flag;
	uint32_t next_data_transfer_handle;
	uint32_t data_length_bytes;
	uint8_t payload[1];
} __attribute__((packed));
/**
 * @brief OperationFlags used in RDEOperationInit request data structure.
 */
union pldm_rde_operation_flags {
	uint8_t byte;
	struct {
		uint8_t locator_valid : 1;
		uint8_t contains_request_payload : 1;
		uint8_t contains_custom_request_parameters : 1;
		uint8_t excerpt_flag : 1;
		uint8_t reserved : 4;
	} __attribute__((packed)) bits;
};
/**
 * @brief RDEOperationInit request data structure.
 */
struct pldm_rde_operation_init_req {
	uint32_t resource_id;
	uint16_t operation_id;
	uint8_t operation_type;
	union pldm_rde_operation_flags operation_flags;
	uint32_t send_data_transfer_handle;
	uint8_t operation_locator_length;
	uint32_t request_payload_length;
	// Variable length data: bejLocator and the payload.
	uint8_t var_data[1];
} __attribute__((packed));
/**
 * @brief OperationExecutionFlags used in RDEOperationInit and
 * RDEOperationStatus response data structures.
 */
union pldm_rde_op_execution_flags {
	uint8_t byte;
	struct {
		uint8_t task_spawned : 1;
		uint8_t have_custom_response_parameters : 1;
		uint8_t have_result_payload : 1;
		uint8_t cache_allowed : 1;
		uint8_t reserved : 4;
	} __attribute__((packed)) bits;
};
/**
 * @brief PermissionFlags used in RDEOperationInit and RDEOperationStatus
 * response data structures.
 */
union pldm_rde_permission_flags {
	uint8_t byte;
	struct {
		uint8_t read_allowed : 1;
		uint8_t update_allowed : 1;
		uint8_t replace_allowed : 1;
		uint8_t create_allowed : 1;
		uint8_t delete_allowed : 1;
		uint8_t head_allowed : 1;
		uint8_t reserved : 2;
	} __attribute__((packed)) bits;
};
/**
 * @brief RDEOperationInit response data structure.
 */
struct pldm_rde_operation_init_resp {
	uint8_t completion_code;
	uint8_t operation_status;
	uint8_t completion_percentage;
	uint32_t completion_time_seconds;
	union pldm_rde_op_execution_flags operation_execution_flags;
	uint32_t result_transfer_handle;
	union pldm_rde_permission_flags permission_flags;
	uint32_t response_payload_length;
	// Variable length data: varstring and the payload.
	uint8_t var_data[1];
} __attribute__((packed));
/**
 * @brief RDEOperationComplete request data structure.
 */
struct pldm_rde_operation_complete_req {
	uint32_t resource_id;
	uint16_t operation_id;
} __attribute__((packed));
/**
 * @brief RDEOperationComplete response data structure.
 */
struct pldm_rde_operation_complete_resp {
	uint8_t completion_code;
} __attribute__((packed));
/**
 * @brief RDEOperationStatus request data structure.
 */
struct pldm_rde_operation_status_req {
	uint32_t resource_id;
	uint16_t operation_id;
} __attribute__((packed));
/**
 * @brief RDEOperationStatus response data structure.
 */
struct pldm_rde_operation_status_resp {
	uint8_t completion_code;
	uint8_t operation_status;
	uint8_t completion_percentage;
	uint32_t completion_time_seconds;
	union pldm_rde_op_execution_flags operation_execution_flags;
	uint32_t result_transfer_handle;
	union pldm_rde_permission_flags permission_flags;
	uint32_t response_payload_length;
	// Variable length data: varstring and the payload.
	uint8_t var_data[1];
} __attribute__((packed));

struct pldm_rde_device_info {
	uint8_t device_concurrency;
	bitfield8_t device_capabilities_flag;
	bitfield16_t device_feature_support;
	uint32_t device_configuration_signature;
	struct pldm_rde_varstring device_provider_name;
	uint32_t device_maximum_transfer_chunk_size; // in bytes
};
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
    uint8_t device_concurrency_support, bitfield8_t device_capabilities_flags,
    bitfield16_t device_feature_support,
    uint32_t device_configuration_signature, const char *device_provider_name,
    enum pldm_rde_varstring_format name_format, struct pldm_msg *msg);

/**
 * @brief Decode NegotiateRedfishParameters Response.
 *
 * @param[in] msg - Request message.
 * @param[in] payload_length - Length of request message payload.
 * @param[out] completion_code - Completion code as set by the responder.
 * @param[out] device - Device Info as sent in the response (Memory to be
 * manager by the caller).
 * @return pldm_completion_codes.
 */
int decode_negotiate_redfish_parameters_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    struct pldm_rde_device_info *device);

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
 * @brief Decode Negotiate Medium Parameters response
 *
 * @param[in] msg: PLDM Msg byte array received from the responder
 * @param[in] payload_length: Length of the payload
 * @param[out] completion_code: Completion code as set by the responder
 * @param[out] device_maximum_transfer_bytes: Max bytes device can transfer
 */
int decode_negotiate_medium_parameters_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint32_t *device_maximum_transfer_bytes);
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
/**
 * @brief Decode Get Schema Dictionary Response
 *
 * @param[in] msg - Response Message
 * @param[in] payload_length - Length of the payload
 * @param[out] completion_code - Completion Code
 * @param[out] dictionary_format - Dictionary Format for the particular resource
 * id
 * @param[out] transfer_handle - Transfer Handle to be used to get dictionary
 */
int decode_get_schema_dictionary_resp(const struct pldm_msg *msg,
				      size_t payload_length,
				      uint8_t *completion_code,
				      uint8_t *dictionary_format,
				      uint32_t *transfer_handle);
/**
 * @brief Encode RDEMultipartReceive request.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] data_transfer_handle - A handle to uniquely identify the chunk
 * of data to be retrieved.
 * @param[in] operation_id - Identification number for this operation.
 * @param[in] transfer_operation - The portion of data requested for the
 * transfer.
 * @param[out] msg - Request will be written to this.
 * @return pldm_completion_codes.
 */
int encode_rde_multipart_receive_req(uint8_t instance_id,
				     uint32_t data_transfer_handle,
				     uint16_t operation_id,
				     uint8_t transfer_operation,
				     struct pldm_msg *msg);
/**
 * @brief Decode RDEMultipartReceive request.
 *
 * @param[in] msg - Request message.
 * @param[in] payload_length - Length of request message payload.
 * @param[out] data_transfer_handle - A handle to uniquely identify the
 * chunk of data to be retrieved.
 * @param[out] operation_id - Identification number for this operation.
 * @param[out] transfer_operation - The portion of data requested for the
 * transfer.
 * @return pldm_completion_codes.
 */
int decode_rde_multipart_receive_req(const struct pldm_msg *msg,
				     size_t payload_length,
				     uint32_t *data_transfer_handle,
				     uint16_t *operation_id,
				     uint8_t *transfer_operation);
/**
 * @brief Encode RDEMultipartReceive response.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] completion_code - PLDM completion code.
 * @param[in] transfer_flag - The portion of data being sent to MC.
 * @param[in] next_data_transfer_handle - A handle to uniquely identify the
 * next chunk of data to be retrieved.
 * @param[in] data_length_bytes - Length of the payload.
 * @param[in] add_checksum - Indicate whether the payload needs to include
 * the provided checksum.
 * @param[in] checksum - Checksum.
 * @param[in] payload - Pointer to the payload.
 * @param[out] msg - Response message will be written to this.
 * @return pldm_completion_codes.
 */
int encode_rde_multipart_receive_resp(
    uint8_t instance_id, uint8_t completion_code, uint8_t transfer_flag,
    uint32_t next_data_transfer_handle, uint32_t data_length_bytes,
    bool add_checksum, uint32_t checksum, const uint8_t *payload,
    struct pldm_msg *msg);
/**
 * @brief Decode RDE Multipart Receive Response
 *
 * @param[in] msg - Response message
 * @param[in] payload_length - Expected length of the response, since the
 * response could be equal to the negotiated transfer chunk size, the requester
 * should usually set it to the negotiated transfer size
 * @param[out] completion_code - Completion code of the response set by RDE
 * @param[out] ret_transfer_flag - Transfer flag returned by RDE
 * @param[out] ret_transfer_operation - Transfer operation returned by RDE
 * @param[out] data_length_bytes - The length of the payload in response
 * @param[out] payload - Pointer to the payload
 */
int decode_rde_multipart_receive_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint8_t *ret_transfer_flag, uint32_t *ret_data_transfer_handle,
    uint32_t *data_length_bytes, uint8_t **payload);
/**
 * @brief Encode RDEOperationInit request.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] resource_id - The ResourceID.
 * @param[in] operation_id - Identification number for this operation.
 * @param[in] operation_type - The type of Redfish Operation being
 * performed.
 * @param[in] operation_flags - Flags associated with this Operation.
 * @param[in] send_data_transfer_handle - Handle to be used with the first
 * RDEMultipartSend command.
 * @param[in] operation_locator_length - Length of the OperationLocator for
 * this Operation.
 * @param[in] request_payload_length - Length of the request payload in this
 * message.
 * @param[in] operation_locator - BEJ locator indicating where the new
 * Operation is to take place within the resource.
 * @param[in] request_payload - The request payload.
 * @param[out] msg - Request will be written to this.
 * @return pldm_completion_codes.
 */
int encode_rde_operation_init_req(
    uint8_t instance_id, uint32_t resource_id, uint16_t operation_id,
    uint8_t operation_type,
    const union pldm_rde_operation_flags *operation_flags,
    uint32_t send_data_transfer_handle, uint8_t operation_locator_length,
    uint32_t request_payload_length, const uint8_t *operation_locator,
    uint8_t *request_payload, struct pldm_msg *msg);
/**
 * @brief Decode RDEOperationInit request.
 *
 * @param[in] msg - Request message.
 * @param[in] payload_length - Length of request message payload.
 * @param[out] resource_id - The ResourceID.
 * @param[out] operation_id - Identification number for this operation.
 * @param[out] operation_type - The type of Redfish Operation being
 * performed.
 * @param[out] operation_flags  - Flags associated with this Operation.
 * @param[out] send_data_transfer_handle - Handle to be used with the first
 * RDEMultipartSend command.
 * @param[out] operation_locator_length - Length of the OperationLocator for
 * this Operation.
 * @param[out] request_payload_length - Length of the request payload in
 * this message.
 * @param[out] operation_locator - BEJ locator indicating where the new
 * Operation is to take place within the resource.
 * @param[out] request_payload - The request payload.
 * @return pldm_completion_codes.
 */
int decode_rde_operation_init_req(
    const struct pldm_msg *msg, size_t payload_length, uint32_t *resource_id,
    uint16_t *operation_id, uint8_t *operation_type,
    union pldm_rde_operation_flags *operation_flags,
    uint32_t *send_data_transfer_handle, uint8_t *operation_locator_length,
    uint32_t *request_payload_length, uint8_t **operation_locator,
    uint8_t **request_payload);
/**
 * @brief Encode RDEOperationInit response.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] completion_code - PLDM completion code.
 * @param[in] operation_status - Status of the operation.
 * @param[in] completion_percentage - Percentage complete.
 * @param[in] completion_time_seconds - An estimate of the number of seconds
 * remaining before the Operation is completed.
 * @param[in] operation_execution_flags - Explains the result of operation.
 * @param[in] result_transfer_handle - A data transfer handle that the MC
 * may use to retrieve a larger response payload.
 * @param[in] permission_flags - Indicates the access level granted to the
 * resource targeted by the Operation.
 * @param[in] response_payload_length - Length of the response payload.
 * @param[in] etag_format - Format of the etag string.
 * @param[in] etag - ETag.
 * @param[in] response_payload - The response payload if the payload fits.
 * @param[out] msg - Response message will be written to this.
 * @return pldm_completion_codes.
 */
int encode_rde_operation_init_resp(
    uint8_t instance_id, uint8_t completion_code, uint8_t operation_status,
    uint8_t completion_percentage, uint32_t completion_time_seconds,
    const union pldm_rde_op_execution_flags *operation_execution_flags,
    uint32_t result_transfer_handle,
    const union pldm_rde_permission_flags *permission_flags,
    uint32_t response_payload_length,
    enum pldm_rde_varstring_format etag_format, const char *etag,
    const uint8_t *response_payload, struct pldm_msg *msg);
/**
 * @brief Decode RDEOperationInit Resp
 */
int decode_rde_operation_init_resp(
    const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint8_t *completion_percentage, uint8_t *operation_status,
    uint32_t *completion_time_seconds, uint32_t *result_transfer_handle,
    uint32_t *response_payload_length,
    union pldm_rde_permission_flags **permission_flags,
    union pldm_rde_op_execution_flags **operation_execution_flags,
    struct pldm_rde_varstring **resp_etag, uint8_t **response_payload);
/**
 * @brief Encode RDEOperationComplete request.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] resource_id - The ResourceID.
 * @param[in] operation_id - Identification number for this operation.
 * @param[out] msg - Request will be written to this.
 * @return pldm_completion_codes.
 */
int encode_rde_operation_complete_req(uint8_t instance_id, uint32_t resource_id,
				      uint16_t operation_id,
				      struct pldm_msg *msg);
/**
 * @brief Decode RDEOperationComplete request.
 *
 * @param[in] msg - Request message.
 * @param[in] payload_length - Length of request message payload.
 * @param[out] resource_id - The ResourceID.
 * @param[out] operation_id - Identification number for this operation.
 * @return pldm_completion_codes.
 */
int decode_rde_operation_complete_req(const struct pldm_msg *msg,
				      size_t payload_length,
				      uint32_t *resource_id,
				      uint16_t *operation_id);
/**
 * @brief Encode RDEOperationComplete response.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] completion_code - PLDM completion code.
 * @param[out] msg - Response message will be written to this.
 * @return pldm_completion_codes.
 */
int encode_rde_operation_complete_resp(uint8_t instance_id,
				       uint8_t completion_code,
				       struct pldm_msg *msg);

/**
 * @brief Encode RDEOperationComplete response.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] completion_code - PLDM completion code.
 * @param[out] msg - Response message will be written to this.
 * @return pldm_completion_codes.
 */
int decode_rde_operation_complete_resp(const struct pldm_msg *msg,
				       size_t payload_length,
				       uint8_t *completion_code);
/**
 * @brief Encode RDEOperationStatus request.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] resource_id - The ResourceID.
 * @param[in] operation_id - Identification number for this operation.
 * @param[out] msg - Request will be written to this.
 * @return pldm_completion_codes.
 */
int encode_rde_operation_status_req(uint8_t instance_id, uint32_t resource_id,
				    uint16_t operation_id,
				    struct pldm_msg *msg);
/**
 * @brief Decode RDEOperationStatus request.
 *
 * @param[in] msg - Request message.
 * @param[in] payload_length - Length of request message payload.
 * @param[out] resource_id - The ResourceID.
 * @param[out] operation_id - Identification number for this operation.
 * @return pldm_completion_codes.
 */
int decode_rde_operation_status_req(const struct pldm_msg *msg,
				    size_t payload_length,
				    uint32_t *resource_id,
				    uint16_t *operation_id);
/**
 * @brief Encode RDEOperationStatus response.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] completion_code - PLDM completion code.
 * @param[in] operation_status - Status of the operation.
 * @param[in] completion_percentage - Percentage complete.
 * @param[in] completion_time_seconds - An estimate of the number of seconds
 * remaining before the Operation is completed.
 * @param[in] operation_execution_flags - Explains the result of operation.
 * @param[in] result_transfer_handle - A data transfer handle that the MC
 * may use to retrieve a larger response payload.
 * @param[in] permission_flags - Indicates the access level granted to the
 * resource targeted by the Operation.
 * @param[in] response_payload_length - Length of the response payload.
 * @param[in] etag_format - Format of the etag string.
 * @param[in] etag - ETag.
 * @param[in] response_payload - The response payload if the payload fits.
 * @param[out] msg - Response message will be written to this.
 * @return pldm_completion_codes.
 */
int encode_rde_operation_status_resp(
    uint8_t instance_id, uint8_t completion_code, uint8_t operation_status,
    uint8_t completion_percentage, uint32_t completion_time_seconds,
    const union pldm_rde_op_execution_flags *operation_execution_flags,
    uint32_t result_transfer_handle,
    const union pldm_rde_permission_flags *permission_flags,
    uint32_t response_payload_length,
    enum pldm_rde_varstring_format etag_format, const char *etag,
    const uint8_t *response_payload, struct pldm_msg *msg);

/**
 * @brief Encode RDEOperationStatus response.
 *
 * @param[in] instance_id - Message's instance id.
 * @param[in] completion_code - PLDM completion code.
 * @param[in] operation_status - Status of the operation.
 * @param[in] completion_percentage - Percentage complete.
 * @param[in] completion_time_seconds - An estimate of the number of seconds
 * remaining before the Operation is completed.
 * @param[in] operation_execution_flags - Explains the result of operation.
 * @param[in] result_transfer_handle - A data transfer handle that the MC
 * may use to retrieve a larger response payload.
 * @param[in] permission_flags - Indicates the access level granted to the
 * resource targeted by the Operation.
 * @param[in] response_payload_length - Length of the response payload.
 * @param[in] etag_format - Format of the etag string.
 * @param[in] etag - ETag.
 * @param[in] response_payload - The response payload if the payload fits.
 * @param[out] msg - Response message will be written to this.
 * @return pldm_completion_codes.
 */
int decode_rde_operation_status_resp(
	const struct pldm_msg *msg, size_t payload_length, uint8_t *completion_code,
    uint8_t *completion_percentage, uint8_t *operation_status,
    uint32_t *completion_time_seconds, uint32_t *result_transfer_handle,
    uint32_t *response_payload_length,
    union pldm_rde_permission_flags **permission_flags,
    union pldm_rde_op_execution_flags **operation_execution_flags,
    struct pldm_rde_varstring **resp_etag, uint8_t **payload);

#ifdef __cplusplus
}
#endif

#endif /* PLDM_RDE_H */