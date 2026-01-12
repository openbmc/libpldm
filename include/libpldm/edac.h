/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_EDAC_H
#define LIBPLDM_EDAC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Compute Crc8(same as the one used by SMBUS)
 *
 *  @param[in] data - Pointer to the target data
 *  @param[in] size - Size of the data
 *  @return The checksum
 */
uint8_t pldm_edac_crc8(const void *data, size_t size);

/** @brief Compute crc32 (same as the one used by IEEE802.3)
 *
 *  @param[in] data - Pointer to the target data
 *  @param[in] size - Size of the data
 *  @return The checksum
 */
uint32_t pldm_edac_crc32(const void *data, size_t size);

/** @brief Compute cumulative crc32 (same as the one used by IEEE802.3)
 *
 *  @param[in] data - Pointer to the target data
 *  @param[in] size - Size of the data
 *  @param[in] crc - cumulative CRC value
 *  @return The checksum
 */
uint32_t pldm_edac_crc32_extend(const void *data, size_t size, uint32_t crc);

#ifdef __cplusplus
}
#endif

#endif
