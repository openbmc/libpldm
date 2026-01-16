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

/*
 * The canonical definition is in base.h. These declarations remain  for
 * historical compatibility
 */
ssize_t pldm_base_ver2str(const ver32_t *version, char *buffer,
			  size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif
