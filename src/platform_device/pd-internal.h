/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#pragma once

#include <stdint.h>

#include <libpldm/pldm.h>
#include <libpldm/platform.h>
#include <libpldm/pdr.h>
#include <libpldm/platform_device.h>

struct pldm_platform_pd {
	uint8_t (*get_sensor_reading)(
		void *ctx, const struct pldm_numeric_sensor_value_pdr *pdr,
		bool8_t rearm_event_state,
		struct pldm_platform_pd_sensor_state *state);
	void *sensor_ops_ctx;

	/* Borrowed pointer — owned by the application */
	const pldm_pdr *pdr;
};
