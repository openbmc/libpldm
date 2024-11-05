/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <libpldm/pldm.h>
#include <libpldm/base.h>
#include <libpldm/utils.h>
#include <libpldm/control.h>
#include <libpldm/firmware_update.h>

/** @struct pldm_firmware_component_standalone
 *
 *  A PLDM Firmware Update Component representation, for use
 *  with pldm_fd_ops callbacks.
*/
struct pldm_firmware_component_standalone {
	uint16_t comp_classification;
	uint16_t comp_identifier;
	uint8_t comp_classification_index;

	struct pldm_firmware_version active_ver;
	struct pldm_firmware_version pending_ver;

	bitfield16_t comp_activation_methods;
	bitfield32_t capabilities_during_update;
};

/** @struct pldm_firmware_update_component
 *
 * An entry for Pass Component Table or Update Component
*/
struct pldm_firmware_update_component {
	uint16_t comp_classification;
	uint16_t comp_identifier;
	uint8_t comp_classification_index;
	struct pldm_firmware_version version;

	/* Not set for PassComponentTable */
	uint32_t comp_image_size;
	/* Not set for PassComponentTable */
	bitfield32_t update_option_flags;
};

/** @struct pldm_fd_ops
 *
 *  Device-specific callbacks provided by an application,
 *  to define the device update behaviour.
 *
 *  These will be called by the FD responder when pldm_fd_handle_msg()
 *  or pldm_fd_progress() are called by the application.
*/
struct pldm_fd_ops {
	/** @brief Provide PLDM descriptors
	 *
	 *  @param[in] ctx - callback context
	 *  @param[out] ret_descriptors_len - length of ret_descriptors returned buffer
	 *  @param[out] ret_descriptors_count - descriptor count for QueryDeviceIdentifiers
	 *  @param[out] ret_descriptors - a pointer to a descriptors buffer
	 *
	 *  @return pldm_completion_codes
	 */
	uint8_t (*device_identifiers)(void *ctx, uint32_t *ret_descriptors_len,
				      uint8_t *ret_descriptors_count,
				      const uint8_t **ret_descriptors);

	/** @brief Provide PLDM component table
	 *
	 *  @param[in] ctx - callback context
	 *  @param[out] ret_entry_count - length of returned ret_entries
	 *  @param[out] ret_entries - an array of component pointers
	 *
	 *  @return pldm_completion_codes
	 */
	uint8_t (*components)(
		void *ctx, uint16_t *ret_entry_count,
		const struct pldm_firmware_component_standalone ***ret_entries);

	/** @brief Return imageset version
	 *
	 *  @param[in] ctx - callback context
	 *  @param[out] ret_active - ActiveComponentVersion string
	 *  @param[out] ret_active - PendingComponentVersion string
	 *
	 *  @return pldm_completion_codes
	 *
	 *  This is used by the FD responder for GetFirmwareParameters.
	 *  It will be called several times in an update flow.
	 */
	uint8_t (*imageset_versions)(void *ctx,
				     struct pldm_firmware_string *ret_active,
				     struct pldm_firmware_string *ret_pending);

	/** @brief Called on PassComponentTable or UpdateComponent
	 *
	 *  @param[in] ctx - callback context
	 *  @param[in] update - will be set for UpdateComponent, and indicates that
	 *  					an update flow is starting, with the same comp used
	 * 					    for subsequent firmware_data, verify, apply callbacks.
	 *  @param[in] comp - the component being used. The FD implementation
	 *					  will only pass comp that has already been
	 *  				  validated against the pldm_fd_ops.components callback.
	 *
	 *  @return PLDM_CRC_COMP_CAN_BE_UPDATED if the component can be updated.
	 */
	enum pldm_component_response_codes (*update_component)(
		void *ctx, bool update,
		const struct pldm_firmware_update_component *comp);

	/** @brief Provide the transfer size to use
	 *
	 *  @param[in] ctx - callback context
	 *  @param[in] ua_max_transfer_size - size requested by the UA.
	 *
	 *  @return The transfer size to use. This will be clamped to
	 *  32 <= size <= ua_max_transfer_size.
	 *  The final data chunk may be shorter.
	 */
	uint32_t (*transfer_size)(void *ctx, uint32_t ua_max_transfer_size);

