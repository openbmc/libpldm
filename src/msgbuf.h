/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef PLDM_MSGBUF_H
#define PLDM_MSGBUF_H

#include "msgbuf/core.h"

#define PLDM__MSGBUF_DEFINE_P(name, mode)                                      \
	struct pldm_msgbuf_##mode _##name LIBPLDM_CC_CLEANUP(                  \
		pldm__msgbuf_##mode##_cleanup) = { NULL, INTMAX_MIN };         \
	struct pldm_msgbuf_##mode *(name) = &(_##name)

#define PLDM_MSGBUF_RO_DEFINE_P(name) PLDM__MSGBUF_DEFINE_P(name, ro)
#define PLDM_MSGBUF_RW_DEFINE_P(name) PLDM__MSGBUF_DEFINE_P(name, rw)

/*
 * Use the C11 `_Generic` keyword to keep pldm_msgbuf related function consistent
 * and to produce compile-time errors when the wrong pldm_msgbuf type is passed.
 *
 * Previously we cast away `const` in `pldm_msgbuf_init_error()`, which was a hack.
 * Instead, introduce:
 *   - pldm_msgbuf_ro: read-only buffer with a `const` cursor
 *   - pldm_msgbuf_rw: read-write buffer with a non-const cursor
 *
 * `pldm_msgbuf_ro` is used by decode APIs to extract payloads into PLDM
 * structures. `pldm_msgbuf_rw` is used by encode APIs to insert payloads from
 * PLDM structures.
 */

#define pldm_msgbuf_init_errno(ctx, minsize, buf, len)                         \
	_Generic((ctx),                                                        \
		struct pldm_msgbuf_rw *: pldm_msgbuf_rw_init_errno,            \
		struct pldm_msgbuf_ro *: pldm_msgbuf_ro_init_errno)(           \
		ctx, minsize, buf, len)

#define pldm_msgbuf_discard(ctx, error)                                        \
	_Generic((ctx),                                                        \
		struct pldm_msgbuf_rw *: pldm_msgbuf_rw_discard,               \
		struct pldm_msgbuf_ro *: pldm_msgbuf_ro_discard)(ctx, error)

#define pldm_msgbuf_complete(ctx)                                              \
	_Generic((ctx),                                                        \
		struct pldm_msgbuf_rw *: pldm_msgbuf_rw_complete,              \
		struct pldm_msgbuf_ro *: pldm_msgbuf_ro_complete)(ctx)

#define pldm_msgbuf_complete_consumed(ctx)                                     \
	_Generic((ctx),                                                        \
		struct pldm_msgbuf_rw *: pldm_msgbuf_rw_complete_consumed,     \
		struct pldm_msgbuf_ro *: pldm_msgbuf_ro_complete_consumed)(    \
		ctx)

#define pldm_msgbuf_validate(ctx)                                              \
	_Generic((ctx),                                                        \
		struct pldm_msgbuf_rw *: pldm_msgbuf_rw_validate,              \
		struct pldm_msgbuf_ro *: pldm_msgbuf_ro_validate)(ctx)

#define pldm_msgbuf_consumed(ctx)                                              \
	_Generic((ctx),                                                        \
		struct pldm_msgbuf_rw *: pldm_msgbuf_rw_consumed,              \
		struct pldm_msgbuf_ro *: pldm_msgbuf_ro_consumed)(ctx)

#define pldm__msgbuf_invalidate(ctx)                                           \
	_Generic((ctx),                                                        \
		struct pldm_msgbuf_rw *: pldm__msgbuf_rw_invalidate,           \
		struct pldm_msgbuf_ro *: pldm__msgbuf_ro_invalidate)(ctx)

#define pldm_msgbuf_span_required(ctx, required, cursor)                       \
	_Generic((ctx),                                                        \
		struct pldm_msgbuf_rw *: pldm_msgbuf_rw_span_required,         \
		struct pldm_msgbuf_ro *: pldm_msgbuf_ro_span_required)(        \
		ctx, required, cursor)

#define pldm_msgbuf_span_until(ctx, trailer, cursor, length)                   \
	_Generic((ctx),                                                        \
		struct pldm_msgbuf_rw *: pldm_msgbuf_rw_span_until,            \
		struct pldm_msgbuf_ro *: pldm_msgbuf_ro_span_until)(           \
		ctx, trailer, cursor, length)

#define pldm_msgbuf_span_string_utf16(ctx, cursor, len)                        \
	_Generic((ctx),                                                        \
		struct pldm_msgbuf_rw *: pldm_msgbuf_rw_span_string_utf16,     \
		struct pldm_msgbuf_ro *: pldm_msgbuf_ro_span_string_utf16)(    \
		ctx, cursor, len)

#define pldm_msgbuf_span_remaining(ctx, cursor, len)                           \
	_Generic((ctx),                                                        \
		struct pldm_msgbuf_rw *: pldm_msgbuf_rw_span_remaining,        \
		struct pldm_msgbuf_ro *: pldm_msgbuf_ro_span_remaining)(       \
		ctx, cursor, len)

#define pldm_msgbuf_span_string_ascii(ctx, cursor, len)                        \
	_Generic((ctx),                                                        \
		struct pldm_msgbuf_rw *: pldm_msgbuf_rw_span_string_ascii,     \
		struct pldm_msgbuf_ro *: pldm_msgbuf_ro_span_string_ascii)(    \
		ctx, cursor, len)

#define pldm_msgbuf_extract_typecheck(ty, fn, dst, ...)                        \
	(pldm_require_obj_type(dst, ty), fn(__VA_ARGS__))

