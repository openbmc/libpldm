#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <libpldm/pldm.h>
#include <libpldm/firmware_update.h>
#include <libpldm/firmware_fd.h>
#include <libpldm/utils.h>

typedef uint64_t pldm_fd_time_t;

struct pldm_fd_req {
	enum pldm_fd_req_state {
		// pldm_fd_req instance is unused
		PLDM_FD_REQ_UNUSED = 0,
		// Ready to send a request
		PLDM_FD_REQ_READY,
		// Waiting for a response
		PLDM_FD_REQ_SENT,
		// Completed and failed, will not send more requests.
		// Waiting for a cancel from the UA.
		PLDM_FD_REQ_FAILED,
	} state;

	/* Set once when ready to move to next state, will return
     * this result for TransferComplete/VerifyComplete/ApplyComplete request. */
	bool complete;
	/* Only valid when complete is set */
	uint8_t result;

	/* Only valid in SENT state */
	uint8_t instance_id;
	uint8_t command;
	pldm_fd_time_t sent_time;
};

struct pldm_fd_download {
	uint32_t offset;
};

struct pldm_fd_verify {
	uint8_t progress_percent;
};

struct pldm_fd_apply {
	uint8_t progress_percent;
};

/* Update mode idle timeout, 120 seconds */
static const uint64_t FD_T1_TIMEOUT = 120000;

struct pldm_fd {
	enum pldm_firmware_device_states state;
	enum pldm_firmware_device_states prev_state;

	/* Reason for last transition to idle state,
     * only valid when state == PLDM_FD_STATE_IDLE */
	enum pldm_get_status_reason_code_values reason;

	/* State-specific content */
	union {
		struct pldm_fd_download download;
		struct pldm_fd_verify verify;
		struct pldm_fd_apply apply;
	} specific;
	/* Details of the component currently being updated.
     * Set by UpdateComponent, available during download/verify/apply.
     * Also used as temporary storage for PassComponentTable */
	struct pldm_firmware_update_component update_comp;
	bitfield32_t update_flags;

	/* Used for download/verify/apply requests */
	struct pldm_fd_req req;

	/* Address of the UA */
	uint8_t ua_address;
	bool ua_address_set;

	/* Maximum size allowed by the UA or platform implementation */
	uint32_t max_transfer;

	/* Timestamp for FD T1 timeout, milliseconds */
	// TODO: datatype?
	uint64_t update_timestamp_fd_t1;

	const struct pldm_fd_ops *ops;
	void *ops_ctx;
};
