/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include "compiler.h"
#include "fd-internal.h"
#include "environ/errno.h"

#include <libpldm/pldm.h>
#include <libpldm/base.h>
#include <libpldm/file.h>
#include <libpldm/file_fd.h>
#include <libpldm/edac.h>
#include <libpldm/control.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdalign.h>
#include <assert.h>

/* DSP0242 v1.0.1 */
#define PLDM_FILE_VERSIONS_COUNT 2
static const uint32_t PLDM_FILE_VERSIONS[PLDM_FILE_VERSIONS_COUNT] = {
	0xf1f0f100,
	/* CRC: hex(crccheck.crc.Crc32.calc(struct.pack('<I', 0xf1f0f100))) */
	0x4b44e5cc,
};

/* Commands 0x00-0x07: DF_OPEN(0x01), DF_CLOSE(0x02), DF_HEARTBEAT(0x03) */
/* Commands 0x20-0x27: DF_READ(0x20) */
static const bitfield8_t PLDM_FILE_COMMANDS[32] = {
	{ .byte = (1u << (PLDM_FILE_CMD_DF_OPEN & 7u)) |
		  (1u << (PLDM_FILE_CMD_DF_CLOSE & 7u)) |
		  (1u << (PLDM_FILE_CMD_DF_HEARTBEAT & 7u)) },
	[4] = { .byte = (1u << (PLDM_FILE_CMD_DF_READ & 7u)) },
};

static_assert(alignof(struct pldm_file_fd) == PLDM_ALIGNOF_PLDM_FILE_FD,
	      "PLDM_ALIGNOF_PLDM_FILE_FD wrong");

/* Send a completion-code-only response using the type/command from req_hdr. */
LIBPLDM_CC_NONNULL
static int file_reply_cc(uint8_t ccode, const struct pldm_header_info *req_hdr,
			 struct pldm_msg *resp, size_t *resp_payload_len)
{
	int status;

	/* 1 byte completion code */
	if (*resp_payload_len < 1) {
		return -EOVERFLOW;
	}
	*resp_payload_len = 1;

	status = encode_cc_only_resp(req_hdr->instance, req_hdr->pldm_type,
				     req_hdr->command, ccode, resp);
	if (status != PLDM_SUCCESS) {
		return -EINVAL;
	}
	return 0;
}

LIBPLDM_CC_NONNULL
static struct pldm_file_fd_slot *file_fd_find_slot(struct pldm_file_fd *fd,
						   uint16_t file_descriptor)
{
	if (file_descriptor == 0) {
		return NULL;
	}

	for (unsigned int i = 0; i < fd->num_fds; i++) {
		if (fd->fds[i].file_descriptor == file_descriptor) {
			return &fd->fds[i];
		}
	}
	return NULL;
}

LIBPLDM_CC_NONNULL
static int file_df_open(struct pldm_file_fd *fd,
			const struct pldm_header_info *hdr,
			const struct pldm_msg *req, size_t req_payload_len,
			struct pldm_msg *resp, size_t *resp_payload_len)
{
	struct pldm_file_df_open_resp response = { 0 };
	struct pldm_file_df_open_req dreq;
	struct pldm_file_fd_slot *slot;
	unsigned int fds_idx;
	void *app_handle;
	size_t plen;
	uint8_t cc;
	int rc;

	rc = decode_pldm_file_df_open_req(req, req_payload_len, &dreq);
	if (rc) {
		return file_reply_cc(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
				     resp_payload_len);
	}

	for (fds_idx = 0; fds_idx < fd->num_fds; fds_idx++) {
		if (fd->fds[fds_idx].file_descriptor == 0) {
			break;
		}
	}
	if (fds_idx == fd->num_fds) {
		return file_reply_cc(PLDM_FILE_CC_MAX_NUM_FDS_EXCEEDED, hdr,
				     resp, resp_payload_len);
	}
	slot = &fd->fds[fds_idx];

	plen = PLDM_DF_OPEN_RESP_BYTES;
	if (*resp_payload_len < plen) {
		return -EOVERFLOW;
	}

	fd->last_fd++;
	if (fd->last_fd == 0) {
		fd->last_fd = 1;
	}

	response.completion_code = PLDM_SUCCESS;
	response.file_descriptor = fd->last_fd;

	rc = encode_pldm_file_df_open_resp(hdr->instance, &response, resp,
					   &plen);
	if (rc) {
		return -EINVAL;
	}

	app_handle = NULL;
	cc = fd->ops.open(fd->ops.ctx, dreq.file_identifier,
			  dreq.file_attribute, &app_handle);
	if (cc != PLDM_SUCCESS) {
		return file_reply_cc(cc, hdr, resp, resp_payload_len);
	}

	slot->file_descriptor = fd->last_fd;
	slot->file_id = dreq.file_identifier;
	slot->attributes = dreq.file_attribute;
	slot->app_handle = app_handle;
	slot->read_xfr_handle = 0;

	*resp_payload_len = plen;
	return 0;
}

LIBPLDM_CC_NONNULL
static int file_df_close(struct pldm_file_fd *fd,
			 const struct pldm_header_info *hdr,
			 const struct pldm_msg *req, size_t req_payload_len,
			 struct pldm_msg *resp, size_t *resp_payload_len)
{
	struct pldm_file_df_close_resp response = { 0 };
	struct pldm_file_df_close_req dreq;
	struct pldm_file_fd_slot *slot;
	size_t plen;
	int rc;

	rc = decode_pldm_file_df_close_req(req, req_payload_len, &dreq);
	if (rc) {
		return file_reply_cc(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
				     resp_payload_len);
	}

	slot = file_fd_find_slot(fd, dreq.file_descriptor);
	if (!slot) {
		return file_reply_cc(PLDM_FILE_CC_INVALID_FILE_DESCRIPTOR, hdr,
				     resp, resp_payload_len);
	}

	if (fd->ops.close) {
		fd->ops.close(fd->ops.ctx, slot->file_id, slot->app_handle);
	}

	slot->file_descriptor = 0;
	slot->app_handle = NULL;

	response.completion_code = PLDM_SUCCESS;
	plen = PLDM_DF_CLOSE_RESP_BYTES;

	if (*resp_payload_len < plen) {
		return -EOVERFLOW;
	}

	rc = encode_pldm_file_df_close_resp(hdr->instance, &response, resp,
					    &plen);
	if (rc) {
		return -EINVAL;
	}

	*resp_payload_len = plen;
	return 0;
}

LIBPLDM_CC_NONNULL
static int file_df_heartbeat(struct pldm_file_fd *fd,
			     const struct pldm_header_info *hdr,
			     const struct pldm_msg *req, size_t req_payload_len,
			     struct pldm_msg *resp, size_t *resp_payload_len)
{
	struct pldm_file_df_heartbeat_resp response = { 0 };
	struct pldm_file_df_heartbeat_req dreq;
	struct pldm_file_fd_slot *slot;
	uint32_t interval_ms;
	size_t plen;
	int rc;

	rc = decode_pldm_file_df_heartbeat_req(req, req_payload_len, &dreq);
	if (rc) {
		return file_reply_cc(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
				     resp_payload_len);
	}

	slot = file_fd_find_slot(fd, dreq.file_descriptor);
	if (!slot) {
		return file_reply_cc(PLDM_FILE_CC_INVALID_FILE_DESCRIPTOR, hdr,
				     resp, resp_payload_len);
	}

	interval_ms = dreq.requester_max_interval;
	if (fd->ops.heartbeat) {
		fd->ops.heartbeat(fd->ops.ctx, slot->file_id, slot->app_handle,
				  &interval_ms);
	}

	response.completion_code = PLDM_SUCCESS;
	response.responder_max_interval = interval_ms;
	plen = PLDM_DF_HEARTBEAT_RESP_BYTES;

	if (*resp_payload_len < plen) {
		return -EOVERFLOW;
	}

	rc = encode_pldm_file_df_heartbeat_resp(hdr->instance, &response, resp,
						&plen);
	if (rc) {
		return -EINVAL;
	}

	*resp_payload_len = plen;
	return 0;
}

/* Handle a PLDM_BASE MultipartReceive request carrying PLDM_FILE data (DfRead).
 *
 * Data is written directly into the response buffer at the data-area offset,
 * then encode_base_multipart_receive_resp encodes the fixed header fields. */
LIBPLDM_CC_NONNULL
static int file_df_read(struct pldm_file_fd *fd,
			const struct pldm_header_info *hdr,
			const struct pldm_msg *req, size_t req_payload_len,
			struct pldm_msg *resp, size_t *resp_payload_len)
{
	struct pldm_base_multipart_receive_req mreq;
	struct pldm_file_fd_slot *slot;
	uint8_t transfer_flag;
	uint32_t next_handle;
	uint32_t actual_len;
	uint32_t max_data;
	uint8_t *data_area;
	uint32_t checksum;
	bool is_eof;
	uint8_t cc;
	int rc;

	rc = decode_pldm_base_multipart_receive_req(req, req_payload_len,
						    &mreq);
	if (rc) {
		return file_reply_cc(PLDM_ERROR_INVALID_DATA, hdr, resp,
				     resp_payload_len);
	}

