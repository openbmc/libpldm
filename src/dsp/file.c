/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include "dsp/base.h"
#include "msgbuf.h"

#include <libpldm/base.h>
#include <libpldm/file.h>
#include <libpldm/utils.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

LIBPLDM_ABI_TESTING
int encode_pldm_file_df_open_req(uint8_t instance_id,
				 const struct pldm_file_df_open_req *req,
				 struct pldm_msg *msg, size_t payload_length)
{
	PLDM_MSGBUF_DEFINE_P(buf);
	int rc;

	if (req == NULL || msg == NULL) {
		return -EINVAL;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FILE;
	header.command = PLDM_FILE_CMD_DF_OPEN;

	rc = pack_pldm_header_errno(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_DF_OPEN_REQ_BYTES, msg->payload,
				    payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, req->file_identifier);
	pldm_msgbuf_insert(buf, req->file_attribute.value);

	return pldm_msgbuf_complete(buf);
}

LIBPLDM_ABI_TESTING
int decode_pldm_file_df_open_resp(const struct pldm_msg *msg,
				  size_t payload_length,
				  struct pldm_file_df_open_resp *resp)
{
	PLDM_MSGBUF_DEFINE_P(buf);
	int rc;

	if (!msg || !resp) {
		return -EINVAL;
	}

	rc = pldm_msg_has_error(msg, payload_length);
	if (rc) {
		resp->completion_code = rc;
		return 0;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_DF_OPEN_RESP_BYTES, msg->payload,
				    payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, resp->completion_code);
	pldm_msgbuf_extract(buf, resp->file_descriptor);

	return pldm_msgbuf_complete_consumed(buf);
}

LIBPLDM_ABI_TESTING
int encode_pldm_file_df_close_req(uint8_t instance_id,
				  const struct pldm_file_df_close_req *req,
				  struct pldm_msg *msg, size_t payload_length)
{
	PLDM_MSGBUF_DEFINE_P(buf);
	int rc;

	if (!req || !msg) {
		return -EINVAL;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FILE;
	header.command = PLDM_FILE_CMD_DF_CLOSE;

	rc = pack_pldm_header_errno(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_DF_CLOSE_REQ_BYTES, msg->payload,
				    payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, req->file_descriptor);
	pldm_msgbuf_insert(buf, req->df_close_options.value);

	return pldm_msgbuf_complete(buf);
}

LIBPLDM_ABI_TESTING
int decode_pldm_file_df_close_resp(const struct pldm_msg *msg,
				   size_t payload_length,
				   struct pldm_file_df_close_resp *resp)
{
	if (!msg || !resp) {
		return -EINVAL;
	}

	resp->completion_code = pldm_msg_has_error(msg, payload_length);

	return 0;
}

LIBPLDM_ABI_TESTING
int encode_pldm_file_df_heartbeat_req(
	uint8_t instance_id, const struct pldm_file_df_heartbeat_req *req,
	struct pldm_msg *msg, size_t payload_length)
{
	PLDM_MSGBUF_DEFINE_P(buf);
	int rc;

	if (!req || !msg) {
		return -EINVAL;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FILE;
	header.command = PLDM_FILE_CMD_DF_HEARTBEAT;

	rc = pack_pldm_header_errno(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_DF_HEARTBEAT_REQ_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, req->file_descriptor);
	pldm_msgbuf_insert(buf, req->requester_max_interval);

	return pldm_msgbuf_complete(buf);
}

LIBPLDM_ABI_TESTING
int decode_pldm_file_df_heartbeat_resp(const struct pldm_msg *msg,
				       size_t payload_length,
				       struct pldm_file_df_heartbeat_resp *resp)
{
	PLDM_MSGBUF_DEFINE_P(buf);
	int rc;

	if (!msg || !resp) {
		return -EINVAL;
	}

	rc = pldm_msg_has_error(msg, payload_length);
	if (rc) {
		resp->completion_code = rc;
		return 0;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_DF_HEARTBEAT_RESP_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, resp->completion_code);
	pldm_msgbuf_extract(buf, resp->responder_max_interval);

	return pldm_msgbuf_complete_consumed(buf);
}
