/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#ifndef LIBPLDM_COMPILER_H
#define LIBPLDM_COMPILER_H

#if defined __has_attribute

#if __has_attribute(always_inline)
#define LIBPLDM_CC_ALWAYS_INLINE __attribute__((always_inline)) static inline
#endif

#if __has_attribute(counted_by)
#define LIBPLDM_CC_COUNTED_BY(x) __attribute__((counted_by(x)))
#endif

#if __has_attribute(nonnull)
#define LIBPLDM_CC_NONNULL __attribute__((nonnull))
#endif

#if __has_attribute(unavailable)
#define LIBPLDM_CC_UNAVAILABLE __attribute__((unavailable))
#endif

#endif

#ifndef LIBPLDM_CC_ALWAYS_INLINE
#define LIBPLDM_CC_ALWAYS_INLINE static inline
#endif

#ifndef LIBPLDM_CC_COUNTED_BY
#define LIBPLDM_CC_COUNTED_BY(x)
#endif

#ifndef LIBPLDM_CC_NONNULL
#define LIBPLDM_CC_NONNULL
#endif

#ifndef LIBPLDM_CC_UNAVAILABLE
#define LIBPLDM_CC_UNAVAILABLE
#endif

#define LIBPLDM_ITERATOR LIBPLDM_CC_NONNULL LIBPLDM_CC_ALWAYS_INLINE

#endif