#define pldm_msgbuf_extract_uint8(ctx, dst)                                    \
	pldm_msgbuf_extract_typecheck(uint8_t, pldm__msgbuf_extract_uint8,     \
				      dst, ctx, (void *)&(dst))

#define pldm_msgbuf_extract_int8(ctx, dst)                                     \
	pldm_msgbuf_extract_typecheck(int8_t, pldm__msgbuf_extract_int8, dst,  \
				      ctx, (void *)&(dst))

#define pldm_msgbuf_extract_uint16(ctx, dst)                                   \
	pldm_msgbuf_extract_typecheck(uint16_t, pldm__msgbuf_extract_uint16,   \
				      dst, ctx, (void *)&(dst))

#define pldm_msgbuf_extract_int16(ctx, dst)                                    \
	pldm_msgbuf_extract_typecheck(int16_t, pldm__msgbuf_extract_int16,     \
				      dst, ctx, (void *)&(dst))

#define pldm_msgbuf_extract_uint32(ctx, dst)                                   \
	pldm_msgbuf_extract_typecheck(uint32_t, pldm__msgbuf_extract_uint32,   \
				      dst, ctx, (void *)&(dst))

#define pldm_msgbuf_extract_int32(ctx, dst)                                    \
	pldm_msgbuf_extract_typecheck(int32_t, pldm__msgbuf_extract_int32,     \
				      dst, ctx, (void *)&(dst))

#define pldm_msgbuf_extract_uint64(ctx, dst)                                   \
	pldm_msgbuf_extract_typecheck(uint64_t, pldm__msgbuf_extract_uint64,   \
				      dst, ctx, (void *)&(dst))

#define pldm_msgbuf_extract_int64(ctx, dst)                                    \
	pldm_msgbuf_extract_typecheck(int64_t, pldm__msgbuf_extract_int64,     \
				      dst, ctx, (void *)&(dst))

#define pldm_msgbuf_extract_real32(ctx, dst)                                   \
	pldm_msgbuf_extract_typecheck(real32_t, pldm__msgbuf_extract_real32,   \
				      dst, ctx, (void *)&(dst))

/**
 * Extract the field at the msgbuf cursor into the lvalue named by dst.
 *
 * @param ctx The msgbuf context object
 * @param dst The lvalue into which the field at the msgbuf cursor should be
 *            extracted
 *
 * @return PLDM_SUCCESS on success, otherwise another value on error
 */
#define pldm_msgbuf_extract(ctx, dst)                                          \
	_Generic((dst),                                                        \
		uint8_t: pldm__msgbuf_extract_uint8,                           \
		int8_t: pldm__msgbuf_extract_int8,                             \
		uint16_t: pldm__msgbuf_extract_uint16,                         \
		int16_t: pldm__msgbuf_extract_int16,                           \
		uint32_t: pldm__msgbuf_extract_uint32,                         \
		int32_t: pldm__msgbuf_extract_int32,                           \
		uint64_t: pldm__msgbuf_extract_uint64,                         \
		int64_t: pldm__msgbuf_extract_int64,                           \
		real32_t: pldm__msgbuf_extract_real32)(ctx, (void *)&(dst))

/**
 * Extract the field at the msgbuf cursor into the object pointed-to by dst.
 *
 * @param ctx The msgbuf context object
 * @param dst The pointer to the object into which the field at the msgbuf
 *            cursor should be extracted
 *
 * @return PLDM_SUCCESS on success, otherwise another value on error
 */
#define pldm_msgbuf_extract_p(ctx, dst)                                        \
	_Generic((dst),                                                        \
		uint8_t *: pldm__msgbuf_extract_uint8,                         \
		int8_t *: pldm__msgbuf_extract_int8,                           \
		uint16_t *: pldm__msgbuf_extract_uint16,                       \
		int16_t *: pldm__msgbuf_extract_int16,                         \
		uint32_t *: pldm__msgbuf_extract_uint32,                       \
		int32_t *: pldm__msgbuf_extract_int32,                         \
		uint64_t *: pldm__msgbuf_extract_uint64,                       \
		int64_t *: pldm__msgbuf_extract_int64,                         \
		real32_t *: pldm__msgbuf_extract_real32)(ctx, dst)

#define pldm_msgbuf_insert(dst, src)                                           \
	_Generic((src),                                                        \
		uint8_t: pldm_msgbuf_insert_uint8,                             \
		int8_t: pldm_msgbuf_insert_int8,                               \
		uint16_t: pldm_msgbuf_insert_uint16,                           \
		int16_t: pldm_msgbuf_insert_int16,                             \
		uint32_t: pldm_msgbuf_insert_uint32,                           \
		int32_t: pldm_msgbuf_insert_int32,                             \
		real32_t: pldm_msgbuf_insert_real32,                           \
		uint64_t: pldm_msgbuf_insert_uint64)(dst, src)

/**
 * Insert an array of data into the msgbuf instance
 *
 * @param ctx - The msgbuf instance into which the array of data should be
 *              inserted
 * @param count - The number of array elements to insert
 * @param src - The array object from which elements should be inserted into
                @p ctx
 * @param src_count - The maximum number of elements to insert from @p src
 *
 * Note that both @p count and @p src_count can only be counted by `sizeof` for
 * arrays where `sizeof(*dst) == 1` holds. Specifically, they count the number
 * of array elements and _not_ the object size of the array.
 */
#define pldm_msgbuf_insert_array(dst, count, src, src_count)                   \
	_Generic((*(src)),                                                     \
		uint8_t: pldm_msgbuf_insert_array_uint8,                       \
		char: pldm_msgbuf_insert_array_char)(dst, count, src,          \
						     src_count)

#endif /* BUF_H */
