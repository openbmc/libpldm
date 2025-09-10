/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include "time.h"

#include <time.h>
#include <limits.h>

LIBPLDM_ABI_TESTING
__attribute__((weak)) int libpldm_clock_gettime(struct timespec *ts)
{
	return clock_gettime(CLOCK_MONOTONIC, ts);
}
