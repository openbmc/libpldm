/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include "compiler.h"
#include "environ/time.h"

#include <time.h>

LIBPLDM_ABI_TESTING LIBPLDM_CC_WEAK int
libpldm_clock_gettime(clockid_t clockid, struct timespec *ts)
{
	return clock_gettime(clockid, ts);
}
