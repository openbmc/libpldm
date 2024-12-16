/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <libpldm/base.h>
#include <libpldm/file.h>
#include <libpldm/utils.h>

#include <assert.h>
#include <endian.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

LIBPLDM_ABI_TESTING
int encode_df_open_req(
    uint8_t instance_id, uint16_t file_identifier,
    bitfield16_t file_attribute, struct pldm_msg *msg,
    size_t payload_length)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_DF_OPEN_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FILE;
	header.command = PLDM_DF_OPEN;

    uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
    if (rc != PLDM_SUCCESS) {
        return rc;
    }

    struct pldm_df_open_req *request =
        (struct pldm_df_open_req *)msg->payload;
    request->file_identifier = htole16(file_identifier);
    request->file_attribute.value = htole16(file_attribute.value);

    return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int decode_df_open_resp(
    const struct pldm_msg *msg, size_t payload_length,
    uint8_t *completion_code, uint16_t *file_descriptor)
{
	if (msg == NULL || completion_code == NULL ||
	    file_descriptor == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length != PLDM_DF_OPEN_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_df_open_resp *response =
		(struct pldm_df_open_resp *)msg->payload;

	*file_descriptor = le16toh(response->file_descriptor);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_df_close_req(
    uint8_t instance_id, uint16_t file_descriptor,
    bitfield16_t options, struct pldm_msg *msg,
    size_t payload_length)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_DF_CLOSE_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FILE;
	header.command = PLDM_DF_CLOSE;

	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_df_close_req *request =
		(struct pldm_df_close_req *)msg->payload;
	request->file_descriptor = htole16(file_descriptor);
	request->options.value = htole16(options.value);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int decode_df_close_resp(
    const struct pldm_msg *msg,
    uint8_t *completion_code)
{
	if (msg == NULL || completion_code == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_df_heartbeat_req(uint8_t instance_id,
                       uint16_t file_descriptor,
                       uint32_t req_max_interval,
                       struct pldm_msg *msg,
                       size_t payload_length)
{
    if (msg == NULL) {
        return PLDM_ERROR_INVALID_DATA;
    }

    if (payload_length != PLDM_DF_HEARTBEAT_REQ_BYTES) {
        return PLDM_ERROR_INVALID_LENGTH;
    }

    struct pldm_header_info header = { 0 };
    header.instance = instance_id;
    header.msg_type = PLDM_REQUEST;
    header.pldm_type = PLDM_FILE;
    header.command = PLDM_DF_HEARBEAT;

    uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
    if (rc != PLDM_SUCCESS) {
        return rc;
    }

    struct pldm_df_heartbeat_req *request =
        (struct pldm_df_heartbeat_req *)msg->payload;
    request->file_descriptor = htole16(file_descriptor);
    request->req_max_interval = htole32(req_max_interval);

    return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_df_heartbeat_resp(
    const struct pldm_msg *msg, size_t payload_length,
    uint8_t *completion_code, uint32_t *rsp_max_interval)
{
    if (msg == NULL || completion_code == NULL ||
        rsp_max_interval == NULL) {
        return PLDM_ERROR_INVALID_DATA;
    }

    *completion_code = msg->payload[0];
    if (PLDM_SUCCESS != *completion_code) {
        return PLDM_SUCCESS;
    }

    if (payload_length != PLDM_DF_HEARTBEAT_RESP_BYTES) {
        return PLDM_ERROR_INVALID_LENGTH;
    }

    struct pldm_df_heartbeat_resp *response =
        (struct pldm_df_heartbeat_resp *)msg->payload;

    *rsp_max_interval = le32toh(response->rsp_max_interval);

    return PLDM_SUCCESS;
}


