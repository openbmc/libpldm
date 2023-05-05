#ifndef PLDM_BASE_REQUESTER_H
#define PLDM_BASE_REQUESTER_H

#ifdef __cplusplus
extern "C" {
#endif
#include "base.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PLDM_TYPES 6
#define MAX_DEV_NAME_SIZE 32

typedef enum requester_return_codes {
	PLDM_BASE_REQUESTER_SUCCESS = 0,
	PLDM_BASE_REQUESTER_NOT_PLDM_BASE_MSG = -1,
	PLDM_BASE_REQUESTER_NOT_RESP_MSG = -2,
	PLDM_BASE_REQUESTER_SEND_FAIL = -3,
	PLDM_BASE_REQUESTER_RECV_FAIL = -4,
	PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND = -5,
	PLDM_BASE_REQUESTER_ENCODING_REQUEST_FAILURE = -6,
	PLDM_BASE_CONTEXT_INITIALIZATION_ERROR = -7,
	PLDM_BASE_CONTEXT_NOT_READY = -8
} pldm_base_requester_rc_t;

typedef enum requester_status {
	PLDM_BASE_REQUESTER_REQUEST_FAILED = -1,
	PLDM_BASE_REQUESTER_READY_TO_PICK_NEXT_REQUEST = 0,
	PLDM_BASE_REQUESTER_WAITING_FOR_RESPONSE = 1,
	PLDM_BASE_REQUESTER_NO_PENDING_ACTION = 2
} req_status_t;

struct requester_base_context {
	bool initialized;
	uint8_t next_command;
	uint8_t requester_status;
	uint8_t command_pldm_type;
	uint8_t tid;
	char dev_name[MAX_DEV_NAME_SIZE];
	int net_id; // MCTP network id
	bitfield8_t pldm_types[PLDM_MAX_TYPES / 8];
	uint8_t pldm_commands[PLDM_MAX_TYPES][PLDM_MAX_CMDS_PER_TYPE];
	ver32_t pldm_versions[PLDM_MAX_TYPES];
};
/**
 * @brief Initializes the context for PLDM Base discovery commands
 *
 * @param[in] ctx - pointer to a context which is to be initialized
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_base_requester_rc_t
pldm_base_init_context(struct requester_base_context *ctx, const char *dev_name,
		       int net_id);

/**
 * @brief Sets the first command to be triggered for base discovery and sets the
 * status of context to "Ready to PICK"
 *
 * @param[in] ctx - pointer to a context which is to be initialized
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_base_requester_rc_t
pldm_base_start_discovery(struct requester_base_context *ctx);
/**
 * @brief Gets the next PLDM command from a request buffer to be processed
 *
 * @param[in] ctx - pointer to a context which is to be initialized
 * @param[in] instance_id - instance id of the pldm requester
 * @param[out] request - byte array that will store the encoded request msg
 * according to the pldm command type. Caller is responsible for allocating and
 * cleaning up memory of this variable
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_base_requester_rc_t
pldm_base_get_next_request(struct requester_base_context *ctx,
			   uint8_t instance_id, struct pldm_msg *request);

/**
 * @brief Pushes the response values to the context based on the command
 * type that was executed and updates the command status. It alse sets the
 * next_command attribute of the context based on the last executed command.
 *
 * @param[in] ctx - a pointer to the context
 * @param[in] resp_msg - a pointer to the response message that the caller
 * received
 * @param[in] resp_size - size of the response message payload
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_base_requester_rc_t
pldm_base_push_response(struct requester_base_context *ctx, void *resp_msg,
			size_t resp_size);

#ifdef __cplusplus
}
#endif

#endif /* PLDM_BASE_REQUESTER_H */