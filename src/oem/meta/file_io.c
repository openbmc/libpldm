/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <libpldm/oem/meta/file_io.h>
#include <endian.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "api.h"
#include "msgbuf.h"
#include "dsp/base.h"

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
	int rc;

	if (msg == NULL || req == NULL) {
		return -EINVAL;
	}

	if (req_length < sizeof(*req)) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf,
				    PLDM_OEM_META_FILE_IO_WRITE_REQ_MIN_LENGTH,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, req->handle);
	rc = pldm_msgbuf_extract(buf, req->length);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_extract_array(buf, req->length, req->data,
				       req_length - sizeof(*req));
	if (rc) {
		return rc;
	}

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
