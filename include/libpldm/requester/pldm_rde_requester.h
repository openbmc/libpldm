#ifndef PLDM_RDE_REQUESTER_H
#define PLDM_RDE_REQUESTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "base.h"
#include "pldm_base_requester.h"
#include "pldm_rde.h"

// Currently RDE supports a maximum of 50 dictionary resources
#define MAX_RESOURCE_IDS 50

typedef enum rde_requester_return_codes {
	PLDM_RDE_REQUESTER_SUCCESS = 0,
	PLDM_RDE_REQUESTER_NOT_PLDM_RDE_MSG = -1,
	PLDM_RDE_REQUESTER_NOT_RESP_MSG = -2,
	PLDM_RDE_REQUESTER_SEND_FAIL = -3,
	PLDM_RDE_REQUESTER_RECV_FAIL = -4,
	PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND = -5,
	PLDM_RDE_REQUESTER_ENCODING_REQUEST_FAILURE = -6,
	PLDM_RDE_CONTEXT_INITIALIZATION_ERROR = -7,
	PLDM_RDE_CONTEXT_NOT_READY = -8,
	PLDM_RDE_NO_PDR_RESOURCES_FOUND = -9
} pldm_rde_requester_rc_t;

typedef enum rde_requester_status {
	PLDM_RDE_REQUESTER_REQUEST_FAILED = -1,
	PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST = 0,
	PLDM_RDE_REQUESTER_WAITING_FOR_RESPONSE = 1,
	PLDM_RDE_REQUESTER_NO_PENDING_ACTION = 2
} rde_req_status_t;

typedef enum rde_context_status {
	CONTEXT_FREE = 0,
	CONTEXT_BUSY = 1,
	CONTEXT_CONTINUE = 2
} rde_context_status;
/**
 * @brief This will hold the PDR Resource information
 * This could be modified when the P&M PLDM Type is implemented
 */
struct pdr_resource {
	uint8_t resource_id_index;
	uint32_t transfer_handle;
	uint8_t dictionary_format;
	uint8_t transfer_operation;
	uint8_t schema_class;
};
/**
 * @brief The entire RDE Update operation is captured by the following struct
 */
struct rde_update_operation {
	uint8_t request_id;
	uint32_t resource_id;
	uint16_t operation_id;
	uint8_t operation_type;
	uint8_t operation_status;
	uint8_t percentage_complete;
	uint32_t completion_time;
	uint32_t result_transfer_handle;

	// Request Data
	union pldm_rde_operation_flags operation_flags;
	uint32_t send_data_transfer_handle;
	uint8_t operation_locator_length;
	uint8_t *operation_locator;
	uint32_t request_payload_length;
	uint8_t *request_payload;

	// Response Data
	uint32_t resp_payload_length;
	uint8_t *response_data;
	union pldm_rde_op_execution_flags *resp_operation_flags;
	union pldm_rde_permission_flags *resp_permission_flags;
	struct pldm_rde_varstring *resp_etag;

	// op complete
	uint8_t completion_code;
};
/**
 * @brief The entire RDE Read operation is captured by the following struct
 */
struct rde_read_operation {
	uint8_t request_id;
	uint32_t resource_id;
	uint16_t operation_id;
	uint8_t operation_type;
	uint8_t operation_status;
	uint8_t percentage_complete;
	uint32_t completion_time;
	uint32_t result_transfer_handle;

	// Request Data
	union pldm_rde_operation_flags operation_flags;
	uint32_t send_data_transfer_handle;
	uint8_t operation_locator_length;
	uint8_t *operation_locator;
	uint32_t request_payload_length;
	uint8_t *request_payload;

	// Response Data
	uint32_t resp_payload_length;
	uint8_t *response_data;
	union pldm_rde_op_execution_flags *resp_operation_flags;
	union pldm_rde_permission_flags *resp_permission_flags;
	struct pldm_rde_varstring *resp_etag;

	// For multipart receive
	uint32_t transfer_handle;
	uint8_t transfer_operation;

	// op complete
	uint8_t completion_code;
};
/**
 * @brief RDE Requester context
 */
struct pldm_rde_requester_context {
	uint8_t context_status;
	int next_command;
	uint8_t requester_status;
	struct pdr_resource *current_pdr_resource;
	void *operation_ctx;
};
/**
 * @brief Context Manager- Manages all the contexts and common information per
 * rde device
 */
