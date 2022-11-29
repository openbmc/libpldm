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

struct rde_pldm_request *requestBuffer[MAX_REQUEST_BUFFER_SIZE];
int currentRequestPtr;
int lastRequestPtr;

rde_requester_rc_t add_next_request_to_queue(struct rde_pldm_request *rde_pldm_request)
{
	if (currentRequestPtr == -1) {
		requestBuffer[++currentRequestPtr] = rde_pldm_request;
		++lastRequestPtr;
	} else if (lastRequestPtr == MAX_REQUEST_BUFFER_SIZE &&
		   currentRequestPtr == 0) {
		// TODO: Skip request - buffer full
		// TODO: lastRequestPtr = -1;
	} else if (lastRequestPtr == MAX_REQUEST_BUFFER_SIZE) {
		// TODO: Resize buffer
	} else {
		requestBuffer[++lastRequestPtr] = rde_pldm_request;
	}
	return RDE_REQUESTER_SUCCESS;
}

void add_base_requests_to_request_queue()
{
	struct rde_pldm_request getTIDRequest;
	getTIDRequest.pldmCommand = PLDM_GET_TID;
	getTIDRequest.pldmType = PLDM_BASE;
	add_next_request_to_queue(&getTIDRequest);

	struct rde_pldm_request getPLDMVersionRequest;
	getPLDMVersionRequest.pldmCommand = PLDM_GET_PLDM_VERSION;
	getPLDMVersionRequest.pldmType = PLDM_BASE;
	add_next_request_to_queue(&getPLDMVersionRequest);

	struct rde_pldm_request getPLDMCommandsRequest;
	getPLDMCommandsRequest.pldmCommand = PLDM_GET_PLDM_COMMANDS;
	getPLDMCommandsRequest.pldmType = PLDM_BASE;
	add_next_request_to_queue(&getPLDMCommandsRequest);

	struct rde_pldm_request getPLDMTypesRequest;
	getPLDMTypesRequest.pldmCommand = PLDM_GET_PLDM_TYPES;
	getPLDMTypesRequest.pldmType = PLDM_BASE;
	add_next_request_to_queue(&getPLDMTypesRequest);
}

rde_requester_rc_t pldm_base_start_discovery(struct requester_base_context *ctx, uint8_t *eid)
{
	ctx->initialized = true;
	ctx->eid = *eid;
	ctx->current_command = PLDM_INIT_DISCOVERY_COMMAND;
	currentRequestPtr = -1;
	lastRequestPtr = -1;

	add_base_requests_to_request_queue();
	return RDE_REQUESTER_SUCCESS;
}

int pldm_get_request_queue_size()
{
	if (currentRequestPtr == -1) {
		return 0;
	}
	if (currentRequestPtr == lastRequestPtr) {
		return 1;
	}
	return lastRequestPtr - currentRequestPtr + 1;
}

rde_requester_rc_t pldm_base_get_next_request(struct requester_base_context *ctx,
			       struct rde_pldm_request **request)
{
	IGNORE(ctx);
	if (currentRequestPtr == -1) {
		return RDE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	}

	if (currentRequestPtr == lastRequestPtr) {
		*request = requestBuffer[currentRequestPtr];
		currentRequestPtr = -1;
		lastRequestPtr = -1;
		return RDE_REQUESTER_SUCCESS;
	}

	*request = requestBuffer[currentRequestPtr++];
	return RDE_REQUESTER_SUCCESS;
}

// TODO: Call handlers for TID, Version, types and commands
rde_requester_rc_t pldm_base_push_response(struct requester_base_context *ctx, void *resp_msg,
			    size_t resp_size)
{
	// TODO: Remove ignores once implemented
	IGNORE((uint8_t *)resp_msg);
	IGNORE(resp_size);

	switch (ctx->current_command) {
	case PLDM_GET_TID:
		// call TID handler - decode_get_tid_resp()
		// decode_get_tid_resp((pldm_msg *)resp_msg, size_t
		// payload_length, 	uint8_t *completion_code, uint8_t *tid);
		// Update context
		return RDE_REQUESTER_SUCCESS;

	case PLDM_GET_PLDM_VERSION:
		// call pldm version handler - decode_get_version_resp()
		// Update context
		return RDE_REQUESTER_SUCCESS;

	case PLDM_GET_PLDM_TYPES:
		// call pldm types handler - decode_get_types_resp()
		// Update context
		return RDE_REQUESTER_SUCCESS;

	case PLDM_GET_PLDM_COMMANDS:
		// call pldm commands handler - decode_get_commands_resp()
		// Update context
		return RDE_REQUESTER_SUCCESS;
	}

	return RDE_REQUESTER_NOT_RDE_MSG;
}