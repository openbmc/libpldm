#ifndef PLDM_BASE_REQUESTER_H
#define PLDM_BASE_REQUESTER_H

#ifdef __cplusplus
extern "C" {
#endif
#include "base.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum requester_status {
	PLDM_BASE_REQUESTER_SUCCESS = 0,
	PLDM_BASE_REQUESTER_NOT_PLDM_BASE_MSG = -1,
	PLDM_BASE_REQUESTER_NOT_RESP_MSG = -2,
	PLDM_BASE_REQUESTER_SEND_FAIL = -3,
	PLDM_BASE_REQUESTER_RECV_FAIL = -4,
	PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND = -5,
	PLDM_BASE_REQUESTER_ENCODING_REQUEST_FAILURE = -6
} pldm_base_requester_rc_t;

typedef enum command_status {
	COMMAND_FAILED = -1,
	COMMAND_COMPLETED = 0,
	COMMAND_NOT_STARTED = 1,
	COMMAND_WAITING = 2,
} status_t;

struct requester_base_context {
	bool initialized;
	uint8_t next_command;
	uint8_t command_status;
	uint8_t command_pldm_type;
	uint8_t tid;
	bitfield8_t pldm_types[PLDM_MAX_TYPES / 8];
	uint8_t pldm_commands[PLDM_MAX_TYPES][PLDM_MAX_CMDS_PER_TYPE / 8];
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
pldm_base_start_discovery(struct requester_base_context *ctx);

/**
 * @brief Gets the next PLDM command in sequence to be executed for PLDM
 * Discovery
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
 * @brief Pushes the response values to the context attributes based on the
 * command type that was executed and updates the command status. It also sets
 * the next_command attribute of the context based on the last executed command.
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