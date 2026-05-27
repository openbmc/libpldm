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

/** @struct pldm_platform_pd_ops
 *
 *  Extensible table of callbacks for a Platform Device responder.
 *  Initialise with zeros; set only the callbacks your application handles.
 */
struct pldm_platform_pd_ops {
	/** Invoked for GET_SENSOR_READING. NULL means unsupported. */
	uint8_t (*get_sensor_reading)(
		void *ctx, const struct pldm_numeric_sensor_value_pdr *pdr,
		bool8_t rearm_event_state,
		struct pldm_platform_pd_sensor_state *state);

	/** Opaque context passed as ctx to get_sensor_reading. */
	void *ctx;
};

/* Static storage can be allocated with the PLDM_SIZEOF_PLDM_PLATFORM_PD macro
 * from <libpldm/sizes.h> */
#define PLDM_ALIGNOF_PLDM_PLATFORM_PD __SIZEOF_POINTER__
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
 *  @param[in] ops      - Required pointer to a pldm_platform_pd_ops table.
 *                        get_sensor_reading may be NULL if the application does
 *                        not support GET_SENSOR_READING.
 *  @param[in] ops_size - sizeof(*ops) as seen by the caller. Pass
 *                        sizeof(struct pldm_platform_pd_ops). This enables
 *                        forward and backward compatibility as the struct grows.
 *
 *  @return a malloc-allocated struct pldm_platform_pd owned by the caller.
 *          pldm_platform_pd_setup() does not need to be called.
 *          Release with free(). Returns NULL on failure.
 */
struct pldm_platform_pd *
pldm_platform_pd_new(const pldm_pdr *pdr, struct pldm_control *control,
		     const struct pldm_platform_pd_ops *ops, size_t ops_size);

/** @brief Initialise a Platform Device responder struct
 *
 *  @param[in] pd           - Pointer to a struct pldm_platform_pd. Applications can
 *                            allocate this in static storage of size
 *                            PLDM_SIZEOF_PLDM_PLATFORM_PD if required.
 *  @param[in] pldm_platform_pd_size - Applications should pass PLDM_SIZEOF_PLDM_PLATFORM_PD, to
 *                            check for consistency with the pd pointer.
 *  @param[in] pdr          - Borrowed pointer to a pldm_pdr repository
 *  @param[in] control      - Optional struct pldm_control
 *  @param[in] ops          - Required pointer to a pldm_platform_pd_ops table.
 *                            get_sensor_reading may be NULL if the application
 *                            does not support GET_SENSOR_READING.
 *  @param[in] ops_size     - sizeof(*ops) as seen by the caller. Pass
 *                            sizeof(struct pldm_platform_pd_ops).
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int pldm_platform_pd_setup(struct pldm_platform_pd *pd,
			   size_t pldm_platform_pd_size, const pldm_pdr *pdr,
			   struct pldm_control *control,
			   const struct pldm_platform_pd_ops *ops,
			   size_t ops_size);

/** @brief Handle a PLDM Platform Monitoring and Control message
 *
 *  @param[in]     pd             - Platform Device responder context
 *  @param[in]     in_msg         - Incoming PLDM message buffer
 *  @param[in]     in_len         - Length of in_msg
 *  @param[out]    out_msg        - Buffer for the outgoing response
 *  @param[in,out] out_len        - Length of out_msg buffer on entry;
 *                                 updated with length written on return
 *
 *  @return 0 on success (and out_len > 0 if a response was written),
 *          a negative errno value on failure.
 */
int pldm_platform_pd_handle_msg(struct pldm_platform_pd *pd, const void *in_msg,
				size_t in_len, void *out_msg, size_t *out_len);

#ifdef __cplusplus
}
#endif
