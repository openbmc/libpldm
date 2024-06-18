/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <libpldm/oem/meta/file_io.h>
#include <endian.h>
#include <string.h>
#include <stdio.h>
#include "msgbuf.h"
#include "dsp/base.h"

#define PLDM_OEM_META_DECODE_WRITE_FILE_IO_MIN_SIZE 6
#define PLDM_OEM_META_DECODE_READ_FILE_IO_MIN_SIZE  1
LIBPLDM_ABI_STABLE
int decode_oem_meta_write_file_io_req(const struct pldm_msg *msg,
				      size_t payload_length,
				      uint8_t *file_handle, uint32_t *length,
				      uint8_t *data)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || file_handle == NULL || length == NULL ||
	    data == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	int rc = pldm_msgbuf_init_cc(
		buf, PLDM_OEM_META_DECODE_WRITE_FILE_IO_MIN_SIZE, msg->payload,
		payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract_p(buf, file_handle);
	pldm_msgbuf_extract_p(buf, length);
	pldm_msgbuf_extract_array_uint8(buf, data, *length);

	return pldm_msgbuf_destroy_consumed(buf);
}

LIBPLDM_ABI_TESTING
int decode_oem_meta_read_file_io_req(const struct pldm_msg *msg,
				     size_t payload_length,
				     struct pldm_oem_meta_read_file_req *req)
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
	pldm_msgbuf_extract_p(buf, &req->file_handle);
	pldm_msgbuf_extract_p(buf, &req->length);
	pldm_msgbuf_extract_p(buf, &req->transferFlag);
	pldm_msgbuf_extract_p(buf, &req->highOffset);
	pldm_msgbuf_extract_p(buf, &req->lowOffset);

	return pldm_msgbuf_destroy_consumed(buf);
}

LIBPLDM_ABI_TESTING
int encode_http_boot_header_resp(uint8_t instance_id, uint8_t completion_code,
				 struct pldm_msg *responseMsg)
{
	if (responseMsg == NULL) {
		return -EINVAL;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_RESPONSE;
	header.pldm_type = PLDM_OEM;
	header.command = PLDM_OEM_META_FILEIO_CMD_READ_FILE;
	uint8_t rc = pack_pldm_header_errno(&header, &(responseMsg->hdr));

	if (rc) {
		return rc;
	}

	responseMsg->payload[0] = completion_code;

	return 0;
}
