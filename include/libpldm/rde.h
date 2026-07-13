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
 *  A varstring (DSP0218 Table 2). string_data.length includes the NULL
 *  terminator, matching the on-wire stringLengthBytes, and must fit in a
 *  uint8. On decode string_data spans the message buffer; the caller copies it
 *  if it must outlive the message. On encode string_data points at the
 *  caller's bytes.
 */
struct pldm_rde_varstring {
	uint8_t string_format;
	struct variable_field string_data;
};

/* NegotiateRedfishParameters (0x01) */

#define PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_BYTES 3

/* completion_code(1) + device_concurrency_support(1) +
 * device_capabilities_flags(1) + device_feature_support(2) +
 * device_configuration_signature(4) + a provider_name varstring holding at
 * least the mandatory NULL terminator
 * (PLDM_RDE_VARSTRING_HEADER_BYTES + string_data NULL(1))
 */
#define PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_BYTES 12

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
 *  Decoded NegotiateRedfishParameters response. On decode provider_name spans
 *  the message buffer; the caller copies it if it must outlive the message.
 */
struct pldm_rde_negotiate_redfish_parameters_resp {
	uint8_t completion_code;
	uint8_t device_concurrency_support;
	bitfield8_t device_capabilities_flags;
	bitfield16_t device_feature_support;
	uint32_t device_configuration_signature;
	struct pldm_rde_varstring provider_name;
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
 *                               device_concurrency_support must be non-zero and
 *                               provider_name.string_data must be a non-empty
 *                               NULL-terminated span whose length fits a uint8.
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

#ifdef __cplusplus
}
#endif
