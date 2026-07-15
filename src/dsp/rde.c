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
	if (resp->provider_name.length > UINT8_MAX) {
		return -EINVAL;
	}
	if (resp->provider_name.length > 0 && resp->provider_name.ptr == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(
		buf,
		(size_t)PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_BYTES +
			resp->provider_name.length,
		msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, resp->completion_code);
	pldm_msgbuf_insert(buf, resp->device_concurrency_support);
	pldm_msgbuf_insert(buf, resp->device_capabilities_flags.byte);
	pldm_msgbuf_insert(buf, resp->device_feature_support.value);
	pldm_msgbuf_insert(buf, resp->device_configuration_signature);
	pldm_msgbuf_insert(buf, resp->provider_name_format);
	pldm_msgbuf_insert_uint8(buf, (uint8_t)resp->provider_name.length);

	if (resp->provider_name.length > 0) {
		rc = pldm_msgbuf_insert_array_uint8(buf,
						    resp->provider_name.length,
						    resp->provider_name.ptr,
						    resp->provider_name.length);
		if (rc) {
			return pldm_msgbuf_discard(buf, rc);
		}
	}

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_negotiate_redfish_parameters_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_negotiate_redfish_parameters_resp *resp)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	const void *name = NULL;
	uint8_t name_length = 0;
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
	pldm_msgbuf_extract(buf, resp->provider_name_format);
	pldm_msgbuf_extract(buf, name_length);

	rc = pldm_msgbuf_span_required(buf, name_length, &name);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}
	resp->provider_name.ptr = name;
	resp->provider_name.length = name_length;

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
	uint8_t string_length_bytes = 0;
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
	rc = pldm_msgbuf_extract(buf, string_length_bytes);
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
	struct pldm_rde_get_schema_uri_resp *resp,
	struct pldm_rde_varstring_iter *uris)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	const void *remaining = NULL;
	size_t remaining_length = 0;
	int rc;

	if (msg == NULL || resp == NULL || uris == NULL) {
		return -EINVAL;
	}

	/* Leave the iterator empty so the error and short paths are safe to
	 * walk. */
	uris->field.ptr = NULL;
	uris->field.length = 0;
	uris->count = 0;

	rc = pldm_msg_has_error(msg, payload_length);
	if (rc) {
		resp->completion_code = rc;
		return 0;
	}

	rc = pldm_msgbuf_init_errno(buf,
				    PLDM_RDE_GET_SCHEMA_URI_RESP_FIXED_BYTES,
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
	 * fragment bytes are validated lazily as the iterator is walked. */
	if (resp->string_fragment_count == 0) {
		return -EBADMSG;
	}
	uris->field.ptr = remaining;
	uris->field.length = remaining_length;
	uris->count = resp->string_fragment_count;
	return 0;
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_get_resource_etag_req(
	uint8_t instance_id, const struct pldm_rde_get_resource_etag_req *req,
	struct pldm_msg *msg, size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(PLDM_REQUEST, instance_id, PLDM_RDE,
					   PLDM_RDE_CMD_GET_RESOURCE_ETAG, msg);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_RDE_GET_RESOURCE_ETAG_REQ_BYTES,
				    msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, req->resource_id);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_get_resource_etag_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_get_resource_etag_req *req)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_RDE_GET_RESOURCE_ETAG_REQ_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, req->resource_id);

	return pldm_msgbuf_complete_consumed(buf);
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_get_resource_etag_resp(
	uint8_t instance_id, const struct pldm_rde_get_resource_etag_resp *resp,
	struct pldm_msg *msg, size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || resp == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(PLDM_RESPONSE, instance_id, PLDM_RDE,
					   PLDM_RDE_CMD_GET_RESOURCE_ETAG, msg);
	if (rc) {
		return rc;
	}

	/* An error response carries only the completion code. */
	if (resp->completion_code != PLDM_SUCCESS) {
		return encode_rde_cc_only_resp(msg, resp->completion_code,
					       payload_length);
	}

	/* The ETag is NUL-terminated per DSP0218 Table 2, so its length
	 * includes at least the terminator and must fit the uint8 wire
	 * field. */
	if (resp->etag.string_data.length == 0 ||
	    resp->etag.string_data.length > UINT8_MAX ||
	    resp->etag.string_data.ptr == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(
		buf,
		(size_t)PLDM_RDE_GET_RESOURCE_ETAG_RESP_MIN_BYTES +
			resp->etag.string_data.length,
		msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, resp->completion_code);
	pldm_msgbuf_insert(buf, resp->etag.string_format);
	pldm_msgbuf_insert_uint8(buf, (uint8_t)resp->etag.string_data.length);
	rc = pldm_msgbuf_insert_array_uint8(buf, resp->etag.string_data.length,
					    resp->etag.string_data.ptr,
					    resp->etag.string_data.length);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_get_resource_etag_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_get_resource_etag_resp *resp)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	uint8_t etag_length = 0;
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
				    PLDM_RDE_GET_RESOURCE_ETAG_RESP_MIN_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, resp->completion_code);
	pldm_msgbuf_extract(buf, resp->etag.string_format);
	pldm_msgbuf_extract(buf, etag_length);

	resp->etag.string_data.ptr = NULL;
	rc = pldm_msgbuf_span_required(
		buf, etag_length, (const void **)&resp->etag.string_data.ptr);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}
	resp->etag.string_data.length = etag_length;

	return pldm_msgbuf_complete_consumed(buf);
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_rde_operation_init_req(
	uint8_t instance_id, const struct pldm_rde_rde_operation_init_req *req,
	struct pldm_msg *msg, size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL || payload_length == NULL) {
		return -EINVAL;
	}
	if (req->operation_type >= PLDM_RDE_OPERATION_TYPE_MAX) {
		return -EINVAL;
	}
	if (req->operation_locator.length > UINT8_MAX) {
		return -EINVAL;
	}
	if (req->operation_locator.length > 0 &&
	    req->operation_locator.ptr == NULL) {
		return -EINVAL;
	}
	if (req->request_payload.length > 0 &&
	    req->request_payload.ptr == NULL) {
		return -EINVAL;
	}
	/* RequestPayloadLength is a uint32 on the wire; the guard is a no-op
	 * where size_t cannot exceed it. */