	/* @brief Provides firmware update data from the UA
	 *
	 *  @param[in] ctx - callback context
	 *  @param[in] offset - offset of the data
	 *  @param[in] data - firmware data buffer
	 *  @param[in] len - length of data
	 *  @param[in] comp - the relevant component
	 *
	 *  @return TransferComplete code - either
	 *			enum pldm_firmware_update_common_error_codes or
	 *			enum pldm_firmware_update_transfer_result_values.
	 *
	 * PLDM_FWUP_TRANSFER_SUCCESS will accept the data chunk, other codes will
	 * abort the transfer, returning that code as TransferComplete
	 */
	uint8_t (*firmware_data)(
		void *ctx, uint32_t offset, const uint8_t *data, uint32_t len,
		const struct pldm_firmware_update_component *comp);

	/* @brief Requests the application verify the update
	 *
	 *  @param[in] ctx - callback context
	 *  @param[in] comp - the relevant component
	 *  @param[out] ret_pending - set when verify will run asynchronously
	 *  @param[out] ret_progress_percent - can optionally be set
	 *                                     during asynchronous verify,
	 *                                     or leave defaulted (101).
	 *
	 *  @return VerifyComplete code - either
	 *			enum pldm_firmware_update_common_error_codes or
	 *			enum pldm_firmware_update_verify_result_values.
	 *
	 * verify() will only be called once all firmware_data (up to the UA-specified
	 * comp_image_size) has been provided. Implementations should check that length
	 * as part of verification, if not already checked.
	 *
	 * If the verify is going to complete asynchronously, implementations set
	 * *ret_pending=true and return PLDM_FWUP_VERIFY_SUCCESS. The FD will then
	 * call verify() again when pldm_fd_progress() is called.
	 */
	uint8_t (*verify)(void *ctx,
			  const struct pldm_firmware_update_component *comp,
			  bool *ret_pending, uint8_t *ret_progress_percent);

	/* @brief Requests the application apply the update
	 *
	 *  @param[in] ctx - callback context
	 *  @param[in] comp - the relevant component
	 *  @param[out] ret_pending - set when apply will run asynchronously
	 *  @param[out] ret_progress_percent - can optionally be set
	 *                                     during asynchronous apply,
	 *                                     or leave defaulted (101).
	 *
	 *  @return ApplyComplete code - either
	 *			enum pldm_firmware_update_common_error_codes or
	 *			enum pldm_firmware_update_apply_result_values.
	 *
	 * If the apply is going to complete asynchronously, implementations set
	 * *ret_pending=true and return PLDM_FWUP_APPLY_SUCCESS. The FD will then
	 * call apply() again when pldm_fd_progress() is called.
	 */
	uint8_t (*apply)(void *ctx,
			 const struct pldm_firmware_update_component *comp,
			 bool *ret_pending, uint8_t *ret_progress_percent);

	/* @brief Activates new firmware
	 *
	 *  @param[in] ctx - callback context
	 *  @param[in] self_contained - Self Contained Activation is requested
	 *  @param[out] ret_estimated_time - a time in seconds to perform
	 *									 self activation, or may be left as 0.
	 *
	 * The device implementation is responsible for checking that
	 * expected components have been updated, returning
	 * PLDM_FWUP_INCOMPLETE_UPDATE if not.
	 */
	uint8_t (*activate)(void *ctx, bool self_contained,
			    uint16_t *ret_estimated_time);

	/* @brief Cancel Update Component
	 *
	 *  @param[in] ctx - callback context
	 *  @param[in] comp - the relevant component
	 *
	 * Called when a component update is cancelled prior to being applied.
	 * This function is called for both Cancel Update Component
	 * and Cancel Update (when a component is currently in progress). */
	void (*cancel_update_component)(
		void *ctx, const struct pldm_firmware_update_component *comp);

