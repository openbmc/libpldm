/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include "api.h"
#include "compiler.h"
#include "dsp/base.h"
#include "msgbuf.h"
#include <libpldm/base.h>
#include <libpldm/pldm_types.h>
#include <libpldm/rde.h>

#include "environ/errno.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/* Encode a response carrying only the completion code, as every RDE response
 * does for a non-SUCCESS completion code. The caller emits the PLDM header
 * first. */
static int encode_rde_cc_only_resp(struct pldm_msg *msg,
				   uint8_t completion_code,
				   size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	rc = pldm_msgbuf_init_errno(buf, sizeof(completion_code), msg->payload,
				    *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, completion_code);
	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_negotiate_redfish_parameters_req(
	uint8_t instance_id,
	const struct pldm_rde_negotiate_redfish_parameters_req *req,
	struct pldm_msg *msg, size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL || payload_length == NULL) {
		return -EINVAL;
	}
	if (req->mc_concurrency_support == 0) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(
		PLDM_REQUEST, instance_id, PLDM_RDE,
		PLDM_RDE_CMD_NEGOTIATE_REDFISH_PARAMETERS, msg);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(
		buf, PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_BYTES,
		msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, req->mc_concurrency_support);
	pldm_msgbuf_insert(buf, req->mc_feature_support.value);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_negotiate_redfish_parameters_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_negotiate_redfish_parameters_req *req)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(
		buf, PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_BYTES,
		msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, req->mc_concurrency_support);
	pldm_msgbuf_extract(buf, req->mc_feature_support.value);

	rc = pldm_msgbuf_complete_consumed(buf);
	if (rc) {
		return rc;
	}

	/* A zero concurrency value is invalid per DSP0218 Table 52. */
	if (req->mc_concurrency_support == 0) {
		return -EBADMSG;
	}
	return 0;
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_negotiate_redfish_parameters_resp(
	uint8_t instance_id,
	const struct pldm_rde_negotiate_redfish_parameters_resp *resp,
	struct pldm_msg *msg, size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || resp == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(
		PLDM_RESPONSE, instance_id, PLDM_RDE,
		PLDM_RDE_CMD_NEGOTIATE_REDFISH_PARAMETERS, msg);
	if (rc) {
		return rc;
	}

	/* An error response carries only the completion code. */
	if (resp->completion_code != PLDM_SUCCESS) {
		return encode_rde_cc_only_resp(msg, resp->completion_code,
					       payload_length);
	}

	if (resp->device_concurrency_support == 0) {
		return -EINVAL;
	}
	/* The provider name is NULL-terminated per DSP0218 Table 2, so its
	 * length includes at least the terminator and must fit the uint8 wire
	 * field. */
	if (resp->provider_name.string_data.length == 0 ||
	    resp->provider_name.string_data.length > UINT8_MAX ||
	    resp->provider_name.string_data.ptr == NULL) {
		return -EINVAL;
	}

	/* RESP_MIN_BYTES already budgets the mandatory NULL terminator, which
	 * string_data.length also counts, so add only the bytes beyond it. */
	rc = pldm_msgbuf_init_errno(
		buf,
		(size_t)PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_BYTES -
			1 + resp->provider_name.string_data.length,
		msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, resp->completion_code);
	pldm_msgbuf_insert(buf, resp->device_concurrency_support);
	pldm_msgbuf_insert(buf, resp->device_capabilities_flags.byte);
	pldm_msgbuf_insert(buf, resp->device_feature_support.value);
	pldm_msgbuf_insert(buf, resp->device_configuration_signature);
	pldm_msgbuf_insert(buf, resp->provider_name.string_format);
	pldm_msgbuf_insert_uint8(
		buf, (uint8_t)resp->provider_name.string_data.length);
	rc = pldm_msgbuf_insert_array_uint8(
		buf, resp->provider_name.string_data.length,
		resp->provider_name.string_data.ptr,
		resp->provider_name.string_data.length);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_negotiate_redfish_parameters_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_negotiate_redfish_parameters_resp *resp)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	size_t name_length = 0;
	int rc;

	if (msg == NULL || resp == NULL) {
		return -EINVAL;
	}

	rc = pldm_msg_has_error(msg, payload_length);
	if (rc) {
		resp->completion_code = rc;
		return 0;
	}

	rc = pldm_msgbuf_init_errno(
		buf, PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_BYTES,
		msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, resp->completion_code);
	pldm_msgbuf_extract(buf, resp->device_concurrency_support);
	pldm_msgbuf_extract(buf, resp->device_capabilities_flags.byte);
	pldm_msgbuf_extract(buf, resp->device_feature_support.value);
	pldm_msgbuf_extract(buf, resp->device_configuration_signature);
	pldm_msgbuf_extract(buf, resp->provider_name.string_format);
	rc = pldm_msgbuf_extract_uint8_to_size(buf, name_length);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	resp->provider_name.string_data.ptr = NULL;
	rc = pldm_msgbuf_span_required(
		buf, name_length,
		(const void **)&resp->provider_name.string_data.ptr);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}
	resp->provider_name.string_data.length = name_length;

	rc = pldm_msgbuf_complete_consumed(buf);
	if (rc) {
		return rc;
	}

	/* DeviceConcurrencySupport must be non-zero per DSP0218 Table 52. */
	if (resp->device_concurrency_support == 0) {
		return -EBADMSG;
	}
	return 0;
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_negotiate_medium_parameters_req(
	uint8_t instance_id,
	const struct pldm_rde_negotiate_medium_parameters_req *req,
	struct pldm_msg *msg, size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL || payload_length == NULL) {
		return -EINVAL;
	}
	if (req->mc_maximum_transfer_chunk_size_bytes <
	    PLDM_RDE_MIN_TRANSFER_SIZE_BYTES) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(
		PLDM_REQUEST, instance_id, PLDM_RDE,
		PLDM_RDE_CMD_NEGOTIATE_MEDIUM_PARAMETERS, msg);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(
		buf, PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_REQ_BYTES,
		msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, req->mc_maximum_transfer_chunk_size_bytes);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_negotiate_medium_parameters_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_negotiate_medium_parameters_req *req)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(
		buf, PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_REQ_BYTES,
		msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, req->mc_maximum_transfer_chunk_size_bytes);

	rc = pldm_msgbuf_complete_consumed(buf);
	if (rc) {
		return rc;
	}

	/* The MC must support a transfer size of at least
	 * PLDM_RDE_MIN_TRANSFER_SIZE_BYTES per DSP0218 Table 53. */
	if (req->mc_maximum_transfer_chunk_size_bytes <
	    PLDM_RDE_MIN_TRANSFER_SIZE_BYTES) {
		return -EBADMSG;
	}
	return 0;
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_negotiate_medium_parameters_resp(
	uint8_t instance_id,
	const struct pldm_rde_negotiate_medium_parameters_resp *resp,
	struct pldm_msg *msg, size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || resp == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(
		PLDM_RESPONSE, instance_id, PLDM_RDE,
		PLDM_RDE_CMD_NEGOTIATE_MEDIUM_PARAMETERS, msg);
	if (rc) {
		return rc;
	}

	/* An error response carries only the completion code. */
	if (resp->completion_code != PLDM_SUCCESS) {
		return encode_rde_cc_only_resp(msg, resp->completion_code,
					       payload_length);
	}

	if (resp->device_maximum_transfer_chunk_size_bytes <
	    PLDM_RDE_MIN_TRANSFER_SIZE_BYTES) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(
		buf, PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_RESP_BYTES,
		msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, resp->completion_code);
	pldm_msgbuf_insert(buf, resp->device_maximum_transfer_chunk_size_bytes);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_negotiate_medium_parameters_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_negotiate_medium_parameters_resp *resp)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	int rc;

	if (msg == NULL || resp == NULL) {
		return -EINVAL;
	}

	rc = pldm_msg_has_error(msg, payload_length);
	if (rc) {
		resp->completion_code = rc;
		return 0;
	}

	rc = pldm_msgbuf_init_errno(
		buf, PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_RESP_BYTES,
		msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, resp->completion_code);
	pldm_msgbuf_extract(buf,
			    resp->device_maximum_transfer_chunk_size_bytes);

	rc = pldm_msgbuf_complete_consumed(buf);
	if (rc) {
		return rc;
	}

	/* The RDE Device must support a transfer size of at least
	 * PLDM_RDE_MIN_TRANSFER_SIZE_BYTES per DSP0218 Table 53. */
	if (resp->device_maximum_transfer_chunk_size_bytes <
	    PLDM_RDE_MIN_TRANSFER_SIZE_BYTES) {
		return -EBADMSG;
	}
	return 0;
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_get_schema_dictionary_req(
	uint8_t instance_id,
	const struct pldm_rde_get_schema_dictionary_req *req,
	struct pldm_msg *msg, size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL || payload_length == NULL) {
		return -EINVAL;
	}
	if (req->requested_schema_class >= PLDM_RDE_SCHEMA_MAX) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(PLDM_REQUEST, instance_id, PLDM_RDE,
					   PLDM_RDE_CMD_GET_SCHEMA_DICTIONARY,
					   msg);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(buf,
				    PLDM_RDE_GET_SCHEMA_DICTIONARY_REQ_BYTES,
				    msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, req->resource_id);
	pldm_msgbuf_insert(buf, req->requested_schema_class);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_get_schema_dictionary_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_get_schema_dictionary_req *req)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf,
				    PLDM_RDE_GET_SCHEMA_DICTIONARY_REQ_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, req->resource_id);
	pldm_msgbuf_extract(buf, req->requested_schema_class);

	rc = pldm_msgbuf_complete_consumed(buf);
	if (rc) {
		return rc;
	}

	/* RequestedSchemaClass must be a defined schemaClass per DSP0218
	 * Table 3. */
	if (req->requested_schema_class >= PLDM_RDE_SCHEMA_MAX) {
		return -EBADMSG;
	}
	return 0;
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_get_schema_dictionary_resp(
	uint8_t instance_id,
	const struct pldm_rde_get_schema_dictionary_resp *resp,
	struct pldm_msg *msg, size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || resp == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(PLDM_RESPONSE, instance_id, PLDM_RDE,
					   PLDM_RDE_CMD_GET_SCHEMA_DICTIONARY,
					   msg);
	if (rc) {
		return rc;
	}

	/* An error response carries only the completion code. */
	if (resp->completion_code != PLDM_SUCCESS) {
		return encode_rde_cc_only_resp(msg, resp->completion_code,
					       payload_length);
	}

	rc = pldm_msgbuf_init_errno(buf,
				    PLDM_RDE_GET_SCHEMA_DICTIONARY_RESP_BYTES,
				    msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, resp->completion_code);
	pldm_msgbuf_insert(buf, resp->dictionary_format);
	pldm_msgbuf_insert(buf, resp->transfer_handle);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_get_schema_dictionary_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_get_schema_dictionary_resp *resp)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	int rc;

	if (msg == NULL || resp == NULL) {
		return -EINVAL;
	}

	rc = pldm_msg_has_error(msg, payload_length);
	if (rc) {
		resp->completion_code = rc;
		return 0;
	}

	rc = pldm_msgbuf_init_errno(buf,
				    PLDM_RDE_GET_SCHEMA_DICTIONARY_RESP_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, resp->completion_code);
	pldm_msgbuf_extract(buf, resp->dictionary_format);
	pldm_msgbuf_extract(buf, resp->transfer_handle);

	return pldm_msgbuf_complete_consumed(buf);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_varstring_from_iter(struct pldm_rde_varstring_iter *iter,
					struct pldm_rde_varstring *varstring)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	size_t string_length_bytes = 0;
	int rc;

	if (iter == NULL || iter->field.ptr == NULL || varstring == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_RDE_VARSTRING_HEADER_BYTES,
				    iter->field.ptr, iter->field.length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, varstring->string_format);
	rc = pldm_msgbuf_extract_uint8_to_size(buf, string_length_bytes);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	varstring->string_data.ptr = NULL;
	pldm_msgbuf_span_required(buf, string_length_bytes,
				  (const void **)&varstring->string_data.ptr);
	varstring->string_data.length = string_length_bytes;
	iter->field.ptr = NULL;
	pldm_msgbuf_span_remaining(buf, (const void **)&iter->field.ptr,
				   &iter->field.length);

	return pldm_msgbuf_complete(buf);
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_get_schema_uri_req(
	uint8_t instance_id, const struct pldm_rde_get_schema_uri_req *req,
	struct pldm_msg *msg, size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL || payload_length == NULL) {
		return -EINVAL;
	}
	if (req->requested_schema_class >= PLDM_RDE_SCHEMA_MAX) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(PLDM_REQUEST, instance_id, PLDM_RDE,
					   PLDM_RDE_CMD_GET_SCHEMA_URI, msg);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_RDE_GET_SCHEMA_URI_REQ_BYTES,
				    msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, req->resource_id);
	pldm_msgbuf_insert(buf, req->requested_schema_class);
	pldm_msgbuf_insert(buf, req->oem_extension_number);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_get_schema_uri_req(const struct pldm_msg *msg,
				       size_t payload_length,
				       struct pldm_rde_get_schema_uri_req *req)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_RDE_GET_SCHEMA_URI_REQ_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, req->resource_id);
	pldm_msgbuf_extract(buf, req->requested_schema_class);
	pldm_msgbuf_extract(buf, req->oem_extension_number);

	rc = pldm_msgbuf_complete_consumed(buf);
	if (rc) {
		return rc;
	}

	/* RequestedSchemaClass must be a defined schemaClass per DSP0218
	 * Table 3. */
	if (req->requested_schema_class >= PLDM_RDE_SCHEMA_MAX) {
		return -EBADMSG;
	}
	return 0;
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_get_schema_uri_resp(
	uint8_t instance_id, const struct pldm_rde_get_schema_uri_resp *resp,
	const struct pldm_rde_varstring *uris, struct pldm_msg *msg,
	size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	size_t needed;
	int rc;

	if (msg == NULL || resp == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(PLDM_RESPONSE, instance_id, PLDM_RDE,
					   PLDM_RDE_CMD_GET_SCHEMA_URI, msg);
	if (rc) {
		return rc;
	}

	/* An error response carries only the completion code. */
	if (resp->completion_code != PLDM_SUCCESS) {
		return encode_rde_cc_only_resp(msg, resp->completion_code,
					       payload_length);
	}

	/* StringFragmentCount must be non-zero per DSP0218 Table 55. */
	if (resp->string_fragment_count == 0 || uris == NULL) {
		return -EINVAL;
	}

	needed = PLDM_RDE_GET_SCHEMA_URI_RESP_FIXED_BYTES;
	for (uint8_t i = 0; i < resp->string_fragment_count; i++) {
		if (uris[i].string_data.length > UINT8_MAX) {
			return -EINVAL;
		}
		if (uris[i].string_data.length > 0 &&
		    uris[i].string_data.ptr == NULL) {
			return -EINVAL;
		}
		needed += (size_t)PLDM_RDE_VARSTRING_HEADER_BYTES +
			  uris[i].string_data.length;
	}

	rc = pldm_msgbuf_init_errno(buf, needed, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, resp->completion_code);
	pldm_msgbuf_insert(buf, resp->string_fragment_count);

	for (uint8_t i = 0; i < resp->string_fragment_count; i++) {
		pldm_msgbuf_insert(buf, uris[i].string_format);
		pldm_msgbuf_insert_uint8(buf,
					 (uint8_t)uris[i].string_data.length);
		if (uris[i].string_data.length > 0) {
			rc = pldm_msgbuf_insert_array_uint8(
				buf, uris[i].string_data.length,
				uris[i].string_data.ptr,
				uris[i].string_data.length);
			if (rc) {
				return pldm_msgbuf_discard(buf, rc);
			}
		}
	}

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_get_schema_uri_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_get_schema_uri_resp *resp)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	const void *remaining = NULL;
	size_t remaining_length = 0;
	int rc;

	if (msg == NULL || resp == NULL) {
		return -EINVAL;
	}

	/* Leave the fragment span empty so the error and short paths are safe
	 * to iterate. */
	resp->fragments.ptr = NULL;
	resp->fragments.length = 0;
	resp->string_fragment_count = 0;

	rc = pldm_msg_has_error(msg, payload_length);
	if (rc) {
		resp->completion_code = rc;
		return 0;
	}

	/* count > 0 guarantees at least one fragment, so a success response is
	 * at least the fixed fields plus one varstring header. */
	rc = pldm_msgbuf_init_errno(buf, PLDM_RDE_GET_SCHEMA_URI_RESP_MIN_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, resp->completion_code);
	pldm_msgbuf_extract(buf, resp->string_fragment_count);

	rc = pldm_msgbuf_span_remaining(buf, &remaining, &remaining_length);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	rc = pldm_msgbuf_complete(buf);
	if (rc) {
		return rc;
	}

	/* StringFragmentCount must be non-zero per DSP0218 Table 55. The
	 * fragment bytes are validated lazily as the fragments are walked. */
	if (resp->string_fragment_count == 0) {
		return -EBADMSG;
	}
	resp->fragments.ptr = remaining;
	resp->fragments.length = remaining_length;
	return 0;
}
