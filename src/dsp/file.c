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
int encode_df_open_req(uint8_t instance_id, uint16_t file_identifier,
		       bitfield16_t file_attribute, struct pldm_msg *msg,
		       size_t payload_length)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	int rc;

	if (!msg) {
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

	pldm_msgbuf_insert(buf, file_identifier);
	pldm_msgbuf_insert(buf, file_attribute.value);

	return pldm_msgbuf_destroy(buf);
}

LIBPLDM_ABI_TESTING
int decode_df_open_resp(const struct pldm_msg *msg, size_t payload_length,
			struct pldm_df_open_resp *resp)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
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

	return pldm_msgbuf_destroy_consumed(buf);
}
