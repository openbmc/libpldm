#pragma once

#include <libpldm/pldm.h>
#include <libpldm/base.h>

#ifdef __cplusplus
extern "C" {
#endif

enum pldm_control_completion_codes {
	PLDM_CONTROL_INVALID_DATA_TRANSFER_HANDLE = 0x80,
	PLDM_CONTROL_INVALID_TRANSFER_OPERATION_FLAG = 0x81,
	PLDM_CONTROL_INVALID_PLDM_TYPE_IN_REQUEST_DATA = 0x83,
	PLDM_CONTROL_INVALID_PLDM_VERSION_IN_REQUEST_DATA = 0x84,
};

// Static storage can be allocated with PLDM_SIZEOF_CONTROL macro */
struct pldm_control;

/** @brief Handle a PLDM Control message
 *
 * @param[in] control
 * @param[in] req_msg - PLDM incoming request message payload
 * @param[in] req_len - length of req_msg buffer
 * @param[out] resp_msg - PLDM outgoing response message payload buffer
 * @param[inout] resp_len - length of available resp_msg buffer, will be updated
 *                         with the length written to resp_msg.
 *
 * @return 0 on success, a negative errno value on failure.
 *
 * Will provide a response to send when resp_len > 0 and returning 0.
 */
int pldm_control_handle_msg(struct pldm_control *control, const void *req_msg,
			    size_t req_len, void *resp_msg, size_t *resp_len);

/** @brief Initialise a struct pldm_control
 *
 * @param[in] control
 * @param[in] pldm_control_size - pass PLDM_SIZEOF_CONTROL
 *
 * @return 0 on success, a negative errno value on failure.
 */
int pldm_control_setup(struct pldm_control *control, size_t pldm_control_size);

/** @brief Add a PLDM type to report.
 *
 * @param[in] control
 * @param[in] type - PLDM type, enum pldm_supported_types
 * @param[in] versions - list of versions for GetPLDMVersion response.
 *			 This is an array of 32-bit version values, followed by
 *			 a CRC32 over the version values. The size of this buffer
 * 			 is 4*versions_count. The versions buffer must remain
 *			 present for the duration of the pldm_control's lifetime.
 * @param[in] versions_count - number of entries in versions, including the trailing CRC32.
 * @param[in] commands - pointer to an array of bitfield8_t[8], for GetPLDMCommands
 * 			 response for this type. The buffer must remain
 *			 present for the duration of the pldm_control's lifetime.
 *
 * @return 0 on success, a negative errno value on failure.
 */
int pldm_control_add_type(struct pldm_control *control, uint8_t pldm_type,
			  const void *versions, size_t versions_count,
			  const bitfield8_t *commands);

/** @brief Set the maximum transfer part size for a PLDM type.
 *
 * DSP0240 §9.6.2: NegotiateTransferParameters part sizes are per PLDM type.
 * The type must have been registered with pldm_control_add_type first.
 *
 * @param[in] control
 * @param[in] pldm_type  - the type to configure
 * @param[in] max_size   - max bytes per part (PLDM header + payload).
 *                         Must be >= 256 (DSP0240 §9.6.2 minimum), or zero to
 *                         remove this type from multipart participation.
 *
 * @return 0 on success, -ENOENT if the type is not registered,
 *         -EINVAL if max_size is non-zero and less than 256.
 */
int pldm_control_set_multipart_size(struct pldm_control *control,
				    uint8_t pldm_type, uint16_t max_size);

/** @brief Query the negotiated multipart transfer size for a PLDM type.
 *
 * Returns the part size agreed in the most recent successful
 * NegotiateTransferParameters exchange for the given type.
 * Returns -ENODATA if negotiation has not yet occurred or has been
 * invalidated by a subsequent call to pldm_control_set_multipart_size.
 *
 * @param[in]  control
 * @param[in]  pldm_type
 * @param[out] negotiated_size - negotiated size in bytes
 *
 * @return 0 on success, -ENOENT if type not registered, -ENODATA if not
 *         yet negotiated.
 */
int pldm_control_get_multipart_size(struct pldm_control *control,
				    uint8_t pldm_type, uint16_t *negotiated_size);
#ifdef __cplusplus
}
#endif