	/* @brief Returns a monotonic timestamp
	 *
	 *  @param[in] ctx - callback context
	 *
	 *  @return timestamp in milliseconds, from an arbitrary origin.
	            Must not go backwards.
	 */
	uint64_t (*now)(void *ctx);
};

/* Static storage can be allocated with
 * PLDM_SIZEOF_PLDM_FD macro */
#define PLDM_ALIGNOF_PLDM_FD 8
struct pldm_fd;

/** @brief Allocate and initialise a FD responder
 *
 * @param[in] ops - Application provided callbacks which define the device
 *                  update behaviour
 * @param[in] ops_ctx - opaque context pointer that will be passed as ctx
 *                      to ops callbacks
 * @param[in] control - an optional struct pldm_control. If provided
 *                      the FD responder will set PLDM FW update type
 *					    and commands for the control.
 *
 * @return a malloced struct pldm_fd, owned by the caller. It should be released
 *         with free(). Returns NULL on failure.
 *
 * This will call pldm_fd_setup() on the allocated pldm_fd.
 */
struct pldm_fd *pldm_fd_new(const struct pldm_fd_ops *ops, void *ops_ctx,
			    struct pldm_control *control);

/** @brief Initialise a FD responder struct
 *
 * @param[in] fd - A pointer to a struct pldm_fd. Applications can allocate this
 *                 in static storage of size PLDM_SIZEOF_PLDM_FD if required.
 * @param[in] pldm_fd_size - applications should pass PLDM_SIZEOF_PLDM_FD, to check
 *                 for consistency with the fd pointer.
 * @param[in] ops - Application provided callbacks which define the device
 *                  update behaviour
 * @param[in] ops_ctx - opaque context pointer that will be passed as ctx
 *                      to ops callbacks
 * @param[in] control - an optional struct pldm_control. If provided
 *                      the FD responder will set PLDM FW update type
 *					    and commands for the control.
 *
 * @return PLDM_REQUESTER_SUCCESS, or a failure code.
 */
pldm_requester_rc_t pldm_fd_setup(struct pldm_fd *fd, size_t pldm_fd_size,
				  const struct pldm_fd_ops *ops, void *ops_ctx,
				  struct pldm_control *control);

/** @brief Handle a PLDM Firmware Update message
 *
 * @param[in] fd
 * @param[in] remote_address - the source address of the message.
 *                             EID for MCTP transport.
 * @param[in] in_msg - PLDM incoming message payload
 * @param[in] in_len - length of in_msg buffer
 * @param[out] out_msg - PLDM outgoing message payload buffer
 * @param[inout] out_len - length of available out_msg buffer, will be updated
 *                         with the length written to out_msg.
 *
 * @return PLDM_REQUESTER_SUCCESS, or a failure code.
 *
 * Will return a message to send if out_len > 0
 * and returning PLDM_REQUESTER_SUCCESS.
 */
pldm_requester_rc_t pldm_fd_handle_msg(struct pldm_fd *fd,
				       uint8_t remote_address,
				       const void *in_msg, size_t in_len,
				       void *out_msg, size_t *out_len);

/** @brief Handle periodic progress events
 *
 * @param[in] fd
 * @param[out] out_msg - PLDM outgoing message payload buffer
 * @param[inout] out_len - length of available out_msg buffer, will be updated
 *                         with the length written to out_msg.
 * @param[out] remote_address - destination address for the message to send.
 *                              EID for MCTP transport.
 * 								This is the address used to initiate the update,
 * 								from a previous pldm_fd_handle_msg call.
 *
 * @return PLDM_REQUESTER_SUCCESS, or a failure code.
 *
 * Will return a message to send to remote_address if out_len > 0
 * and returning PLDM_REQUESTER_SUCCESS.
 *
 * This could be called periodically by the application to send retries
 * during an update flow. A 1 second interval is recommended.
 */
pldm_requester_rc_t pldm_fd_progress(struct pldm_fd *fd, void *out_msg,
				     size_t *out_len, uint8_t *remote_address);

#ifdef __cplusplus
}
#endif
