/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef PLDM_MSGBUF_CORE_H
#define PLDM_MSGBUF_CORE_H

/*
 * Historically, many of the structs exposed in libpldm's public headers are
 * defined with __attribute__((packed)). This is unfortunate: it gives the
 * impression that a wire-format buffer can be cast to the message type to make
 * the message's fields easily accessible. As it turns out, that's not
 * that's valid for several reasons:
 *
 * 1. Casting the wire-format buffer to a struct of the message type doesn't
 *    abstract the endianness of message field values
 *
 * 2. Some messages contain packed tagged union fields which cannot be properly
 *    described in a C struct.
 *
 * The msgbuf APIs exist to assist with (un)packing the wire-format in a way
 * that is type-safe, spatially memory-safe, endian-safe, performant, and
 * free of undefined-behaviour. Message structs that are added to the public
 * library API should no-longer be marked __attribute__((packed)), and the
 * implementation of their encode and decode functions must exploit the msgbuf
 * API.
 *
 * However, we would like to allow implementation of codec functions in terms of
 * msgbuf APIs even if they're decoding a message into a (historically) packed
 * struct. Some of the complexity that follows is a consequence of the packed/
 * unpacked conflict.
 */

#ifdef __cplusplus
/*
 * Fix up C11's _Static_assert() vs C++'s static_assert().
 *
 * Can we please have nice things for once.
 */
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#define _Static_assert(...) static_assert(__VA_ARGS__)

