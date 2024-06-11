/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <libpldm/oem/meta/file_io.h>
#include <endian.h>
#include <string.h>
#include <stdio.h>
#include "msgbuf.h"

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

	int rc = pldm__msgbuf_init(buf,
				   PLDM_OEM_META_DECODE_WRITE_FILE_IO_MIN_SIZE,
				   msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract_p(buf, file_handle);
	pldm_msgbuf_extract_p(buf, length);
	pldm_msgbuf_extract_array_uint8(buf, data, *length);

	return pldm_msgbuf_destroy_consumed(buf);
}

LIBPLDM_ABI_STABLE
int decode_oem_meta_read_file_io_req(const struct pldm_msg *msg,
				     size_t payload_length,
				     uint8_t *file_handle, uint8_t *length,
				     uint8_t *transferFlag, uint8_t *highOffset,
				     uint8_t *lowOffset)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || file_handle == NULL || length == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	int rc = pldm__msgbuf_init(buf,
				   PLDM_OEM_META_DECODE_READ_FILE_IO_MIN_SIZE,
				   msg->payload, payload_length);
	if (rc) {
		return rc;
	}
	pldm_msgbuf_extract_p(buf, file_handle);
	pldm_msgbuf_extract_p(buf, length);
	pldm_msgbuf_extract_p(buf, transferFlag);
	pldm_msgbuf_extract_p(buf, highOffset);
	pldm_msgbuf_extract_p(buf, lowOffset);

	return pldm_msgbuf_destroy_consumed(buf);
}

LIBPLDM_ABI_STABLE
int encode_http_boot_header_resp(uint8_t instance_id, uint8_t completion_code,
				 struct pldm_msg *responseMsg)
{
	if (responseMsg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_RESPONSE;
	header.pldm_type = PLDM_OEM;
	header.command = PLDM_OEM_META_FILEIO_CMD_READ_FILE;
	uint8_t rc = pack_pldm_header(&header, &(responseMsg->hdr));

	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	responseMsg->payload[0] = completion_code;

	return PLDM_SUCCESS;
}
