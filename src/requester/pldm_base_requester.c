#include "requester/pldm_base_requester.h"

#include "base.h"
#include "pldm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pldm_base_requester_rc_t
pldm_base_init_context(struct requester_base_context *ctx,
		       const char *device_id, int net_id)
{
	if (ctx->initialized) {
		fprintf(stderr, "No memory allocated to base context\n");
		return PLDM_BASE_CONTEXT_INITIALIZATION_ERROR;
	}

	ctx->initialized = true;
	ctx->requester_status = PLDM_BASE_REQUESTER_NO_PENDING_ACTION;
	strcpy(ctx->dev_name, device_id);
	ctx->net_id = net_id;
	return PLDM_BASE_REQUESTER_SUCCESS;
}

pldm_base_requester_rc_t
pldm_base_start_discovery(struct requester_base_context *ctx)
{
	if (!ctx->initialized &&
	    ctx->requester_status != PLDM_BASE_REQUESTER_NO_PENDING_ACTION) {
		return PLDM_BASE_CONTEXT_NOT_READY;
	}
	ctx->next_command = PLDM_GET_TID;
	ctx->requester_status = PLDM_BASE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
	return PLDM_BASE_REQUESTER_SUCCESS;
}

pldm_base_requester_rc_t
pldm_base_get_next_request(struct requester_base_context *ctx,
			   uint8_t instance_id, struct pldm_msg *request)
{
	int rc;
	switch (ctx->next_command) {
	case PLDM_GET_TID: {
		rc = encode_get_tid_req(instance_id, request);
		break;
	}
	case PLDM_GET_PLDM_TYPES: {
		rc = encode_get_types_req(instance_id, request);
		break;
	}
	case PLDM_GET_PLDM_VERSION: {
		uint8_t pldm_type = ctx->command_pldm_type;
		rc = encode_get_version_req(instance_id, /*transfer_handle=*/0,
					    PLDM_GET_FIRSTPART, pldm_type,
					    request);
		break;
	}

	case PLDM_GET_PLDM_COMMANDS: {
		uint8_t pldmType = ctx->command_pldm_type;
		rc = encode_get_commands_req(instance_id, pldmType,
					     ctx->pldm_versions[pldmType],
					     request);
		break;
	}

	default:
		return PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	}

	if (rc) {
		fprintf(stderr, "Unable to encode request with rc: %d", rc);
		return PLDM_BASE_REQUESTER_ENCODING_REQUEST_FAILURE;
	}
	return PLDM_BASE_REQUESTER_SUCCESS;
}

// TODO(@harshtya): Add code for pldm_base_push_response() 