extern "C" {
#endif

#include "compiler.h"

#include <libpldm/pldm_types.h>

#include <endian.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <uchar.h>

/*
 * We can't use static_assert() outside of some other C construct. Deal
 * with high-level global assertions by burying them in an unused struct
 * declaration, that has a sole member for compliance with the requirement that
 * types must have a size.
*/
static struct {
	static_assert(
		INTMAX_MAX != SIZE_MAX,
		"Extraction and insertion value comparisons may be broken");
	static_assert(INTMAX_MIN + INTMAX_MAX <= 0,
		      "Extraction and insertion arithmetic may be broken");
	int compliance;
} build_assertions LIBPLDM_CC_UNUSED;

struct pldm_msgbuf_rw {
	uint8_t *cursor;
	intmax_t remaining;
};

struct pldm_msgbuf_ro {
	const uint8_t *cursor;
	intmax_t remaining;
};

LIBPLDM_CC_ALWAYS_INLINE
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
void pldm__msgbuf_cleanup(const void *cursor LIBPLDM_CC_UNUSED,
			  intmax_t remaining LIBPLDM_CC_UNUSED)
{
	assert(cursor == NULL && remaining == INTMAX_MIN);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
void pldm__msgbuf_rw_cleanup(struct pldm_msgbuf_rw *ctx LIBPLDM_CC_UNUSED)
{
	pldm__msgbuf_cleanup((const void *)ctx->cursor, ctx->remaining);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
void pldm__msgbuf_ro_cleanup(struct pldm_msgbuf_ro *ctx LIBPLDM_CC_UNUSED)
{
	pldm__msgbuf_cleanup((const void *)ctx->cursor, ctx->remaining);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
int pldm__msgbuf_set_invalid(intmax_t *remaining)
{
	*remaining = INTMAX_MIN;
	return -EOVERFLOW;
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
int pldm__msgbuf_rw_invalidate(struct pldm_msgbuf_rw *ctx)
{
	return pldm__msgbuf_set_invalid(&ctx->remaining);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
int pldm__msgbuf_ro_invalidate(struct pldm_msgbuf_ro *ctx)
{
	return pldm__msgbuf_set_invalid(&ctx->remaining);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_init_errno(const uint8_t **cursor, intmax_t *remaining,
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

/**
 * @brief Initialize pldm buf struct for buf extractor
 *
 * @param[out] ctx - pldm_msgbuf_rw context for extractor
 * @param[in] minsize - The minimum required length of buffer `buf`
 * @param[in] buf - buffer to be extracted
 * @param[in] len - size of buffer
 *
 * @return 0 on success, otherwise an error code appropriate for the current
 *         personality.
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm_msgbuf_rw_init_errno(struct pldm_msgbuf_rw *ctx, size_t minsize,
			  const void *buf, size_t len)
{
	return pldm__msgbuf_init_errno((const uint8_t **)&ctx->cursor,
				       &ctx->remaining, minsize, buf, len);
}

/**
 * @brief Initialize pldm buf struct for buf extractor
 *
 * @param[out] ctx - pldm_msgbuf_ro context for extractor
 * @param[in] minsize - The minimum required length of buffer `buf`
 * @param[in] buf - buffer to be extracted
 * @param[in] len - size of buffer
 *
 * @return 0 on success, otherwise an error code appropriate for the current
 *         personality.
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm_msgbuf_ro_init_errno(struct pldm_msgbuf_ro *ctx, size_t minsize,
			  const void *buf, size_t len)
{
	return pldm__msgbuf_init_errno(&ctx->cursor, &ctx->remaining, minsize,
				       buf, len);
}

/**
 * @brief Validate buffer overflow state
 *
 * @param[in] ctx - msgbuf context for extractor
 *
 * @return PLDM_SUCCESS if there are zero or more bytes of data that remain
 * unread from the buffer. Otherwise, PLDM_ERROR_INVALID_LENGTH indicates that a
 * prior accesses would have occurred beyond the bounds of the buffer, and
 * PLDM_ERROR_INVALID_DATA indicates that the provided context was not a valid
 * pointer.
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
int pldm__msgbuf_validate(intmax_t remaining)
{
	if (remaining < 0) {
		return -EOVERFLOW;
	}

	return 0;
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int pldm_msgbuf_ro_validate(struct pldm_msgbuf_ro *ctx)
{
	return pldm__msgbuf_validate(ctx->remaining);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int pldm_msgbuf_rw_validate(struct pldm_msgbuf_rw *ctx)
{
	return pldm__msgbuf_validate(ctx->remaining);
}

/**
 * @brief Test whether a message buffer has been exactly consumed
 *
 * @param[in] ctx - pldm_msgbuf context for extractor
 *
 * @return 0 iff there are zero bytes of data that remain unread from the buffer
 * and no overflow has occurred. Otherwise, -EBADMSG if the buffer has not been
 * completely consumed, or -EOVERFLOW if accesses were attempted beyond the
 * bounds of the buffer.
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
int pldm__msgbuf_consumed(intmax_t remaining)
{
	if (remaining > 0) {
		return -EBADMSG;
	}

	if (remaining < 0) {
		return -EOVERFLOW;
	}

	return 0;
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int pldm_msgbuf_ro_consumed(struct pldm_msgbuf_ro *ctx)
{
	return pldm__msgbuf_consumed(ctx->remaining);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int pldm_msgbuf_rw_consumed(struct pldm_msgbuf_rw *ctx)
{
	return pldm__msgbuf_consumed(ctx->remaining);
}

/**
 * @brief End use of a msgbuf under error conditions
 *
 * @param[in] ctx - The msgbuf instance to discard
 * @param[in] error - The error value to propagate
 *
 * Under normal conditions use of a msgbuf instance must be ended using @ref
 * pldm_msgbuf_complete or one of its related APIs. Under error conditions, @ref
 * pldm_msgbuf_discard should be used instead, as it makes it straight-forward
 * to finalise the msgbuf while propagating the existing error code.
 *
 * @return The value provided in @param error
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
int pldm__msgbuf_discard(const uint8_t **cursor, intmax_t *remaining, int error)
{
	*cursor = NULL;
	pldm__msgbuf_set_invalid(remaining);
	return error;
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int pldm_msgbuf_rw_discard(struct pldm_msgbuf_rw *ctx, int error)
{
	return pldm__msgbuf_discard((const uint8_t **)&ctx->cursor,
				    &ctx->remaining, error);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int pldm_msgbuf_ro_discard(struct pldm_msgbuf_ro *ctx, int error)
{
	return pldm__msgbuf_discard(&ctx->cursor, &ctx->remaining, error);
}

/**
 * @brief Complete the pldm_msgbuf_rw instance
 *
 * @param[in] ctx - pldm_msgbuf_rw context for extractor
 *
 * @return 0 if all buffer accesses were in-bounds, -EOVERFLOW otherwise.
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int pldm_msgbuf_rw_complete(struct pldm_msgbuf_rw *ctx)
{
	return pldm_msgbuf_rw_discard(ctx, pldm_msgbuf_rw_validate(ctx));
}

/**
 * @brief Complete the pldm_msgbuf_ro instance
 *
 * @param[in] ctx - pldm_msgbuf_ro context for extractor
 *
 * @return 0 if all buffer accesses were in-bounds, -EOVERFLOW otherwise.
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int pldm_msgbuf_ro_complete(struct pldm_msgbuf_ro *ctx)
{
	return pldm_msgbuf_ro_discard(ctx, pldm_msgbuf_ro_validate(ctx));
}

/**
 * @brief Complete the pldm_msgbuf_rw instance, and check that the underlying buffer
 * has been entirely consumed without overflow
 *
 * @param[in] ctx - pldm_msgbuf_rw context
 *
 * @return 0 if all buffer access were in-bounds and completely consume the
 * underlying buffer. Otherwise, -EBADMSG if the buffer has not been completely
 * consumed, or -EOVERFLOW if accesses were attempted beyond the bounds of the
 * buffer.
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int pldm_msgbuf_rw_complete_consumed(struct pldm_msgbuf_rw *ctx)
{
	return pldm_msgbuf_rw_discard(ctx, pldm_msgbuf_rw_consumed(ctx));
}

/**
 * @brief Complete the pldm_msgbuf_ro instance, and check that the underlying buffer
 * has been entirely consumed without overflow
 *
 * @param[in] ctx - pldm_msgbuf_ro context
 *
 * @return 0 if all buffer access were in-bounds and completely consume the
 * underlying buffer. Otherwise, -EBADMSG if the buffer has not been completely
 * consumed, or -EOVERFLOW if accesses were attempted beyond the bounds of the
 * buffer.
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int pldm_msgbuf_ro_complete_consumed(struct pldm_msgbuf_ro *ctx)
{
	return pldm_msgbuf_ro_discard(ctx, pldm_msgbuf_ro_consumed(ctx));
}

/*
 * Exploit the pre-processor to perform type checking by macro substitution.
 *
 * A C type is defined by its alignment as well as its object
 * size, and compilers have a hammer to enforce it in the form of
 * `-Waddress-of-packed-member`. Due to the unpacked/packed struct conflict in
 * the libpldm public API this presents a problem: Naively attempting to use the
 * msgbuf APIs on a member of a packed struct would yield an error.
 *
 * The msgbuf APIs are implemented such that data is moved through unaligned
 * pointers in a safe way, but to mitigate `-Waddress-of-packed-member` we must
 * make the object pointers take a trip through `void *` at its API boundary.
 * That presents a bit too much of an opportunity to non-surgically remove your
 * own foot, so here we set about doing something to mitigate that as well.
 *
 * pldm_msgbuf_extract_typecheck() exists to enforce pointer type correctness
 * only for the purpose of object sizes, disregarding alignment. We have a few
 * constraints that cause some headaches:
 *
 * 1. We have to perform the type-check before a call through a C function,
 *    as the function must take the object pointer argument as `void *`.
 *    Essentially, this constrains us to doing something with macros.
 *
 * 2. While libpldm is a C library, its test suite is written in C++ to take
 *    advantage of gtest.
 *
 * 3. Ideally we'd do something with C's `static_assert()`, however
 *    `static_assert()` is defined as void, and as we're constrained to macros,
 *    using `static_assert()` would require a statement-expression
 *
 * 4. Currently the project is built with `-std=c17`. CPP statement-expressions
 *    are a GNU extension. We prefer to avoid switching to `-std=gnu17` just for
 *    the purpose of enabling statement-expressions in this one instance.
 *
 * 5. We can achieve a conditional build error using `pldm_require_obj_type()`,
 *    however it's implemented in terms of `_Generic()`, which is not available
 *    in C++.
 *
 * Combined this means we need separate solutions for C and C++.
 *
 * For C, as we don't have statement-expressions, we need to exploit some other
 * language feature to inject a `pldm_require_obj_type()` prior to the msgbuf
 * API function call. We also have to take care of the fact that the call-sites
 * may be in the context of a variable assignment for error-handling purposes.
 * The key observation is that we can use the comma operator as a sequence point
 * to order the type check before the API call, discarding the "result" value of
 * the type check and yielding the return value of the API call.
 *
 * C++ could be less of a headache than the C as we can leverage template
 * functions. An advantage of template functions is that while their definition
 * is driven by instantion, the definition does not appear at the source
 * location of the instantiation, which gives it a great leg-up over the problems
 * we have in the C path. However, the use of the msgbuf APIs in the test suite
 * still makes things somewhat tricky, as the call-sites in the test suite are
 * wrapped up in EXPECT_*() gtest macros. Ideally we'd implement functions that
 * takes both the object type and the required type as template arguments, and
 * then define the object pointer parameter as `void *` for a call through to
 * the appropriate msgbuf API. However, because the msgbuf API call-sites are
 * encapsulated in gtest macros, use of commas in the template specification
 * causes pre-processor confusion. In this way we're constrained to only one
 * template argument per function.
 *
 * Implement the C++ path using template functions that take the destination
 * object type as a template argument, while the name of the function symbols
 * are derived from the required type. The manual implementations of these
 * appear at the end of the header. The type safety is actually enforced
 * by `static_assert()` this time, as we can use statements as we're not
 * constrained to an expression in the templated function body.
 *
 * The invocations of pldm_msgbuf_extract_typecheck() typically result in
 * double-evaluation of some arguments. We're not yet bothered by this for two
 * reasons:
 *
 * 1. The nature of the current call-sites are such that there are no
 *    argument expressions that result in undesirable side-effects
 *
 * 2. It's an API internal to the libpldm implementation, and we can fix things
 *    whenever something crops up the violates the observation in 1.
 */

/**
 * @brief pldm_msgbuf extractor for a uint8_t
 *
 * @param[in,out] ctx - pldm_msgbuf context for extractor
 * @param[out] dst - destination of extracted value
 *
 * @return PLDM_SUCCESS if buffer accesses were in-bounds,
 * PLDM_ERROR_INVALID_LENGTH otherwise.
 * PLDM_ERROR_INVALID_DATA if input a invalid ctx
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_uint8(struct pldm_msgbuf_ro *ctx, void *dst)
{
	if (ctx->remaining >= (intmax_t)sizeof(uint8_t)) {
		assert(ctx->cursor);
		memcpy(dst, ctx->cursor, sizeof(uint8_t));
		ctx->cursor++;
		ctx->remaining -= sizeof(uint8_t);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(uint8_t)) {
		ctx->remaining -= sizeof(uint8_t);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_ro_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_int8(struct pldm_msgbuf_ro *ctx, void *dst)
{
	if (ctx->remaining >= (intmax_t)sizeof(int8_t)) {
		assert(ctx->cursor);
		memcpy(dst, ctx->cursor, sizeof(int8_t));
		ctx->cursor++;
		ctx->remaining -= sizeof(int8_t);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(int8_t)) {
		ctx->remaining -= sizeof(int8_t);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_ro_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_uint16(struct pldm_msgbuf_ro *ctx, void *dst)
{
	uint16_t ldst;

	// Check for underflow while tracking the magnitude of the buffer overflow
	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(ldst) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(ldst)) {
		assert(ctx->cursor);

		// Use memcpy() to have the compiler deal with any alignment
		// issues on the target architecture
		memcpy(&ldst, ctx->cursor, sizeof(ldst));

		// Only assign the target value once it's correctly decoded
		ldst = le16toh(ldst);

		// Allow storing to unaligned
		memcpy(dst, &ldst, sizeof(ldst));

		ctx->cursor += sizeof(ldst);
		ctx->remaining -= sizeof(ldst);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(ldst)) {
		ctx->remaining -= sizeof(ldst);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_ro_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_int16(struct pldm_msgbuf_ro *ctx, void *dst)
{
	int16_t ldst;

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(ldst) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(ldst)) {
		assert(ctx->cursor);
		memcpy(&ldst, ctx->cursor, sizeof(ldst));
		ldst = le16toh(ldst);
		memcpy(dst, &ldst, sizeof(ldst));
		ctx->cursor += sizeof(ldst);
		ctx->remaining -= sizeof(ldst);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(ldst)) {
		ctx->remaining -= sizeof(ldst);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_ro_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_uint32(struct pldm_msgbuf_ro *ctx, void *dst)
{
	uint32_t ldst;

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(ldst) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(ldst)) {
		assert(ctx->cursor);
		memcpy(&ldst, ctx->cursor, sizeof(ldst));
		ldst = le32toh(ldst);
		memcpy(dst, &ldst, sizeof(ldst));
		ctx->cursor += sizeof(ldst);
		ctx->remaining -= sizeof(ldst);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(ldst)) {
		ctx->remaining -= sizeof(ldst);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_ro_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_int32(struct pldm_msgbuf_ro *ctx, void *dst)
{
	int32_t ldst;

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(ldst) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(ldst)) {
		assert(ctx->cursor);
		memcpy(&ldst, ctx->cursor, sizeof(ldst));
		ldst = le32toh(ldst);
		memcpy(dst, &ldst, sizeof(ldst));
		ctx->cursor += sizeof(ldst);
		ctx->remaining -= sizeof(ldst);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(ldst)) {
		ctx->remaining -= sizeof(ldst);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_ro_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_uint64(struct pldm_msgbuf_ro *ctx, void *dst)
{
	uint64_t ldst;

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(ldst) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(ldst)) {
		assert(ctx->cursor);
		memcpy(&ldst, ctx->cursor, sizeof(ldst));
		ldst = le64toh(ldst);
		memcpy(dst, &ldst, sizeof(ldst));
		ctx->cursor += sizeof(ldst);
		ctx->remaining -= sizeof(ldst);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(ldst)) {
		ctx->remaining -= sizeof(ldst);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_ro_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_int64(struct pldm_msgbuf_ro *ctx, void *dst)
{
	int64_t ldst;

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(ldst) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(ldst)) {
		assert(ctx->cursor);
		memcpy(&ldst, ctx->cursor, sizeof(ldst));
		ldst = le64toh(ldst);
		memcpy(dst, &ldst, sizeof(ldst));
		ctx->cursor += sizeof(ldst);
		ctx->remaining -= sizeof(ldst);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(ldst)) {
		ctx->remaining -= sizeof(ldst);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_ro_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_real32(struct pldm_msgbuf_ro *ctx, void *dst)
{
	uint32_t ldst;

	static_assert(sizeof(real32_t) == sizeof(ldst),
		      "Mismatched type sizes for dst and ldst");

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(ldst) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(ldst)) {
		assert(ctx->cursor);
		memcpy(&ldst, ctx->cursor, sizeof(ldst));
		ldst = le32toh(ldst);
		memcpy(dst, &ldst, sizeof(ldst));
		ctx->cursor += sizeof(ldst);
		ctx->remaining -= sizeof(ldst);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(ldst)) {
		ctx->remaining -= sizeof(ldst);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_ro_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_WARN_UNUSED_RESULT
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_array_void(struct pldm_msgbuf_ro *ctx, size_t count,
				void *dst, size_t dst_count)
{
	if (count > dst_count) {
		return -EINVAL;
	}

	if (!count) {
		return 0;
	}

#if INTMAX_MAX < SIZE_MAX
	if (count > INTMAX_MAX) {
		return pldm__msgbuf_ro_invalidate(ctx);
	}
#endif

	if (ctx->remaining >= (intmax_t)count) {
		assert(ctx->cursor);
		memcpy(dst, ctx->cursor, count);
		ctx->cursor += count;
		ctx->remaining -= (intmax_t)count;
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)count) {
		ctx->remaining -= (intmax_t)count;
		return -EOVERFLOW;
	}

	return pldm__msgbuf_ro_invalidate(ctx);
}

/**
 * @ref pldm_msgbuf_extract_array
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_WARN_UNUSED_RESULT
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_extract_array_char(struct pldm_msgbuf_ro *ctx, size_t count,
			       char *dst, size_t dst_count)
{
	return pldm__msgbuf_extract_array_void(ctx, count, dst, dst_count);
}

/**
 * @ref pldm_msgbuf_extract_array
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_WARN_UNUSED_RESULT
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_extract_array_uint8(struct pldm_msgbuf_ro *ctx, size_t count,
				uint8_t *dst, size_t dst_count)
{
	return pldm__msgbuf_extract_array_void(ctx, count, dst, dst_count);
}

/**
 * Extract an array of data from the msgbuf instance
 *
 * @param ctx - The msgbuf instance from which to extract an array of data
 * @param count - The number of array elements to extract
 * @param dst - The array object into which elements from @p ctx should be
                extracted
 * @param dst_count - The maximum number of elements to place into @p dst
 *
 * Note that both @p count and @p dst_count can only be counted by `sizeof` for
 * arrays where `sizeof(*dst) == 1` holds. Specifically, they count the number
 * of array elements and _not_ the object size of the array.
 */
#define pldm_msgbuf_extract_array(ctx, count, dst, dst_count)                  \
	_Generic((*(dst)),                                                     \
		uint8_t: pldm_msgbuf_extract_array_uint8,                      \
		char: pldm_msgbuf_extract_array_char)(ctx, count, dst,         \
						      dst_count)

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_insert_uint64(struct pldm_msgbuf_rw *ctx, const uint64_t src)
{
	uint64_t val = htole64(src);

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(src)) {
		assert(ctx->cursor);
		memcpy(ctx->cursor, &val, sizeof(val));
		ctx->cursor += sizeof(src);
		ctx->remaining -= sizeof(src);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(src)) {
		ctx->remaining -= sizeof(src);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_rw_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_insert_real32(struct pldm_msgbuf_rw *ctx, const real32_t src)
{
	uint32_t raw;
	memcpy(&raw, &src, sizeof(raw));
	uint32_t val = htole32(raw);

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(src)) {
		assert(ctx->cursor);
		memcpy(ctx->cursor, &val, sizeof(val));
		ctx->cursor += sizeof(src);
		ctx->remaining -= sizeof(src);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(src)) {
		ctx->remaining -= sizeof(src);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_rw_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_insert_uint32(struct pldm_msgbuf_rw *ctx, const uint32_t src)
{
	uint32_t val = htole32(src);

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(src)) {
		assert(ctx->cursor);
		memcpy(ctx->cursor, &val, sizeof(val));
		ctx->cursor += sizeof(src);
		ctx->remaining -= sizeof(src);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(src)) {
		ctx->remaining -= sizeof(src);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_rw_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_insert_uint16(struct pldm_msgbuf_rw *ctx, const uint16_t src)
{
	uint16_t val = htole16(src);

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(src)) {
		assert(ctx->cursor);
		memcpy(ctx->cursor, &val, sizeof(val));
		ctx->cursor += sizeof(src);
		ctx->remaining -= sizeof(src);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(src)) {
		ctx->remaining -= sizeof(src);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_rw_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_insert_uint8(struct pldm_msgbuf_rw *ctx, const uint8_t src)
{
	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(src)) {
		assert(ctx->cursor);
		memcpy(ctx->cursor, &src, sizeof(src));
		ctx->cursor += sizeof(src);
		ctx->remaining -= sizeof(src);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(src)) {
		ctx->remaining -= sizeof(src);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_rw_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_insert_int32(struct pldm_msgbuf_rw *ctx, const int32_t src)
{
	int32_t val = htole32(src);

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(src)) {
		assert(ctx->cursor);
		memcpy(ctx->cursor, &val, sizeof(val));
		ctx->cursor += sizeof(src);
		ctx->remaining -= sizeof(src);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(src)) {
		ctx->remaining -= sizeof(src);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_rw_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_insert_int16(struct pldm_msgbuf_rw *ctx, const int16_t src)
{
	int16_t val = htole16(src);

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(src)) {
		assert(ctx->cursor);
		memcpy(ctx->cursor, &val, sizeof(val));
		ctx->cursor += sizeof(src);
		ctx->remaining -= sizeof(src);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(src)) {
		ctx->remaining -= sizeof(src);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_rw_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_insert_int8(struct pldm_msgbuf_rw *ctx,
						     const int8_t src)
{
	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");

	if (ctx->remaining >= (intmax_t)sizeof(src)) {
		assert(ctx->cursor);
		memcpy(ctx->cursor, &src, sizeof(src));
		ctx->cursor += sizeof(src);
		ctx->remaining -= sizeof(src);
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)sizeof(src)) {
		ctx->remaining -= sizeof(src);
		return -EOVERFLOW;
	}

	return pldm__msgbuf_rw_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_WARN_UNUSED_RESULT
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_insert_array_void(struct pldm_msgbuf_rw *ctx, size_t count,
			       const void *src, size_t src_count)
{
	if (count > src_count) {
		return -EINVAL;
	}

	if (!count) {
		return 0;
	}

#if INTMAX_MAX < SIZE_MAX
	if (count > INTMAX_MAX) {
		return pldm__msgbuf_rw_invalidate(ctx);
	}
#endif

	if (ctx->remaining >= (intmax_t)count) {
		assert(ctx->cursor);
		memcpy(ctx->cursor, src, count);
		ctx->cursor += count;
		ctx->remaining -= (intmax_t)count;
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)count) {
		ctx->remaining -= (intmax_t)count;
		return -EOVERFLOW;
	}

	return pldm__msgbuf_rw_invalidate(ctx);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_WARN_UNUSED_RESULT
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_insert_array_char(struct pldm_msgbuf_rw *ctx, size_t count,
			      const char *src, size_t src_count)
{
	return pldm__msgbuf_insert_array_void(ctx, count, src, src_count);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_WARN_UNUSED_RESULT
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_insert_array_uint8(struct pldm_msgbuf_rw *ctx, size_t count,
			       const uint8_t *src, size_t src_count)
{
	return pldm__msgbuf_insert_array_void(ctx, count, src, src_count);
}

LIBPLDM_CC_NONNULL_ARGS(1)
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_span_required(const uint8_t **buf, intmax_t *remaining,
			   size_t required, const void **cursor)
{
#if INTMAX_MAX < SIZE_MAX
	if (required > INTMAX_MAX) {
		return pldm__msgbuf_set_invalid(remaining);
	}
#endif

	if (*remaining >= (intmax_t)required) {
		assert(*buf);
		if (cursor) {
			*cursor = *buf;
		}
		*buf += required;
		*remaining -= (intmax_t)required;
		return 0;
	}

	if (*remaining > INTMAX_MIN + (intmax_t)required) {
		*remaining -= (intmax_t)required;
		return -EOVERFLOW;
	}

	return pldm__msgbuf_set_invalid(remaining);
}

LIBPLDM_CC_NONNULL_ARGS(1)
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_span_required(struct pldm_msgbuf_ro *ctx, size_t required,
			     const void **cursor)
{
	return pldm__msgbuf_span_required(&ctx->cursor, &ctx->remaining,
					  required, cursor);
}

LIBPLDM_CC_NONNULL_ARGS(1)
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_span_required(struct pldm_msgbuf_rw *ctx, size_t required,
			     void **cursor)
{
	return pldm__msgbuf_span_required((const uint8_t **)&ctx->cursor,
					  &ctx->remaining, required,
					  (const void **)cursor);
}

LIBPLDM_CC_NONNULL_ARGS(1)
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_span_string_ascii(const uint8_t **buf, intmax_t *remaining,
			       const void **cursor, size_t *length)
{
	intmax_t measured;

	if (*remaining < 0) {
		return pldm__msgbuf_set_invalid(remaining);
	}
	assert(*buf);

	measured = (intmax_t)strnlen((const char *)*buf, *remaining);
	if (measured == *remaining) {
		return pldm__msgbuf_set_invalid(remaining);
	}

	/* Include the NUL terminator in the span length, as spans are opaque */
	measured++;

	if (*remaining >= measured) {
		assert(*buf);
		if (cursor) {
			*cursor = *buf;
		}

		*buf += measured;

		if (length) {
			*length = measured;
		}

		*remaining -= measured;
		return 0;
	}

	if (*remaining > INTMAX_MIN + measured) {
		*remaining -= measured;
		return -EOVERFLOW;
	}

	return pldm__msgbuf_set_invalid(remaining);
}

LIBPLDM_CC_NONNULL_ARGS(1)
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_span_string_ascii(struct pldm_msgbuf_rw *ctx, void **cursor,
				 size_t *length)
{
	return pldm__msgbuf_span_string_ascii((const uint8_t **)&ctx->cursor,
					      &ctx->remaining,
					      (const void **)cursor, length);
}

LIBPLDM_CC_NONNULL_ARGS(1)
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_span_string_ascii(struct pldm_msgbuf_ro *ctx,
				 const void **cursor, size_t *length)
{
	return pldm__msgbuf_span_string_ascii(&ctx->cursor, &ctx->remaining,
					      cursor, length);
}

LIBPLDM_CC_NONNULL_ARGS(1)
LIBPLDM_CC_ALWAYS_INLINE
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
int pldm__msgbuf_span_string_utf16(const uint8_t **buf, intmax_t *remaining,
				   const void **cursor, size_t *length)
{
	static const char16_t term = 0;
	ptrdiff_t measured;
	const void *end;

	if (*remaining < 0) {
		return pldm__msgbuf_set_invalid(remaining);
	}
	assert(*buf);

	/*
	 * Avoid tripping up on UTF16-LE: We may have consecutive NUL _bytes_ that do
	 * not form a UTF16 NUL _code-point_ due to alignment with respect to the
	 * start of the string
	 */
	end = *buf;
	do {
		if (end != *buf) {
			/*
			 * If we've looped we've found a relatively-unaligned NUL code-point.
			 * Scan again from a relatively-aligned start point.
			 */
			end = (char *)end + 1;
		}
		measured = (char *)end - (char *)*buf;
		end = memmem(end, *remaining - measured, &term, sizeof(term));
	} while (end && ((uintptr_t)end & 1) != ((uintptr_t)*buf & 1));

	if (!end) {
		/*
		 * Optimistically, the last required pattern byte was one beyond the end of
		 * the buffer. Setting ctx->remaining negative ensures the
		 * `pldm_msgbuf_complete*()` APIs also return an error.
		 */
		return pldm__msgbuf_set_invalid(remaining);
	}

	end = (char *)end + sizeof(char16_t);
	measured = (char *)end - (char *)*buf;

#if INTMAX_MAX < PTRDIFF_MAX
	if (measured >= INTMAX_MAX) {
		return pldm__msgbuf_set_invalid(remaining);
	}
#endif

	if (*remaining >= (intmax_t)measured) {
		assert(*buf);
		if (cursor) {
			*cursor = *buf;
		}

		*buf += measured;

		if (length) {
			*length = (size_t)measured;
		}

		*remaining -= (intmax_t)measured;
		return 0;
	}

	if (*remaining > INTMAX_MIN + (intmax_t)measured) {
		*remaining -= (intmax_t)measured;
		return -EOVERFLOW;
	}

	return pldm__msgbuf_set_invalid(remaining);
}

LIBPLDM_CC_NONNULL_ARGS(1)
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_span_string_utf16(struct pldm_msgbuf_ro *ctx,
				 const void **cursor, size_t *length)
{
	return pldm__msgbuf_span_string_utf16(&ctx->cursor, &ctx->remaining,
					      cursor, length);
}

LIBPLDM_CC_NONNULL_ARGS(1)
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_span_string_utf16(struct pldm_msgbuf_rw *ctx, void **cursor,
				 size_t *length)
{
	return pldm__msgbuf_span_string_utf16((const uint8_t **)&ctx->cursor,
					      &ctx->remaining,
					      (const void **)cursor, length);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_span_remaining(const uint8_t **buf, intmax_t *remaining,
			    const void **cursor, size_t *len)
{
	if (*remaining < 0) {
		return -EOVERFLOW;
	}

	assert(*buf);
	*cursor = *buf;
	*buf += *remaining;
	*len = *remaining;
	*remaining = 0;

	return 0;
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_rw_span_remaining(struct pldm_msgbuf_rw *ctx, void **cursor,
			      size_t *len)
{
	return pldm__msgbuf_span_remaining((const uint8_t **)&ctx->cursor,
					   &ctx->remaining,
					   (const void **)cursor, len);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_ro_span_remaining(struct pldm_msgbuf_ro *ctx, const void **cursor,
			      size_t *len)
{
	return pldm__msgbuf_span_remaining(&ctx->cursor, &ctx->remaining,
					   cursor, len);
}

LIBPLDM_CC_NONNULL_ARGS(1)
LIBPLDM_CC_ALWAYS_INLINE
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
int pldm__msgbuf_span_until(const uint8_t **buf, intmax_t *remaining,
			    size_t trailer, const void **cursor, size_t *length)
{
#if INTMAX_MAX < SIZE_MAX
	if (trailer > INTMAX_MAX) {
		return pldm__msgbuf_set_invalid(remaining);
	}
#endif

	if (*remaining >= (intmax_t)trailer) {
		ptrdiff_t delta;

		assert(*buf);

		delta = *remaining - (intmax_t)trailer;
		if (cursor) {
			*cursor = *buf;
		}
		*buf += delta;
		if (length) {
			*length = delta;
		}
		*remaining = (intmax_t)trailer;
		return 0;
	}

	if (*remaining > INTMAX_MIN + (intmax_t)trailer) {
		*remaining = INTMAX_MIN + (intmax_t)trailer;
		return -EOVERFLOW;
	}

	return pldm__msgbuf_set_invalid(remaining);
}

LIBPLDM_CC_NONNULL_ARGS(1)
LIBPLDM_CC_ALWAYS_INLINE
int pldm_msgbuf_ro_span_until(struct pldm_msgbuf_ro *ctx, size_t trailer,
			      const void **cursor, size_t *length)
{
	return pldm__msgbuf_span_until(&ctx->cursor, &ctx->remaining, trailer,
				       cursor, length);
}

LIBPLDM_CC_NONNULL_ARGS(1)
LIBPLDM_CC_ALWAYS_INLINE
int pldm_msgbuf_rw_span_until(struct pldm_msgbuf_rw *ctx, size_t trailer,
			      void **cursor, size_t *length)
{
	return pldm__msgbuf_span_until((const uint8_t **)&ctx->cursor,
				       &ctx->remaining, trailer,
				       (const void **)cursor, length);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_peek_remaining(struct pldm_msgbuf_rw *ctx, void **cursor,
			   size_t *len)
{
	if (ctx->remaining < 0) {
		return -EOVERFLOW;
	}

	assert(ctx->cursor);
	*cursor = ctx->cursor;
	*len = ctx->remaining;

	return 0;
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_skip(struct pldm_msgbuf_rw *ctx,
					      size_t count)
{
#if INTMAX_MAX < SIZE_MAX
	if (count > INTMAX_MAX) {
		return pldm__msgbuf_rw_invalidate(ctx);
	}
#endif

	if (ctx->remaining >= (intmax_t)count) {
		assert(ctx->cursor);
		ctx->cursor += count;
		ctx->remaining -= (intmax_t)count;
		return 0;
	}

	if (ctx->remaining > INTMAX_MIN + (intmax_t)count) {
		ctx->remaining -= (intmax_t)count;
		return -EOVERFLOW;
	}

	return pldm__msgbuf_rw_invalidate(ctx);
}

/**
 * @brief Complete the pldm_msgbuf instance and return the number of bytes
 * consumed.
 *
 * @param ctx - The msgbuf.
 * @param orig_len - The original size of the msgbuf, the `len` argument passed to
 * 		pldm_msgbuf_init_errno().
 * @param ret_used_len - The number of bytes that have been used from the msgbuf instance.
 *
 * This can be called after a number of pldm_msgbuf_insert...() calls to
 * determine the total size that was written.
 *
 * @return 0 on success, -EOVERFLOW if an implausible orig_len was provided or
 * an out-of-bounds access occurred.
 */
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE
LIBPLDM_CC_WARN_UNUSED_RESULT
int pldm_msgbuf_complete_used(struct pldm_msgbuf_rw *ctx, size_t orig_len,
			      size_t *ret_used_len)
{
	int rc;

	ctx->cursor = NULL;
	rc = pldm_msgbuf_rw_validate(ctx);
	if (rc) {
		pldm__msgbuf_rw_invalidate(ctx);
		return rc;
	}

	if ((size_t)ctx->remaining > orig_len) {
		/* Caller passed incorrect orig_len */
		return pldm__msgbuf_rw_invalidate(ctx);
	}

	*ret_used_len = orig_len - ctx->remaining;
	pldm__msgbuf_rw_invalidate(ctx);
	return 0;
}

/**
 * @brief pldm_msgbuf copy data between two msg buffers
 *
 * @param[in,out] src - pldm_msgbuf for source from where value should be copied
 * @param[in,out] dst - destination of copy from source
 * @param[in] size - size of data to be copied
 * @param[in] description - description of data copied
 *
 * @return PLDM_SUCCESS if buffer accesses were in-bounds,
 * PLDM_ERROR_INVALID_LENGTH otherwise.
 * PLDM_ERROR_INVALID_DATA if input is invalid
 */
#define pldm_msgbuf_copy(dst, src, type, name)                                 \
	pldm__msgbuf_copy(dst, src, sizeof(type), #name)
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_copy(struct pldm_msgbuf_rw *dst, struct pldm_msgbuf_ro *src,
		  size_t size, const char *description LIBPLDM_CC_UNUSED)
{
#if INTMAX_MAX < SIZE_MAX
	if (size > INTMAX_MAX) {
		pldm__msgbuf_ro_invalidate(src);
		pldm__msgbuf_rw_invalidate(dst);
		return -EOVERFLOW;
	}
#endif

	if (src->remaining >= (intmax_t)size &&
	    dst->remaining >= (intmax_t)size) {
		assert(src->cursor && dst->cursor);
		memcpy(dst->cursor, src->cursor, size);
		src->cursor += size;
		src->remaining -= (intmax_t)size;
		dst->cursor += size;
		dst->remaining -= (intmax_t)size;
		return 0;
	}

	if (src->remaining > INTMAX_MIN + (intmax_t)size) {
		src->remaining -= (intmax_t)size;
	} else {
		pldm__msgbuf_ro_invalidate(src);
	}

	if (dst->remaining > INTMAX_MIN + (intmax_t)size) {
		dst->remaining -= (intmax_t)size;
	} else {
		pldm__msgbuf_rw_invalidate(dst);
	}

	return -EOVERFLOW;
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_WARN_UNUSED_RESULT
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_copy_string_ascii(struct pldm_msgbuf_rw *dst,
			      struct pldm_msgbuf_ro *src)
{
	const void *ascii = NULL;
	size_t len = 0;
	int rc;

	rc = pldm_msgbuf_ro_span_string_ascii(src, &ascii, &len);
	if (rc < 0) {
		return rc;
	}

	return pldm__msgbuf_insert_array_void(dst, len, ascii, len);
}

LIBPLDM_CC_NONNULL
LIBPLDM_CC_WARN_UNUSED_RESULT
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_copy_string_utf16(struct pldm_msgbuf_rw *dst,
			      struct pldm_msgbuf_ro *src)
{
	const void *utf16 = NULL;
	size_t len = 0;
	int rc;

	rc = pldm_msgbuf_ro_span_string_utf16(src, &utf16, &len);
	if (rc < 0) {
		return rc;
	}

	return pldm__msgbuf_insert_array_void(dst, len, utf16, len);
}

/**
 * @brief pldm_msgbuf uint8_t extractor for a size_t
 *
 * @param[in,out] ctx - pldm_msgbuf context for extractor
 * @param[out] dst - destination of extracted value
 *
 * @return 0 if buffer accesses were in-bounds,
 * -EINVAL if dst pointer is invalid,
 * -EOVERFLOW is the buffer was out of bound.
 */
#define pldm_msgbuf_extract_uint8_to_size(ctx, dst)                            \
	pldm__msgbuf_extract_uint8_to_size(ctx, &(dst))
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_uint8_to_size(struct pldm_msgbuf_ro *ctx, size_t *dst)
{
	uint8_t value = 0;
	int rc;

	rc = pldm__msgbuf_extract_uint8(ctx, &value);
	if (rc) {
		return rc;
	}

	static_assert(SIZE_MAX >= UINT8_MAX, "Invalid promotion");

	*dst = value;
	return 0;
}

/**
 * @brief pldm_msgbuf uint16_t extractor for a size_t
 *
 * @param[in,out] ctx - pldm_msgbuf context for extractor
 * @param[out] dst - destination of extracted value
 *
 * @return 0 if buffer accesses were in-bounds,
 * -EINVAL if dst pointer is invalid,
 * -EOVERFLOW is the buffer was out of bound.
 */
#define pldm_msgbuf_extract_uint16_to_size(ctx, dst)                           \
	pldm__msgbuf_extract_uint16_to_size(ctx, &(dst))
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_uint16_to_size(struct pldm_msgbuf_ro *ctx, size_t *dst)
{
	uint16_t value = 0;
	int rc;

	rc = pldm__msgbuf_extract_uint16(ctx, &value);
	if (rc) {
		return rc;
	}

	static_assert(SIZE_MAX >= UINT16_MAX, "Invalid promotion");

	*dst = value;
	return 0;
}

/**
 * @brief pldm_msgbuf uint32_t extractor for a size_t
 *
 * @param[in,out] ctx - pldm_msgbuf context for extractor
 * @param[out] dst - destination of extracted value
 *
 * @return 0 if buffer accesses were in-bounds,
 * -EINVAL if dst pointer is invalid,
 * -EOVERFLOW is the buffer was out of bound.
 */
#define pldm_msgbuf_extract_uint32_to_size(ctx, dst)                           \
	pldm__msgbuf_extract_uint32_to_size(ctx, &(dst))
LIBPLDM_CC_NONNULL
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_uint32_to_size(struct pldm_msgbuf_ro *ctx, size_t *dst)
{
	uint32_t value = 0;
	int rc;

	rc = pldm__msgbuf_extract_uint32(ctx, &value);
	if (rc) {
		return rc;
	}

	static_assert(SIZE_MAX >= UINT32_MAX, "Invalid promotion");

	*dst = value;
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif
