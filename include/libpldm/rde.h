/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#pragma once

#include "compiler.h"
#include <libpldm/api.h>
#include <libpldm/base.h>
#include <libpldm/pldm_types.h>

#include <stdbool.h>
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

/* GetSchemaURI (0x04) */

/** @brief stringFormat enumeration per DSP0218 Table 2. */
enum pldm_rde_varstring_format {
	PLDM_RDE_VARSTRING_UNKNOWN = 0,
	PLDM_RDE_VARSTRING_ASCII = 1,
	PLDM_RDE_VARSTRING_UTF_8 = 2,
	PLDM_RDE_VARSTRING_UTF_16 = 3,
	PLDM_RDE_VARSTRING_UTF_16LE = 4,
	PLDM_RDE_VARSTRING_UTF_16BE = 5,
};

/* string_format(1) + string_length_bytes(1) */
#define PLDM_RDE_VARSTRING_HEADER_BYTES 2

/** @struct pldm_rde_varstring
 *
 *  A varstring (DSP0218 Table 2). string_data.length includes the NUL
 *  terminator, matching the on-wire stringLengthBytes, and must fit in a
 *  uint8. On decode string_data spans the message buffer; the caller copies it
 *  if it must outlive the message. On encode string_data points at the
 *  caller's bytes.
 */
struct pldm_rde_varstring {
	uint8_t string_format;
	struct variable_field string_data;
};

/** @struct pldm_rde_varstring_iter
 *
 *  Iterator over a run of varstrings trailing a decoded message. Initialised
 *  by the owning decode_*() function; walked with foreach_pldm_rde_varstring().
 */
struct pldm_rde_varstring_iter {
	struct variable_field field;
	size_t count;
};

/** @brief Test whether a varstring iterator is exhausted. */
LIBPLDM_ITERATOR
bool pldm_rde_varstring_iter_end(const struct pldm_rde_varstring_iter *iter)
{
	return !iter->count;
}

/** @brief Advance a varstring iterator by one entry. */
LIBPLDM_ITERATOR
bool pldm_rde_varstring_iter_next(struct pldm_rde_varstring_iter *iter)
{
	if (!iter->count) {
		return false;
	}
	iter->count--;
	return true;
}

/** @brief Decode the varstring at the iterator's cursor.
 *
 *  @param[in,out] iter      - Iterator; its span is advanced past the entry.
 *  @param[out]    varstring - Decoded entry, spanning the message buffer.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_varstring_from_iter(struct pldm_rde_varstring_iter *iter,
					struct pldm_rde_varstring *varstring);

/** @brief Iterate the varstrings yielded by a decode_*() call.
 *
 *  @param iter      - The struct pldm_rde_varstring_iter lvalue set by decode.
 *  @param varstring - The struct pldm_rde_varstring lvalue for each entry.
 *  @param rc        - An int lvalue set non-zero if an entry fails to decode.
 */
#define foreach_pldm_rde_varstring(iter, varstring, rc)                        \
	for ((rc) = 0; !pldm_rde_varstring_iter_end(&(iter)) &&                \
		       !((rc) = decode_pldm_rde_varstring_from_iter(           \
				 &(iter), &(varstring)));                      \
	     pldm_rde_varstring_iter_next(&(iter)))

/* resource_id(4) + requested_schema_class(1) + oem_extension_number(1) */
#define PLDM_RDE_GET_SCHEMA_URI_REQ_BYTES 6

/* completion_code(1) + string_fragment_count(1) */
#define PLDM_RDE_GET_SCHEMA_URI_RESP_FIXED_BYTES 2

/** @struct pldm_rde_get_schema_uri_req
 *
 *  Decoded GetSchemaURI request.
 */
struct pldm_rde_get_schema_uri_req {
	uint32_t resource_id;
	uint8_t requested_schema_class;
	uint8_t oem_extension_number;
};

/** @struct pldm_rde_get_schema_uri_resp
 *
 *  Decoded GetSchemaURI response fixed fields. The URI fragments follow via a
 *  separate struct pldm_rde_varstring_iter set by the decode function.
 */
struct pldm_rde_get_schema_uri_resp {
	uint8_t completion_code;
	uint8_t string_fragment_count;
};

