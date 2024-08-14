/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#ifndef LIBPLDM_COMPILER_H
#define LIBPLDM_COMPILER_H

#if defined __has_attribute
#if __has_attribute(counted_by)
#define libpldm_cc_counted_by(x) __attribute__((counted_by(x)))
#endif
#endif

#ifndef libpldm_cc_counted_by
#define libpldm_cc_counted_by(x)
#endif

#endif
