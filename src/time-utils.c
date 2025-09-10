/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include "time-utils.h"

#include <time.h>
#include <limits.h>

static void timespec_to_timeval(const struct timespec *ts, struct timeval *tv)
{
	tv->tv_sec = ts->tv_sec;
	tv->tv_usec = ts->tv_nsec / 1000;
}

LIBPLDM_ABI_TESTING
__attribute__((weak)) int libpldm_clock_gettimeval(struct timeval *tv)
{
	struct timespec now;
	int rc;

	rc = clock_gettime(CLOCK_MONOTONIC, &now);
	if (rc < 0) {
		return rc;
	}

	timespec_to_timeval(&now, tv);

	return 0;
}