	if (mreq.pldm_type != PLDM_FILE) {
		return -ENOMSG;
	}

	if (mreq.transfer_ctx > UINT16_MAX) {
		return file_reply_cc(PLDM_ERROR_INVALID_DATA, hdr, resp,
				     resp_payload_len);
	}

	slot = file_fd_find_slot(fd, (uint16_t)mreq.transfer_ctx);
	if (!slot) {
		return file_reply_cc(PLDM_ERROR_INVALID_DATA, hdr, resp,
				     resp_payload_len);
	}

	/* Bytes reserved for: cc(1) + transfer_flag(1) + next_handle(4) +
	 * data_length(4) + checksum(4) = PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES + 4 */
	if (*resp_payload_len <
	    PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES + 4u) {
		return -EOVERFLOW;
	}

	if (mreq.transfer_opflag == PLDM_XFER_FIRST_PART) {
		fd->next_xfr_handle++;
		if (fd->next_xfr_handle == 0) {
			fd->next_xfr_handle = 1;
		}
		slot->read_xfr_handle = fd->next_xfr_handle;
	} else {
		if (!slot->read_xfr_handle ||
		    mreq.transfer_handle != slot->read_xfr_handle) {
			return file_reply_cc(PLDM_ERROR_INVALID_DATA, hdr, resp,
					     resp_payload_len);
		}
	}

	max_data = (uint32_t)(*resp_payload_len -
			      PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES - 4u);

	/* Cap by negotiated multipart size for PLDM_FILE if available */
	if (fd->control) {
		uint16_t part_size;
		uint32_t hdr_overhead =
			(uint32_t)sizeof(struct pldm_msg_hdr) +
			PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES + 4u;
		if (pldm_control_get_multipart_size(fd->control, PLDM_FILE,
						    &part_size) == 0 &&
		    (uint32_t)part_size > hdr_overhead) {
			uint32_t cap = (uint32_t)part_size - hdr_overhead;
			if (cap < max_data) {
				max_data = cap;
			}
		}
	}

	if (mreq.section_length < max_data) {
		max_data = mreq.section_length;
	}

	/* Write data directly into the response buffer at the data-area offset
	 * so the app fills it in place with no extra copy. */
	data_area = resp->payload + PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES;
	actual_len = max_data;
	cc = fd->ops.read(fd->ops.ctx, slot->file_id, slot->app_handle,
			  mreq.section_offset, data_area, max_data,
			  &actual_len);
	if (cc != PLDM_SUCCESS) {
		slot->read_xfr_handle = 0;
		return file_reply_cc(cc, hdr, resp, resp_payload_len);
	}

	is_eof = actual_len < max_data;
	if (mreq.transfer_opflag == PLDM_XFER_FIRST_PART) {
		transfer_flag =
			is_eof ?
				PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_START_AND_END :
				PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_START;
	} else {
		transfer_flag =
			is_eof ?
				PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_END :
				PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_MIDDLE;
	}

	if (!is_eof) {
		fd->next_xfr_handle++;
		if (fd->next_xfr_handle == 0) {
			fd->next_xfr_handle = 1;
		}
		next_handle = fd->next_xfr_handle;
		slot->read_xfr_handle = next_handle;
	} else {
		slot->read_xfr_handle = 0;
		next_handle = 0;
	}

	checksum = pldm_edac_crc32(data_area, actual_len);

	struct pldm_base_multipart_receive_resp mresp = {
		.completion_code = PLDM_SUCCESS,
		.transfer_flag = transfer_flag,
		.next_transfer_handle = next_handle,
		.data = { .ptr = data_area, .length = actual_len },
	};
	return encode_base_multipart_receive_resp(
		hdr->instance, &mresp, checksum, resp, resp_payload_len);
}

/* ------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------ */

