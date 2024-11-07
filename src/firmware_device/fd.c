#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <libpldm/pldm.h>
#include <libpldm/firmware_update.h>
#include <libpldm/firmware_fd.h>
#include <libpldm/utils.h>
#include <compiler.h>
#include <msgbuf.h>

#include "fd-internal.h"

/* 1 second.
 * DSP0240 allows T2 in [300ms, 4800ms].
 * This same timeout is used for
 * FD_T2 "Retry request for firmware data", which [1s, 5s]. */
static const pldm_fd_time_t RETRY_TIME = 1000;

static const uint8_t INSTANCE_ID_COUNT = 32;
static const uint8_t PROGRESS_PERCENT_NOT_SUPPORTED = 101;

static pldm_requester_rc_t
pldm_fd_reply_error(uint8_t ccode, const struct pldm_header_info *req_hdr,
		    struct pldm_msg *resp, size_t *resp_payload_len)
{
	int rc;

	/* 1 byte completion code */
	if (*resp_payload_len < 1) {
		return PLDM_REQUESTER_RESP_MSG_TOO_SMALL;
	}
	*resp_payload_len = 1;

	rc = encode_cc_only_resp(req_hdr->instance, PLDM_FWUP, req_hdr->command,
				 ccode, resp);
	if (rc != PLDM_SUCCESS) {
		return PLDM_REQUESTER_RECV_FAIL;
	}
	return PLDM_REQUESTER_SUCCESS;
}

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

static void pldm_fd_set_idle(struct pldm_fd *fd,
			     enum pldm_get_status_reason_code_values reason)
{
	fd->prev_state = fd->state;
	fd->state = PLDM_FD_STATE_IDLE;
	fd->reason = reason;
	fd->ua_address_set = false;
}

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

static bool pldm_fd_req_should_send(struct pldm_fd_req *req, pldm_fd_time_t now)
{
	switch (req->state) {
	case PLDM_FD_REQ_UNUSED:
		assert(0);
		return false;
	case PLDM_FD_REQ_READY:
		return true;
	case PLDM_FD_REQ_FAILED:
		return false;
	case PLDM_FD_REQ_SENT:
		if (now < req->sent_time) {
			/* Time went backwards */
			return false;
		}

		/* Send if retry time has elapsed */
		bool send = (now - req->sent_time) >= RETRY_TIME;
		return send;
	}
	return false;
}

/* Allocate the next instance ID. Only one request is outstanding so cycling
 * through the range is OK */
static uint8_t pldm_fd_req_next_instance(struct pldm_fd_req *req)
{
	req->instance_id = (req->instance_id + 1) % INSTANCE_ID_COUNT;
	return req->instance_id;
}

static uint64_t pldm_fd_now(struct pldm_fd *fd)
{
	return fd->ops->now(fd->ops_ctx);
}

