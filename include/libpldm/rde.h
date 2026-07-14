/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#pragma once

#include <libpldm/api.h>
#include <libpldm/base.h>
#include <libpldm/pldm_types.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief PLDM for Redfish Device Completion Code */
enum pldm_rde_completion_codes {
	PLDM_RDE_CC_ERROR_BAD_CHECKSUM = 0x80,
	PLDM_RDE_CC_ERROR_CANNOT_CREATE_OPERATION = 0x81,
	PLDM_RDE_CC_ERROR_NOT_ALLOWED = 0x82,
	PLDM_RDE_CC_ERROR_WRONG_LOCATION_TYPE = 0x83,
	PLDM_RDE_CC_ERROR_OPERATION_ABANDONED = 0x84,
	PLDM_RDE_CC_ERROR_OPERATION_UNKILLABLE = 0x85,
	PLDM_RDE_CC_ERROR_OPERATION_EXISTS = 0x86,
	PLDM_RDE_CC_ERROR_OPERATION_FAILED = 0x87,
	PLDM_RDE_CC_ERROR_UNEXPECTED = 0x88,
	PLDM_RDE_CC_ERROR_UNSUPPORTED = 0x89,
	PLDM_RDE_CC_ERROR_UNRECOGNIZED_CUSTOM_HEADER = 0x90,
	PLDM_RDE_CC_ERROR_ETAG_MATCH = 0x91,
	PLDM_RDE_CC_ERROR_NO_SUCH_RESOURCE = 0x92,
	PLDM_RDE_CC_ETAG_CALCULATION_ONGOING = 0x93,
	PLDM_RDE_CC_ERROR_INSUFFICIENT_STORAGE = 0x94,
};

/** @brief PLDM for Redfish Device Enablement Command */
enum pldm_rde_commands {
	PLDM_RDE_CMD_NEGOTIATE_REDFISH_PARAMETERS = 0x01,
	PLDM_RDE_CMD_NEGOTIATE_MEDIUM_PARAMETERS = 0x02,
	PLDM_RDE_CMD_GET_SCHEMA_DICTIONARY = 0x03,
	PLDM_RDE_CMD_GET_SCHEMA_URI = 0x04,
	PLDM_RDE_CMD_GET_RESOURCE_ETAG = 0x05,
	PLDM_RDE_CMD_GET_OEM_COUNT = 0x06,
	PLDM_RDE_CMD_GET_OEM_NAME = 0x07,
	PLDM_RDE_CMD_GET_REGISTRY_COUNT = 0x08,
	PLDM_RDE_CMD_GET_REGISTRY_DETAILS = 0x09,
	PLDM_RDE_CMD_SELECT_REGISTRY_VERSION = 0x0a,
	PLDM_RDE_CMD_GET_MESSAGE_REGISTRY = 0x0b,
	PLDM_RDE_CMD_GET_SCHEMA_FILE = 0x0c,
	PLDM_RDE_CMD_RDE_OPERATION_INIT = 0x10,
	PLDM_RDE_CMD_SUPPLY_CUSTOM_REQUEST_PARAMETERS = 0x11,
	PLDM_RDE_CMD_RETRIEVE_CUSTOM_RESPONSE_PARAMETERS = 0x12,
	PLDM_RDE_CMD_RDE_OPERATION_COMPLETE = 0x13,
	PLDM_RDE_CMD_RDE_OPERATION_STATUS = 0x14,
	PLDM_RDE_CMD_RDE_OPERATION_KILL = 0x15,
	PLDM_RDE_CMD_RDE_OPERATION_ENUMERATE = 0x16,
	PLDM_RDE_CMD_RDE_MULTIPART_SEND = 0x30,
	PLDM_RDE_CMD_RDE_MULTIPART_RECEIVE = 0x31,
};

/* NegotiateRedfishParameters (0x01) */

#define PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_BYTES 3

/* completion_code(1) + device_concurrency_support(1) +
 * device_capabilities_flags(1) + device_feature_support(2) +
 * device_configuration_signature(4) + provider_name_format(1) +
 * provider_name_length(1)
 */
#define PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_BYTES 11

/** @struct pldm_rde_negotiate_redfish_parameters_req
 *
 *  Decoded NegotiateRedfishParameters request.
 */
