/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include "api.h"
#include "dsp/base.h"
#include "msgbuf.h"

#include <assert.h>
#include <libpldm/base.h>
#include <libpldm/pldm_types.h>

#include <endian.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

int pack_pldm_header_errno(const struct pldm_header_info *hdr,
			   struct pldm_msg_hdr *msg)
{
	if (msg == NULL || hdr == NULL) {
		return -EINVAL;
	}

	if (hdr->msg_type != PLDM_RESPONSE && hdr->msg_type != PLDM_REQUEST &&
	    hdr->msg_type != PLDM_ASYNC_REQUEST_NOTIFY) {
		return -EINVAL;
	}

	if (hdr->instance > PLDM_INSTANCE_MAX) {
		return -EINVAL;
	}

	if (hdr->pldm_type > (PLDM_MAX_TYPES - 1)) {
		return -ENOMSG;
	}

	uint8_t datagram = (hdr->msg_type == PLDM_ASYNC_REQUEST_NOTIFY) ? 1 : 0;

	if (hdr->msg_type == PLDM_RESPONSE) {
		msg->request = PLDM_RESPONSE;
	} else if (hdr->msg_type == PLDM_REQUEST ||
		   hdr->msg_type == PLDM_ASYNC_REQUEST_NOTIFY) {
		msg->request = PLDM_REQUEST;
	}
	msg->datagram = datagram;
	msg->reserved = 0;
	msg->instance_id = hdr->instance;
	msg->header_ver = PLDM_CURRENT_VERSION;
	msg->type = hdr->pldm_type;
	msg->command = hdr->command;

	return 0;
}

int unpack_pldm_header_errno(const struct pldm_msg_hdr *msg,
			     struct pldm_header_info *hdr)
{
	if (msg == NULL) {
		return -EINVAL;
	}

	if (msg->request == PLDM_RESPONSE) {
		hdr->msg_type = PLDM_RESPONSE;
	} else {
		hdr->msg_type = msg->datagram ? PLDM_ASYNC_REQUEST_NOTIFY :
						PLDM_REQUEST;
	}

	hdr->instance = msg->instance_id;
	hdr->pldm_type = msg->type;
	hdr->command = msg->command;

	return 0;
}

LIBPLDM_ABI_STABLE
uint8_t pack_pldm_header(const struct pldm_header_info *hdr,
			 struct pldm_msg_hdr *msg)
{
	enum pldm_completion_codes cc;
	int rc;

	rc = pack_pldm_header_errno(hdr, msg);
	if (!rc) {
		return PLDM_SUCCESS;
	}

	cc = pldm_xlate_errno(rc);
	assert(cc < UINT8_MAX);
	if (cc > UINT8_MAX) {
		static_assert(PLDM_ERROR < UINT8_MAX, "Unable to report error");
		return PLDM_ERROR;
	}

	return cc;
}

LIBPLDM_ABI_STABLE
uint8_t unpack_pldm_header(const struct pldm_msg_hdr *msg,
			   struct pldm_header_info *hdr)
{
	enum pldm_completion_codes cc;
	int rc;

	rc = unpack_pldm_header_errno(msg, hdr);
	if (!rc) {
		return PLDM_SUCCESS;
	}

	cc = pldm_xlate_errno(rc);
	assert(cc < UINT8_MAX);
	if (cc > UINT8_MAX) {
		static_assert(PLDM_ERROR < UINT8_MAX, "Unable to report error");
		return PLDM_ERROR;
	}

	return cc;
}

LIBPLDM_ABI_STABLE
bool pldm_msg_hdr_correlate_response(const struct pldm_msg_hdr *req,
				     const struct pldm_msg_hdr *resp)
{
	return req->instance_id == resp->instance_id && req->request &&
	       !resp->request && req->type == resp->type &&
	       req->command == resp->command;
}

LIBPLDM_ABI_STABLE
int encode_get_types_req(uint8_t instance_id, struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_GET_PLDM_TYPES;

	return pack_pldm_header(&header, &(msg->hdr));
}

LIBPLDM_ABI_STABLE
int encode_get_commands_req(uint8_t instance_id, uint8_t type, ver32_t version,
			    struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_GET_PLDM_COMMANDS;

	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_get_commands_req *request =
		(struct pldm_get_commands_req *)msg->payload;

	request->type = type;
	request->version = version;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_get_types_resp(uint8_t instance_id, uint8_t completion_code,
			  const bitfield8_t *types, struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_RESPONSE;
	header.command = PLDM_GET_PLDM_TYPES;

	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_get_types_resp *response =
		(struct pldm_get_types_resp *)msg->payload;
	response->completion_code = completion_code;
	if (response->completion_code == PLDM_SUCCESS) {
		if (types == NULL) {
			return PLDM_ERROR_INVALID_DATA;
		}
		memcpy(response->types, &(types->byte), PLDM_MAX_TYPES / 8);
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_get_commands_req(const struct pldm_msg *msg, size_t payload_length,
			    uint8_t *type, ver32_t *version)
{
	if (msg == NULL || type == NULL || version == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_GET_COMMANDS_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_commands_req *request =
		(struct pldm_get_commands_req *)msg->payload;
	*type = request->type;
	*version = request->version;
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_get_commands_resp(uint8_t instance_id, uint8_t completion_code,
			     const bitfield8_t *commands, struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_RESPONSE;
	header.command = PLDM_GET_PLDM_COMMANDS;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_get_commands_resp *response =
		(struct pldm_get_commands_resp *)msg->payload;
	response->completion_code = completion_code;
	if (response->completion_code == PLDM_SUCCESS) {
		if (commands == NULL) {
			return PLDM_ERROR_INVALID_DATA;
		}
		memcpy(response->commands, &(commands->byte),
		       PLDM_MAX_CMDS_PER_TYPE / 8);
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_get_types_resp(const struct pldm_msg *msg, size_t payload_length,
			  uint8_t *completion_code, bitfield8_t *types)
{
	if (msg == NULL || types == NULL || completion_code == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length != PLDM_GET_TYPES_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_types_resp *response =
		(struct pldm_get_types_resp *)msg->payload;

	memcpy(&(types->byte), response->types, PLDM_MAX_TYPES / 8);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_get_commands_resp(const struct pldm_msg *msg, size_t payload_length,
			     uint8_t *completion_code, bitfield8_t *commands)
{
	if (msg == NULL || commands == NULL || completion_code == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length != PLDM_GET_COMMANDS_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_commands_resp *response =
		(struct pldm_get_commands_resp *)msg->payload;

	memcpy(&(commands->byte), response->commands,
	       PLDM_MAX_CMDS_PER_TYPE / 8);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_get_version_req(uint8_t instance_id, uint32_t transfer_handle,
			   uint8_t transfer_opflag, uint8_t type,
			   struct pldm_msg *msg)
{
	if (NULL == msg) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.msg_type = PLDM_REQUEST;
	header.instance = instance_id;
	header.pldm_type = PLDM_BASE;
	header.command = PLDM_GET_PLDM_VERSION;

	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_get_version_req *request =
		(struct pldm_get_version_req *)msg->payload;
	transfer_handle = htole32(transfer_handle);
	request->transfer_handle = transfer_handle;
	request->transfer_opflag = transfer_opflag;
	request->type = type;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_DEPRECATED_UNSAFE
int encode_get_version_resp(uint8_t instance_id, uint8_t completion_code,
			    uint32_t next_transfer_handle,
			    uint8_t transfer_flag, const ver32_t *version_data,
			    size_t version_size, struct pldm_msg *msg)
{
	if (NULL == msg || NULL == version_data) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_BASE;
	header.command = PLDM_GET_PLDM_VERSION;

	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_get_version_resp *response =
		(struct pldm_get_version_resp *)msg->payload;
	response->completion_code = completion_code;
	if (response->completion_code == PLDM_SUCCESS) {
		response->next_transfer_handle = htole32(next_transfer_handle);
		response->transfer_flag = transfer_flag;
		memcpy(response->version_data, (uint8_t *)version_data,
		       version_size);
	}
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_get_version_req(const struct pldm_msg *msg, size_t payload_length,
			   uint32_t *transfer_handle, uint8_t *transfer_opflag,
			   uint8_t *type)
{
	if (payload_length != PLDM_GET_VERSION_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_version_req *request =
		(struct pldm_get_version_req *)msg->payload;
	*transfer_handle = le32toh(request->transfer_handle);
	*transfer_opflag = request->transfer_opflag;
	*type = request->type;
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_get_version_resp(const struct pldm_msg *msg, size_t payload_length,
			    uint8_t *completion_code,
			    uint32_t *next_transfer_handle,
			    uint8_t *transfer_flag, ver32_t *version)
{
	if (msg == NULL || next_transfer_handle == NULL ||
	    transfer_flag == NULL || completion_code == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length < PLDM_GET_VERSION_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_version_resp *response =
		(struct pldm_get_version_resp *)msg->payload;

	*next_transfer_handle = le32toh(response->next_transfer_handle);
	*transfer_flag = response->transfer_flag;
	memcpy(version, (uint8_t *)response->version_data, sizeof(ver32_t));

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_get_tid_req(uint8_t instance_id, struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_GET_TID;

	return pack_pldm_header(&header, &(msg->hdr));
}

LIBPLDM_ABI_STABLE
int encode_get_tid_resp(uint8_t instance_id, uint8_t completion_code,
			uint8_t tid, struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_RESPONSE;
	header.command = PLDM_GET_TID;

	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_get_tid_resp *response =
		(struct pldm_get_tid_resp *)msg->payload;
	response->completion_code = completion_code;
	response->tid = tid;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_get_tid_resp(const struct pldm_msg *msg, size_t payload_length,
			uint8_t *completion_code, uint8_t *tid)
{
	if (msg == NULL || tid == NULL || completion_code == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length != PLDM_GET_TID_RESP_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_tid_resp *response =
		(struct pldm_get_tid_resp *)msg->payload;

	*tid = response->tid;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_set_tid_req(uint8_t instance_id, uint8_t tid, struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (tid == 0x0 || tid == 0xff) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_SET_TID;

	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_set_tid_req *request =
		(struct pldm_set_tid_req *)msg->payload;
	request->tid = tid;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int decode_set_tid_req(const struct pldm_msg *msg, size_t payload_length,
		       uint8_t *tid)
{
	PLDM_MSGBUF_DEFINE_P(buf);
	int rc;

	if (!msg || !tid) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_SET_TID_REQ_BYTES, msg->payload,
				    payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract_p(buf, tid);

	return pldm_msgbuf_complete_consumed(buf);
}

LIBPLDM_ABI_STABLE
int decode_multipart_receive_req(const struct pldm_msg *msg,
				 size_t payload_length, uint8_t *pldm_type,
				 uint8_t *transfer_opflag,
				 uint32_t *transfer_ctx,
				 uint32_t *transfer_handle,
				 uint32_t *section_offset,
				 uint32_t *section_length)
{
	if (msg == NULL || pldm_type == NULL || transfer_opflag == NULL ||
	    transfer_ctx == NULL || transfer_handle == NULL ||
	    section_offset == NULL || section_length == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_MULTIPART_RECEIVE_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_multipart_receive_req *request =
		(struct pldm_multipart_receive_req *)msg->payload;

	if (request->pldm_type != PLDM_BASE &&
	    request->pldm_type != PLDM_FILE) {
		return PLDM_ERROR_INVALID_PLDM_TYPE;
	}

	// Any enum value above PLDM_XFER_CURRENT_PART is invalid.
	if (request->transfer_opflag > PLDM_XFER_CURRENT_PART) {
		return PLDM_ERROR_UNEXPECTED_TRANSFER_FLAG_OPERATION;
	}

	// A section offset of 0 is only valid on FIRST_PART or COMPLETE Xfers.
	uint32_t sec_offset = le32toh(request->section_offset);
	if (sec_offset == 0 &&
	    (request->transfer_opflag != PLDM_XFER_FIRST_PART &&
	     request->transfer_opflag != PLDM_XFER_COMPLETE)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	uint32_t handle = le32toh(request->transfer_handle);
	if (handle == 0 && request->transfer_opflag != PLDM_XFER_COMPLETE) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*pldm_type = request->pldm_type;
	*transfer_opflag = request->transfer_opflag;
	*transfer_ctx = request->transfer_ctx;
	*transfer_handle = handle;
	*section_offset = sec_offset;
	*section_length = le32toh(request->section_length);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_base_multipart_receive_req(
	uint8_t instance_id, const struct pldm_multipart_receive_req *req,
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
	header.pldm_type = PLDM_BASE;
	header.command = PLDM_MULTIPART_RECEIVE;

	rc = pack_pldm_header_errno(&header, &msg->hdr);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_MULTIPART_RECEIVE_REQ_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, req->pldm_type);
	pldm_msgbuf_insert(buf, req->transfer_opflag);
	pldm_msgbuf_insert(buf, req->transfer_ctx);
	pldm_msgbuf_insert(buf, req->transfer_handle);
	pldm_msgbuf_insert(buf, req->section_offset);
	pldm_msgbuf_insert(buf, req->section_length);

	return pldm_msgbuf_complete(buf);
}

LIBPLDM_ABI_TESTING
int decode_base_multipart_receive_resp(const struct pldm_msg *msg,
				       size_t payload_length,
				       struct pldm_multipart_receive_resp *resp,
				       uint32_t *data_integrity_checksum)
{
	PLDM_MSGBUF_DEFINE_P(buf);
	int rc;

	if (msg == NULL || resp == NULL || data_integrity_checksum == NULL) {
		return -EINVAL;
	}

	rc = pldm_msg_has_error(msg, payload_length);

	if (rc) {
		resp->completion_code = rc;
		return 0;
	}

	rc = pldm_msgbuf_init_errno(buf,
				    PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, resp->completion_code);
	rc = pldm_msgbuf_extract(buf, resp->transfer_flag);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}
	pldm_msgbuf_extract(buf, resp->next_transfer_handle);

	rc = pldm_msgbuf_extract_uint32_to_size(buf, resp->data.length);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	if (resp->data.length > 0) {
		resp->data.ptr = NULL;
		pldm_msgbuf_span_required(buf, resp->data.length,
					  (void **)&resp->data.ptr);
	}

	if (resp->transfer_flag ==
		    PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_END ||
	    resp->transfer_flag ==
		    PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_START_AND_END) {
		pldm_msgbuf_extract_p(buf, data_integrity_checksum);
	}

	return pldm_msgbuf_complete_consumed(buf);
}

LIBPLDM_ABI_TESTING
int encode_base_multipart_receive_resp(
	uint8_t instance_id, const struct pldm_multipart_receive_resp *resp,
	uint32_t checksum, struct pldm_msg *msg, size_t *payload_length)
{
	PLDM_MSGBUF_DEFINE_P(buf);
	int rc;

	if (!msg || !resp || !payload_length || !resp->data.ptr) {
		return -EINVAL;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_RESPONSE;
	header.pldm_type = PLDM_BASE;
	header.command = PLDM_MULTIPART_RECEIVE;

	rc = pack_pldm_header_errno(&header, &msg->hdr);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(buf,
				    PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES,
				    msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, resp->completion_code);
	if (resp->completion_code != PLDM_SUCCESS) {
		// Return without encoding the rest of the field
		return pldm_msgbuf_complete_used(buf, *payload_length,
						 payload_length);
	}

	pldm_msgbuf_insert(buf, resp->transfer_flag);
	pldm_msgbuf_insert(buf, resp->next_transfer_handle);

	pldm_msgbuf_insert_uint32(buf, resp->data.length);
	if (resp->data.length == 0) {
		// Return without encoding data payload
		return pldm_msgbuf_complete_used(buf, *payload_length,
						 payload_length);
	}

	rc = pldm_msgbuf_insert_array(buf, resp->data.length, resp->data.ptr,
				      resp->data.length);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	if (resp->transfer_flag == PLDM_END ||
	    resp->transfer_flag == PLDM_START_AND_END) {
		pldm_msgbuf_insert(buf, checksum);
	}

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_STABLE
int encode_cc_only_resp(uint8_t instance_id, uint8_t type, uint8_t command,
			uint8_t cc, struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_RESPONSE;
	header.pldm_type = type;
	header.command = command;

	uint8_t rc = pack_pldm_header(&header, &msg->hdr);
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	msg->payload[0] = cc;

	return PLDM_SUCCESS;
}

int encode_pldm_header_only_errno(uint8_t msg_type, uint8_t instance_id,
				  uint8_t pldm_type, uint8_t command,
				  struct pldm_msg *msg)
{
	if (msg == NULL) {
		return -EINVAL;
	}

	struct pldm_header_info header = { 0 };
	header.msg_type = msg_type;
	header.instance = instance_id;
	header.pldm_type = pldm_type;
	header.command = command;
	return pack_pldm_header_errno(&header, &(msg->hdr));
}

LIBPLDM_ABI_STABLE
int encode_pldm_header_only(uint8_t msg_type, uint8_t instance_id,
			    uint8_t pldm_type, uint8_t command,
			    struct pldm_msg *msg)
{
	int rc = encode_pldm_header_only_errno(msg_type, instance_id, pldm_type,
					       command, msg);
	if (rc) {
		return pldm_xlate_errno(rc);
	}
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_pldm_base_negotiate_transfer_params_req(
	uint8_t instance_id,
	const struct pldm_base_negotiate_transfer_params_req *req,
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
	header.pldm_type = PLDM_BASE;
	header.command = PLDM_NEGOTIATE_TRANSFER_PARAMETERS;

	rc = pack_pldm_header_errno(&header, &msg->hdr);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(
		buf, PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES,
		msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, req->requester_part_size);
	rc = pldm_msgbuf_insert_array(
		buf, sizeof(req->requester_protocol_support),
		(uint8_t *)req->requester_protocol_support,
		sizeof(req->requester_protocol_support));
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	return pldm_msgbuf_complete(buf);
}

LIBPLDM_ABI_TESTING
int decode_pldm_base_negotiate_transfer_params_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_base_negotiate_transfer_params_resp *resp)
{
	PLDM_MSGBUF_DEFINE_P(buf);
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
		buf, PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES,
		msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, resp->completion_code);
	pldm_msgbuf_extract(buf, resp->responder_part_size);
	rc = pldm_msgbuf_extract_array(
		buf, sizeof(resp->responder_protocol_support),
		(uint8_t *)resp->responder_protocol_support,
		sizeof(resp->responder_protocol_support));
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	return pldm_msgbuf_complete_consumed(buf);
}