/** @brief Encode GetSchemaURI request.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  req            - Request to encode. requested_schema_class must
 *                               be less than PLDM_RDE_SCHEMA_MAX.
 *  @param[out] msg            - Request message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               must be >= PLDM_RDE_GET_SCHEMA_URI_REQ_BYTES.
 *                               On exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_get_schema_uri_req(
	uint8_t instance_id, const struct pldm_rde_get_schema_uri_req *req,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode GetSchemaURI request.
 *
 *  @param[in]  msg            - Request message.
 *  @param[in]  payload_length - Length of request payload.
 *  @param[out] req            - Decoded request. requested_schema_class is
 *                               validated to be less than PLDM_RDE_SCHEMA_MAX.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_get_schema_uri_req(const struct pldm_msg *msg,
				       size_t payload_length,
				       struct pldm_rde_get_schema_uri_req *req);

/** @brief Encode GetSchemaURI response.
 *
 *  On a non-SUCCESS completion_code only the completion code is emitted.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  resp           - Response fixed fields. On success
 *                               string_fragment_count must be non-zero.
 *  @param[in]  uris           - Array of resp->string_fragment_count
 *                               varstrings.
 *  @param[out] msg            - Response message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               on exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_get_schema_uri_resp(
	uint8_t instance_id, const struct pldm_rde_get_schema_uri_resp *resp,
	const struct pldm_rde_varstring *uris, struct pldm_msg *msg,
	size_t *payload_length);

/** @brief Decode GetSchemaURI response.
 *
 *  On a non-SUCCESS completion code only resp->completion_code is populated and
 *  the iterator is left empty.
 *
 *  @param[in]  msg            - Response message.
 *  @param[in]  payload_length - Length of response payload.
 *  @param[out] resp           - Decoded fixed fields.
 *  @param[out] uris           - Iterator over the URI fragments; walk it with
 *                               foreach_pldm_rde_varstring().
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_get_schema_uri_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_get_schema_uri_resp *resp,
	struct pldm_rde_varstring_iter *uris);

/* GetResourceETag (0x05) */

/* resource_id(4) */
#define PLDM_RDE_GET_RESOURCE_ETAG_REQ_BYTES 4

/* completion_code(1) + etag string_format(1) + etag string_length_bytes(1);
 * the etag data follows. */
#define PLDM_RDE_GET_RESOURCE_ETAG_RESP_MIN_BYTES 3

/** @struct pldm_rde_get_resource_etag_req
 *
 *  Decoded GetResourceETag request.
 */
struct pldm_rde_get_resource_etag_req {
	uint32_t resource_id;
};

/** @struct pldm_rde_get_resource_etag_resp
 *
 *  Decoded GetResourceETag response. On decode etag spans the message buffer.
 */
struct pldm_rde_get_resource_etag_resp {
	uint8_t completion_code;
	struct pldm_rde_varstring etag;
};

/** @brief Encode GetResourceETag request.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  req            - Request to encode.
 *  @param[out] msg            - Request message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               must be >=
 *                               PLDM_RDE_GET_RESOURCE_ETAG_REQ_BYTES.
 *                               On exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_get_resource_etag_req(
	uint8_t instance_id, const struct pldm_rde_get_resource_etag_req *req,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode GetResourceETag request.
 *
 *  @param[in]  msg            - Request message.
 *  @param[in]  payload_length - Length of request payload.
 *  @param[out] req            - Decoded request.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_get_resource_etag_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_get_resource_etag_req *req);

/** @brief Encode GetResourceETag response.
 *
 *  On a non-SUCCESS completion_code only the completion code is emitted.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  resp           - Response to encode. On success etag.string_data
 *                               must be non-empty (it includes the NUL
 *                               terminator) and fit in a uint8 length.
 *  @param[out] msg            - Response message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               on exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_get_resource_etag_resp(
	uint8_t instance_id, const struct pldm_rde_get_resource_etag_resp *resp,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode GetResourceETag response.
 *
 *  On a non-SUCCESS completion code only resp->completion_code is populated.
 *
 *  @param[in]  msg            - Response message.
 *  @param[in]  payload_length - Length of response payload.
 *  @param[out] resp           - Decoded response. etag spans @p msg's buffer.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_get_resource_etag_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_get_resource_etag_resp *resp);

/* RDEOperationInit (0x10) */

/** @brief OperationType enumeration per DSP0218 Table 64. */
enum pldm_rde_operation_type {
	PLDM_RDE_OPERATION_TYPE_HEAD = 0,
	PLDM_RDE_OPERATION_TYPE_READ = 1,
	PLDM_RDE_OPERATION_TYPE_CREATE = 2,
	PLDM_RDE_OPERATION_TYPE_DELETE = 3,
	PLDM_RDE_OPERATION_TYPE_UPDATE = 4,
	PLDM_RDE_OPERATION_TYPE_REPLACE = 5,
	PLDM_RDE_OPERATION_TYPE_ACTION = 6,
	PLDM_RDE_OPERATION_TYPE_MAX,
};

