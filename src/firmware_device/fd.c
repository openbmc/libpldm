/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <libpldm/pldm.h>
#include <libpldm/firmware_update.h>
#include <libpldm/firmware_fd.h>
#include <libpldm/utils.h>
#include <compiler.h>
#include <msgbuf.h>

#include "fd-internal.h"

/* FD_T1 Update mode idle timeout, 120 seconds (range [60s, 120s])*/
static const pldm_fd_time_t DEFAULT_FD_T1_TIMEOUT = 120000;

/* FD_T2 "Retry request for firmware data", 1 second (range [1s, 5s]) */
static const pldm_fd_time_t DEFAULT_FD_T2_RETRY_TIME = 1000;

static const uint8_t INSTANCE_ID_COUNT = 32;
static const uint8_t PROGRESS_PERCENT_NOT_SUPPORTED = 101;

#define PLDM_FD_VERSIONS_COUNT 2
static const uint32_t PLDM_FD_VERSIONS[PLDM_FD_VERSIONS_COUNT] = {
	/* Only PLDM Firmware 1.1.0 is current implemented. */
	0xf1f1f000,
	/* CRC. Calculated with python:
	hex(crccheck.crc.Crc32.calc(struct.pack('<I', 0xf1f1f000)))
	*/
	0x539dbeba,
};
const bitfield8_t PLDM_FD_COMMANDS[32] = {
	// 0x00..0x07
	{ .byte = (1 << PLDM_QUERY_DEVICE_IDENTIFIERS |
		   1 << PLDM_GET_FIRMWARE_PARAMETERS) },
	{ 0 },
	// 0x10..0x17
	{ .byte = (1u << PLDM_REQUEST_UPDATE | 1u << PLDM_PASS_COMPONENT_TABLE |
		   1u << PLDM_UPDATE_COMPONENT) >>
		  0x10 },
	// 0x18..0x1f
	{
		.byte = (1u << PLDM_ACTIVATE_FIRMWARE | 1u << PLDM_GET_STATUS |
			 1u << PLDM_CANCEL_UPDATE_COMPONENT |
			 1u << PLDM_CANCEL_UPDATE) >>
			0x18,
	},
};

/* Ensure that public definition is kept updated */
static_assert(alignof(struct pldm_fd) == PLDM_ALIGNOF_PLDM_FD,
	      "PLDM_ALIGNOF_PLDM_FD wrong");

/* Maybe called with success or failure completion codes, though
 * success only makes sense for responses without a body.
 * Returns 0 or negative errno. */
LIBPLDM_CC_NONNULL
static int pldm_fd_reply_cc(uint8_t ccode,
			    const struct pldm_header_info *req_hdr,
			    struct pldm_msg *resp, size_t *resp_payload_len)
{
	int status;

	/* 1 byte completion code */
	if (*resp_payload_len < 1) {
		return -EOVERFLOW;
	}
	*resp_payload_len = 1;

	status = encode_cc_only_resp(req_hdr->instance, PLDM_FWUP,
				     req_hdr->command, ccode, resp);
	if (status != PLDM_SUCCESS) {
		return -EINVAL;
	}
	return 0;
}

/* Must be called with a negative errno.
 * Returns 0 or negative errno. */
LIBPLDM_CC_NONNULL
static int pldm_fd_reply_errno(int err, const struct pldm_header_info *req_hdr,
			       struct pldm_msg *resp, size_t *resp_payload_len)
{
	uint8_t ccode = PLDM_ERROR;

	assert(err < 0);
	switch (err) {
	case -EINVAL:
		// internal error, shouldn't occur.
		ccode = PLDM_ERROR;
		break;
	case -EPROTO:
		// Bad data from peer
		ccode = PLDM_ERROR_INVALID_DATA;
		break;
	case -EOVERFLOW:
	case -EBADMSG:
		// Bad data from peer
		ccode = PLDM_ERROR_INVALID_LENGTH;
		break;
	default:
		// general error
		ccode = PLDM_ERROR;
	}

	return pldm_fd_reply_cc(ccode, req_hdr, resp, resp_payload_len);
}

LIBPLDM_CC_NONNULL
static void pldm_fd_set_state(struct pldm_fd *fd,
			      enum pldm_firmware_device_states state)
{
	/* pldm_fd_set_idle should be used instead */
	assert(state != PLDM_FD_STATE_IDLE);

	if (fd->state == state) {
		return;
	}

	fd->prev_state = fd->state;
	fd->state = state;
}

LIBPLDM_CC_NONNULL
static void pldm_fd_set_idle(struct pldm_fd *fd,
			     enum pldm_get_status_reason_code_values reason)
{
	fd->prev_state = fd->state;
	fd->state = PLDM_FD_STATE_IDLE;
	fd->reason = reason;
	fd->ua_address_set = false;
}

LIBPLDM_CC_NONNULL
static void pldm_fd_idle_timeout(struct pldm_fd *fd)
{
	enum pldm_get_status_reason_code_values reason = PLDM_FD_INITIALIZATION;

	switch (fd->state) {
	case PLDM_FD_STATE_IDLE:
		return;
	case PLDM_FD_STATE_LEARN_COMPONENTS:
		reason = PLDM_FD_TIMEOUT_LEARN_COMPONENT;
		break;
	case PLDM_FD_STATE_READY_XFER:
		reason = PLDM_FD_TIMEOUT_READY_XFER;
		break;
	case PLDM_FD_STATE_DOWNLOAD:
		reason = PLDM_FD_TIMEOUT_DOWNLOAD;
		break;
	case PLDM_FD_STATE_VERIFY:
		reason = PLDM_FD_TIMEOUT_VERIFY;
		break;
	case PLDM_FD_STATE_APPLY:
		reason = PLDM_FD_TIMEOUT_APPLY;
		break;
	case PLDM_FD_STATE_ACTIVATE:
		/* Not a timeout */
		reason = PLDM_FD_ACTIVATE_FW;
		break;
	}

	pldm_fd_set_idle(fd, reason);
}

