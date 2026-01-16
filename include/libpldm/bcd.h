/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_BCD_H
#define LIBPLDM_BCD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Convert bcd number(uint8_t) to decimal
 *  @param[in] bcd - bcd number
 *  @return the decimal number
 */
uint8_t pldm_bcd_bcd2dec8(uint8_t bcd);

/** @brief Convert decimal number(uint8_t) to bcd
 *  @param[in] dec - decimal number
 *  @return the bcd number
 */
uint8_t pldm_bcd_dec2bcd8(uint8_t dec);

/** @brief Convert bcd number(uint16_t) to decimal
 *  @param[in] bcd - bcd number
 *  @return the decimal number
 */
uint16_t pldm_bcd_bcd2dec16(uint16_t bcd);

/** @brief Convert decimal number(uint16_t) to bcd
 *  @param[in] dec - decimal number
 *  @return the bcd number
 */
uint16_t pldm_bcd_dec2bcd16(uint16_t dec);

/** @brief Convert bcd number(uint32_t) to decimal
 *  @param[in] bcd - bcd number
 *  @return the decimal number
 */
uint32_t pldm_bcd_bcd2dec32(uint32_t bcd);

/** @brief Convert decimal number(uint32_t) to bcd
 *  @param[in] dec - decimal number
 *  @return the bcd number
 */
uint32_t pldm_bcd_dec2bcd32(uint32_t dec);

#ifdef __cplusplus
}
#endif

#endif
