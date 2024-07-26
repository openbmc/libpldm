/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <libpldm/oem/meta/file_io.h>
#include <endian.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "api.h"
#include "msgbuf.h"
#include "dsp/base.h"

#define PLDM_OEM_META_DECODE_WRITE_FILE_IO_MIN_SIZE 6u
#define PLDM_OEM_META_DECODE_READ_FILE_IO_MIN_SIZE  3u
#define PLDM_OEM_META_ENCODE_READ_FILE_IO_MIN_SIZE  4u

LIBPLDM_ABI_TESTING
void *pldm_oem_meta_file_io_write_req_data(
	struct pldm_oem_meta_file_io_write_req *req)
{
	return req->data;
}

LIBPLDM_ABI_TESTING
int decode_oem_meta_file_io_write_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_oem_meta_file_io_write_req *req, size_t req_length)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || req == NULL) {
		return -EINVAL;
	}

	if (req_length < sizeof(*req)) {
		return -EINVAL;
	}

	int rc = pldm_msgbuf_init_errno(
		buf, PLDM_OEM_META_DECODE_WRITE_FILE_IO_MIN_SIZE, msg->payload,
		payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, req->handle);
	rc = pldm_msgbuf_extract(buf, req->length);
	if (rc) {
		return rc;
	}

	if (req->length > (req_length - sizeof(*req))) {
		// The total length of the request message exceeds the length provided
		return -EOVERFLOW;
	}

	pldm_msgbuf_extract_array_uint8(buf, req->data, req->length);

	return pldm_msgbuf_destroy_consumed(buf);
}

LIBPLDM_ABI_DEPRECATED
int decode_oem_meta_file_io_req(const struct pldm_msg *msg,
				size_t payload_length, uint8_t *file_handle,
				uint32_t *length, uint8_t *data)
{
	struct pldm_oem_meta_file_io_write_req *request_msg;
	size_t request_msg_len;
	int rc;

	if (msg == NULL || file_handle == NULL || length == NULL ||
	    data == NULL) {
		return pldm_xlate_errno(-EINVAL);
	}

	request_msg_len = sizeof(*request_msg) + payload_length;
	request_msg = malloc(request_msg_len);

	rc = decode_oem_meta_file_io_write_req(msg, payload_length, request_msg,
					       request_msg_len);
	if (rc < 0) {
		free(request_msg);
		return pldm_xlate_errno(rc);
	}

	*file_handle = request_msg->handle;
	*length = request_msg->length;

	/* NOTE: Unsafe, memory safety is not possible due to API constraints. */
	memcpy(data, request_msg->data, request_msg->length);

	free(request_msg);

	return 0;
}

LIBPLDM_ABI_TESTING
int decode_oem_meta_file_io_read_req(const struct pldm_msg *msg,
				     size_t payload_length,
				     struct pldm_oem_meta_file_io_read_req *req)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || req == NULL) {
		return -EINVAL;
	}

	int rc = pldm_msgbuf_init_errno(
		buf, PLDM_OEM_META_DECODE_READ_FILE_IO_MIN_SIZE, msg->payload,
		payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, req->handle);
	rc = pldm_msgbuf_extract(buf, req->option);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_extract(buf, req->length);
	if (rc) {
		return rc;
	}

	switch (req->option) {
	case PLDM_OEM_META_FILE_IO_READ_ATTR:
		if (req->length != 0) {
			return -EINVAL;
		}
		break;
	case PLDM_OEM_META_FILE_IO_READ_DATA:
		pldm_msgbuf_extract(buf, req->info.data.transferFlag);
		pldm_msgbuf_extract(buf, req->info.data.highOffset);
		pldm_msgbuf_extract(buf, req->info.data.lowOffset);
		break;
	default:
		return -EINVAL;
	}

	return pldm_msgbuf_destroy_consumed(buf);
}

LIBPLDM_ABI_TESTING
int encode_oem_meta_file_io_read_resp(
	uint8_t instance_id, struct pldm_oem_meta_file_io_read_resp *resp,
	struct pldm_msg *responseMsg, uint16_t payload_length)
{
	if (resp == NULL || responseMsg == NULL) {
		return -EINVAL;
	}

	if (payload_length < sizeof(*resp)) {
		return -EOVERFLOW;
	}

	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_RESPONSE;
	header.pldm_type = PLDM_OEM;
	header.command = PLDM_OEM_META_FILE_IO_CMD_READ_FILE;
	int rc = pack_pldm_header_errno(&header, &(responseMsg->hdr));
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(buf,
				    PLDM_OEM_META_ENCODE_READ_FILE_IO_MIN_SIZE,
				    responseMsg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, resp->completion_code);
	pldm_msgbuf_insert(buf, resp->handle);
	pldm_msgbuf_insert(buf, resp->option);
	rc = pldm_msgbuf_insert(buf, resp->length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert_array_uint8(buf, resp->data, resp->length);

	return pldm_msgbuf_destroy_consumed(buf);
}