#if SIZE_MAX > UINT32_MAX
	if (req->request_payload.length > UINT32_MAX) {
		return -EINVAL;
	}
#endif

	rc = encode_pldm_header_only_errno(PLDM_REQUEST, instance_id, PLDM_RDE,
					   PLDM_RDE_CMD_RDE_OPERATION_INIT,
					   msg);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(
		buf,
		(size_t)PLDM_RDE_OPERATION_INIT_REQ_FIXED_BYTES +
			req->operation_locator.length +
			req->request_payload.length,
		msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, req->resource_id);
	pldm_msgbuf_insert(buf, req->operation_id);
	pldm_msgbuf_insert(buf, req->operation_type);
	pldm_msgbuf_insert(buf, req->operation_flags.byte);
	pldm_msgbuf_insert(buf, req->send_data_transfer_handle);
	pldm_msgbuf_insert_uint8(buf, (uint8_t)req->operation_locator.length);
	pldm_msgbuf_insert_uint32(buf, (uint32_t)req->request_payload.length);

	if (req->operation_locator.length > 0) {
		rc = pldm_msgbuf_insert_array_uint8(
			buf, req->operation_locator.length,
			req->operation_locator.ptr,
			req->operation_locator.length);
		if (rc) {
			return pldm_msgbuf_discard(buf, rc);
		}
	}
	if (req->request_payload.length > 0) {
		rc = pldm_msgbuf_insert_array_uint8(
			buf, req->request_payload.length,
			req->request_payload.ptr, req->request_payload.length);
		if (rc) {
			return pldm_msgbuf_discard(buf, rc);
		}
	}

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_rde_operation_init_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_rde_operation_init_req *req)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	uint8_t operation_locator_length = 0;
	uint32_t request_payload_length = 0;
	int rc;

	if (msg == NULL || req == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf,
				    PLDM_RDE_OPERATION_INIT_REQ_FIXED_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, req->resource_id);
	pldm_msgbuf_extract(buf, req->operation_id);
	pldm_msgbuf_extract(buf, req->operation_type);
	pldm_msgbuf_extract(buf, req->operation_flags.byte);
	pldm_msgbuf_extract(buf, req->send_data_transfer_handle);
	pldm_msgbuf_extract(buf, operation_locator_length);
	pldm_msgbuf_extract(buf, request_payload_length);

	req->operation_locator.ptr = NULL;
	rc = pldm_msgbuf_span_required(
		buf, operation_locator_length,
		(const void **)&req->operation_locator.ptr);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}
	req->operation_locator.length = operation_locator_length;

	req->request_payload.ptr = NULL;
	rc = pldm_msgbuf_span_required(
		buf, request_payload_length,
		(const void **)&req->request_payload.ptr);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}
	req->request_payload.length = request_payload_length;

	rc = pldm_msgbuf_complete_consumed(buf);
	if (rc) {
		return rc;
	}

	/* OperationType must be a defined operation per DSP0218 Table 64. */
	if (req->operation_type >= PLDM_RDE_OPERATION_TYPE_MAX) {
		return -EBADMSG;
	}
	return 0;
}