LIBPLDM_ABI_TESTING
int pldm_file_fd_handle_msg(struct pldm_file_fd *fd, const void *in_msg,
			    size_t in_len, void *out_msg, size_t *out_len)
{
	const struct pldm_msg *req = in_msg;
	struct pldm_msg *resp = out_msg;
	struct pldm_header_info hdr;
	size_t resp_payload_len;
	size_t req_payload_len;
	int rc;

	if (!fd || !in_msg || !out_msg || !out_len) {
		return -EINVAL;
	}

	if (in_len < sizeof(struct pldm_msg_hdr)) {
		return -EOVERFLOW;
	}
	req_payload_len = in_len - sizeof(struct pldm_msg_hdr);

	rc = unpack_pldm_header(&req->hdr, &hdr);
	if (rc != PLDM_SUCCESS) {
		return -EINVAL;
	}

	if (hdr.pldm_type != PLDM_FILE &&
	    !(hdr.pldm_type == PLDM_BASE &&
	      hdr.command == PLDM_MULTIPART_RECEIVE)) {
		/* Caller should have passed a file/type 7 message, or a
		 * PLDM_BASE MultipartReceive request (used to realise DfRead) */
		return -ENOMSG;
	}

	if (hdr.msg_type == PLDM_RESPONSE) {
		/* PD does not send requests; responses are unexpected */
		return -EPROTO;
	}

	if (hdr.msg_type != PLDM_REQUEST) {
		return -EPROTO;
	}

	/* Space for header plus at least completion code */
	if (*out_len < sizeof(struct pldm_msg_hdr) + 1) {
		return -EOVERFLOW;
	}

	resp_payload_len = *out_len - sizeof(struct pldm_msg_hdr);

	if (hdr.pldm_type == PLDM_BASE) {
		/* DfRead is realised as a PLDM_BASE MultipartReceive request
		 * carrying pldm_type==PLDM_FILE in its payload. */
		rc = file_df_read(fd, &hdr, req, req_payload_len, resp,
				  &resp_payload_len);
		goto out;
	}

	switch (hdr.command) {
	case PLDM_FILE_CMD_DF_OPEN:
		rc = file_df_open(fd, &hdr, req, req_payload_len, resp,
				  &resp_payload_len);
		break;
	case PLDM_FILE_CMD_DF_CLOSE:
		rc = file_df_close(fd, &hdr, req, req_payload_len, resp,
				   &resp_payload_len);
		break;
	case PLDM_FILE_CMD_DF_HEARTBEAT:
		rc = file_df_heartbeat(fd, &hdr, req, req_payload_len, resp,
				       &resp_payload_len);
		break;
	default:
		rc = file_reply_cc(PLDM_ERROR_UNSUPPORTED_PLDM_CMD, &hdr, resp,
				   &resp_payload_len);
	}

out:

	if (rc == 0) {
		*out_len = resp_payload_len + sizeof(struct pldm_msg_hdr);
	}

	return rc;
}

LIBPLDM_ABI_TESTING
struct pldm_file_fd *pldm_file_fd_new(size_t num_fds,
				      const struct pldm_file_fd_ops *ops,
				      size_t ops_size,
				      struct pldm_control *control)
{
	struct pldm_file_fd *fd;
	size_t total_size;

	if (!num_fds || num_fds > UINT16_MAX) {
		return NULL;
	}

	total_size = offsetof(struct pldm_file_fd, fds) +
		     num_fds * sizeof(struct pldm_file_fd_slot);

	fd = malloc(total_size);
	if (!fd) {
		return NULL;
	}

	if (pldm_file_fd_setup(fd, total_size, num_fds, ops, ops_size,
			       control)) {
		free(fd);
		return NULL;
	}

	return fd;
}

LIBPLDM_ABI_TESTING
int pldm_file_fd_setup(struct pldm_file_fd *fd, size_t pldm_file_fd_size,
		       size_t num_fds, const struct pldm_file_fd_ops *ops,
		       size_t ops_size, struct pldm_control *control)
{
	size_t required_size;
	int rc;

	if (!fd || !ops || !ops_size || !num_fds) {
		return -EINVAL;
	}

	if (num_fds > UINT16_MAX) {
		return -EINVAL;
	}

	required_size = offsetof(struct pldm_file_fd, fds) +
			num_fds * sizeof(struct pldm_file_fd_slot);
	if (pldm_file_fd_size < required_size) {
		return -EINVAL;
	}

	memset(fd, 0, required_size);

	if (ops_size > sizeof(fd->ops)) {
		const uint8_t *tail = (const uint8_t *)ops + sizeof(fd->ops);
		size_t tail_size = ops_size - sizeof(fd->ops);
		for (size_t i = 0; i < tail_size; i++) {
			if (tail[i] != 0) {
				return -E2BIG;
			}
		}
		memcpy(&fd->ops, ops, sizeof(fd->ops));
	} else {
		memcpy(&fd->ops, ops, ops_size);
	}

	fd->control = control;
	fd->next_xfr_handle = 0;
	fd->last_fd = 0;
	fd->num_fds = (uint16_t)num_fds;

	if (control) {
		rc = pldm_control_add_type(control, PLDM_FILE,
					   PLDM_FILE_VERSIONS,
					   PLDM_FILE_VERSIONS_COUNT,
					   PLDM_FILE_COMMANDS);
		if (rc) {
			return rc;
		}
	}

	return 0;
}
