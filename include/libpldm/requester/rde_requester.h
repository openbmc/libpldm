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
#define MAX_REQUEST_BUFFER_SIZE 256
#define NO_NEXT_COMMAND 0xff
#define PLDM_TYPES 6

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
	RDE_REQUESTER_CTX_INITIALIZATION_FAILURE = -11,
} rde_requester_rc_t;

struct requester_base_context {
	bool initialized;
	bool waiting_for_response;
	uint8_t current_command;
	uint8_t eid;
	uint8_t tid;
	uint8_t pldm_types[8];
	uint8_t pldm_commands[32];
	uint8_t pldm_versions[PLDM_TYPES];
};

struct rde_pldm_request {
	uint8_t pldmType;
	uint8_t pldmCommand;
};

/**
 * @brief Initializes the context for PLDM Base discovery commands
 *
 * @param[in] ctx - pointer to a context which is to be initialized
 * @param[in] eid - pointer to destination eid
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
rde_requester_rc_t pldm_base_start_discovery(struct requester_base_context *ctx,
					     uint8_t *eid);

/**
 * @brief Gets the next PLDM command from a request buffer to be processed
 *
 * @param[in] ctx - pointer to a context which is to be initialized
 * @param[in] request - *request is a pointer to the request that is present in
 * the buffer
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
rde_requester_rc_t
pldm_base_get_next_request(struct requester_base_context *ctx,
			   struct rde_pldm_request **request);

// TODO: Write Brief and implement
rde_requester_rc_t
pldm_base_get_current_status(struct requester_base_context *ctx);

// TODO: Write Brief and implement
rde_requester_rc_t pldm_base_push_response(struct requester_base_context *ctx,
					   void *resp_msg, size_t resp_size);

/**
 * @brief Adds a new request to the request buffer
 *
 * @param[in] request - *request is a pointer to the request that is to be added
 * in the request buffer
 *
 * @return pldm_requester_rc_t (errno may be set)
 */

rde_requester_rc_t
add_next_request_to_queue(struct rde_pldm_request *rde_pldm_request);

/**
 * @brief Gets the current request buffer size
 *
 * @return int
 */
int pldm_get_request_queue_size();

#ifdef __cplusplus
}
#endif

#endif /* RDE_REQUESTER_H */