#ifndef PLDM_BASE_REQUESTER_H
#define PLDM_BASE_REQUESTER_H

#ifdef __cplusplus
extern "C" {
#endif
#include "base.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
pldm_base_init_context(struct requester_base_context *ctx);

/**
 * @brief Sets the next_command to the first PLDM Base discovery commands
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

/**
 * @brief Gets the next PLDM Type based on the current index provided For
 * instance if the first byte value is 101 (in binary) and the current_type sent
 * is 0 (0th index = Type 0) this function would return 2 as the next type
 *
 * @param[in] ctx for getting the PLDM Type response array
 * @param[in] current_type - Current PLDM Type from where we need to find the
 * next set bit in the PLDM Types byte array
 * @param[out] next_type - To store the next PLDM Type that is to be executed
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_base_requester_rc_t
pldm_base_get_next_pldm_type(struct requester_base_context *ctx,
			     uint8_t current_type, uint8_t *next_type);

#ifdef __cplusplus
}
#endif

#endif /* PLDM_BASE_REQUESTER_H */