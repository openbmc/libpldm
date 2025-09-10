// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
#ifndef LIBPLDM_SRC_TIME_H
#define LIBPLDM_SRC_TIME_H
#include <stdbool.h>
#include <sys/time.h>

/**
* @brief Convert timeval to milli-seconds
*
* @param[in]   tv           The input timeval.
* @return      time in milliseconds.
*/
long libpldm_timeval_to_msec(const struct timeval *tv);

/**
* @brief Check if timeval is valid
*
* @param[in]   tv           The input timeval.
* @return      true if its within acceptable range for poll.
*/
bool libpldm_timeval_is_valid(const struct timeval *tv);

/**
* @brief Get current time
*
* @param[out]   tv           The output current timeval.
* @return      0 on success. Negative error code on failure.
*/
int libpldm_clock_gettimeval(struct timeval *tv);

#endif
