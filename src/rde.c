#include "rde.h"

#include "base.h"
#include "msgbuf.h"

#include <endian.h>
#include <stdio.h>
#include <string.h>

// Minimum transfer size allowed is 64 bytes.
#define PLDM_RDE_MIN_TRANSFER_SIZE_BYTES 64

/**
 * @brief Minimum data sizes required for variable size RDE commands.
 */
#define PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_SIZE 12

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
