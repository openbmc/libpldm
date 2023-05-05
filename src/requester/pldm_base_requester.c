#include "requester/pldm_base_requester.h"

#include "base.h"
#include "pldm.h"

#include <errno.h>
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

// Get the next pldm_type from the byte array
// Each bit in a byte represent a pldm type whether it is supported or not
pldm_base_requester_rc_t
pldm_base_get_next_pldm_type(struct requester_base_context *ctx,
			     uint8_t current_type, uint8_t *next_type)
{
	int byte = current_type / 8;
	int bit = current_type % 8;
	bool is_bit_set = false;

	while (byte < 8 && !is_bit_set) {
		uint8_t current_byte = ctx->pldm_types[byte].byte;
		int index = bit + 1;

		// Skip already traversed bits of the current byte
		current_byte = current_byte >> (bit + 1);
		while (current_byte) {
			if (current_byte & 1) {
				is_bit_set = true;
				bit = index;
				break;
			}
			index++;
			current_byte = current_byte >> 1;
		}

		if (!is_bit_set) {
			byte++;
			bit = -1; // We need to start from 0th bit of
				  // the next byte
		}
	}

	if (byte == 8 && !is_bit_set) {
		return PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	}
	*next_type = bit + byte * 8;
	return PLDM_BASE_REQUESTER_SUCCESS;
}

pldm_base_requester_rc_t
pldm_base_push_response(struct requester_base_context *ctx, void *resp_msg,
			size_t resp_size)
{
	switch (ctx->next_command) {
	case PLDM_GET_TID: {
		uint8_t completionCode;
		uint8_t tid;
		int rc = decode_get_tid_resp(
		    resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
		    &completionCode, &tid);
		if (rc || completionCode) {
			ctx->requester_status =
			    PLDM_BASE_REQUESTER_REQUEST_FAILED;
			fprintf(stderr,
				"Response decode failed with rc: %d, "
				"completion code: %d and err: %d",
				rc, completionCode, errno);
			return PLDM_BASE_REQUESTER_NOT_RESP_MSG;
		}
		ctx->tid = tid;
		ctx->next_command = PLDM_GET_PLDM_TYPES;
		ctx->requester_status =
		    PLDM_BASE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		return PLDM_BASE_REQUESTER_SUCCESS;
	}

	case PLDM_GET_PLDM_TYPES: {
		uint8_t completionCode;
		int rc = decode_get_types_resp(
		    resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
		    &completionCode, ctx->pldm_types);
		if (rc || completionCode) {
			ctx->requester_status =
			    PLDM_BASE_REQUESTER_REQUEST_FAILED;
			fprintf(stderr,
				"Response decode failed with rc: %d, "
				"completion code: %d and err: %d",
				rc, completionCode, errno);
			return PLDM_BASE_REQUESTER_NOT_RESP_MSG;
		}

		// Setting the initial pldm_type as PLDM_BASE
		ctx->command_pldm_type = PLDM_BASE;
		ctx->next_command = PLDM_GET_PLDM_VERSION;
		ctx->requester_status =
		    PLDM_BASE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		return PLDM_BASE_REQUESTER_SUCCESS;
	}

	case PLDM_GET_PLDM_VERSION: {
		ver32_t versionOut;
		uint8_t completionCode;
		uint8_t retFlag;
		uint32_t retTransferHandle;
		int rc = decode_get_version_resp(
		    resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
		    &completionCode, &retTransferHandle, &retFlag, &versionOut);

		if (rc || completionCode) {
			ctx->requester_status =
			    PLDM_BASE_REQUESTER_REQUEST_FAILED;
			fprintf(stderr,
				"Response decode failed with rc: %d, "
				"completion code: %d and err: %d",
				rc, completionCode, errno);
			return PLDM_BASE_REQUESTER_NOT_RESP_MSG;
		}
		uint8_t current_pldm_type = ctx->command_pldm_type;

		ctx->pldm_versions[current_pldm_type] = versionOut;

		// Get the next version of the next pldm command
		// if the type is not available then move to next
		rc = pldm_base_get_next_pldm_type(ctx, current_pldm_type,
						  &(ctx->command_pldm_type));

		if (rc == PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND) {
			ctx->next_command = PLDM_GET_PLDM_COMMANDS;
			ctx->requester_status =
			    PLDM_BASE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
			ctx->command_pldm_type =
			    PLDM_BASE; // Setting the first PLMD_TYPE
		} else {
			ctx->next_command = PLDM_GET_PLDM_VERSION;
			ctx->requester_status =
			    PLDM_BASE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		}

		return PLDM_BASE_REQUESTER_SUCCESS;
	}

	case PLDM_GET_PLDM_COMMANDS: {
		uint8_t completionCode;
		bitfield8_t pldmCmds[PLDM_MAX_CMDS_PER_TYPE / 8];
		int rc = decode_get_commands_resp(
		    resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
		    &completionCode, pldmCmds);
		if (rc || completionCode) {
			ctx->requester_status =
			    PLDM_BASE_REQUESTER_REQUEST_FAILED;
			fprintf(stderr,
				"Response decode failed with rc: %d, "
				"completion code: %d and err: %d",
				rc, completionCode, errno);
			return PLDM_BASE_REQUESTER_NOT_RESP_MSG;
		}
		uint8_t current_pldm_type = ctx->command_pldm_type;

		for (int i = 0; i < (PLDM_MAX_CMDS_PER_TYPE / 8); i++) {
			ctx->pldm_commands[current_pldm_type][i] =
			    pldmCmds[i].byte;
		}

		rc = pldm_base_get_next_pldm_type(ctx, current_pldm_type,
						  &(ctx->command_pldm_type));

		if (rc == PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND) {
			ctx->next_command =
			    PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND;
			ctx->requester_status =
			    PLDM_BASE_REQUESTER_NO_PENDING_ACTION;
			// Setting base PLDM type the PLDM Commands
			ctx->command_pldm_type = PLDM_BASE;
		} else {
			ctx->next_command = PLDM_GET_PLDM_COMMANDS;
			ctx->requester_status =
			    PLDM_BASE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		}
		return PLDM_BASE_REQUESTER_SUCCESS;
	}

	default:
		return PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	}

	return PLDM_BASE_REQUESTER_NOT_PLDM_BASE_MSG;
}