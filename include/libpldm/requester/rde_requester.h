#ifndef RDE_REQUESTER_H
#define RDE_REQUESTER_H

#ifdef __cplusplus
extern "C" {
#endif
#include "base.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PLDM_INIT_DISCOVERY_COMMAND 0x00
#define NO_NEXT_COMMAND 0xff

typedef enum requester_status {
	RDE_REQUESTER_SUCCESS = 0,
	RDE_REQUESTER_OPEN_FAIL = -1,
	RDE_REQUESTER_NOT_RDE_MSG = -2,
	RDE_REQUESTER_NOT_RESP_MSG = -3,
	RDE_REQUESTER_NOT_REQ_MSG = -4,
	RDE_REQUESTER_RESP_MSG_TOO_SMALL = -5,
	RDE_REQUESTER_INSTANCE_ID_MISMATCH = -6,
	RDE_REQUESTER_SEND_FAIL = -7,
	RDE_REQUESTER_RECV_FAIL = -8,
	RDE_REQUESTER_INVALID_RECV_LEN = -9,
	RDE_REQUESTER_NO_NEXT_COMMAND_FOUND = -10,
} rde_requester_rc_t;

struct requester_base_context {
	bool initialized;
	bool waiting_for_response;
	uint8_t current_command;
	uint8_t eid;
	uint8_t tid;
	uint8_t pldm_types[8];
	uint8_t pldm_commands[32];
	uint8_t pldm_versions[256]; // Check these bytes
};

// Initialize the struct
int pldm_base_start_discovery(struct requester_base_context *ctx, uint8_t *eid);

// Getting the next command
int pldm_base_get_next_request(struct requester_base_context *ctx);

// Getting status of responder
int pldm_base_get_current_status(struct requester_base_context *ctx);

// Response Handler for command type
int pldm_base_push_response(struct requester_base_context *ctx, void *resp_msg,
			    size_t resp_size);

#ifdef __cplusplus
}
#endif

#endif /* RDE_REQUESTER_H */