/** @brief OperationStatus enumeration per DSP0218 Table 64. */
enum pldm_rde_operation_status {
	PLDM_RDE_OPERATION_STATUS_INACTIVE = 0,
	PLDM_RDE_OPERATION_STATUS_NEEDS_INPUT = 1,
	PLDM_RDE_OPERATION_STATUS_TRIGGERED = 2,
	PLDM_RDE_OPERATION_STATUS_RUNNING = 3,
	PLDM_RDE_OPERATION_STATUS_HAVE_RESULTS = 4,
	PLDM_RDE_OPERATION_STATUS_COMPLETED = 5,
	PLDM_RDE_OPERATION_STATUS_FAILED = 6,
	PLDM_RDE_OPERATION_STATUS_ABANDONED = 7,
	PLDM_RDE_OPERATION_STATUS_MAX,
};

/* resource_id(4) + operation_id(2) + operation_type(1) + operation_flags(1) +
 * send_data_transfer_handle(4) + operation_locator_length(1) +
 * request_payload_length(4)
 */
#define PLDM_RDE_OPERATION_INIT_REQ_FIXED_BYTES 17

/* completion_code(1) + operation_status(1) + completion_percentage(1) +
 * completion_time_seconds(4) + operation_execution_flags(1) +
 * result_transfer_handle(4) + permission_flags(1) + response_payload_length(4)
 */
#define PLDM_RDE_OPERATION_INIT_RESP_FIXED_BYTES 17

/** @struct pldm_rde_rde_operation_init_req
 *
 *  Decoded RDEOperationInit request. operation_locator and request_payload are
 *  spans into the message buffer on decode and point at the caller's bytes on
 *  encode; their lengths carry the on-wire OperationLocatorLength (uint8) and
 *  RequestPayloadLength (uint32) fields.
 */
struct pldm_rde_rde_operation_init_req {
	uint32_t resource_id;
	uint16_t operation_id;
	uint8_t operation_type;
	bitfield8_t operation_flags;
	uint32_t send_data_transfer_handle;
	struct variable_field operation_locator;
	struct variable_field request_payload;
};

/** @struct pldm_rde_rde_operation_init_resp
 *
 *  Decoded RDEOperationInit response. etag spans the message buffer on decode
 *  and points at the caller's bytes on encode; response_payload likewise, with
 *  its length carrying the on-wire ResponsePayloadLength (uint32) field. This
 *  response is byte-identical to the RDEOperationStatus response per DSP0218
 *  Table 68.
 */
struct pldm_rde_rde_operation_init_resp {
	uint8_t completion_code;
	uint8_t operation_status;
	uint8_t completion_percentage;
	uint32_t completion_time_seconds;
	bitfield8_t operation_execution_flags;
	uint32_t result_transfer_handle;
	bitfield8_t permission_flags;
	struct pldm_rde_varstring etag;
	struct variable_field response_payload;
};

/** @brief Encode RDEOperationInit request.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  req            - Request to encode. operation_type must be less
 *                               than PLDM_RDE_OPERATION_TYPE_MAX,
 *                               operation_locator.length must fit in a uint8,
 *                               and request_payload.length in a uint32.
 *  @param[out] msg            - Request message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               on exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_rde_operation_init_req(
	uint8_t instance_id, const struct pldm_rde_rde_operation_init_req *req,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode RDEOperationInit request.
 *
 *  @param[in]  msg            - Request message.
 *  @param[in]  payload_length - Length of request payload.
 *  @param[out] req            - Decoded request. operation_locator and
 *                               request_payload span @p msg's buffer.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_rde_operation_init_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_rde_operation_init_req *req);

/** @brief Encode RDEOperationInit response.
 *
 *  On a non-SUCCESS completion_code only the completion code is emitted.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  resp           - Response to encode. On success etag.string_data
 *                               must be non-empty (it includes the NUL
 *                               terminator) and fit in a uint8 length, and
 *                               response_payload.length must fit in a uint32.
 *  @param[out] msg            - Response message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               on exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_rde_operation_init_resp(
	uint8_t instance_id,
	const struct pldm_rde_rde_operation_init_resp *resp,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode RDEOperationInit response.
 *
 *  On a non-SUCCESS completion code only resp->completion_code is populated.
 *  On success etag and response_payload span @p msg's buffer.
 *
 *  @param[in]  msg            - Response message.
 *  @param[in]  payload_length - Length of response payload.
 *  @param[out] resp           - Decoded response. Output member values are
 *                               host-endian.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_rde_operation_init_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_rde_operation_init_resp *resp);

/* RDEOperationComplete (0x13) */

/* resource_id(4) + operation_id(2) */
#define PLDM_RDE_OPERATION_COMPLETE_REQ_BYTES 6

/* completion_code(1) */
#define PLDM_RDE_OPERATION_COMPLETE_RESP_BYTES 1

/** @struct pldm_rde_rde_operation_complete_req
 *
 *  Decoded RDEOperationComplete request.
 */
struct pldm_rde_rde_operation_complete_req {
	uint32_t resource_id;
	uint16_t operation_id;
};

