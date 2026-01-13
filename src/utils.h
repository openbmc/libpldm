// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
#ifndef LIBPLDM_SRC_UTILS_H
#define LIBPLDM_SRC_UTILS_H

#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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

/** @brief Check whether the input time is legal
 *
 *  @param[in] seconds. Value range 0~59
 *  @param[in] minutes. Value range 0~59
 *  @param[in] hours. Value range 0~23
 *  @param[in] day. Value range 1~31
 *  @param[in] month. Value range 1~12
 *  @param[in] year. Value range 1970~
 *  @return true if time is legal,false if time is illegal
 */
bool is_time_legal(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t day,
		   uint8_t month, uint16_t year);

/** @brief Check whether transfer flag is valid
 *
 *  @param[in] transfer_flag - TransferFlag
 *
 *  @return true if transfer flag is valid, false if not
 */
bool is_transfer_flag_valid(uint8_t transfer_flag);
#endif
