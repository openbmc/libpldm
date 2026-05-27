/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <libpldm/pldm.h>
#include <libpldm/base.h>
#include <libpldm/control.h>
#include <libpldm/platform.h>
#include <libpldm/pdr.h>

/** @struct pldm_platform_pd_sensor_state
 *
 *  Holds all output fields for a GetSensorReading operation.
 */
struct pldm_platform_pd_sensor_state {
	uint8_t operational_state;
	uint8_t event_enable;
	uint8_t present_state;
	uint8_t previous_state;
	uint8_t event_state;
	union_sensor_data_size current_reading;
};

/* Static storage can be allocated with the PLDM_SIZEOF_PLDM_PLATFORM_PD macro
 * from <libpldm/sizes.h> */
#define PLDM_ALIGNOF_PLDM_PLATFORM_PD 8
struct pldm_platform_pd;

/** @brief Allocate and initialise a Platform Device responder
 *
 *  @param[in] pdr      - Borrowed pointer to a pldm_pdr repository.
 *                        The repository must remain valid for the lifetime of
 *                        the pldm_platform_pd instance. The application is responsible
 *                        for populating it before calls to pldm_platform_pd_handle_msg.
 *  @param[in] control  - Optional struct pldm_control. If provided, the PD
 *                        responder will register PLDM Platform type and its
 *                        supported commands.
 *
 *  @return a malloc-allocated struct pldm_platform_pd owned by the caller.
 *          Release with free(). Returns NULL on failure.
 */
struct pldm_platform_pd *pldm_platform_pd_new(const pldm_pdr *pdr,
					      struct pldm_control *control);

/** @brief Initialise a Platform Device responder struct
 *
 *  @param[in] pd           - Pointer to a struct pldm_platform_pd. Applications can
 *                            allocate this in static storage of size
 *                            PLDM_SIZEOF_PLDM_PLATFORM_PD if required.
 *  @param[in] pldm_platform_pd_size - Applications should pass PLDM_SIZEOF_PLDM_PLATFORM_PD, to
 *                            check for consistency with the pd pointer.
 *  @param[in] pdr          - Borrowed pointer to a pldm_pdr repository
 *  @param[in] control      - Optional struct pldm_control
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int pldm_platform_pd_setup(struct pldm_platform_pd *pd,
			   size_t pldm_platform_pd_size, const pldm_pdr *pdr,
			   struct pldm_control *control);

/** @brief Handle a PLDM Platform Monitoring and Control message
 *
 *  @param[in]    pd             - Platform Device responder context
 *  @param[in]    in_msg         - Incoming PLDM message buffer
 *  @param[in]    in_len         - Length of in_msg
 *  @param[out]   out_msg        - Buffer for the outgoing response
 *  @param[inout] out_len        - Length of out_msg buffer on entry;
 *                                 updated with length written on return
 *
 *  @return 0 on success (and out_len > 0 if a response was written),
 *          a negative errno value on failure.
 */
int pldm_platform_pd_handle_msg(struct pldm_platform_pd *pd, const void *in_msg,
				size_t in_len, void *out_msg, size_t *out_len);

/** @brief Set the get_sensor_reading callback on an existing Platform Device
 *
 *  @param[in] pd                 - Platform Device responder context
 *  @param[in] get_sensor_reading - Callback invoked for GET_SENSOR_READING
 *                                  requests; must not be NULL
 *  @param[in] opt_ctx            - Opaque context pointer passed as ctx to
 *                                  the callback
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int pldm_platform_pd_set_sensor_ops(
	struct pldm_platform_pd *pd,
	uint8_t (*get_sensor_reading)(
		void *ctx, const struct pldm_numeric_sensor_value_pdr *pdr,
		bool8_t rearm_event_state,
		struct pldm_platform_pd_sensor_state *state),
	void *opt_ctx);

#ifdef __cplusplus
}
#endif