/** @struct pldm_rde_rde_operation_complete_resp
 *
 *  Decoded RDEOperationComplete response, which carries only a completion
 *  code.
 */
struct pldm_rde_rde_operation_complete_resp {
	uint8_t completion_code;
};

/** @brief Encode RDEOperationComplete request.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  req            - Request to encode.
 *  @param[out] msg            - Request message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               must be >=
 *                               PLDM_RDE_OPERATION_COMPLETE_REQ_BYTES.
 *                               On exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_rde_operation_complete_req(
	uint8_t instance_id,
	const struct pldm_rde_rde_operation_complete_req *req,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode RDEOperationComplete request.
 *
 *  @param[in]  msg            - Request message.
 *  @param[in]  payload_length - Length of request payload.
 *  @param[out] req            - Decoded request.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_rde_operation_complete_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_rde_operation_complete_req *req);

/** @brief Encode RDEOperationComplete response.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  resp           - Response to encode.
 *  @param[out] msg            - Response message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               on exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_rde_operation_complete_resp(
	uint8_t instance_id,
	const struct pldm_rde_rde_operation_complete_resp *resp,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode RDEOperationComplete response.
 *
 *  @param[in]  msg            - Response message.
 *  @param[in]  payload_length - Length of response payload.
 *  @param[out] resp           - Decoded response.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_rde_operation_complete_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_rde_operation_complete_resp *resp);

/* RDEOperationStatus (0x14) */

/* resource_id(4) + operation_id(2) */
#define PLDM_RDE_OPERATION_STATUS_REQ_BYTES 6

/* The response is byte-identical to the RDEOperationInit response per DSP0218
 * Table 68. */
#define PLDM_RDE_OPERATION_STATUS_RESP_FIXED_BYTES                             \
	PLDM_RDE_OPERATION_INIT_RESP_FIXED_BYTES

/** @struct pldm_rde_rde_operation_status_req
 *
 *  Decoded RDEOperationStatus request.
 */
struct pldm_rde_rde_operation_status_req {
	uint32_t resource_id;
	uint16_t operation_id;
};

/** @struct pldm_rde_rde_operation_status_resp
 *
 *  Decoded RDEOperationStatus response. Its fields and wire layout are
 *  identical to struct pldm_rde_rde_operation_init_resp per DSP0218 Table 68.
 *  etag and response_payload span the message buffer on decode.
 */
struct pldm_rde_rde_operation_status_resp {
	uint8_t completion_code;
	uint8_t operation_status;
	uint8_t completion_percentage;
	uint32_t completion_time_seconds;
	bitfield8_t operation_execution_flags;
	uint32_t result_transfer_handle;
	bitfield8_t permission_flags;
	struct pldm_rde_varstring etag;
	struct variable_field response_payload;
};

/** @brief Encode RDEOperationStatus request.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  req            - Request to encode.
 *  @param[out] msg            - Request message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               must be >= PLDM_RDE_OPERATION_STATUS_REQ_BYTES.
 *                               On exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_rde_operation_status_req(
	uint8_t instance_id,
	const struct pldm_rde_rde_operation_status_req *req,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode RDEOperationStatus request.
 *
 *  @param[in]  msg            - Request message.
 *  @param[in]  payload_length - Length of request payload.
 *  @param[out] req            - Decoded request.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_rde_operation_status_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_rde_operation_status_req *req);

/** @brief Encode RDEOperationStatus response.
 *
 *  On a non-SUCCESS completion_code only the completion code is emitted.
 *
 *  @param[in]  instance_id    - Message's instance id.
 *  @param[in]  resp           - Response to encode. On success etag.string_data
 *                               must be non-empty (it includes the NUL
 *                               terminator) and fit in a uint8 length, and
 *                               response_payload.length must fit in a uint32.
 *  @param[out] msg            - Response message.
 *  @param[in,out] payload_length - On entry the caller-allocated buffer size;
 *                               on exit the encoded message length.
 *  @return 0 on success, a negative errno value on failure.
 */
int encode_pldm_rde_rde_operation_status_resp(
	uint8_t instance_id,
	const struct pldm_rde_rde_operation_status_resp *resp,
	struct pldm_msg *msg, size_t *payload_length);

/** @brief Decode RDEOperationStatus response.
 *
 *  On a non-SUCCESS completion code only resp->completion_code is populated.
 *  On success etag and response_payload span @p msg's buffer.
 *
 *  @param[in]  msg            - Response message.
 *  @param[in]  payload_length - Length of response payload.
 *  @param[out] resp           - Decoded response. Output member values are
 *                               host-endian.
 *  @return 0 on success, a negative errno value on failure.
 */
int decode_pldm_rde_rde_operation_status_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_rde_operation_status_resp *resp);

#ifdef __cplusplus
}
#endif