LIBPLDM_CC_NONNULL
static void pldm_fd_get_aux_state(const struct pldm_fd *fd, uint8_t *aux_state,
				  uint8_t *aux_state_status)
{
	*aux_state_status = 0;

	switch (fd->req.state) {
	case PLDM_FD_REQ_UNUSED:
		*aux_state = PLDM_FD_IDLE_LEARN_COMPONENTS_READ_XFER;
		break;
	case PLDM_FD_REQ_SENT:
		*aux_state = PLDM_FD_OPERATION_IN_PROGRESS;
		break;
	case PLDM_FD_REQ_READY:
		if (fd->req.complete) {
			*aux_state = PLDM_FD_OPERATION_SUCCESSFUL;
		} else {
			*aux_state = PLDM_FD_OPERATION_IN_PROGRESS;
		}
		break;
	case PLDM_FD_REQ_FAILED:
		*aux_state = PLDM_FD_OPERATION_FAILED;
		*aux_state_status = fd->req.result;
		break;
	}
}

LIBPLDM_CC_NONNULL
static uint64_t pldm_fd_now(struct pldm_fd *fd)
{
	return fd->ops->now(fd->ops_ctx);
}

LIBPLDM_CC_NONNULL
static bool pldm_fd_req_should_send(struct pldm_fd *fd)
{
	pldm_fd_time_t now = pldm_fd_now(fd);

	switch (fd->req.state) {
	case PLDM_FD_REQ_UNUSED:
		assert(false);
		return false;
	case PLDM_FD_REQ_READY:
		return true;
	case PLDM_FD_REQ_FAILED:
		return false;
	case PLDM_FD_REQ_SENT:
		if (now < fd->req.sent_time) {
			/* Time went backwards */
			return false;
		}

		/* Send if retry time has elapsed */
		return (now - fd->req.sent_time) >= fd->fd_t2_retry_time;
	}
	return false;
}

/* Allocate the next instance ID. Only one request is outstanding so cycling
 * through the range is OK */
