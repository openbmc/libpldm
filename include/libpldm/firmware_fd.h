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

struct pldm_firmware_component_standalone {
	uint16_t comp_classification;
	uint16_t comp_identifier;
	uint8_t comp_classification_index;

	struct pldm_firmware_version active_ver;
	struct pldm_firmware_version pending_ver;

	bitfield16_t comp_activation_methods;
	bitfield32_t capabilities_during_update;
};

/* An entry for Pass Component Table or Update Component */
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

/* Device-specific callbacks provided by an application.
 * Functions return a ccode unless otherwise specified */
struct pldm_fd_ops {
	uint8_t (*device_identifiers)(void *ctx, uint32_t *ret_descriptors_len,
				      uint8_t *ret_descriptors_count,
				      const uint8_t **ret_descriptors);

	uint8_t (*components)(
		void *ctx, uint16_t *ret_entry_count,
		const struct pldm_firmware_component_standalone ***ret_entries);

	uint8_t (*imageset_versions)(void *ctx,
				     struct pldm_firmware_string *ret_active,
				     struct pldm_firmware_string *ret_pending);

	enum pldm_component_response_codes (*update_component)(
		void *ctx, bool update,
		const struct pldm_firmware_update_component *comp);

	/** @brief Provide the transfer size
	 *
	 *  @param[in] ctx - callback context
	 *  @param[in] ua_max_transfer_size - size requested by the UA.
	 *
	 *  @return The transfer size to use. This will be clamped to
	 *  32 <= size <= ua_max_transfer_size.
	 *  The final data chunk may be shorter.
	 */
	uint32_t (*transfer_size)(void *ctx, uint32_t ua_max_transfer_size);

	/* Returns a TransferComplete code,
	 * enum pldm_firmware_update_common_error_codes or
	 * enum pldm_firmware_update_transfer_result_values.
	 *
	 * PLDM_FWUP_TRANSFER_SUCCESS will accept the data chunk, other codes will
	 * abort the transfer, returning that code as TransferComplete */
	uint8_t (*firmware_data)(
		void *ctx, uint32_t offset, const uint8_t *data, uint32_t len,
		const struct pldm_firmware_update_component *comp);

	/* Returns a VerifyComplete code,
	 * enum pldm_firmware_update_common_error_codes or
	 * enum pldm_firmware_update_verify_result_values.
	 *
	 * If the verify is going to complete asynchronously, implementations set
	 * *ret_pending=true and return PLDM_FWUP_VERIFY_SUCCESS. The FD will then
	 * call verify() again when pldm_fd_progress() is called. ret_progress_percent
	 * can optionally be set, or left default (101) for "Not supported".
	 */
	uint8_t (*verify)(void *ctx,
			  const struct pldm_firmware_update_component *comp,
			  bool *ret_pending, uint8_t *ret_progress_percent);

	/* Returns a ApplyComplete code,
	 * enum pldm_firmware_update_common_error_codes or
	 * enum pldm_firmware_update_verify_result_values.
	 * A code of PLDM_FWUP_APPLY_SUCCESS_WITH_ACTIVATION_METHOD will
	 * currently be converted to 0x00.
	 *
	 * If the apply is going to complete asynchronously, implementations set
	 * *ret_pending=true and return PLDM_FWUP_VERIFY_SUCCESS. The FD will then
	 * call verify() again when pldm_fd_progress() is called. ret_progress_percent
	 * can optionally be set, or left default (101) for "Not supported".
	 */
	uint8_t (*apply)(void *ctx,
			 const struct pldm_firmware_update_component *comp,
			 bool *ret_pending, uint8_t *ret_progress_percent);

	/* Activates new firmware
	 *
	 * The Device implementation is responsible for checking that
	 * expected components have been updated, returning
	 * PLDM_FWUP_INCOMPLETE_UPDATE if not.
	 * ret_estimated_time is a time in seconds to perform self activation,
	 * or may be left as 0. */
	uint8_t (*activate)(void *ctx, bool self_contained,
			    uint16_t *ret_estimated_time);

	/* Cancel Update Component
	 *
	 * Called when a component update is cancelled prior to being applied.
	 * This function is called for both Cancel Update Component
	 * and Cancel Update (when a component is currently in progress). */
	void (*cancel_update_component)(
		void *ctx, const struct pldm_firmware_update_component *comp);

	/* Returns a monotonic timestamp, in milliseconds. Must not go backwards. */
	uint64_t (*now)(void *ctx);
};

/* Static storage can be allocated with
 * PLDM_SIZEOF_PLDM_FD macro */
struct pldm_fd;

pldm_requester_rc_t pldm_fd_setup(struct pldm_fd *fd, size_t pldm_fd_size,
				  struct pldm_control *control,
				  const struct pldm_fd_ops *ops, void *ops_ctx);

pldm_requester_rc_t pldm_fd_handle_msg(struct pldm_fd *fd,
				       uint8_t remote_address,
				       const void *req_msg, size_t req_len,
				       void *resp_msg, size_t *resp_len);

pldm_requester_rc_t pldm_fd_progress(struct pldm_fd *fd, void *req_msg,
				     size_t *req_len, uint8_t *remote_address);

#ifdef __cplusplus
}
#endif
