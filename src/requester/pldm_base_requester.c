#include "requester/pldm_base_requester.h"
#include "base.h"
#include "pldm.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

pldm_base_requester_rc_t
pldm_base_start_discovery(struct requester_base_context *ctx)
{
	ctx->initialized = true;
	ctx->next_command = PLDM_GET_TID;
	ctx->command_status = COMMAND_NOT_STARTED;
	return PLDM_BASE_REQUESTER_SUCCESS;
}

pldm_base_requester_rc_t
pldm_base_get_next_request(struct requester_base_context *ctx,
			   uint8_t instance_id, struct pldm_msg *request)
{
	int rc;
	switch (ctx->next_command) {
	case PLDM_GET_TID:
		rc = encode_get_tid_req(instance_id, request);
		break;
	case PLDM_GET_PLDM_TYPES:
		rc = encode_get_types_req(instance_id, request);
		break;

	case PLDM_GET_PLDM_VERSION: {
		uint8_t pldm_type = ctx->command_pldm_type;
		rc = encode_get_version_req(instance_id, 0, PLDM_GET_FIRSTPART,
					    pldm_type, request);
		break;
	}

	case PLDM_GET_PLDM_COMMANDS: {
		uint8_t pldm_type = ctx->command_pldm_type;
		rc = encode_get_commands_req(instance_id, pldm_type,
					     ctx->pldm_versions[pldm_type],
					     request);
		break;
	}

	default:
		return PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	}
	if (rc) {
		fprintf(stderr,
			"Unable to encode request with rc: %d and errno: %d",
			rc, errno);
		return PLDM_BASE_REQUESTER_ENCODING_REQUEST_FAILURE;
	}
	return PLDM_BASE_REQUESTER_SUCCESS;
}

pldm_base_requester_rc_t
pldm_base_push_response(struct requester_base_context *ctx, void *resp_msg,
			size_t resp_size)
{
	switch (ctx->next_command) {
	case PLDM_GET_TID: {
		uint8_t cc;
		uint8_t tid;
		int rc =
		    decode_get_tid_resp((struct pldm_msg *)resp_msg,
					resp_size - sizeof(struct pldm_msg_hdr),
					&cc, &tid);
		if (rc || cc) {
			ctx->command_status = COMMAND_FAILED;
			fprintf(stderr,
				"Response decode failed with rc: %d, "
				"completion code: %d and err: %d",
				rc, cc, errno);
			return PLDM_BASE_REQUESTER_NOT_RESP_MSG;
		}
		ctx->tid = tid;
		ctx->command_status = COMMAND_COMPLETED;
		ctx->next_command = PLDM_GET_PLDM_TYPES;
		ctx->command_status = COMMAND_NOT_STARTED;
		return PLDM_BASE_REQUESTER_SUCCESS;
	}

	case PLDM_GET_PLDM_TYPES: {
		uint8_t cc;
		int rc = decode_get_types_resp((struct pldm_msg *)resp_msg,
					       resp_size -
						   sizeof(struct pldm_msg_hdr),
					       &cc, ctx->pldm_types);
		if (rc || cc) {
			ctx->command_status = COMMAND_FAILED;
			fprintf(stderr,
				"Response decode failed with rc: %d, "
				"completion code: %d and err: %d",
				rc, cc, errno);
			return PLDM_BASE_REQUESTER_NOT_RESP_MSG;
		}

		ctx->command_status = COMMAND_COMPLETED;

		// TODO: Remove after support for all pldm_type requests to get
		// versions and commands is added
		ctx->command_pldm_type = PLDM_BASE;

		ctx->next_command = PLDM_GET_PLDM_VERSION;
		ctx->command_status = COMMAND_NOT_STARTED;
		return PLDM_BASE_REQUESTER_SUCCESS;
	}

	case PLDM_GET_PLDM_VERSION: {
		// TODO: Add support to get PLDM_VERSION for all the PLDM_TYPES
		// TODO: Add checksum verification
		ver32_t version_out;
		uint8_t cc;
		uint8_t ret_flag;
		uint32_t ret_transfer_handle;
		int rc = decode_get_version_resp(
		    (struct pldm_msg *)resp_msg,
		    resp_size - sizeof(struct pldm_msg_hdr), &cc,
		    &ret_transfer_handle, &ret_flag, &version_out);

		if (rc || cc) {
			ctx->command_status = COMMAND_FAILED;
			fprintf(stderr,
				"Response decode failed with rc: %d, "
				"completion code: %d and err: %d",
				rc, cc, errno);
			return PLDM_BASE_REQUESTER_NOT_RESP_MSG;
		}

		ctx->pldm_versions[ctx->command_pldm_type] = version_out;
		ctx->command_status = COMMAND_COMPLETED;
		ctx->next_command = PLDM_GET_PLDM_COMMANDS;
		ctx->command_status = COMMAND_NOT_STARTED;
		return PLDM_BASE_REQUESTER_SUCCESS;
	}

	case PLDM_GET_PLDM_COMMANDS: {
		// TODO: Add support to get PLDM_COMMANDS for available
		// PLDM_TYPES
		uint8_t cc;
		bitfield8_t pldm_cmds[PLDM_MAX_CMDS_PER_TYPE / 8];
		int rc = decode_get_commands_resp(
		    (struct pldm_msg *)resp_msg,
		    resp_size - sizeof(struct pldm_msg_hdr), &cc,
		    pldm_cmds);
		if (rc || cc) {
			ctx->command_status = COMMAND_FAILED;
			fprintf(stderr,
				"Response decode failed with rc: %d, "
				"completion code: %d and err: %d",
				rc, cc, errno);
			return PLDM_BASE_REQUESTER_NOT_RESP_MSG;
		}

		for (int i = 0; i < (PLDM_MAX_CMDS_PER_TYPE / 8); i++) {
			ctx->pldm_commands[ctx->command_pldm_type][i] = pldm_cmds[i].byte;
		}

		ctx->command_status = COMMAND_COMPLETED;
		ctx->next_command = PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND;
		ctx->command_status = COMMAND_NOT_STARTED;
		return PLDM_BASE_REQUESTER_SUCCESS;
	}

	default:
		return PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	}

	return PLDM_BASE_REQUESTER_NOT_PLDM_BASE_MSG;
}
