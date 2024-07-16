/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <libpldm/oem/meta/file_io.h>
#include <endian.h>
#include <string.h>
#include <stdio.h>

#include "api.h"
#include "msgbuf.h"
#include "dsp/base.h"

#define PLDM_OEM_META_DECODE_WRITE_FILE_IO_MIN_SIZE 6
#define PLDM_OEM_META_DECODE_READ_FILE_IO_MIN_SIZE  4
LIBPLDM_ABI_TESTING
int decode_oem_meta_file_io_write_req(const struct pldm_msg *msg,
				      size_t payload_length,
				      struct pldm_oem_meta_write_file_req *req,
				      size_t req_length)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || req == NULL || req->file_data == NULL) {
		return -EINVAL;
	}

	int rc = pldm_msgbuf_init_errno(
		buf, PLDM_OEM_META_DECODE_WRITE_FILE_IO_MIN_SIZE, msg->payload,
		payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, req->file_handle);
	rc = pldm_msgbuf_extract(buf, req->length);
	if (rc) {
		return rc;
	}

	if ((req->length + PLDM_OEM_META_DECODE_WRITE_FILE_IO_MIN_SIZE) >
	    req_length) {
		// The total length of the request message exceeds the length provided
		return -EOVERFLOW;
	}
	pldm_msgbuf_extract_array_uint8(buf, req->file_data, req->length);

	return pldm_msgbuf_destroy_consumed(buf);
}

LIBPLDM_ABI_DEPRECATED
int decode_oem_meta_file_io_req(const struct pldm_msg *msg,
				size_t payload_length, uint8_t *file_handle,
				uint32_t *length, uint8_t *data)
{
	if (msg == NULL || file_handle == NULL || length == NULL ||
	    data == NULL) {
		return pldm_xlate_errno(-EINVAL);
	}

	int rc;
	struct pldm_oem_meta_write_file_req request_msg;
	request_msg.file_data = data;

	rc = decode_oem_meta_file_io_write_req(msg, payload_length,
					       &request_msg, UINT32_MAX);
	if (rc < 0) {
		return pldm_xlate_errno(rc);
	}

	*file_handle = request_msg.file_handle;
	*length = request_msg.length;
	return 0;
}

LIBPLDM_ABI_TESTING
int decode_oem_meta_file_io_read_req(const struct pldm_msg *msg,
				     size_t payload_length,
				     struct pldm_oem_meta_file_io_read_req *req,
				     uint16_t req_length)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || req == NULL || req->read_info == NULL) {
		return -EINVAL;
	}

	int rc = pldm_msgbuf_init_errno(
		buf, PLDM_OEM_META_DECODE_READ_FILE_IO_MIN_SIZE, msg->payload,
		payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, req->file_handle);
	pldm_msgbuf_extract(buf, req->read_option);
	rc = pldm_msgbuf_extract(buf, req->length);
	if (rc) {
		return rc;
	}

	if ((req->length + PLDM_OEM_META_DECODE_READ_FILE_IO_MIN_SIZE) >
	    req_length) {
		// The total length of the request message exceeds the length provided
		return -EOVERFLOW;
	}

	pldm_msgbuf_extract_array_uint8(buf, req->read_info, req->length);

	return pldm_msgbuf_destroy_consumed(buf);
}