struct pldm_rde_negotiate_redfish_parameters_req {
	uint8_t mc_concurrency_support;
	bitfield16_t mc_feature_support;
};

/** @struct pldm_rde_negotiate_redfish_parameters_resp
 *
 *  Decoded NegotiateRedfishParameters response. On decode provider_name is a
 *  span into the message buffer; the caller copies it if it must outlive the
 *  message.
 */
struct pldm_rde_negotiate_redfish_parameters_resp {
	uint8_t completion_code;
	uint8_t device_concurrency_support;
	bitfield8_t device_capabilities_flags;
	bitfield16_t device_feature_support;
	uint32_t device_configuration_signature;
	uint8_t provider_name_format;
	struct variable_field provider_name;
};

/** @brief Encode NegotiateRedfishParameters request.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  req            - Request to encode. mc_concurrency_support
 *                               must be non-zero.
 *  @param[out] msg            - Request message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               must be >=
 *                               PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_BYTES.
 *                               On exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_negotiate_redfish_parameters_req(
	uint8_t instance_id,
	const struct pldm_rde_negotiate_redfish_parameters_req *req,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode NegotiateRedfishParameters request.
 *
 *  @param[in]  msg            - Request message.
 *  @param[in]  payload_length - Length of request payload.
 *  @param[out] req            - Decoded request. mc_concurrency_support is
 *                               validated to be non-zero.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_negotiate_redfish_parameters_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_negotiate_redfish_parameters_req *req);

/** @brief Encode NegotiateRedfishParameters response.
 *
 *  On a non-SUCCESS completion_code only the completion code is emitted.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  resp           - Response to encode. On success
 *                               device_concurrency_support must be non-zero
 *                               and provider_name.length must fit in a uint8.
 *  @param[out] msg            - Response message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               on exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_negotiate_redfish_parameters_resp(
	uint8_t instance_id,
	const struct pldm_rde_negotiate_redfish_parameters_resp *resp,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode NegotiateRedfishParameters response.
 *
 *  On a non-SUCCESS completion code only resp->completion_code is populated.
 *  On success provider_name is a span into @p msg's buffer.
 *
 *  @param[in]  msg            - Response message.
 *  @param[in]  payload_length - Length of response payload.
 *  @param[out] resp           - Decoded response. Output member values are
 *                               host-endian.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_negotiate_redfish_parameters_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_negotiate_redfish_parameters_resp *resp);

/* NegotiateMediumParameters (0x02) */

/* All MC and RDE Device implementations shall support a transfer size of at
 * least 64 bytes per DSP0218 Table 53.
 */
#define PLDM_RDE_MIN_TRANSFER_SIZE_BYTES 64

#define PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_REQ_BYTES 4

/* completion_code(1) + device_maximum_transfer_chunk_size_bytes(4) */
#define PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_RESP_BYTES 5

/** @struct pldm_rde_negotiate_medium_parameters_req
 *
 *  Decoded NegotiateMediumParameters request.
 */
struct pldm_rde_negotiate_medium_parameters_req {
	uint32_t mc_maximum_transfer_chunk_size_bytes;
};

/** @struct pldm_rde_negotiate_medium_parameters_resp
 *
 *  Decoded NegotiateMediumParameters response.
 */
struct pldm_rde_negotiate_medium_parameters_resp {
	uint8_t completion_code;
	uint32_t device_maximum_transfer_chunk_size_bytes;
};

