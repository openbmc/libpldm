#include "api.h"
#include "compiler.h"
#include "dsp/base.h"
#include "msgbuf.h"
#include <libpldm/base.h>
#include <libpldm/pldm_types.h>
#include <libpldm/rde.h>

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

LIBPLDM_ABI_TESTING
int encode_pldm_rde_negotiate_redfish_parameters_req(
	uint8_t instance_id,
	const struct pldm_rde_negotiate_redfish_parameters_req *req,
	struct pldm_msg *msg, size_t payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || req == NULL) {
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
		msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_insert(buf, req->mc_concurrency_support);
	pldm_msgbuf_insert(buf, req->mc_feature_support.value);

	return pldm_msgbuf_complete(buf);
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
	struct pldm_msg *msg, size_t payload_length)
{
	PLDM_MSGBUF_RW_DEFINE_P(buf);
	int rc;

	if (msg == NULL || resp == NULL) {
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
		rc = pldm_msgbuf_init_errno(buf, sizeof(resp->completion_code),
					    msg->payload, payload_length);
		if (rc) {
			return rc;
		}
		pldm_msgbuf_insert(buf, resp->completion_code);
		return pldm_msgbuf_complete(buf);
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
		msg->payload, payload_length);
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

	return pldm_msgbuf_complete(buf);
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

	rc = pldm_msgbuf_init_errno(buf, sizeof(resp->completion_code),
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_extract(buf, resp->completion_code);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}
	if (resp->completion_code != PLDM_SUCCESS) {
		return pldm_msgbuf_discard(buf, 0);
	}

	if (payload_length <
	    (size_t)PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_BYTES) {
		return pldm_msgbuf_discard(buf, -EOVERFLOW);
	}
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
