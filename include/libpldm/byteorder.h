/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef BYTEORDER_H
#define BYTEORDER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __ZEPHYR__
#include <zephyr/sys/byteorder.h>

#define HTOLE32(X) ((X) = sys_cpu_to_le32(X))
#define HTOLE16(X) ((X) = sys_cpu_to_le16(X))
#define LE32TOH(X) ((X) = sys_le32_to_cpu(X))
#define LE16TOH(X) ((X) = sys_le16_to_cpu(X))

#ifdef CONFIG_LITTLE_ENDIAN
#define __LITTLE_ENDIAN_BITFIELD
#elif defined(CONFIG_BIG_ENDIAN)
#define __BIG_ENDIAN_BITFIELD
#endif

#else
#include <asm/byteorder.h>

#define HTOLE32(X) ((X) = htole32(X))
#define HTOLE16(X) ((X) = htole16(X))
#define LE32TOH(X) ((X) = le32toh(X))
#define LE16TOH(X) ((X) = le16toh(X))

#endif

#ifdef __cplusplus
}
#endif

#endif
