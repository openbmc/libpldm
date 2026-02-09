/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_SRC_ENVIRON_ERRNO_H
#define LIBPLDM_SRC_ENVIRON_ERRNO_H

#include <errno.h>

/* EUCLEAN is not defined on Zephyr RTOS libc */
#ifndef EUCLEAN
#define EUCLEAN 117
#endif

#endif