/* Encode the RDEOperationInit/RDEOperationStatus response body, which is
 * byte-identical for both commands per DSP0218 Table 68. The caller emits the
 * PLDM header for its own command code first; this is reached only on a
 * SUCCESS completion code. Parameters are decomposed so the helper privileges
 * neither command's response struct. */
static int encode_rde_operation_resp_body(
	struct pldm_msg *msg, uint8_t completion_code, uint8_t operation_status,
	uint8_t completion_percentage, uint32_t completion_time_seconds,
	uint8_t operation_execution_flags, uint32_t result_transfer_handle,
	uint8_t permission_flags, const struct pldm_rde_varstring *etag,
	const struct variable_field *response_payload, size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	/* The ETag is NUL-terminated per DSP0218 Table 2, so its length
	 * includes at least the terminator and must fit the uint8 wire
	 * field. */
	if (etag->string_data.length == 0 ||
	    etag->string_data.length > UINT8_MAX ||
	    etag->string_data.ptr == NULL) {
		return -EINVAL;
	}
	if (response_payload->length > 0 && response_payload->ptr == NULL) {
		return -EINVAL;
	}
	/* ResponsePayloadLength is a uint32 on the wire; the guard is a no-op
	 * where size_t cannot exceed it. */
#if SIZE_MAX > UINT32_MAX
	if (response_payload->length > UINT32_MAX) {
		return -EINVAL;
	}
#endif

	rc = pldm_msgbuf_init_errno(
		buf,
		(size_t)PLDM_RDE_OPERATION_INIT_RESP_FIXED_BYTES +
			PLDM_RDE_VARSTRING_HEADER_BYTES +
			etag->string_data.length + response_payload->length,
		msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, completion_code);
	pldm_msgbuf_insert(buf, operation_status);
	pldm_msgbuf_insert(buf, completion_percentage);
	pldm_msgbuf_insert(buf, completion_time_seconds);
	pldm_msgbuf_insert(buf, operation_execution_flags);
	pldm_msgbuf_insert(buf, result_transfer_handle);
	pldm_msgbuf_insert(buf, permission_flags);
	pldm_msgbuf_insert_uint32(buf, (uint32_t)response_payload->length);
	pldm_msgbuf_insert(buf, etag->string_format);
	pldm_msgbuf_insert_uint8(buf, (uint8_t)etag->string_data.length);
	rc = pldm_msgbuf_insert_array_uint8(buf, etag->string_data.length,
					    etag->string_data.ptr,
					    etag->string_data.length);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}
	if (response_payload->length > 0) {
		rc = pldm_msgbuf_insert_array_uint8(buf,
						    response_payload->length,
						    response_payload->ptr,
						    response_payload->length);
		if (rc) {
			return pldm_msgbuf_discard(buf, rc);
		}
	}

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

/* Decode the RDEOperationInit/RDEOperationStatus response body. The caller
 * handles the null and error-completion-code paths first; this is reached only
 * for a SUCCESS response. See encode_rde_operation_resp_body. */