LIBPLDM_CC_NONNULL
static uint8_t pldm_fd_req_next_instance(struct pldm_fd_req *req)
{
	req->instance_id = (req->instance_id + 1) % INSTANCE_ID_COUNT;
	return req->instance_id;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_qdi(struct pldm_fd *fd, const struct pldm_header_info *hdr,
		       const struct pldm_msg *req LIBPLDM_CC_UNUSED,
		       size_t req_payload_len, struct pldm_msg *resp,
		       size_t *resp_payload_len)
{
	uint8_t descriptor_count;
	const struct pldm_descriptor *descriptors;
	int rc;

	/* QDI has no request data */
	if (req_payload_len != PLDM_QUERY_DEVICE_IDENTIFIERS_REQ_BYTES) {
		return pldm_fd_reply_cc(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
					resp_payload_len);
	}

	/* Retrieve platform-specific data */
	rc = fd->ops->device_identifiers(fd->ops_ctx, &descriptor_count,
					 &descriptors);
	if (rc) {
		return pldm_fd_reply_cc(PLDM_ERROR, hdr, resp,
					resp_payload_len);
	}

	rc = encode_query_device_identifiers_resp(hdr->instance,
						  descriptor_count, descriptors,
						  resp, resp_payload_len);

	if (rc) {
		return pldm_fd_reply_errno(rc, hdr, resp, resp_payload_len);
	}

	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_fw_param(struct pldm_fd *fd,
			    const struct pldm_header_info *hdr,
			    const struct pldm_msg *req LIBPLDM_CC_UNUSED,
			    size_t req_payload_len, struct pldm_msg *resp,
			    size_t *resp_payload_len)
{
	uint16_t entry_count;
	const struct pldm_firmware_component_standalone **entries;
	PLDM_MSGBUF_DEFINE_P(buf);
	int rc;

	/* No request data */
	if (req_payload_len != PLDM_GET_FIRMWARE_PARAMETERS_REQ_BYTES) {
		return pldm_fd_reply_cc(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
					resp_payload_len);
	}

	/* Retrieve platform-specific data */
	rc = fd->ops->components(fd->ops_ctx, &entry_count, &entries);
	if (rc) {
		return pldm_fd_reply_cc(PLDM_ERROR, hdr, resp,
					resp_payload_len);
	}

	rc = pldm_msgbuf_init_errno(buf, 0, resp->payload, *resp_payload_len);
	if (rc) {
		return rc;
	}

	/* Add the fixed parameters */
	{
		struct pldm_get_firmware_parameters_resp_full fwp = {
			.completion_code = PLDM_SUCCESS,
			// TODO defaulted to 0, could have a callback.
			.capabilities_during_update = { 0 },
			.comp_count = entry_count,
		};
		/* fill active and pending strings */
		rc = fd->ops->imageset_versions(
			fd->ops_ctx, &fwp.active_comp_image_set_ver_str,
			&fwp.pending_comp_image_set_ver_str);
		if (rc) {
			return pldm_msgbuf_discard(
				buf, pldm_fd_reply_cc(PLDM_ERROR, hdr, resp,
						      resp_payload_len));
		}

		size_t len = buf->remaining;
		rc = encode_get_firmware_parameters_resp(hdr->instance, &fwp,
							 resp, &len);
		if (rc) {
			return pldm_msgbuf_discard(
				buf, pldm_fd_reply_errno(rc, hdr, resp,
							 resp_payload_len));
		}
		rc = pldm_msgbuf_skip(buf, len);
		if (rc) {
			return pldm_msgbuf_discard(buf, rc);
		}
	}

	/* Add the component table entries */
	for (uint16_t i = 0; i < entry_count; i++) {
		const struct pldm_firmware_component_standalone *e = entries[i];
		void *out = NULL;
		size_t len;

		struct pldm_component_parameter_entry_full comp = {
			.comp_classification = e->comp_classification,
			.comp_identifier = e->comp_identifier,
			.comp_classification_index =
				e->comp_classification_index,

			.active_ver = e->active_ver,
			.pending_ver = e->pending_ver,

			.comp_activation_methods = e->comp_activation_methods,
			.capabilities_during_update =
				e->capabilities_during_update,
		};

		if (pldm_msgbuf_peek_remaining(buf, &out, &len)) {
			return pldm_msgbuf_discard(buf, rc);
		}

		rc = encode_get_firmware_parameters_resp_comp_entry(&comp, out,
								    &len);
		if (rc) {
			return pldm_msgbuf_discard(
				buf, pldm_fd_reply_errno(rc, hdr, resp,
							 resp_payload_len));
		}

		pldm_msgbuf_skip(buf, len);
	}

	return pldm_msgbuf_complete_used(buf, *resp_payload_len,
					 resp_payload_len);
}

LIBPLDM_CC_NONNULL
static int pldm_fd_request_update(struct pldm_fd *fd,
				  const struct pldm_header_info *hdr,
				  const struct pldm_msg *req,
				  size_t req_payload_len, struct pldm_msg *resp,
				  size_t *resp_payload_len, uint8_t address)
{
	struct pldm_request_update_req_full upd;
	const struct pldm_request_update_resp resp_data = {
		.fd_meta_data_len = 0,
		.fd_will_send_pkg_data = 0,
	};
	int rc;

	if (fd->state != PLDM_FD_STATE_IDLE) {
		return pldm_fd_reply_cc(PLDM_FWUP_ALREADY_IN_UPDATE_MODE, hdr,
					resp, resp_payload_len);
	}

	rc = decode_request_update_req(req, req_payload_len, &upd);
	if (rc) {
		return pldm_fd_reply_errno(rc, hdr, resp, resp_payload_len);
	}

	/* No metadata nor pkg data */
	rc = encode_request_update_resp(hdr->instance, &resp_data, resp,
					resp_payload_len);
	if (rc) {
		return pldm_fd_reply_errno(rc, hdr, resp, resp_payload_len);
	}

	// TODO pass num_of_comp and image_set_ver to application?

	fd->max_transfer =
		fd->ops->transfer_size(fd->ops_ctx, upd.max_transfer_size);
	if (fd->max_transfer > upd.max_transfer_size) {
		/* Limit to UA's size */
		fd->max_transfer = upd.max_transfer_size;
	}
	if (fd->max_transfer < PLDM_FWUP_BASELINE_TRANSFER_SIZE) {
		/* Don't let it be zero, that will loop forever  */
		fd->max_transfer = PLDM_FWUP_BASELINE_TRANSFER_SIZE;
	}
	fd->ua_address = address;
	fd->ua_address_set = true;

	pldm_fd_set_state(fd, PLDM_FD_STATE_LEARN_COMPONENTS);

	return 0;
}

/* wrapper around ops->cancel, will only run ops->cancel when a component update is
 * active */
LIBPLDM_CC_NONNULL
static void pldm_fd_maybe_cancel_component(struct pldm_fd *fd)
{
	bool cancel = false;

	switch (fd->state) {
	case PLDM_FD_STATE_DOWNLOAD:
	case PLDM_FD_STATE_VERIFY:
		cancel = true;
		break;
	case PLDM_FD_STATE_APPLY:
		/* In apply state, once the application ops->apply() has completed
		 * successfully the component is no longer in update state.
		 * In that case the cancel should not be forwarded to the application.
		 * This can occur if a cancel is received while waiting for the
		 * response to a success ApplyComplete. */
		cancel = !(fd->req.complete &&
			   fd->req.result == PLDM_FWUP_APPLY_SUCCESS);
		break;
	default:
		break;
	}

	if (cancel) {
		/* Call the platform handler for the current component in progress */
		fd->ops->cancel_update_component(fd->ops_ctx, &fd->update_comp);
	}
}

/* Wrapper around ops->update_component() that first checks that the component
 * is in the list returned from ops->components() */
LIBPLDM_CC_NONNULL
static enum pldm_component_response_codes pldm_fd_check_update_component(
	struct pldm_fd *fd, bool update,
	const struct pldm_firmware_update_component *comp)
{
	bool found;
	uint16_t entry_count;
	int rc;

	const struct pldm_firmware_component_standalone **entries;
	rc = fd->ops->components(fd->ops_ctx, &entry_count, &entries);
	if (rc) {
		return PLDM_CRC_COMP_NOT_SUPPORTED;
	}

	found = false;
	for (uint16_t i = 0; i < entry_count; i++) {
		if (entries[i]->comp_classification ==
			    comp->comp_classification &&
		    entries[i]->comp_identifier == comp->comp_identifier &&
		    entries[i]->comp_classification_index ==
			    comp->comp_classification_index) {
			found = true;
			break;
		}
	}
	if (found) {
		return fd->ops->update_component(fd->ops_ctx, update, comp);
	}
	return PLDM_CRC_COMP_NOT_SUPPORTED;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_pass_comp(struct pldm_fd *fd,
			     const struct pldm_header_info *hdr,
			     const struct pldm_msg *req, size_t req_payload_len,
			     struct pldm_msg *resp, size_t *resp_payload_len)
{
	struct pldm_pass_component_table_req_full pcomp;
	uint8_t comp_response_code;
	int rc;

	if (fd->state != PLDM_FD_STATE_LEARN_COMPONENTS) {
		return pldm_fd_reply_cc(PLDM_FWUP_INVALID_STATE_FOR_COMMAND,
					hdr, resp, resp_payload_len);
	}

	/* fd->update_comp is used as temporary storage during PassComponent validation */
	/* Some portions are unused for PassComponentTable */
	fd->update_comp.comp_image_size = 0;
	fd->update_comp.update_option_flags.value = 0;

	rc = decode_pass_component_table_req(req, req_payload_len, &pcomp);
	if (rc) {
		return pldm_fd_reply_errno(rc, hdr, resp, resp_payload_len);
	}

	fd->update_comp.comp_classification = pcomp.comp_classification;
	fd->update_comp.comp_identifier = pcomp.comp_identifier;
	fd->update_comp.comp_classification_index =
		pcomp.comp_classification_index;
	fd->update_comp.comp_comparison_stamp = pcomp.comp_comparison_stamp;
	memcpy(&fd->update_comp.version, &pcomp.version, sizeof(pcomp.version));

	comp_response_code =
		pldm_fd_check_update_component(fd, false, &fd->update_comp);

	const struct pldm_pass_component_table_resp resp_data = {
		/* Component Response Code is 0 for ComponentResponse, 1 otherwise */
		.comp_resp = (comp_response_code != 0),
		.comp_resp_code = comp_response_code,
	};

	rc = encode_pass_component_table_resp(hdr->instance, &resp_data, resp,
					      resp_payload_len);
	if (rc) {
		return pldm_fd_reply_errno(rc, hdr, resp, resp_payload_len);
	}

	if (pcomp.transfer_flag & PLDM_END) {
		pldm_fd_set_state(fd, PLDM_FD_STATE_READY_XFER);
	}

	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_update_comp(struct pldm_fd *fd,
			       const struct pldm_header_info *hdr,
			       const struct pldm_msg *req,
			       size_t req_payload_len, struct pldm_msg *resp,
			       size_t *resp_payload_len)
{
	struct pldm_update_component_req_full up;
	uint8_t comp_response_code;
	int rc;

	if (fd->state != PLDM_FD_STATE_READY_XFER) {
		return pldm_fd_reply_cc(PLDM_FWUP_INVALID_STATE_FOR_COMMAND,
					hdr, resp, resp_payload_len);
	}

	rc = decode_update_component_req(req, req_payload_len, &up);
	if (rc) {
		return pldm_fd_reply_errno(rc, hdr, resp, resp_payload_len);
	}

	/* Store update_comp to pass to further callbacks. This persists
	 * until the component update completes or is cancelled */
	fd->update_comp.comp_classification = up.comp_classification;
	fd->update_comp.comp_identifier = up.comp_identifier;
	fd->update_comp.comp_classification_index =
		up.comp_classification_index;
	fd->update_comp.comp_comparison_stamp = up.comp_comparison_stamp;
	fd->update_comp.comp_image_size = up.comp_image_size;
	fd->update_comp.update_option_flags = up.update_option_flags;
	memcpy(&fd->update_comp.version, &up.version, sizeof(up.version));

	comp_response_code =
		pldm_fd_check_update_component(fd, true, &fd->update_comp);

	// Mask to only the "Force Update" flag, others are not handled.
	bitfield32_t update_flags = {
		.bits.bit0 = fd->update_comp.update_option_flags.bits.bit0
	};

	const struct pldm_update_component_resp resp_data = {
		/* Component Response Code is 0 for ComponentResponse, 1 otherwise */
		.comp_compatibility_resp = (comp_response_code != 0),
		.comp_compatibility_resp_code = comp_response_code,
		.update_option_flags_enabled = update_flags,
		.time_before_req_fw_data = 0,
	};

	rc = encode_update_component_resp(hdr->instance, &resp_data, resp,
					  resp_payload_len);
	if (rc) {
		/* Encoding response failed */
		if (comp_response_code == PLDM_CRC_COMP_CAN_BE_UPDATED) {
			/* Inform the application of cancellation. Call it directly
			 * rather than going through pldm_fd_maybe_cancel_component() */
			fd->ops->cancel_update_component(fd->ops_ctx,
							 &fd->update_comp);
		}
		return pldm_fd_reply_errno(rc, hdr, resp, resp_payload_len);
	}

	/* Set up download state */
	if (comp_response_code == PLDM_CRC_COMP_CAN_BE_UPDATED) {
		memset(&fd->specific, 0x0, sizeof(fd->specific));
		fd->update_flags = update_flags;
		fd->req.state = PLDM_FD_REQ_READY;
		fd->req.complete = false;
		pldm_fd_set_state(fd, PLDM_FD_STATE_DOWNLOAD);
	}

	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_get_status(struct pldm_fd *fd,
			      const struct pldm_header_info *hdr,
			      const struct pldm_msg *req LIBPLDM_CC_UNUSED,
			      size_t req_payload_len, struct pldm_msg *resp,
			      size_t *resp_payload_len)
{
	int rc;

	/* No request data */
	if (req_payload_len != PLDM_GET_STATUS_REQ_BYTES) {
		return pldm_fd_reply_cc(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
					resp_payload_len);
	}

	/* Defaults */
	struct pldm_get_status_resp st = {
		.current_state = fd->state,
		.previous_state = fd->prev_state,
		.progress_percent = PROGRESS_PERCENT_NOT_SUPPORTED,
	};

	pldm_fd_get_aux_state(fd, &st.aux_state, &st.aux_state_status);

	switch (fd->state) {
	case PLDM_FD_STATE_IDLE:
		st.reason_code = fd->reason;
		break;
	case PLDM_FD_STATE_DOWNLOAD:
		if (fd->update_comp.comp_image_size > 0) {
			uint32_t one_percent =
				fd->update_comp.comp_image_size / 100;
			if (fd->update_comp.comp_image_size % 100 != 0) {
				one_percent += 1;
			}
			st.progress_percent =
				(fd->specific.download.offset / one_percent);
		}
		st.update_option_flags_enabled = fd->update_flags;
		break;
	case PLDM_FD_STATE_VERIFY:
		st.update_option_flags_enabled = fd->update_flags;
		st.progress_percent = fd->specific.verify.progress_percent;
		break;
	case PLDM_FD_STATE_APPLY:
		st.update_option_flags_enabled = fd->update_flags;
		st.progress_percent = fd->specific.apply.progress_percent;
		break;
	default:
		break;
	}

	rc = encode_get_status_resp(hdr->instance, &st, resp, resp_payload_len);
	if (rc) {
		return pldm_fd_reply_errno(rc, hdr, resp, resp_payload_len);
	}

	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_cancel_update_comp(
	struct pldm_fd *fd, const struct pldm_header_info *hdr,
	const struct pldm_msg *req LIBPLDM_CC_UNUSED, size_t req_payload_len,
	struct pldm_msg *resp, size_t *resp_payload_len)
{
	int rc;

	/* No request data */
	if (req_payload_len != PLDM_CANCEL_UPDATE_COMPONENT_REQ_BYTES) {
		return pldm_fd_reply_cc(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
					resp_payload_len);
	}

	switch (fd->state) {
	case PLDM_FD_STATE_DOWNLOAD:
	case PLDM_FD_STATE_VERIFY:
	case PLDM_FD_STATE_APPLY:
		break;
	default:
		return pldm_fd_reply_cc(PLDM_FWUP_NOT_IN_UPDATE_MODE, hdr, resp,
					resp_payload_len);
	}

	/* No response payload */
	rc = pldm_fd_reply_cc(PLDM_SUCCESS, hdr, resp, resp_payload_len);
	if (rc) {
		return rc;
	}

	pldm_fd_maybe_cancel_component(fd);
	pldm_fd_set_state(fd, PLDM_FD_STATE_READY_XFER);

	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_cancel_update(struct pldm_fd *fd,
				 const struct pldm_header_info *hdr,
				 const struct pldm_msg *req LIBPLDM_CC_UNUSED,
				 size_t req_payload_len, struct pldm_msg *resp,
				 size_t *resp_payload_len)
{
	int rc;

	/* No request data */
	if (req_payload_len != PLDM_CANCEL_UPDATE_REQ_BYTES) {
		return pldm_fd_reply_cc(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
					resp_payload_len);
	}

	if (fd->state == PLDM_FD_STATE_IDLE) {
		return pldm_fd_reply_cc(PLDM_FWUP_NOT_IN_UPDATE_MODE, hdr, resp,
					resp_payload_len);
	}

	/* Assume non_functioning_component_indication = False, in future
	 * could add a platform callback */
	const struct pldm_cancel_update_resp resp_data = {
		.non_functioning_component_indication = 0,
		.non_functioning_component_bitmap = 0,
	};
	rc = encode_cancel_update_resp(hdr->instance, &resp_data, resp,
				       resp_payload_len);
	if (rc) {
		return pldm_fd_reply_errno(rc, hdr, resp, resp_payload_len);
	}

	pldm_fd_maybe_cancel_component(fd);
	pldm_fd_set_idle(fd, PLDM_FD_CANCEL_UPDATE);

	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_activate_firmware(struct pldm_fd *fd,
				     const struct pldm_header_info *hdr,
				     const struct pldm_msg *req,
				     size_t req_payload_len,
				     struct pldm_msg *resp,
				     size_t *resp_payload_len)
{
	uint16_t estimated_time;
	uint8_t ccode;
	int rc;
	bool self_contained;

	rc = decode_activate_firmware_req(req, req_payload_len,
					  &self_contained);
	if (rc) {
		return pldm_fd_reply_errno(rc, hdr, resp, resp_payload_len);
	}

	if (fd->state != PLDM_FD_STATE_READY_XFER) {
		return pldm_fd_reply_cc(PLDM_FWUP_INVALID_STATE_FOR_COMMAND,
					hdr, resp, resp_payload_len);
	}

	estimated_time = 0;
	ccode = fd->ops->activate(fd->ops_ctx, self_contained, &estimated_time);

	if (ccode == PLDM_SUCCESS ||
	    ccode == PLDM_FWUP_ACTIVATION_NOT_REQUIRED) {
		/* Transition through states so that the prev_state is correct */
		pldm_fd_set_state(fd, PLDM_FD_STATE_ACTIVATE);
		pldm_fd_set_idle(fd, PLDM_FD_ACTIVATE_FW);
	}

	if (ccode == PLDM_SUCCESS) {
		const struct pldm_activate_firmware_resp resp_data = {
			.estimated_time_activation = estimated_time,
		};
		rc = encode_activate_firmware_resp(hdr->instance, &resp_data,
						   resp, resp_payload_len);
		if (rc) {
			return pldm_fd_reply_errno(rc, hdr, resp,
						   resp_payload_len);
		}
	} else {
		return pldm_fd_reply_cc(ccode, hdr, resp, resp_payload_len);
	}

	return 0;
}

LIBPLDM_CC_NONNULL
static uint32_t pldm_fd_fwdata_size(struct pldm_fd *fd)
{
	uint32_t size;

	if (fd->state != PLDM_FD_STATE_DOWNLOAD) {
		assert(false);
		return 0;
	}

	if (fd->specific.download.offset > fd->update_comp.comp_image_size) {
		assert(false);
		return 0;
	}
	size = fd->update_comp.comp_image_size - fd->specific.download.offset;

	if (size > fd->max_transfer) {
		size = fd->max_transfer;
	}
	return size;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_handle_fwdata_resp(struct pldm_fd *fd,
				      const struct pldm_msg *resp,
				      size_t resp_payload_len)
{
	struct pldm_fd_download *dl;
	uint32_t fwdata_size;
	uint8_t res;

	if (fd->state != PLDM_FD_STATE_DOWNLOAD) {
		return -EPROTO;
	}

	if (fd->req.state != PLDM_FD_REQ_SENT) {
		/* Not waiting for a response, ignore it */
		return -EPROTO;
	}

	dl = &fd->specific.download;
	if (fd->req.complete) {
		/* Received data after completion */
		return -EPROTO;
	}

	switch (resp->payload[0]) {
	case PLDM_SUCCESS:
		break;
	case PLDM_FWUP_RETRY_REQUEST_FW_DATA:
		/* Just return, let the retry timer send another request later */
		return 0;
	default:
		/* Send a TransferComplete failure */
		fd->req.state = PLDM_FD_REQ_READY;
		fd->req.complete = true;
		fd->req.result = PLDM_FWUP_FD_ABORTED_TRANSFER;
		return 0;
	}

	/* Handle the received data */

	fwdata_size = pldm_fd_fwdata_size(fd);
	if (resp_payload_len != fwdata_size + 1) {
		/* Data is incorrect size. Could indicate MCTP corruption, drop it
		 * and let retry timer handle it */
		return -EOVERFLOW;
	}

	/* Check pldm_fd_fwdata_size calculation, should not fail */
	if (dl->offset + fwdata_size < dl->offset ||
	    dl->offset + fwdata_size > fd->update_comp.comp_image_size) {
		assert(false);
		return -EINVAL;
	}

	/* Provide the data chunk to the device */
	res = fd->ops->firmware_data(fd->ops_ctx, dl->offset, &resp->payload[1],
				     fwdata_size, &fd->update_comp);

	fd->req.state = PLDM_FD_REQ_READY;
	if (res == PLDM_FWUP_TRANSFER_SUCCESS) {
		/* Move to next offset */
		dl->offset += fwdata_size;
		if (dl->offset == fd->update_comp.comp_image_size) {
			/* Mark as complete, next progress() call will send the TransferComplete request */
			fd->req.complete = true;
			fd->req.result = PLDM_FWUP_TRANSFER_SUCCESS;
		}
	} else {
		/* Pass the callback error as the TransferResult */
		fd->req.complete = true;
		fd->req.result = res;
	}

	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_handle_transfer_complete_resp(
	struct pldm_fd *fd, const struct pldm_msg *resp LIBPLDM_CC_UNUSED,
	size_t resp_payload_len LIBPLDM_CC_UNUSED)
{
	if (fd->state != PLDM_FD_STATE_DOWNLOAD) {
		return -EPROTO;
	}

	if (fd->req.state != PLDM_FD_REQ_SENT) {
		/* Not waiting for a response, ignore it */
		return -EPROTO;
	}

	if (!fd->req.complete) {
		/* Were waiting for RequestFirmwareData instead, ignore it */
		return -EPROTO;
	}

	/* Disregard the response completion code */

	/* Next state depends whether the transfer succeeded */
	if (fd->req.result == PLDM_FWUP_TRANSFER_SUCCESS) {
		/* Switch to Verify */
		memset(&fd->specific, 0x0, sizeof(fd->specific));
		fd->specific.verify.progress_percent =
			PROGRESS_PERCENT_NOT_SUPPORTED;
		fd->req.state = PLDM_FD_REQ_READY;
		fd->req.complete = false;
		pldm_fd_set_state(fd, PLDM_FD_STATE_VERIFY);
	} else {
		/* Wait for UA to cancel */
		fd->req.state = PLDM_FD_REQ_FAILED;
	}
	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_handle_verify_complete_resp(
	struct pldm_fd *fd, const struct pldm_msg *resp LIBPLDM_CC_UNUSED,
	size_t resp_payload_len LIBPLDM_CC_UNUSED)
{
	if (fd->state != PLDM_FD_STATE_VERIFY) {
		return -EPROTO;
	}

	if (fd->req.state != PLDM_FD_REQ_SENT) {
		/* Not waiting for a response, ignore it */
		return -EPROTO;
	}

	assert(fd->req.complete);

	/* Disregard the response completion code */

	/* Next state depends whether the verify succeeded */
	if (fd->req.result == PLDM_FWUP_VERIFY_SUCCESS) {
		/* Switch to Apply */
		memset(&fd->specific, 0x0, sizeof(fd->specific));
		fd->specific.apply.progress_percent =
			PROGRESS_PERCENT_NOT_SUPPORTED;
		fd->req.state = PLDM_FD_REQ_READY;
		fd->req.complete = false;
		pldm_fd_set_state(fd, PLDM_FD_STATE_APPLY);
	} else {
		/* Wait for UA to cancel */
		fd->req.state = PLDM_FD_REQ_FAILED;
	}
	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_handle_apply_complete_resp(
	struct pldm_fd *fd, const struct pldm_msg *resp LIBPLDM_CC_UNUSED,
	size_t resp_payload_len LIBPLDM_CC_UNUSED)
{
	if (fd->state != PLDM_FD_STATE_APPLY) {
		return -EPROTO;
	}

	if (fd->req.state != PLDM_FD_REQ_SENT) {
		/* Not waiting for a response, ignore it */
		return -EPROTO;
	}

	assert(fd->req.complete);

	/* Disregard the response completion code */

	/* Next state depends whether the apply succeeded */
	if (fd->req.result == PLDM_FWUP_APPLY_SUCCESS) {
		/* Switch to ReadyXfer */
		fd->req.state = PLDM_FD_REQ_UNUSED;
		pldm_fd_set_state(fd, PLDM_FD_STATE_READY_XFER);
	} else {
		/* Wait for UA to cancel */
		fd->req.state = PLDM_FD_REQ_FAILED;
	}
	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_handle_resp(struct pldm_fd *fd, pldm_tid_t address,
			       const void *resp_msg, size_t resp_len)
{
	size_t resp_payload_len;
	const struct pldm_msg *resp = resp_msg;

	if (!(fd->ua_address_set && fd->ua_address == address)) {
		// Either an early response, or a response from a wrong TID */
		return -EBUSY;
	}

	/* Must have a ccode */
	if (resp_len < sizeof(struct pldm_msg_hdr) + 1) {
		return -EINVAL;
	}
	resp_payload_len = resp_len - sizeof(struct pldm_msg_hdr);

	if (fd->req.state != PLDM_FD_REQ_SENT) {
		// No response was expected
		return -EPROTO;
	}

	if (fd->req.instance_id != resp->hdr.instance_id) {
		// Response wasn't for the expected request
		return -EPROTO;
	}
	if (fd->req.command != resp->hdr.command) {
		// Response wasn't for the expected request
		return -EPROTO;
	}

	fd->update_timestamp_fd_t1 = pldm_fd_now(fd);

	switch (resp->hdr.command) {
	case PLDM_REQUEST_FIRMWARE_DATA:
		return pldm_fd_handle_fwdata_resp(fd, resp, resp_payload_len);
		break;
	case PLDM_TRANSFER_COMPLETE:
		return pldm_fd_handle_transfer_complete_resp(fd, resp,
							     resp_payload_len);
		break;
	case PLDM_VERIFY_COMPLETE:
		return pldm_fd_handle_verify_complete_resp(fd, resp,
							   resp_payload_len);
		break;
	case PLDM_APPLY_COMPLETE:
		return pldm_fd_handle_apply_complete_resp(fd, resp,
							  resp_payload_len);
		break;
	default:
		/* Unsolicited response. Already compared to command above */
		assert(false);
		return -EINVAL;
	}
}

LIBPLDM_CC_NONNULL
static int pldm_fd_progress_download(struct pldm_fd *fd, struct pldm_msg *req,
				     size_t *req_payload_len)
{
	uint8_t instance_id;
	struct pldm_fd_download *dl;
	int rc;

	if (!pldm_fd_req_should_send(fd)) {
		/* Nothing to do */
		*req_payload_len = 0;
		return 0;
	}

	instance_id = pldm_fd_req_next_instance(&fd->req);
	dl = &fd->specific.download;
	if (fd->req.complete) {
		/* Send TransferComplete */
		rc = encode_transfer_complete_req(instance_id, fd->req.result,
						  req, req_payload_len);
	} else {
		/* Send a new RequestFirmwareData */
		const struct pldm_request_firmware_data_req req_params = {
			.offset = dl->offset,
			.length = pldm_fd_fwdata_size(fd),
		};

		rc = encode_request_firmware_data_req(instance_id, &req_params,
						      req, req_payload_len);
	}

	if (rc) {
		return rc;
	}

	/* Wait for response */
	fd->req.state = PLDM_FD_REQ_SENT;
	fd->req.instance_id = req->hdr.instance_id;
	fd->req.command = req->hdr.command;
	fd->req.sent_time = pldm_fd_now(fd);

	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_progress_verify(struct pldm_fd *fd, struct pldm_msg *req,
				   size_t *req_payload_len)
{
	uint8_t instance_id;
	int rc;

	if (!pldm_fd_req_should_send(fd)) {
		/* Nothing to do */
		*req_payload_len = 0;
		return 0;
	}

	if (!fd->req.complete) {
		bool pending = false;
		uint8_t res;
		res = fd->ops->verify(fd->ops_ctx, &fd->update_comp, &pending,
				      &fd->specific.verify.progress_percent);
		if (pending) {
			if (res == PLDM_FWUP_VERIFY_SUCCESS) {
				/* Return without a VerifyComplete request.
				* Will call verify() again on next call */
				*req_payload_len = 0;
				return 0;
			}
			/* This is an API infraction by the implementer, return a distinctive failure */
			res = PLDM_FWUP_VENDOR_VERIFY_RESULT_RANGE_MAX;
		}
		fd->req.result = res;
		fd->req.complete = true;
	}

	instance_id = pldm_fd_req_next_instance(&fd->req);
	rc = encode_verify_complete_req(instance_id, fd->req.result, req,
					req_payload_len);
	if (rc) {
		return rc;
	}

	/* Wait for response */
	fd->req.state = PLDM_FD_REQ_SENT;
	fd->req.instance_id = req->hdr.instance_id;
	fd->req.command = req->hdr.command;
	fd->req.sent_time = pldm_fd_now(fd);

	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_fd_progress_apply(struct pldm_fd *fd, struct pldm_msg *req,
				  size_t *req_payload_len)
{
	uint8_t instance_id;
	int rc;

	if (!pldm_fd_req_should_send(fd)) {
		/* Nothing to do */
		*req_payload_len = 0;
		return 0;
	}

	if (!fd->req.complete) {
		bool pending = false;
		uint8_t res;
		res = fd->ops->apply(fd->ops_ctx, &fd->update_comp, &pending,
				     &fd->specific.apply.progress_percent);
		if (pending) {
			if (res == PLDM_FWUP_APPLY_SUCCESS) {
				/* Return without a ApplyComplete request.
				* Will call apply() again on next call */
				*req_payload_len = 0;
				return 0;
			}
			/* This is an API infraction by the implementer, return a distinctive failure */
			res = PLDM_FWUP_VENDOR_APPLY_RESULT_RANGE_MAX;
		}
		fd->req.result = res;
		fd->req.complete = true;
		if (fd->req.result ==
		    PLDM_FWUP_APPLY_SUCCESS_WITH_ACTIVATION_METHOD) {
			/* modified activation method isn't currently handled */
			fd->req.result = PLDM_FWUP_APPLY_SUCCESS;
		}
	}

	instance_id = pldm_fd_req_next_instance(&fd->req);
	const struct pldm_apply_complete_req req_data = {
		.apply_result = fd->req.result,
		.comp_activation_methods_modification = { 0 },
	};
	rc = encode_apply_complete_req(instance_id, &req_data, req,
				       req_payload_len);
	if (rc) {
		return rc;
	}

	/* Wait for response */
	fd->req.state = PLDM_FD_REQ_SENT;
	fd->req.instance_id = req->hdr.instance_id;
	fd->req.command = req->hdr.command;
	fd->req.sent_time = pldm_fd_now(fd);

	return 0;
}

LIBPLDM_ABI_TESTING
struct pldm_fd *pldm_fd_new(const struct pldm_fd_ops *ops, void *ops_ctx,
			    struct pldm_control *control)
{
	struct pldm_fd *fd = malloc(sizeof(*fd));
	if (fd) {
		if (pldm_fd_setup(fd, sizeof(*fd), ops, ops_ctx, control) ==
		    0) {
			return fd;
		}
		free(fd);
		fd = NULL;
	}
	return fd;
}

LIBPLDM_ABI_TESTING
int pldm_fd_setup(struct pldm_fd *fd, size_t pldm_fd_size,
		  const struct pldm_fd_ops *ops, void *ops_ctx,
		  struct pldm_control *control)
{
	int rc;

	if (fd == NULL || ops == NULL) {
		return -EINVAL;
	}

	if (ops->device_identifiers == NULL || ops->components == NULL ||
	    ops->imageset_versions == NULL || ops->update_component == NULL ||
	    ops->transfer_size == NULL || ops->firmware_data == NULL ||
	    ops->verify == NULL || ops->activate == NULL ||
	    ops->cancel_update_component == NULL || ops->now == NULL) {
		return -EINVAL;
	}

	if (pldm_fd_size < sizeof(struct pldm_fd)) {
		/* Safety check that sufficient storage was provided for *fd,
		 * in case PLDM_SIZEOF_PLDM_FD is incorrect */
		return -EINVAL;
	}
	memset(fd, 0x0, sizeof(*fd));
	fd->ops = ops;
	fd->ops_ctx = ops_ctx;
	fd->fd_t1_timeout = DEFAULT_FD_T1_TIMEOUT;
	fd->fd_t2_retry_time = DEFAULT_FD_T2_RETRY_TIME;

	if (control) {
		rc = pldm_control_add_type(control, PLDM_FWUP,
					   &PLDM_FD_VERSIONS,
					   PLDM_FD_VERSIONS_COUNT,
					   PLDM_FD_COMMANDS);
		if (rc) {
			return rc;
		}
	}

	return 0;
}

LIBPLDM_ABI_TESTING
int pldm_fd_handle_msg(struct pldm_fd *fd, pldm_tid_t remote_address,
		       const void *in_msg, size_t in_len, void *out_msg,
		       size_t *out_len)
{
	size_t req_payload_len;
	size_t resp_payload_len;
	struct pldm_header_info hdr;
	const struct pldm_msg *req = in_msg;
	struct pldm_msg *resp = out_msg;
	uint8_t rc;

	if (fd == NULL || in_msg == NULL || out_msg == NULL ||
	    out_len == NULL) {
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

	if (hdr.pldm_type != PLDM_FWUP) {
		/* Caller should not have passed non-pldmfw */
		return -ENOMSG;
	}

	if (hdr.msg_type == PLDM_RESPONSE) {
		*out_len = 0;
		return pldm_fd_handle_resp(fd, remote_address, in_msg, in_len);
	}

	if (hdr.msg_type != PLDM_REQUEST) {
		return -EPROTO;
	}

	/* Space for header plus completion code */
	if (*out_len < sizeof(struct pldm_msg_hdr) + 1) {
		return -EOVERFLOW;
	}
	resp_payload_len = *out_len - sizeof(struct pldm_msg_hdr);

	/* Check address */
	switch (hdr.command) {
	/* Information or cancel commands are always allowed */
	case PLDM_QUERY_DEVICE_IDENTIFIERS:
	case PLDM_GET_FIRMWARE_PARAMETERS:
	case PLDM_GET_STATUS:
	case PLDM_CANCEL_UPDATE:
	case PLDM_QUERY_DOWNSTREAM_DEVICES:
	case PLDM_QUERY_DOWNSTREAM_IDENTIFIERS:
	case PLDM_QUERY_DOWNSTREAM_FIRMWARE_PARAMETERS:
	/* Request Update handler will set address */
	case PLDM_REQUEST_UPDATE:
		break;
	default:
		/* Requests must come from the same address that requested the update */
		if (!fd->ua_address_set || remote_address != fd->ua_address) {
			return pldm_fd_reply_cc(PLDM_ERROR_NOT_READY, &hdr,
						resp, &resp_payload_len);
		}
	}

	/* Update timeout */
	switch (hdr.command) {
	case PLDM_REQUEST_UPDATE:
	case PLDM_PASS_COMPONENT_TABLE:
	case PLDM_UPDATE_COMPONENT:
	case PLDM_CANCEL_UPDATE:
		fd->update_timestamp_fd_t1 = pldm_fd_now(fd);
		break;
	default:
		break;
	}

	/* Dispatch command.
	 Update PLDM_FD_COMMANDS if adding new handlers */
	switch (hdr.command) {
	case PLDM_QUERY_DEVICE_IDENTIFIERS:
		rc = pldm_fd_qdi(fd, &hdr, req, req_payload_len, resp,
				 &resp_payload_len);
		break;
	case PLDM_GET_FIRMWARE_PARAMETERS:
		rc = pldm_fd_fw_param(fd, &hdr, req, req_payload_len, resp,
				      &resp_payload_len);
		break;
	case PLDM_REQUEST_UPDATE:
		rc = pldm_fd_request_update(fd, &hdr, req, req_payload_len,
					    resp, &resp_payload_len,
					    remote_address);
		break;
	case PLDM_PASS_COMPONENT_TABLE:
		rc = pldm_fd_pass_comp(fd, &hdr, req, req_payload_len, resp,
				       &resp_payload_len);
		break;
	case PLDM_UPDATE_COMPONENT:
		rc = pldm_fd_update_comp(fd, &hdr, req, req_payload_len, resp,
					 &resp_payload_len);
		break;
	case PLDM_GET_STATUS:
		rc = pldm_fd_get_status(fd, &hdr, req, req_payload_len, resp,
					&resp_payload_len);
		break;
	case PLDM_CANCEL_UPDATE_COMPONENT:
		rc = pldm_fd_cancel_update_comp(fd, &hdr, req, req_payload_len,
						resp, &resp_payload_len);
		break;
	case PLDM_CANCEL_UPDATE:
		rc = pldm_fd_cancel_update(fd, &hdr, req, req_payload_len, resp,
					   &resp_payload_len);
		break;
	case PLDM_ACTIVATE_FIRMWARE:
		rc = pldm_fd_activate_firmware(fd, &hdr, req, req_payload_len,
					       resp, &resp_payload_len);
		break;
	default:
		rc = pldm_fd_reply_cc(PLDM_ERROR_UNSUPPORTED_PLDM_CMD, &hdr,
				      resp, &resp_payload_len);
	}

	if (rc == 0) {
		*out_len = resp_payload_len + sizeof(struct pldm_msg_hdr);
	}

	return rc;
}

LIBPLDM_ABI_TESTING
int pldm_fd_progress(struct pldm_fd *fd, void *out_msg, size_t *out_len,
		     pldm_tid_t *address)
{
	size_t req_payload_len;
	struct pldm_msg *req = out_msg;
	int rc = -EINVAL;
	bool ua_timeout_check = false;

	if (fd == NULL || out_msg == NULL || out_len == NULL) {
		return -EINVAL;
	}

	/* Space for header */
	if (*out_len < sizeof(struct pldm_msg_hdr)) {
		return -EOVERFLOW;
	}
	req_payload_len = *out_len - sizeof(struct pldm_msg_hdr);
	*out_len = 0;

	// Handle FD-driven states
	switch (fd->state) {
	case PLDM_FD_STATE_DOWNLOAD:
		rc = pldm_fd_progress_download(fd, req, &req_payload_len);
		break;
	case PLDM_FD_STATE_VERIFY:
		rc = pldm_fd_progress_verify(fd, req, &req_payload_len);
		break;
	case PLDM_FD_STATE_APPLY:
		rc = pldm_fd_progress_apply(fd, req, &req_payload_len);
		break;
	default:
		req_payload_len = 0;
		break;
	}

	// Cancel update if expected UA message isn't received within timeout
	switch (fd->state) {
	case PLDM_FD_STATE_DOWNLOAD:
	case PLDM_FD_STATE_VERIFY:
	case PLDM_FD_STATE_APPLY:
		// FD-driven states will time out if a response isn't received
		ua_timeout_check = (fd->req.state == PLDM_FD_REQ_SENT);
		break;
	case PLDM_FD_STATE_IDLE:
		ua_timeout_check = false;
		break;
	default:
		// Other Update Mode states have a timeout for the UA to
		// send a request
		ua_timeout_check = true;
	}

	if (ua_timeout_check) {
		if ((pldm_fd_now(fd) - fd->update_timestamp_fd_t1) >
		    fd->fd_t1_timeout) {
			pldm_fd_maybe_cancel_component(fd);
			pldm_fd_idle_timeout(fd);
			req_payload_len = 0;
		}
	}

	if (rc == 0 && fd->ua_address_set && req_payload_len > 0) {
		*out_len = req_payload_len + sizeof(struct pldm_msg_hdr);
		*address = fd->ua_address;
	}

	return rc;
}

LIBPLDM_ABI_TESTING
int pldm_fd_set_update_idle_timeout(struct pldm_fd *fd, uint32_t time)
{
	if (fd == NULL) {
		return -EINVAL;
	}

	fd->fd_t1_timeout = time;
	return 0;
}

LIBPLDM_ABI_TESTING
int pldm_fd_set_request_retry_time(struct pldm_fd *fd, uint32_t time)
{
	if (fd == NULL) {
		return -EINVAL;
	}

	fd->fd_t2_retry_time = time;
	return 0;
}