struct pldm_rde_requester_manager {
	bool initialized;
	uint8_t n_ctx;
	char device_name[8];
	int net_id;

	uint8_t mc_concurrency;
	uint32_t mc_transfer_size;
	bitfield16_t *mc_feature_support;
	uint32_t negotiated_transfer_size;

	uint32_t resource_ids[MAX_RESOURCE_IDS];
	uint8_t number_of_resources;

	struct pldm_rde_device_info *device;
	// Pointer to an array of contexts of size n_ctx.
	struct pldm_rde_requester_context *ctx;
	// A callback to free the pldm_rde_requester_context memory.
	void (*free_requester_ctx)(void *ctx_memory);
};
/**
 * @brief Callback function for letting the requester handle response payload
 */
typedef void (*callback_funct)(struct pldm_rde_requester_manager *manager,
			       struct pldm_rde_requester_context *ctx,
			       /*payload_array*/ uint8_t **,
			       /*payload_length*/ uint32_t,
			       /*has_checksum*/ bool);
/**
 * @brief Initializes the context for PLDM RDE discovery commands
 *
 * @param[in] device_id - Device id of the RDE device
 * @param[in] net_id - Network ID to distinguish between RDE Devices
 * @param[in] manager - Pointer to Context Manager
 * @param[in] mc_concurrency - Concurrency supported by MC
 * @param[in] mc_transfer_size - Transfer Size of MC
 * @param[in] mc_features - Pointer to MC Features
 * @param[in] number_of_resources - Number of resources supported (until PDR is
 * implemented)
 * @param[in] resource_id_address - The initial resource id index to begin
 * Discovery
 * @param[in] alloc_requester_ctx - Pointer to a function to allocated contexts
 * for a RDE device
 * @param[in] free_requester_ctx - Pointer to a function that frees allocated
 * memory to contexts
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t
pldm_rde_init_context(const char *device_id, int net_id,
		      struct pldm_rde_requester_manager *manager,
		      uint8_t mc_concurrency, uint32_t mc_transfer_size,
		      bitfield16_t *mc_features, uint8_t number_of_resources,
		      uint32_t *resource_id_address,
		      struct pldm_rde_requester_context *(*alloc_requester_ctx)(
			  uint8_t number_of_ctx),

		      // Callback function to clean any context memory
		      void (*free_requester_ctx)(void *ctx_memory));
/**
 * @brief Sets the first command to be triggered for base discovery and sets
 * the status of context to "Ready to PICK
 *
 * @param[in] ctx - pointer to a context which is to be initialized
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t
pldm_rde_start_discovery(struct pldm_rde_requester_context *ctx);
/**
 * @brief Pushes the response values to the context of the PLDM_RDE type
 * command that was executed and updates the command status. It alse sets
 * the next_command attribute of the context based on the last executed
 * command.
 *
 * @param[in] manager - Context Manager
 * @param[in] ctx - a pointer to the context
 * @param[in] resp_msg - a pointer to the response message that the caller
 * received
 * @param[in] resp_size - size of the response message payload
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t
pldm_rde_discovery_push_response(struct pldm_rde_requester_manager *manager,
				 struct pldm_rde_requester_context *ctx,
				 void *resp_msg, size_t resp_size);
/**
 * @brief Gets the next Discovery Command required for RDE
 *
 * @param[in] instance_id - Getting the instance_id
 * @param[in] manager - PLDM RDE Manager object
 * @param[in] current_ctx - PLDM RDE Requester context which would be
 * responsible for all actions of discovery commands
 * @param[out] request - Request byte stream
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t pldm_rde_get_next_discovery_command(
    uint8_t instance_id, struct pldm_rde_requester_manager *manager,
    struct pldm_rde_requester_context *current_ctx, struct pldm_msg *request);
/**
 * @brief Creates the RDE context required for RDE operation. Sets the initial
 * state of the context
 *
 * @param[in] current_ctx - Context to be set with the inital state
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_rde_requester_rc_t
pldm_rde_create_context(struct pldm_rde_requester_context *current_ctx);

#ifdef __cplusplus
}
#endif

#endif /* PLDM_RDE_REQUESTER_H */
