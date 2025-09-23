// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
#ifndef LIBPLDM_SRC_UTILS_H
#define LIBPLDM_SRC_UTILS_H

#include <errno.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Validate the CRC32 checksum of the given data.
 *
 * @param[in]   expected    The expected CRC32 value.
 * @param[in]   data        Pointer to the data to validate.
 * @param[in]   size        Size of the data in bytes.
 * @return      0           if the checksum matches,
 *              -EUCLEAN    if the checksum mismatches,
 *              -EINVAL     if the arguments are invalid
 */
int pldm_edac_crc32_validate(uint32_t expected, const void *data, size_t size);
#endif
