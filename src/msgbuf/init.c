/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#include "msgbuf.h"

#include <stdint.h>
#include <limits.h>

/* Exported function for ABI stability */
int pldm__msgbuf_init_errno(const uint8_t **cursor, intmax_t *remaining,
			    size_t minsize, const void *buf, size_t len)
{
	*cursor = NULL;

	if ((minsize > len)) {
		return pldm__msgbuf_set_invalid(remaining);
	}

#if INTMAX_MAX < SIZE_MAX
	if (len > INTMAX_MAX) {
		return pldm__msgbuf_set_invalid(remaining);
	}
#endif

	if (UINTPTR_MAX - (uintptr_t)buf < len) {
		return pldm__msgbuf_set_invalid(remaining);
	}

	*cursor = (const uint8_t *)buf;
	*remaining = (intmax_t)len;

	return 0;
}
