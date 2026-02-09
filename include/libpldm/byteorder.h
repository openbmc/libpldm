/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef BYTEORDER_H
#define BYTEORDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <asm/byteorder.h>

#define HTOLE32(X) ((X) = htole32(X))
#define HTOLE16(X) ((X) = htole16(X))
#define LE32TOH(X) ((X) = le32toh(X))
#define LE16TOH(X) ((X) = le16toh(X))

#ifdef __cplusplus
}
#endif

#endif
