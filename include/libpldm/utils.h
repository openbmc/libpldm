/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_UTILS_H
#define LIBPLDM_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libpldm/bcd.h>
#include <libpldm/edac.h>
#include <libpldm/pldm_types.h>

#include <stdbool.h>
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

#ifdef __cplusplus
}
#endif

#endif
