#include "requester/rde_requester.h"
#include "base.h"
#include "pldm.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define IGNORE(x) (void)(x)

int pldm_base_start_discovery(struct requester_base_context *ctx, uint8_t *eid)
{
	ctx->initialized = true;
	ctx->eid = *eid;
	ctx->current_command = PLDM_INIT_DISCOVERY_COMMAND;
	return 0;
}

int pldm_base_get_next_request(struct requester_base_context *ctx)
{
	uint8_t nextCommand = NO_NEXT_COMMAND;
	switch (ctx->current_command) {
	case PLDM_INIT_DISCOVERY_COMMAND:
		nextCommand = PLDM_GET_TID;
		break;

	case PLDM_GET_TID:
		nextCommand = PLDM_GET_PLDM_VERSION;
		break;

	case PLDM_GET_PLDM_VERSION:
		nextCommand = PLDM_GET_PLDM_TYPES;
		break;

	case PLDM_GET_PLDM_TYPES:
		nextCommand = PLDM_GET_PLDM_COMMANDS;
		break;

	case PLDM_GET_PLDM_COMMANDS:
		nextCommand = NO_NEXT_COMMAND;
		break;
	}
	ctx->current_command = nextCommand;
	if (nextCommand == NO_NEXT_COMMAND) {
		return RDE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	}
	return PLDM_SUCCESS;
}

// TODO: Call handlers for TID, Version, types and commands
// TODO: Override response pldm_base_push_response

int pldm_base_push_response(struct requester_base_context *ctx, void *resp_msg,
			    size_t resp_size)
{
	// TODO: Remove ignores once implemented
	IGNORE((uint8_t *)resp_msg);
	IGNORE(resp_size);

	switch (ctx->current_command) {
	case PLDM_GET_TID:
		// call TID handler - encode_get_tid_resp()
		// Update context
		return PLDM_SUCCESS;

	case PLDM_GET_PLDM_VERSION:
		// call pldm version handler - encode_get_version_resp()
        
		// Update context
		return PLDM_SUCCESS;

	case PLDM_GET_PLDM_TYPES:
		// call pldm types handler - encode_get_types_resp()
		// Update context
		return PLDM_SUCCESS;

	case PLDM_GET_PLDM_COMMANDS:
		// call pldm commands handler - encode_get_commands_resp()
		// Update context
		return PLDM_SUCCESS;
	}

	return RDE_REQUESTER_NOT_RDE_MSG;
}