static pldm_requester_rc_t
pldm_fd_qdi(struct pldm_fd *fd, const struct pldm_header_info *hdr,
	    const struct pldm_msg *req LIBPLDM_CC_UNUSED,
	    size_t req_payload_len, struct pldm_msg *resp,
	    size_t *resp_payload_len)
{
	uint8_t ccode;

	/* QDI has no request data */
	if (req_payload_len != PLDM_QUERY_DEVICE_IDENTIFIERS_REQ_BYTES) {
		return pldm_fd_reply_error(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
					   resp_payload_len);
	}

	/* Retrieve platform-specific data */
	uint32_t descriptors_len;
	uint8_t descriptor_count;
	const uint8_t *descriptors;
	ccode = fd->ops->device_identifiers(fd->ops_ctx, &descriptors_len,
					    &descriptor_count, &descriptors);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	ccode = encode_query_device_identifiers_resp(
		hdr->instance, descriptors_len, descriptor_count, descriptors,
		resp, resp_payload_len);

	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t
pldm_fd_fw_param(struct pldm_fd *fd, const struct pldm_header_info *hdr,
		 const struct pldm_msg *req LIBPLDM_CC_UNUSED,
		 size_t req_payload_len, struct pldm_msg *resp,
		 size_t *resp_payload_len)
{
	uint8_t ccode;
	int rc;

	/* No request data */
	if (req_payload_len != PLDM_GET_FIRMWARE_PARAMETERS_REQ_BYTES) {
		return pldm_fd_reply_error(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
					   resp_payload_len);
	}

	/* Retrieve platform-specific data */
	uint16_t entry_count;
	const struct pldm_firmware_component_standalone **entries;
	ccode = fd->ops->components(fd->ops_ctx, &entry_count, &entries);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}
	struct pldm_firmware_string active;
	struct pldm_firmware_string pending;
	ccode = fd->ops->imageset_versions(fd->ops_ctx, &active, &pending);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	rc = pldm_msgbuf_init_errno(buf, 0, resp->payload, *resp_payload_len);
	if (rc) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	/* Add the fixed parameters */
	{
		const struct pldm_get_firmware_parameters_resp fwp = {
			.completion_code = PLDM_SUCCESS,
			// TODO defaulted to 0, could have a callback.
			.capabilities_during_update = { 0 },
			.comp_count = entry_count,
			.active_comp_image_set_ver_str_type = active.str_type,
			.active_comp_image_set_ver_str_len = active.str_len,
			.pending_comp_image_set_ver_str_type = pending.str_type,
			.pending_comp_image_set_ver_str_len = pending.str_len,
		};

		const struct variable_field active_ver = {
			.ptr = active.str_data,
			.length = active.str_len,
		};
		const struct variable_field pending_ver = {
			.ptr = pending.str_data,
			.length = pending.str_len,
		};
		size_t len = buf->remaining;
		ccode = encode_get_firmware_parameters_resp(hdr->instance, &fwp,
							    &active_ver,
							    &pending_ver, resp,
							    &len);
		if (ccode) {
			return pldm_fd_reply_error(ccode, hdr, resp,
						   resp_payload_len);
		}
		rc = pldm_msgbuf_increment(buf, len);
		if (rc) {
			return PLDM_REQUESTER_RESP_MSG_TOO_SMALL;
		}
	}

	/* Add the component table entries */
	for (uint16_t i = 0; i < entry_count; i++) {
		const struct pldm_firmware_component_standalone *e = entries[i];
		struct pldm_component_parameter_entry comp = {
			.comp_classification = e->comp_classification,
			.comp_identifier = e->comp_identifier,
			.comp_classification_index =
				e->comp_classification_index,
			.active_comp_comparison_stamp =
				e->active_ver.comparison_stamp,
			.active_comp_ver_str_type = e->active_ver.str.str_type,
			.active_comp_ver_str_len = e->active_ver.str.str_len,
			.pending_comp_comparison_stamp =
				e->pending_ver.comparison_stamp,
			.pending_comp_ver_str_type =
				e->pending_ver.str.str_type,
			.pending_comp_ver_str_len = e->pending_ver.str.str_len,
			.comp_activation_methods = e->comp_activation_methods,
			.capabilities_during_update =
				e->capabilities_during_update,
		};
		memcpy(comp.active_comp_release_date, e->active_ver.date,
		       PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN);
		memcpy(comp.pending_comp_release_date, e->pending_ver.date,
		       PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN);
		const struct variable_field active_ver = {
			.ptr = e->active_ver.str.str_data,
			.length = e->active_ver.str.str_len,
		};
		const struct variable_field pending_ver = {
			.ptr = e->pending_ver.str.str_data,
			.length = e->pending_ver.str.str_len,
		};

		void *out = NULL;
		size_t len;
		if (pldm_msgbuf_peek_remaining(buf, &out, &len)) {
			return PLDM_REQUESTER_RESP_MSG_TOO_SMALL;
		}
		ccode = encode_get_firmware_parameters_resp_comp_entry(
			&comp, &active_ver, &pending_ver, out, &len);
		if (ccode) {
			return pldm_fd_reply_error(ccode, hdr, resp,
						   resp_payload_len);
		}
		rc = pldm_msgbuf_increment(buf, len);
		if (rc) {
			return PLDM_REQUESTER_RESP_MSG_TOO_SMALL;
		}
	}

	*resp_payload_len = *resp_payload_len - buf->remaining;
	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t
pldm_fd_request_update(struct pldm_fd *fd, const struct pldm_header_info *hdr,
		       const struct pldm_msg *req, size_t req_payload_len,
		       struct pldm_msg *resp, size_t *resp_payload_len,
		       uint8_t address)
{
	uint8_t ccode;

	if (fd->state != PLDM_FD_STATE_IDLE) {
		return pldm_fd_reply_error(PLDM_FWUP_ALREADY_IN_UPDATE_MODE,
					   hdr, resp, resp_payload_len);
	}

	uint32_t ua_max_transfer_size;
	uint16_t num_of_comp;
	uint8_t max_outstanding_transfer_req;
	uint16_t pkg_data_len;
	uint8_t comp_image_set_ver_str_type;
	struct variable_field comp_img_set_ver_str;

	ccode = decode_request_update_req(
		req, req_payload_len, &ua_max_transfer_size, &num_of_comp,
		&max_outstanding_transfer_req, &pkg_data_len,
		&comp_image_set_ver_str_type, &comp_img_set_ver_str);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	/* No metadata nor pkg data */
	ccode = encode_request_update_resp(hdr->instance, 0, 0, resp,
					   resp_payload_len);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	fd->max_transfer =
		fd->ops->transfer_size(fd->ops_ctx, ua_max_transfer_size);
	if (fd->max_transfer > ua_max_transfer_size) {
		/* Limit to UA's size */
		fd->max_transfer = ua_max_transfer_size;
	}
	if (fd->max_transfer < PLDM_FWUP_BASELINE_TRANSFER_SIZE) {
		/* Don't let it be zero, that will loop forever  */
		fd->max_transfer = PLDM_FWUP_BASELINE_TRANSFER_SIZE;
	}
	fd->ua_address = address;
	fd->ua_address_set = true;

	pldm_fd_set_state(fd, PLDM_FD_STATE_LEARN_COMPONENTS);

	return PLDM_REQUESTER_SUCCESS;
}

/* wrapper around ops->cancel, will only run ops->cancel when a component update is
 * active */
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
static enum pldm_component_response_codes pldm_fd_check_update_component(
	struct pldm_fd *fd, bool update,
	const struct pldm_firmware_update_component *comp)
{
	uint8_t ccode;

	uint16_t entry_count;
	const struct pldm_firmware_component_standalone **entries;
	ccode = fd->ops->components(fd->ops_ctx, &entry_count, &entries);
	if (ccode) {
		return PLDM_CRC_COMP_NOT_SUPPORTED;
	}

	bool found = false;
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

static pldm_requester_rc_t
pldm_fd_pass_comp(struct pldm_fd *fd, const struct pldm_header_info *hdr,
		  const struct pldm_msg *req, size_t req_payload_len,
		  struct pldm_msg *resp, size_t *resp_payload_len)
{
	uint8_t ccode;

	if (fd->state != PLDM_FD_STATE_LEARN_COMPONENTS) {
		return pldm_fd_reply_error(PLDM_FWUP_INVALID_STATE_FOR_COMMAND,
					   hdr, resp, resp_payload_len);
	}

	uint8_t transfer_flag;

	/* Some portions are unused for PassComponentTable */
	fd->update_comp.comp_image_size = 0;
	fd->update_comp.update_option_flags.value = 0;

	struct variable_field ver;
	uint8_t str_type;
	ccode = decode_pass_component_table_req(
		req, req_payload_len, &transfer_flag,
		&fd->update_comp.comp_classification,
		&fd->update_comp.comp_identifier,
		&fd->update_comp.comp_classification_index,
		&fd->update_comp.version.comparison_stamp, &str_type, &ver);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	/* Copy to a fixed string */
	ccode = pldm_firmware_variable_to_string(str_type, &ver,
						 &fd->update_comp.version.str);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	uint8_t comp_response_code =
		pldm_fd_check_update_component(fd, false, &fd->update_comp);

	/* Component Response Code is 0 for ComponentResponse, 1 otherwise */
	uint8_t comp_resp = (comp_response_code != 0);

	ccode = encode_pass_component_table_resp(hdr->instance, comp_resp,
						 comp_response_code, resp,
						 resp_payload_len);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	if (transfer_flag & PLDM_END) {
		pldm_fd_set_state(fd, PLDM_FD_STATE_READY_XFER);
	}

	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t
pldm_fd_update_comp(struct pldm_fd *fd, const struct pldm_header_info *hdr,
		    const struct pldm_msg *req, size_t req_payload_len,
		    struct pldm_msg *resp, size_t *resp_payload_len)
{
	uint8_t ccode;

	if (fd->state != PLDM_FD_STATE_READY_XFER) {
		return pldm_fd_reply_error(PLDM_FWUP_INVALID_STATE_FOR_COMMAND,
					   hdr, resp, resp_payload_len);
	}

	struct variable_field ver;
	uint8_t str_type;
	ccode = decode_update_component_req(
		req, req_payload_len, &fd->update_comp.comp_classification,
		&fd->update_comp.comp_identifier,
		&fd->update_comp.comp_classification_index,
		&fd->update_comp.version.comparison_stamp,
		&fd->update_comp.comp_image_size,
		&fd->update_comp.update_option_flags, &str_type, &ver);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	/* Copy to a fixed string */
	ccode = pldm_firmware_variable_to_string(str_type, &ver,
						 &fd->update_comp.version.str);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	uint8_t comp_response_code =
		pldm_fd_check_update_component(fd, true, &fd->update_comp);
	// Mask to only the "Force Update" flag, others are not handled.
	bitfield32_t update_flags = {
		.bits.bit0 = fd->update_comp.update_option_flags.bits.bit0
	};

	/* Component Response Code is 0 for ComponentResponse, 1 otherwise */
	uint8_t comp_resp = (comp_response_code != 0);
	uint16_t estimated_time = 0;

	ccode = encode_update_component_resp(hdr->instance, comp_resp,
					     comp_response_code, update_flags,
					     estimated_time, resp,
					     resp_payload_len);
	if (ccode) {
		if (comp_response_code == PLDM_CRC_COMP_CAN_BE_UPDATED) {
			/* Inform the application of cancellation. Call it directly
			 * rather than going through pldm_fd_maybe_cancel_component() */
			fd->ops->cancel_update_component(fd->ops_ctx,
							 &fd->update_comp);
		}
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	/* Set up download state */
	if (comp_response_code == PLDM_CRC_COMP_CAN_BE_UPDATED) {
		memset(&fd->specific, 0x0, sizeof(fd->specific));
		fd->update_flags = update_flags;
		fd->req.state = PLDM_FD_REQ_READY;
		fd->req.complete = false;
		pldm_fd_set_state(fd, PLDM_FD_STATE_DOWNLOAD);
	}

	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t
pldm_fd_get_status(struct pldm_fd *fd, const struct pldm_header_info *hdr,
		   const struct pldm_msg *req, size_t req_payload_len,
		   struct pldm_msg *resp, size_t *resp_payload_len)
{
	uint8_t ccode;

	/* No request data */
	if (req_payload_len != PLDM_GET_STATUS_REQ_BYTES) {
		return pldm_fd_reply_error(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
					   resp_payload_len);
	}
	(void)req;

	/* Defaults */
	uint8_t aux_state = 0;
	uint8_t aux_state_status = 0;
	uint8_t progress_percent = PROGRESS_PERCENT_NOT_SUPPORTED;
	uint8_t reason_code = 0;
	bitfield32_t update_option_flags_enabled = { .value = 0 };

	pldm_fd_get_aux_state(fd, &aux_state, &aux_state_status);

	switch (fd->state) {
	case PLDM_FD_STATE_IDLE:
		reason_code = fd->reason;
		break;
	case PLDM_FD_STATE_DOWNLOAD:
		if (fd->update_comp.comp_image_size > 0) {
			uint32_t one_percent =
				fd->update_comp.comp_image_size / 100;
			if (fd->update_comp.comp_image_size % 100 != 0) {
				one_percent += 1;
			}
			progress_percent =
				(fd->specific.download.offset / one_percent);
		}
		update_option_flags_enabled = fd->update_flags;
		break;
	case PLDM_FD_STATE_VERIFY:
		update_option_flags_enabled = fd->update_flags;
		progress_percent = fd->specific.verify.progress_percent;
		break;
	case PLDM_FD_STATE_APPLY:
		update_option_flags_enabled = fd->update_flags;
		progress_percent = fd->specific.apply.progress_percent;
		break;
	default:
		break;
	}

	ccode = encode_get_status_resp(hdr->instance, fd->state, fd->prev_state,
				       aux_state, aux_state_status,
				       progress_percent, reason_code,
				       update_option_flags_enabled, resp,
				       resp_payload_len);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t
pldm_fd_cancel_update_comp(struct pldm_fd *fd,
			   const struct pldm_header_info *hdr,
			   const struct pldm_msg *req, size_t req_payload_len,
			   struct pldm_msg *resp, size_t *resp_payload_len)
{
	pldm_requester_rc_t rc;

	/* No request data */
	if (req_payload_len != PLDM_CANCEL_UPDATE_COMPONENT_REQ_BYTES) {
		return pldm_fd_reply_error(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
					   resp_payload_len);
	}
	(void)req;

	switch (fd->state) {
	case PLDM_FD_STATE_DOWNLOAD:
	case PLDM_FD_STATE_VERIFY:
	case PLDM_FD_STATE_APPLY:
		break;
	default:
		return pldm_fd_reply_error(PLDM_FWUP_NOT_IN_UPDATE_MODE, hdr,
					   resp, resp_payload_len);
	}

	/* No response payload */
	rc = pldm_fd_reply_error(PLDM_SUCCESS, hdr, resp, resp_payload_len);
	if (rc) {
		return rc;
	}

	pldm_fd_maybe_cancel_component(fd);
	pldm_fd_set_state(fd, PLDM_FD_STATE_READY_XFER);

	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t
pldm_fd_cancel_update(struct pldm_fd *fd, const struct pldm_header_info *hdr,
		      const struct pldm_msg *req, size_t req_payload_len,
		      struct pldm_msg *resp, size_t *resp_payload_len)
{
	uint8_t ccode;

	/* No request data */
	if (req_payload_len != PLDM_CANCEL_UPDATE_REQ_BYTES) {
		return pldm_fd_reply_error(PLDM_ERROR_INVALID_LENGTH, hdr, resp,
					   resp_payload_len);
	}
	(void)req;

	if (fd->state == PLDM_FD_STATE_IDLE) {
		return pldm_fd_reply_error(PLDM_FWUP_NOT_IN_UPDATE_MODE, hdr,
					   resp, resp_payload_len);
	}

	/* Assume non_functioning_component_indication = False, in future
	 * could add a platform callback */
	bitfield64_t zerobf = { .value = 0 };
	ccode = encode_cancel_update_resp(hdr->instance, 0, zerobf, resp,
					  resp_payload_len);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	pldm_fd_maybe_cancel_component(fd);
	pldm_fd_set_idle(fd, PLDM_FD_CANCEL_UPDATE);

	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t
pldm_fd_activate_firmware(struct pldm_fd *fd,
			  const struct pldm_header_info *hdr,
			  const struct pldm_msg *req, size_t req_payload_len,
			  struct pldm_msg *resp, size_t *resp_payload_len)
{
	uint8_t ccode;
	bool self_contained;

	ccode = decode_activate_firmware_req(req, req_payload_len,
					     &self_contained);
	if (ccode) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	if (fd->state != PLDM_FD_STATE_READY_XFER) {
		return pldm_fd_reply_error(PLDM_FWUP_INVALID_STATE_FOR_COMMAND,
					   hdr, resp, resp_payload_len);
	}

	uint16_t estimated_time = 0;
	ccode = fd->ops->activate(fd->ops_ctx, self_contained, &estimated_time);

	if (ccode == PLDM_SUCCESS ||
	    ccode == PLDM_FWUP_ACTIVATION_NOT_REQUIRED) {
		/* Transition through states so that the prev_state is correct */
		pldm_fd_set_state(fd, PLDM_FD_STATE_ACTIVATE);
		pldm_fd_set_idle(fd, PLDM_FD_ACTIVATE_FW);
		ccode = encode_activate_firmware_resp(hdr->instance, ccode,
						      estimated_time, resp,
						      resp_payload_len);
	}
	if (ccode != PLDM_SUCCESS) {
		return pldm_fd_reply_error(ccode, hdr, resp, resp_payload_len);
	}

	return PLDM_REQUESTER_SUCCESS;
}

static uint32_t pldm_fd_fwdata_size(struct pldm_fd *fd)
{
	if (fd->state != PLDM_FD_STATE_DOWNLOAD) {
		assert(false);
		return 0;
	}

	if (fd->specific.download.offset > fd->update_comp.comp_image_size) {
		assert(false);
		return 0;
	}
	uint32_t size =
		fd->update_comp.comp_image_size - fd->specific.download.offset;

	if (size > fd->max_transfer) {
		size = fd->max_transfer;
	}
	return size;
}

static pldm_requester_rc_t
pldm_fd_handle_fwdata_resp(struct pldm_fd *fd, const struct pldm_msg *resp,
			   size_t resp_payload_len)
{
	if (fd->state != PLDM_FD_STATE_DOWNLOAD) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	if (fd->req.state != PLDM_FD_REQ_SENT) {
		/* Not waiting for a response, ignore it */
		return PLDM_REQUESTER_RECV_FAIL;
	}

	struct pldm_fd_download *dl = &fd->specific.download;
	if (fd->req.complete) {
		/* Received data after completion */
		return PLDM_REQUESTER_RECV_FAIL;
	}

	switch (resp->payload[0]) {
	case PLDM_SUCCESS:
		break;
	case PLDM_FWUP_RETRY_REQUEST_FW_DATA:
		/* Just return, let the retry timer send another request later */
		return PLDM_REQUESTER_SUCCESS;
	default:
		/* Send a TransferComplete failure */
		fd->req.state = PLDM_FD_REQ_READY;
		fd->req.complete = true;
		fd->req.result = PLDM_FWUP_FD_ABORTED_TRANSFER;
		return PLDM_REQUESTER_SUCCESS;
	}

	/* Handle the received data */

	uint32_t fwdata_size = pldm_fd_fwdata_size(fd);
	if (resp_payload_len != fwdata_size + 1) {
		/* Data is incorrect size. Could indicate MCTP corruption, drop it
		 * and let retry timer handle it */
		return PLDM_REQUESTER_RECV_FAIL;
	}

	/* Check pldm_fd_fwdata_size calculation, should not fail */
	if (dl->offset + fwdata_size < dl->offset ||
	    dl->offset + fwdata_size > fd->update_comp.comp_image_size) {
		assert(0);
		return PLDM_REQUESTER_RECV_FAIL;
	}

	/* Provide the data chunk to the device */
	uint8_t res = fd->ops->firmware_data(fd->ops_ctx, dl->offset,
					     &resp->payload[1], fwdata_size,
					     &fd->update_comp);

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

	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t
pldm_fd_handle_transfer_complete_resp(struct pldm_fd *fd,
				      const struct pldm_msg *resp,
				      size_t resp_payload_len)
{
	if (fd->state != PLDM_FD_STATE_DOWNLOAD) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	if (fd->req.state != PLDM_FD_REQ_SENT) {
		/* Not waiting for a response, ignore it */
		return PLDM_REQUESTER_RECV_FAIL;
	}

	if (!fd->req.complete) {
		/* Were waiting for RequestFirmwareData instead, ignore it */
		return PLDM_REQUESTER_RECV_FAIL;
	}

	/* Disregard the response completion code */
	(void)resp_payload_len;
	(void)resp;

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
		/* TODO: Set AuxStateStatus */
	}
	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t
pldm_fd_handle_verify_complete_resp(struct pldm_fd *fd,
				    const struct pldm_msg *resp,
				    size_t resp_payload_len)
{
	if (fd->state != PLDM_FD_STATE_VERIFY) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	if (fd->req.state != PLDM_FD_REQ_SENT) {
		/* Not waiting for a response, ignore it */
		return PLDM_REQUESTER_RECV_FAIL;
	}

	assert(fd->req.complete);

	/* Disregard the response completion code */
	(void)resp_payload_len;
	(void)resp;

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
		/* TODO: Set AuxStateStatus */
	}
	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t
pldm_fd_handle_apply_complete_resp(struct pldm_fd *fd,
				   const struct pldm_msg *resp,
				   size_t resp_payload_len)
{
	if (fd->state != PLDM_FD_STATE_APPLY) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	if (fd->req.state != PLDM_FD_REQ_SENT) {
		/* Not waiting for a response, ignore it */
		return PLDM_REQUESTER_RECV_FAIL;
	}

	assert(fd->req.complete);

	/* Disregard the response completion code */
	(void)resp_payload_len;
	(void)resp;

	/* Next state depends whether the apply succeeded */
	if (fd->req.result == PLDM_FWUP_APPLY_SUCCESS) {
		/* Switch to ReadyXfer */
		fd->req.state = PLDM_FD_REQ_UNUSED;
		pldm_fd_set_state(fd, PLDM_FD_STATE_READY_XFER);
	} else {
		/* Wait for UA to cancel */
		fd->req.state = PLDM_FD_REQ_FAILED;
		/* TODO: Set AuxStateStatus */
	}
	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t pldm_fd_handle_resp(struct pldm_fd *fd,
					       uint8_t address,
					       const void *resp_msg,
					       size_t resp_len)
{
	if (!(fd->ua_address_set && fd->ua_address == address)) {
		// Either an early response, or a resopnse from a wrong EID */
		return PLDM_REQUESTER_RECV_FAIL;
	}

	/* Must have a ccode */
	if (resp_len < sizeof(struct pldm_msg_hdr) + 1) {
		return PLDM_REQUESTER_INVALID_RECV_LEN;
	}
	size_t resp_payload_len = resp_len - sizeof(struct pldm_msg_hdr);
	const struct pldm_msg *resp = resp_msg;

	if (fd->req.state != PLDM_FD_REQ_SENT) {
		// No response was expected
		return PLDM_REQUESTER_RECV_FAIL;
	}

	if (fd->req.instance_id != resp->hdr.instance_id) {
		// Response wasn't for the expected request
		return PLDM_REQUESTER_RECV_FAIL;
	}
	if (fd->req.command != resp->hdr.command) {
		// Response wasn't for the expected request
		return PLDM_REQUESTER_RECV_FAIL;
	}

	// TODO this may be a bit loose
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
		/* Unsolicited response */
		return PLDM_REQUESTER_RECV_FAIL;
	}
}

static pldm_requester_rc_t pldm_fd_progress_download(struct pldm_fd *fd,
						     struct pldm_msg *req,
						     size_t *req_payload_len)
{
	int rc;

	if (!pldm_fd_req_should_send(&fd->req, pldm_fd_now(fd))) {
		/* Nothing to do */
		*req_payload_len = 0;
		return PLDM_REQUESTER_SUCCESS;
	}

	uint8_t instance_id = pldm_fd_req_next_instance(&fd->req);
	struct pldm_fd_download *dl = &fd->specific.download;
	if (fd->req.complete) {
		/* Send TransferComplete */
		rc = encode_transfer_complete_req(instance_id, fd->req.result,
						  req, req_payload_len);
	} else {
		/* Send a new RequestFirmwareData */
		rc = encode_request_firmware_data_req(instance_id, dl->offset,
						      pldm_fd_fwdata_size(fd),
						      req, req_payload_len);
	}

	if (rc) {
		return PLDM_REQUESTER_SEND_FAIL;
	}

	/* Wait for response */
	fd->req.state = PLDM_FD_REQ_SENT;
	fd->req.instance_id = req->hdr.instance_id;
	fd->req.command = req->hdr.command;
	fd->req.sent_time = pldm_fd_now(fd);

	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t pldm_fd_progress_verify(struct pldm_fd *fd,
						   struct pldm_msg *req,
						   size_t *req_payload_len)
{
	int rc;

	if (!pldm_fd_req_should_send(&fd->req, pldm_fd_now(fd))) {
		/* Nothing to do */
		*req_payload_len = 0;
		return PLDM_REQUESTER_SUCCESS;
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
				return PLDM_REQUESTER_SUCCESS;
			}
			/* This is an API infraction by the implementer, return a distinctive failure */
			res = PLDM_FWUP_VENDOR_VERIFY_RESULT_RANGE_MAX;
		}
		fd->req.result = res;
		fd->req.complete = true;
	}

	uint8_t instance_id = pldm_fd_req_next_instance(&fd->req);
	rc = encode_verify_complete_req(instance_id, fd->req.result, req,
					req_payload_len);
	if (rc) {
		return PLDM_REQUESTER_SEND_FAIL;
	}

	/* Wait for response */
	fd->req.state = PLDM_FD_REQ_SENT;
	fd->req.instance_id = req->hdr.instance_id;
	fd->req.command = req->hdr.command;
	fd->req.sent_time = pldm_fd_now(fd);

	return PLDM_REQUESTER_SUCCESS;
}

static pldm_requester_rc_t pldm_fd_progress_apply(struct pldm_fd *fd,
						  struct pldm_msg *req,
						  size_t *req_payload_len)
{
	int rc;

	if (!pldm_fd_req_should_send(&fd->req, pldm_fd_now(fd))) {
		/* Nothing to do */
		*req_payload_len = 0;
		return PLDM_REQUESTER_SUCCESS;
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
				return PLDM_REQUESTER_SUCCESS;
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

	uint8_t instance_id = pldm_fd_req_next_instance(&fd->req);
	bitfield16_t appmeth = { .value = 0 };
	rc = encode_apply_complete_req(instance_id, fd->req.result, appmeth,
				       req, req_payload_len);
	if (rc) {
		return PLDM_REQUESTER_SEND_FAIL;
	}

	/* Wait for response */
	fd->req.state = PLDM_FD_REQ_SENT;
	fd->req.instance_id = req->hdr.instance_id;
	fd->req.command = req->hdr.command;
	fd->req.sent_time = pldm_fd_now(fd);

	return PLDM_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_TESTING
pldm_requester_rc_t pldm_fd_setup(struct pldm_fd *fd, size_t pldm_fd_size,
				  const struct pldm_fd_ops *ops, void *ops_ctx)
{
	if (pldm_fd_size < sizeof(struct pldm_fd)) {
		/* Safety check that sufficient storage was provided for *fd,
		 * in case PLDM_SIZEOF_PLDM_FD is incorrect */
		return PLDM_REQUESTER_INVALID_SETUP;
	}
	memset(fd, 0x0, sizeof(*fd));
	fd->ops = ops;
	fd->ops_ctx = ops_ctx;

	return PLDM_REQUESTER_SUCCESS;
}

/* A response should only be used when this returns PLDM_REQUESTER_SUCCESS, and *resp_len > 0 */
LIBPLDM_ABI_TESTING
pldm_requester_rc_t pldm_fd_handle_msg(struct pldm_fd *fd,
				       uint8_t remote_address,
				       const void *req_msg, size_t req_len,
				       void *resp_msg, size_t *resp_len)
{
	uint8_t rc;

	/* Space for header plus completion code */
	if (*resp_len < sizeof(struct pldm_msg_hdr) + 1) {
		return PLDM_REQUESTER_RESP_MSG_TOO_SMALL;
	}
	size_t resp_payload_len = *resp_len - sizeof(struct pldm_msg_hdr);
	struct pldm_msg *resp = resp_msg;

	if (req_len < sizeof(struct pldm_msg_hdr)) {
		return PLDM_REQUESTER_INVALID_RECV_LEN;
	}
	size_t req_payload_len = req_len - sizeof(struct pldm_msg_hdr);
	const struct pldm_msg *req = req_msg;

	struct pldm_header_info hdr;
	rc = unpack_pldm_header(&req->hdr, &hdr);
	if (rc != PLDM_SUCCESS) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	if (hdr.pldm_type != PLDM_FWUP) {
		/* Caller should not have passed non-pldmfw */
		return PLDM_REQUESTER_RECV_FAIL;
	}

	if (hdr.msg_type == PLDM_RESPONSE) {
		*resp_len = 0;
		return pldm_fd_handle_resp(fd, remote_address, req_msg,
					   req_len);
	}

	if (hdr.msg_type != PLDM_REQUEST) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

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
			return pldm_fd_reply_error(PLDM_ERROR_NOT_READY, &hdr,
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

	/* Dispatch command */
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
		rc = pldm_fd_reply_error(PLDM_ERROR_UNSUPPORTED_PLDM_CMD, &hdr,
					 resp, &resp_payload_len);
	}

	if (rc == PLDM_REQUESTER_SUCCESS) {
		*resp_len = resp_payload_len + sizeof(struct pldm_msg_hdr);
	}

	return rc;
}

LIBPLDM_ABI_TESTING
pldm_requester_rc_t pldm_fd_progress(struct pldm_fd *fd, void *req_msg,
				     size_t *req_len, uint8_t *address)
{
	int rc = PLDM_REQUESTER_RECV_FAIL;

	/* Space for header */
	if (*req_len < sizeof(struct pldm_msg_hdr)) {
		return PLDM_REQUESTER_SETUP_FAIL;
	}
	size_t req_payload_len = *req_len - sizeof(struct pldm_msg_hdr);
	struct pldm_msg *req = req_msg;
	*req_len = 0;

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
	case PLDM_FD_STATE_IDLE:
		req_payload_len = 0;
		break;
	default:
		req_payload_len = 0;
		// Other Update Mode states have a timeout
		if ((pldm_fd_now(fd) - fd->update_timestamp_fd_t1) >
		    FD_T1_TIMEOUT) {
			// TODO cancel device update
			pldm_fd_idle_timeout(fd);
		}
		break;
	}

	if (rc == PLDM_REQUESTER_SUCCESS && fd->ua_address_set &&
	    req_payload_len > 0) {
		*req_len = req_payload_len + sizeof(struct pldm_msg_hdr);
		*address = fd->ua_address;
	}

	return rc;
}
