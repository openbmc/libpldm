/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include "time-utils.h"

#include <time.h>
#include <limits.h>

static void timespec_to_timeval(const struct timespec *ts, struct timeval *tv)
{
	tv->tv_sec = ts->tv_sec;
	tv->tv_usec = ts->tv_nsec / 1000;
}

/* Overflow safety must be upheld before call */
long libpldm_timeval_to_msec(const struct timeval *tv)
{
	return tv->tv_sec * 1000 + tv->tv_usec / 1000;
}

/* If calculations on `tv` don't overflow then operations on derived
 * intervals can't either.
 */
bool libpldm_timeval_is_valid(const struct timeval *tv)
{
	if (tv->tv_sec < 0 || tv->tv_usec < 0 || tv->tv_usec >= 1000000) {
		return false;
	}

	if (tv->tv_sec > (LONG_MAX - tv->tv_usec / 1000) / 1000) {
		return false;
	}

	return true;
}

int __attribute__((weak)) libpldm_clock_gettimeval(struct timeval *tv)
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
