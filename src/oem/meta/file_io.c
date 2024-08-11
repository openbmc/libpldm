/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <libpldm/oem/meta/file_io.h>
#include <endian.h>
#include <string.h>
#include <stdio.h>
#include "msgbuf.h"

#define PLDM_OEM_META_DECODE_WRITE_FILE_IO_MIN_SIZE 6
LIBPLDM_ABI_STABLE
int decode_oem_meta_file_io_req(const struct pldm_msg *msg,
				size_t payload_length, uint8_t *file_handle,
				uint32_t *length, uint8_t *data)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	int rc;

	if (msg == NULL || file_handle == NULL || length == NULL ||
	    data == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	rc = pldm_msgbuf_init_cc(buf,
				 PLDM_OEM_META_DECODE_WRITE_FILE_IO_MIN_SIZE,
				 msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract_p(buf, file_handle);
	pldm_msgbuf_extract_p(buf, length);

	/* NOTE: Memory safety failure */
	rc = pldm_msgbuf_extract_array_uint8(buf, (size_t)(*length), data,
					     UINT32_MAX);
	if (rc) {
		return rc;
	}

	return pldm_msgbuf_destroy_consumed(buf);
}
