/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef MCTP_H
#define MCTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/* Delete when deleting old api */
typedef uint8_t mctp_eid_t;

typedef enum pldm_requester_error_codes {
	PLDM_REQUESTER_SUCCESS = 0,
	PLDM_REQUESTER_OPEN_FAIL = -1,
	PLDM_REQUESTER_NOT_PLDM_MSG = -2,
	PLDM_REQUESTER_NOT_RESP_MSG = -3,
	PLDM_REQUESTER_NOT_REQ_MSG = -4,
	PLDM_REQUESTER_RESP_MSG_TOO_SMALL = -5,
	PLDM_REQUESTER_INSTANCE_ID_MISMATCH = -6,
	PLDM_REQUESTER_SEND_FAIL = -7,
	PLDM_REQUESTER_RECV_FAIL = -8,
	PLDM_REQUESTER_INVALID_RECV_LEN = -9,
	PLDM_REQUESTER_SETUP_FAIL = -10,
	PLDM_REQUESTER_INVALID_SETUP = -11,
	PLDM_REQUESTER_POLL_FAIL = -12,
	PLDM_REQUESTER_TRANSPORT_BUSY = -13,
} pldm_requester_rc_t;

#ifdef __cplusplus
}
#endif

#endif /* MCTP_H */