static int decode_rde_operation_resp_body(
	const struct pldm_msg *msg, size_t payload_length,
	uint8_t *completion_code, uint8_t *operation_status,
	uint8_t *completion_percentage, uint32_t *completion_time_seconds,
	uint8_t *operation_execution_flags, uint32_t *result_transfer_handle,
	uint8_t *permission_flags, struct pldm_rde_varstring *etag,
	struct variable_field *response_payload)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	uint32_t response_payload_length = 0;
	uint8_t etag_length = 0;
	int rc;

	rc = pldm_msgbuf_init_errno(buf,
				    PLDM_RDE_OPERATION_INIT_RESP_FIXED_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract_p(buf, completion_code);
	pldm_msgbuf_extract_p(buf, operation_status);
	pldm_msgbuf_extract_p(buf, completion_percentage);
	pldm_msgbuf_extract_p(buf, completion_time_seconds);
	pldm_msgbuf_extract_p(buf, operation_execution_flags);
	pldm_msgbuf_extract_p(buf, result_transfer_handle);
	pldm_msgbuf_extract_p(buf, permission_flags);
	pldm_msgbuf_extract(buf, response_payload_length);
	pldm_msgbuf_extract(buf, etag->string_format);
	pldm_msgbuf_extract(buf, etag_length);

	etag->string_data.ptr = NULL;
	rc = pldm_msgbuf_span_required(buf, etag_length,
				       (const void **)&etag->string_data.ptr);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}
	etag->string_data.length = etag_length;

	response_payload->ptr = NULL;
	rc = pldm_msgbuf_span_required(buf, response_payload_length,
				       (const void **)&response_payload->ptr);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}
	response_payload->length = response_payload_length;

	return pldm_msgbuf_complete_consumed(buf);
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_rde_operation_init_resp(
	uint8_t instance_id,
	const struct pldm_rde_rde_operation_init_resp *resp,
	struct pldm_msg *msg, size_t *payload_length)
{
	int rc;

	if (msg == NULL || resp == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(PLDM_RESPONSE, instance_id, PLDM_RDE,
					   PLDM_RDE_CMD_RDE_OPERATION_INIT,
					   msg);
	if (rc) {
		return rc;
	}

	/* An error response carries only the completion code. */
	if (resp->completion_code != PLDM_SUCCESS) {
		return encode_rde_cc_only_resp(msg, resp->completion_code,
					       payload_length);
	}

	return encode_rde_operation_resp_body(
		msg, resp->completion_code, resp->operation_status,
		resp->completion_percentage, resp->completion_time_seconds,
		resp->operation_execution_flags.byte,
		resp->result_transfer_handle, resp->permission_flags.byte,
		&resp->etag, &resp->response_payload, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_rde_operation_init_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_rde_operation_init_resp *resp)
{
	int rc;

	if (msg == NULL || resp == NULL) {
		return -EINVAL;
	}

	rc = pldm_msg_has_error(msg, payload_length);
	if (rc) {
		resp->completion_code = rc;
		return 0;
	}

	return decode_rde_operation_resp_body(
		msg, payload_length, &resp->completion_code,
		&resp->operation_status, &resp->completion_percentage,
		&resp->completion_time_seconds,
		&resp->operation_execution_flags.byte,
		&resp->result_transfer_handle, &resp->permission_flags.byte,
		&resp->etag, &resp->response_payload);
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_rde_operation_complete_req(
	uint8_t instance_id,
	const struct pldm_rde_rde_operation_complete_req *req,
	struct pldm_msg *msg, size_t *payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(PLDM_REQUEST, instance_id, PLDM_RDE,
					   PLDM_RDE_CMD_RDE_OPERATION_COMPLETE,
					   msg);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_RDE_OPERATION_COMPLETE_REQ_BYTES,
				    msg->payload, *payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, req->resource_id);
	pldm_msgbuf_insert(buf, req->operation_id);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_rde_operation_complete_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_rde_operation_complete_req *req)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_RDE_OPERATION_COMPLETE_REQ_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, req->resource_id);
	pldm_msgbuf_extract(buf, req->operation_id);

	return pldm_msgbuf_complete_consumed(buf);
}

LIBPLDM_ABI_TESTING
int encode_pldm_rde_rde_operation_complete_resp(
	uint8_t instance_id,
	const struct pldm_rde_rde_operation_complete_resp *resp,
	struct pldm_msg *msg, size_t *payload_length)
{
	int rc;

	if (msg == NULL || resp == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only_errno(PLDM_RESPONSE, instance_id, PLDM_RDE,
					   PLDM_RDE_CMD_RDE_OPERATION_COMPLETE,
					   msg);
	if (rc) {
		return rc;
	}

	/* The response carries only the completion code per DSP0218 Table 67. */
	return encode_rde_cc_only_resp(msg, resp->completion_code,
				       payload_length);
}

LIBPLDM_ABI_TESTING
int decode_pldm_rde_rde_operation_complete_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_rde_rde_operation_complete_resp *resp)
{
	PLDM_MSGBUF_RO_DEFINE_P(buf);
	int rc;

	if (msg == NULL || resp == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, sizeof(resp->completion_code),
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract(buf, resp->completion_code);

	return pldm_msgbuf_complete_consumed(buf);
}
