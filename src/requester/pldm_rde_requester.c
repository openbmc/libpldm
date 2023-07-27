#include "requester/pldm_rde_requester.h"

#include "base.h"
#include "pldm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pldm_rde_requester_rc_t
pldm_rde_init_context(const char *device_id, int net_id,
		      struct pldm_rde_requester_manager *manager,
		      uint8_t mc_concurrency, uint32_t mc_transfer_size,
		      bitfield16_t *mc_features, uint8_t number_of_resources,
		      uint32_t *resource_id_address,
		      struct pldm_rde_requester_context *(*alloc_requester_ctx)(
			  uint8_t number_of_ctx),
		      void (*free_requester_ctx)(void *ctx_memory))
{
	fprintf(stdout, "Initializing Context Manager...\n");
	if (manager == NULL) {
		fprintf(stderr, "Memory not allocated to context manager.\n");
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}

	if ((device_id == NULL) || (strlen(device_id) == 0) ||
	    (strlen(device_id) > 8)) {
		fprintf(stderr, "Incorrect device id provided\n");
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}

	if ((alloc_requester_ctx == NULL) || (free_requester_ctx == NULL)) {
		fprintf(stderr, "No callback functions for handling request "
				"contexts found.\n");
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}

	manager->initialized = true;
	manager->mc_concurrency = mc_concurrency;
	manager->mc_transfer_size = mc_transfer_size;
	manager->mc_feature_support = mc_features;
	strcpy(manager->device_name, device_id);
	manager->net_id = net_id;
	// The resource IDs will be set during PDR Retrieval when PDR Type is
	// implemented in the future
	for (int i = 0; i < number_of_resources; i++) {
		manager->resource_ids[i] = *resource_id_address;
		resource_id_address++;
	}
	manager->number_of_resources = number_of_resources;
	// alloactor returns the index of the first context in the array
	manager->ctx = alloc_requester_ctx(
	    mc_concurrency);

	manager->free_requester_ctx = *free_requester_ctx;
	return PLDM_RDE_REQUESTER_SUCCESS;
}

pldm_rde_requester_rc_t
pldm_rde_start_discovery(struct pldm_rde_requester_context *ctx)
{
	if (ctx->context_status == CONTEXT_BUSY) {
		return PLDM_RDE_CONTEXT_NOT_READY;
	}
	ctx->next_command = PLDM_NEGOTIATE_REDFISH_PARAMETERS;
	return PLDM_RDE_REQUESTER_SUCCESS;
}

pldm_rde_requester_rc_t
pldm_rde_create_context(struct pldm_rde_requester_context *ctx)
{
	if (ctx == NULL) {
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}
	ctx->context_status = CONTEXT_FREE;
	ctx->next_command = PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	ctx->requester_status = PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
	return PLDM_RDE_REQUESTER_SUCCESS;
}

pldm_rde_requester_rc_t pldm_rde_get_next_discovery_command(
    uint8_t instance_id, struct pldm_rde_requester_manager *manager,
    struct pldm_rde_requester_context *current_ctx, struct pldm_msg *request)
{
	if (!manager->initialized) {
		return PLDM_RDE_CONTEXT_INITIALIZATION_ERROR;
	}

	if (current_ctx->context_status == CONTEXT_BUSY) {
		return PLDM_RDE_CONTEXT_NOT_READY;
	}

	int rc = 0;
	switch (current_ctx->next_command) {
	case PLDM_NEGOTIATE_REDFISH_PARAMETERS: {
		rc = encode_negotiate_redfish_parameters_req(
		    instance_id, manager->mc_concurrency,
		    manager->mc_feature_support, request);
		break;
	}
	case PLDM_NEGOTIATE_MEDIUM_PARAMETERS: {
		rc = encode_negotiate_medium_parameters_req(
		    instance_id, manager->mc_transfer_size, request);
		break;
	}
	default:
		rc = PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	}
	if (rc) {
		fprintf(stderr, "Unable to encode request with rc: %d\n", rc);
		return PLDM_RDE_REQUESTER_ENCODING_REQUEST_FAILURE;
	}
	return PLDM_RDE_REQUESTER_SUCCESS;
}

pldm_rde_requester_rc_t
pldm_rde_discovery_push_response(struct pldm_rde_requester_manager *manager,
				 struct pldm_rde_requester_context *ctx,
				 void *resp_msg, size_t resp_size)
{
	int rc = 0;
	switch (ctx->next_command) {
	case PLDM_NEGOTIATE_REDFISH_PARAMETERS: {
		uint8_t completion_code;
		struct pldm_rde_device_info *deviceInfo =
		    malloc(sizeof(struct pldm_rde_device_info));
		rc = decode_negotiate_redfish_parameters_resp(
		    resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
		    &completion_code, deviceInfo);
		if (rc || completion_code) {
			ctx->requester_status =
			    PLDM_RDE_REQUESTER_REQUEST_FAILED;
		}
		manager->device = deviceInfo;
		ctx->next_command = PLDM_NEGOTIATE_MEDIUM_PARAMETERS;
		ctx->context_status = CONTEXT_FREE;
		ctx->requester_status =
		    PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST;
		return PLDM_RDE_REQUESTER_SUCCESS;
	}
	case PLDM_NEGOTIATE_MEDIUM_PARAMETERS: {
		uint8_t completion_code;
		uint32_t max_device_transfer_size;
		rc = decode_negotiate_medium_parameters_resp(
		    resp_msg, resp_size - sizeof(struct pldm_msg_hdr),
		    &completion_code, &max_device_transfer_size);
		if (rc || completion_code) {
			ctx->requester_status =
			    PLDM_RDE_REQUESTER_REQUEST_FAILED;
		}
		manager->device->device_maximum_transfer_chunk_size =
		    max_device_transfer_size;

		manager->negotiated_transfer_size =
		    max_device_transfer_size > manager->mc_transfer_size
			? manager->mc_transfer_size
			: max_device_transfer_size;

		ctx->next_command = 0;
		ctx->context_status = CONTEXT_FREE;
		ctx->requester_status = PLDM_RDE_REQUESTER_NO_PENDING_ACTION;
		return PLDM_RDE_REQUESTER_SUCCESS;
	}

	default:
		return PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND;
	}
}
