#include "rde.h"

#include "base.h"
#include "msgbuf.h"

#include <endian.h>
#include <stdio.h>
#include <string.h>

// Minimum transfer size allowed is 64 bytes.
#define PLDM_RDE_MIN_TRANSFER_SIZE_BYTES 64

LIBPLDM_ABI_TESTING
int encode_rde_negotiate_redfish_parameters_req(uint8_t instance_id,
						uint8_t concurrency_support,
						bitfield16_t *feature_support,
						size_t payload_length,
						struct pldm_msg *msg)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	int rc;

	if (msg == NULL || feature_support == NULL ||
	    concurrency_support == 0) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_NEGOTIATE_REDFISH_PARAMETERS;
	rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	rc = pldm_msgbuf_init(buf,
			      PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_SIZE,
			      msg->payload, payload_length);
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	pldm_msgbuf_insert_uint8(buf, concurrency_support);
	pldm_msgbuf_insert_uint16(buf, feature_support->value);

	return pldm_msgbuf_destroy_consumed(buf);
}

LIBPLDM_ABI_TESTING
int decode_rde_negotiate_redfish_parameters_req(
	const struct pldm_msg *msg, size_t payload_length,
	uint8_t *mc_concurrency_support, bitfield16_t *mc_feature_support)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	int rc;

	if (msg == NULL || mc_concurrency_support == NULL ||
	    mc_feature_support == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	rc = pldm_msgbuf_init(buf,
			      PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_SIZE,
			      msg->payload, payload_length);
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	pldm_msgbuf_extract(buf, mc_concurrency_support);
	if (*mc_concurrency_support == 0) {
		fprintf(stderr,
			"Concurrency support has to be greater than 0\n");
		return PLDM_ERROR_INVALID_DATA;
	}

	pldm_msgbuf_extract(buf, &mc_feature_support->value);

	return pldm_msgbuf_destroy_consumed(buf);
}

LIBPLDM_ABI_TESTING
int encode_negotiate_redfish_parameters_resp(
	uint8_t instance_id, uint8_t completion_code,
	uint8_t device_concurrency_support,
	bitfield8_t *device_capabilities_flags,
	bitfield16_t *device_feature_support,
	uint32_t device_configuration_signature,
	const char *device_provider_name,
	enum pldm_rde_varstring_format name_format, size_t payload_length,
	struct pldm_msg *msg)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	int rc;

	if (msg == NULL || device_capabilities_flags == NULL ||
	    device_feature_support == NULL || device_provider_name == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (completion_code != PLDM_SUCCESS) {
		return encode_cc_only_resp(instance_id, PLDM_RDE,
					   PLDM_NEGOTIATE_REDFISH_PARAMETERS,
					   completion_code, msg);
	}

	// Length should include NULL terminator.
	size_t str_len = strlen(device_provider_name) + 1;
	if (str_len > 255) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.command = PLDM_NEGOTIATE_REDFISH_PARAMETERS;

	rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	rc = pldm_msgbuf_init(
		buf, PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_SIZE,
		msg->payload, payload_length);
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	pldm_msgbuf_insert_uint8(buf, completion_code);
	pldm_msgbuf_insert_uint8(buf, device_concurrency_support);
	pldm_msgbuf_insert_uint8(buf, device_capabilities_flags->byte);
	pldm_msgbuf_insert_uint16(buf, device_feature_support->value);
	pldm_msgbuf_insert_uint32(buf, device_configuration_signature);
	pldm_msgbuf_insert_uint8(buf, name_format);
	pldm_msgbuf_insert_uint8(buf, (uint8_t)str_len);
	pldm_msgbuf_insert_array_uint8(
		buf, (const uint8_t *)device_provider_name, str_len);

	return pldm_msgbuf_destroy_consumed(buf);
}

LIBPLDM_ABI_TESTING
int decode_negotiate_redfish_parameters_resp(
	const struct pldm_msg *msg, size_t payload_length,
	uint8_t *completion_code, uint8_t *device_concurrency_support,
	bitfield8_t *device_capabilities_flags,
	bitfield16_t *device_feature_support,
	uint32_t *device_configuration_signature,
	struct pldm_rde_varstring *provider_name)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	int rc;

	if (msg == NULL || completion_code == NULL ||
	    device_concurrency_support == NULL ||
	    device_capabilities_flags == NULL ||
	    device_feature_support == NULL ||
	    device_configuration_signature == NULL || provider_name == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	rc = pldm_msgbuf_init(
		buf, PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_SIZE,
		msg->payload, payload_length);
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	pldm_msgbuf_extract(buf, completion_code);
	if (*completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}

	pldm_msgbuf_extract(buf, device_concurrency_support);
	pldm_msgbuf_extract(buf, &device_capabilities_flags->byte);
	pldm_msgbuf_extract(buf, &device_feature_support->value);
	pldm_msgbuf_extract(buf, device_configuration_signature);
	pldm_msgbuf_extract(buf, &provider_name->string_format);
	pldm_msgbuf_extract(buf, &provider_name->string_length_bytes);
	pldm_msgbuf_span_required(buf, provider_name->string_length_bytes,
				  (void **)&provider_name->string_data);

	return pldm_msgbuf_destroy_consumed(buf);
}