/** @brief Encode NegotiateMediumParameters request.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  req            - Request to encode.
 *                               mc_maximum_transfer_chunk_size_bytes must be
 *                               at least PLDM_RDE_MIN_TRANSFER_SIZE_BYTES.
 *  @param[out] msg            - Request message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               must be >=
 *                               PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_REQ_BYTES.
 *                               On exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_negotiate_medium_parameters_req(
	uint8_t instance_id,
	const struct pldm_rde_negotiate_medium_parameters_req *req,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode NegotiateMediumParameters request.
 *
 *  @param[in]  msg            - Request message.
 *  @param[in]  payload_length - Length of request payload.
 *  @param[out] req            - Decoded request.
 *                               mc_maximum_transfer_chunk_size_bytes is
 *                               validated to be at least
 *                               PLDM_RDE_MIN_TRANSFER_SIZE_BYTES.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_negotiate_medium_parameters_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_negotiate_medium_parameters_req *req);

/** @brief Encode NegotiateMediumParameters response.
 *
 *  On a non-SUCCESS completion_code only the completion code is emitted.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  resp           - Response to encode. On success
 *                               device_maximum_transfer_chunk_size_bytes must
 *                               be at least PLDM_RDE_MIN_TRANSFER_SIZE_BYTES.
 *  @param[out] msg            - Response message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               on exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_negotiate_medium_parameters_resp(
	uint8_t instance_id,
	const struct pldm_rde_negotiate_medium_parameters_resp *resp,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode NegotiateMediumParameters response.
 *
 *  On a non-SUCCESS completion code only resp->completion_code is populated.
 *
 *  @param[in]  msg            - Response message.
 *  @param[in]  payload_length - Length of response payload.
 *  @param[out] resp           - Decoded response. Output member values are
 *                               host-endian.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_negotiate_medium_parameters_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_negotiate_medium_parameters_resp *resp);

/* GetSchemaDictionary (0x03) */

/** @brief schemaClass enumeration per DSP0218 Table 3. */
enum pldm_rde_schema_type {
	PLDM_RDE_SCHEMA_MAJOR = 0,
	PLDM_RDE_SCHEMA_EVENT = 1,
	PLDM_RDE_SCHEMA_ANNOTATION = 2,
	PLDM_RDE_SCHEMA_COLLECTION_MEMBER_TYPE = 3,
	PLDM_RDE_SCHEMA_ERROR = 4,
	PLDM_RDE_SCHEMA_REGISTRY = 5,
	PLDM_RDE_SCHEMA_MAX,
};

/* resource_id(4) + requested_schema_class(1) */
#define PLDM_RDE_GET_SCHEMA_DICTIONARY_REQ_BYTES 5

/* completion_code(1) + dictionary_format(1) + transfer_handle(4) */
#define PLDM_RDE_GET_SCHEMA_DICTIONARY_RESP_BYTES 6

/** @struct pldm_rde_get_schema_dictionary_req
 *
 *  Decoded GetSchemaDictionary request.
 */
struct pldm_rde_get_schema_dictionary_req {
	uint32_t resource_id;
	uint8_t requested_schema_class;
};

/** @struct pldm_rde_get_schema_dictionary_resp
 *
 *  Decoded GetSchemaDictionary response.
 */
struct pldm_rde_get_schema_dictionary_resp {
	uint8_t completion_code;
	uint8_t dictionary_format;
	uint32_t transfer_handle;
};

/** @brief Encode GetSchemaDictionary request.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  req            - Request to encode. requested_schema_class must
 *                               be less than PLDM_RDE_SCHEMA_MAX.
 *  @param[out] msg            - Request message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               must be >=
 *                               PLDM_RDE_GET_SCHEMA_DICTIONARY_REQ_BYTES.
 *                               On exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_get_schema_dictionary_req(
	uint8_t instance_id,
	const struct pldm_rde_get_schema_dictionary_req *req,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode GetSchemaDictionary request.
 *
 *  @param[in]  msg            - Request message.
 *  @param[in]  payload_length - Length of request payload.
 *  @param[out] req            - Decoded request. requested_schema_class is
 *                               validated to be less than PLDM_RDE_SCHEMA_MAX.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_get_schema_dictionary_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_get_schema_dictionary_req *req);

/** @brief Encode GetSchemaDictionary response.
 *
 *  On a non-SUCCESS completion_code only the completion code is emitted.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  resp           - Response to encode.
 *  @param[out] msg            - Response message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               on exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_get_schema_dictionary_resp(
	uint8_t instance_id,
	const struct pldm_rde_get_schema_dictionary_resp *resp,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode GetSchemaDictionary response.
 *
 *  On a non-SUCCESS completion code only resp->completion_code is populated.
 *
 *  @param[in]  msg            - Response message.
 *  @param[in]  payload_length - Length of response payload.
 *  @param[out] resp           - Decoded response. Output member values are
 *                               host-endian.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_get_schema_dictionary_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_get_schema_dictionary_resp *resp);

#ifdef __cplusplus
}
#endif
