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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* A varstring's code unit is one byte for the single-byte formats and two for
 * the UTF-16 formats, per DSP0218 Table 2. */
static size_t rde_varstring_code_unit_size(uint8_t string_format)
{
	switch (string_format) {
	case PLDM_RDE_VARSTRING_UTF_16:
	case PLDM_RDE_VARSTRING_UTF_16LE:
	case PLDM_RDE_VARSTRING_UTF_16BE:
		return 2;
	default:
		return 1;
	}
}

/* A varstring is well formed per DSP0218 Table 2: stringFormat names a defined
 * format, and stringData is NULL-terminated with a length that counts the
 * terminator, holds whole code units, and fits the uint8 stringLengthBytes
 * wire field. */
static bool rde_varstring_valid(const struct pldm_rde_varstring *varstring)
{
	const uint8_t *tail;
	size_t unit;

	if (varstring->string_format > PLDM_RDE_VARSTRING_MAX) {
		return false;
	}

	unit = rde_varstring_code_unit_size(varstring->string_format);
	if (varstring->string_data.ptr == NULL ||
	    varstring->string_data.length < unit ||
	    varstring->string_data.length > UINT8_MAX ||
	    varstring->string_data.length % unit != 0) {
		return false;
	}

	/* The trailing code unit is the NULL terminator; checking its first
	 * and last byte covers both widths. */
	tail = varstring->string_data.ptr + varstring->string_data.length -
	       unit;
	return tail[0] == '\0' && tail[unit - 1] == '\0';
}

/* Encode a response carrying only the completion code, which each command's
 * own clause in DSP0218 states its non-SUCCESS response reduces to. The caller
 * emits the PLDM header first. */
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
		return -EPROTO;
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
	if (!rde_varstring_valid(&resp->provider_name)) {
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
	rc = pldm_msgbuf_extract_uint8_to_size(
		buf, resp->provider_name.string_data.length);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	resp->provider_name.string_data.ptr = NULL;
	rc = pldm_msgbuf_span_required(
		buf, resp->provider_name.string_data.length,
		(const void **)&resp->provider_name.string_data.ptr);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	rc = pldm_msgbuf_complete_consumed(buf);
	if (rc) {
		return rc;
	}

	/* DeviceConcurrencySupport must be non-zero per DSP0218 Table 52. */
	if (resp->device_concurrency_support == 0) {
		return -EPROTO;
	}
	if (!rde_varstring_valid(&resp->provider_name)) {
		return -EPROTO;
	}
	return 0;
}
