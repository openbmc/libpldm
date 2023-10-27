#include "oem/meta/libpldm/file_io.h"
#include "base.h"
#include <endian.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "msgbuf.h"

LIBPLDM_ABI_TESTING
int decode_write_file_io_req_oem_meta(const struct pldm_msg *msg,
				      size_t payload_length,
				      uint8_t *file_handle, uint32_t *length,
				      uint8_t *data)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	int rc;

	if (msg == NULL || file_handle == NULL || length == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	rc = pldm_msgbuf_init(buf, PLDM_OEM_META_DECODE_WRITE_FILE_IO_MIN_SIZE,
			      msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, file_handle);
	rc = pldm_msgbuf_extract(buf, length);

	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract_array_uint8(buf, data, *length);

	return pldm_msgbuf_destroy(buf);
}
