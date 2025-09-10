// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
#ifndef LIBPLDM_ENVIRON_TIME_H
#define LIBPLDM_ENVIRON_TIME_H
#include <sys/time.h>
#include <time.h>

/** @brief Weak symbol wrapping clock_gettime() */
int libpldm_clock_gettime(clockid_t clockid, struct timespec *ts);

#endif
