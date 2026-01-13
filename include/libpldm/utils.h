/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_UTILS_H
#define LIBPLDM_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libpldm/bcd.h>
#include <libpldm/edac.h>
#include <libpldm/pldm_types.h>

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/** @struct variable_field
 *
 *  Structure representing variable field in the pldm message
 */
struct variable_field {
	const uint8_t *ptr;
	size_t length;
};

/*
 * The canonical definition is in base.h. These declarations remain  for
 * historical compatibility
 */
ssize_t ver2str(const ver32_t *version, char *buffer, size_t buffer_size);
ssize_t pldm_base_ver2str(const ver32_t *version, char *buffer,
			  size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif
