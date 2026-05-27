/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#pragma once

#include <stdint.h>

#include <libpldm/pldm.h>
#include <libpldm/platform.h>
#include <libpldm/pdr.h>
#include <libpldm/platform_pd.h>

struct pldm_platform_pd {
	struct pldm_platform_pd_ops ops;

	/* Borrowed pointer — owned by the application */
	const pldm_pdr *pdr;
};
