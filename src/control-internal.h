#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <libpldm/pldm.h>
#include <libpldm/utils.h>
#include <compiler.h>
#include <msgbuf.h>

#ifndef PLDM_CONTROL_MAX_VERSION_TYPES
#define PLDM_CONTROL_MAX_VERSION_TYPES 6
#endif

struct pldm_type_versions {
	/* A buffer of ver32_t/uint32_t of version values, followed by crc32 */
	/* NULL for unused entries */
	const void *versions;
	/* Includes the trailing crc32 entry */
	uint8_t versions_count;

	/* A buffer of 32 entries, for commands 0-0xff */
	const bitfield8_t *commands;

	uint8_t pldm_type;
};

struct pldm_control {
	struct pldm_type_versions types[PLDM_CONTROL_MAX_VERSION_TYPES];
};
