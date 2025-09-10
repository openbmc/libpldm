// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
#ifndef LIBPLDM_SRC_TIME_H
#define LIBPLDM_SRC_TIME_H
#include <stdbool.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
* @brief Get current time
*
* @param[out]   tv           The output current timeval.
* @return      0 on success. Negative error code on failure.
*/
int libpldm_clock_gettimeval(struct timeval *tv);

#ifdef __cplusplus
}
#endif

#endif
