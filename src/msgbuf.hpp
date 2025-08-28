/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef PLDM_MSGBUF_HPP
#define PLDM_MSGBUF_HPP

#ifdef __cplusplus

/*
 * Use the C++ Function Overload to keep pldm_msgbuf related function consistent
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

#include <type_traits>
#include <cstdio>
#include <cstdint>

extern "C" {
#endif

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_span_string_ascii(struct pldm_msgbuf_rw *ctx, void **cursor,
				 size_t *length);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_span_string_ascii(struct pldm_msgbuf_ro *ctx,
				 const void **cursor, size_t *length);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_span_remaining(struct pldm_msgbuf_rw *ctx, void **cursor,
			      size_t *len);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_span_remaining(struct pldm_msgbuf_ro *ctx, const void **cursor,
			      size_t *len);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_span_string_utf16(struct pldm_msgbuf_ro *ctx,
				 const void **cursor, size_t *length);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_span_string_utf16(struct pldm_msgbuf_rw *ctx, void **cursor,
				 size_t *length);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_span_required(struct pldm_msgbuf_ro *ctx, size_t required,
			     const void **cursor);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_span_required(struct pldm_msgbuf_rw *ctx, size_t required,
			     void **cursor);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_span_until(struct pldm_msgbuf_ro *ctx, size_t trailer,
			  const void **cursor, size_t *length);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_span_until(struct pldm_msgbuf_rw *ctx, size_t trailer,
			  void **cursor, size_t *length);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_consumed(struct pldm_msgbuf_ro *ctx);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_consumed(struct pldm_msgbuf_rw *ctx);

LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_ro_invalidate(struct pldm_msgbuf_ro *ctx);

LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_rw_invalidate(struct pldm_msgbuf_rw *ctx);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_validate(struct pldm_msgbuf_ro *ctx);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_validate(struct pldm_msgbuf_rw *ctx);

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_ro_discard(struct pldm_msgbuf_ro *ctx,
						    int error);

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_rw_discard(struct pldm_msgbuf_rw *ctx,
						    int error);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_complete(struct pldm_msgbuf_ro *ctx);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_complete(struct pldm_msgbuf_rw *ctx);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_complete_consumed(struct pldm_msgbuf_ro *ctx);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_complete_consumed(struct pldm_msgbuf_rw *ctx);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_init_errno(struct pldm_msgbuf_ro *ctx, size_t minsize,
			  const void *buf, size_t len);

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_init_errno(struct pldm_msgbuf_rw *ctx, size_t minsize,
			  const void *buf, size_t len);

#ifdef __cplusplus
}
#endif

static inline int pldm_msgbuf_span_string_ascii(struct pldm_msgbuf_rw *ctx,
						void **cursor, size_t *length)
{
	return pldm_msgbuf_rw_span_string_ascii(ctx, cursor, length);
}

static inline int pldm_msgbuf_span_string_ascii(struct pldm_msgbuf_ro *ctx,
						const void **cursor,
						size_t *length)
{
	return pldm_msgbuf_ro_span_string_ascii(ctx, cursor, length);
}

static inline int pldm_msgbuf_span_remaining(struct pldm_msgbuf_rw *ctx,
					     void **cursor, size_t *len)
{
	return pldm_msgbuf_rw_span_remaining(ctx, cursor, len);
}

static inline int pldm_msgbuf_span_remaining(struct pldm_msgbuf_ro *ctx,
					     const void **cursor, size_t *len)
{
	return pldm_msgbuf_ro_span_remaining(ctx, cursor, len);
}

static inline int pldm_msgbuf_span_string_utf16(pldm_msgbuf_ro *ctx,
						const void **cursor,
						size_t *length)
{
	return pldm_msgbuf_ro_span_string_utf16(ctx, cursor, length);
}

static inline int pldm_msgbuf_span_string_utf16(pldm_msgbuf_rw *ctx,
						void **cursor, size_t *length)
{
	return pldm_msgbuf_rw_span_string_utf16(ctx, cursor, length);
}

static inline int pldm_msgbuf_span_required(struct pldm_msgbuf_ro *ctx,
					    size_t required,
					    const void **cursor)
{
	return pldm_msgbuf_ro_span_required(ctx, required, cursor);
}

static inline int pldm_msgbuf_span_required(struct pldm_msgbuf_rw *ctx,
					    size_t required, void **cursor)
{
	return pldm_msgbuf_rw_span_required(ctx, required, cursor);
}

static inline int pldm_msgbuf_span_until(struct pldm_msgbuf_ro *ctx,
					 size_t trailer, const void **cursor,
					 size_t *length)
{
	return pldm_msgbuf_ro_span_until(ctx, trailer, cursor, length);
}

static inline int pldm_msgbuf_span_until(struct pldm_msgbuf_rw *ctx,
					 size_t trailer, void **cursor,
					 size_t *length)
{
	return pldm_msgbuf_rw_span_until(ctx, trailer, cursor, length);
}

static inline int pldm_msgbuf_consumed(struct pldm_msgbuf_ro *ctx)
{
	return pldm_msgbuf_ro_consumed(ctx);
}

static inline int pldm_msgbuf_consumed(struct pldm_msgbuf_rw *ctx)
{
	return pldm_msgbuf_rw_consumed(ctx);
}

// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
static inline int pldm__msgbuf_invalidate(struct pldm_msgbuf_ro *ctx)
{
	return pldm__msgbuf_ro_invalidate(ctx);
}

// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
static inline int pldm__msgbuf_invalidate(struct pldm_msgbuf_rw *ctx)
{
	return pldm__msgbuf_rw_invalidate(ctx);
}

static inline int pldm_msgbuf_validate(struct pldm_msgbuf_ro *ctx)
{
	return pldm_msgbuf_ro_validate(ctx);
}

static inline int pldm_msgbuf_validate(struct pldm_msgbuf_rw *ctx)
{
	return pldm_msgbuf_rw_validate(ctx);
}

static inline int pldm_msgbuf_discard(struct pldm_msgbuf_ro *ctx, int error)
{
	return pldm_msgbuf_ro_discard(ctx, error);
}

static inline int pldm_msgbuf_discard(struct pldm_msgbuf_rw *ctx, int error)
{
	return pldm_msgbuf_rw_discard(ctx, error);
}

static inline int pldm_msgbuf_complete(struct pldm_msgbuf_ro *ctx)
{
	return pldm_msgbuf_ro_complete(ctx);
}

static inline int pldm_msgbuf_complete(struct pldm_msgbuf_rw *ctx)
{
	return pldm_msgbuf_rw_complete(ctx);
}

static inline int pldm_msgbuf_complete_consumed(struct pldm_msgbuf_ro *ctx)
{
	return pldm_msgbuf_ro_complete_consumed(ctx);
}

static inline int pldm_msgbuf_complete_consumed(struct pldm_msgbuf_rw *ctx)
{
	return pldm_msgbuf_rw_complete_consumed(ctx);
}

static inline int pldm_msgbuf_init_errno(struct pldm_msgbuf_ro *ctx,
					 size_t minsize, const void *buf,
					 size_t len)
{
	return pldm_msgbuf_ro_init_errno(ctx, minsize, buf, len);
}

static inline int pldm_msgbuf_init_errno(struct pldm_msgbuf_rw *ctx,
					 size_t minsize, const void *buf,
					 size_t len)
{
	return pldm_msgbuf_rw_init_errno(ctx, minsize, buf, len);
}

#endif /* BUF_HPP */
