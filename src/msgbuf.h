/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef PLDM_MSGBUF_H
#define PLDM_MSGBUF_H

#include "compiler.h"

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

#include <libpldm/base.h>
#include <libpldm/pldm_types.h>

#include "compiler.h"

#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
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
	static_assert(PLDM_SUCCESS == 0, "Error handling is broken");
	int compliance;
} build_assertions LIBPLDM_CC_UNUSED;

enum pldm_msgbuf_error_mode {
	PLDM_MSGBUF_PLDM_CC = 0x5a,
	PLDM_MSGBUF_C_ERRNO = 0xa5,
};

struct pldm_msgbuf {
	uint8_t *cursor;
	intmax_t remaining;
	enum pldm_msgbuf_error_mode mode;
};

/**
 * @brief Either negate an errno value or return a value mapped to a PLDM
 * completion code.
 *
 * Note that `pldm_msgbuf_status()` is purely internal to the msgbuf API
 * for ergonomics. It's preferred that we don't try to unify this with
 * `pldm_xlate_errno()` from src/api.h despite the similarities.
 *
 * @param[in] ctx - The msgbuf context providing the personality info
 * @param[in] err - The positive errno value to translate
 *
 * @return Either the negated value of @p err if the context's error mode is
 *         `PLDM_MSGBUF_C_ERRNO`, or the equivalent PLDM completion code if the
 *         error mode is `PLDM_MSGBUF_PLDM_CC`.
 */
LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_status(struct pldm_msgbuf *ctx,
						unsigned int err)
{
	int rc;

	assert(err != 0);
	assert(err <= INT_MAX);

	if (ctx->mode == PLDM_MSGBUF_C_ERRNO) {
		if (err > INT_MAX) {
			return -EINVAL;
		}

		static_assert(INT_MIN + INT_MAX < 0,
			      "Arithmetic assumption failure");
		return -((int)err);
	}

	if (err > INT_MAX) {
		return PLDM_ERROR;
	}

	assert(ctx->mode == PLDM_MSGBUF_PLDM_CC);
	switch (err) {
	case EINVAL:
		rc = PLDM_ERROR_INVALID_DATA;
		break;
	case EBADMSG:
	case EOVERFLOW:
		rc = PLDM_ERROR_INVALID_LENGTH;
		break;
	default:
		assert(false);
		rc = PLDM_ERROR;
		break;
	}

	assert(rc > 0);
	return rc;
}

/**
 * @brief Initialize pldm buf struct for buf extractor
 *
 * @param[out] ctx - pldm_msgbuf context for extractor
 * @param[in] minsize - The minimum required length of buffer `buf`
 * @param[in] buf - buffer to be extracted
 * @param[in] len - size of buffer
 *
 * @return 0 on success, otherwise an error code appropriate for the current
 *         personality.
 */
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_init(struct pldm_msgbuf *ctx, size_t minsize, const void *buf,
		  size_t len)
{
	assert(ctx);
	assert(ctx->mode == PLDM_MSGBUF_PLDM_CC ||
	       ctx->mode == PLDM_MSGBUF_C_ERRNO);

	if (!buf) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	if ((minsize > len)) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

#if INTMAX_MAX < SIZE_MAX
	if (len > INTMAX_MAX) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
#endif

	if ((uintptr_t)buf + len < len) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	ctx->cursor = (uint8_t *)buf;
	ctx->remaining = (intmax_t)len;

	return 0;
}

/**
 * @brief Initialise a msgbuf instance to return errors as PLDM completion codes
 *
 * @see pldm__msgbuf_init
 *
 * @param[out] ctx - pldm_msgbuf context for extractor
 * @param[in] minsize - The minimum required length of buffer `buf`
 * @param[in] buf - buffer to be extracted
 * @param[in] len - size of buffer
 *
 * @return PLDM_SUCCESS if the provided buffer region is sensible,
 *         otherwise PLDM_ERROR_INVALID_DATA if pointer parameters are invalid,
 *         or PLDM_ERROR_INVALID_LENGTH if length constraints are violated.
 */
LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_init_cc(struct pldm_msgbuf *ctx,
						 size_t minsize,
						 const void *buf, size_t len)
{
	if (!ctx) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->mode = PLDM_MSGBUF_PLDM_CC;
	return pldm__msgbuf_init(ctx, minsize, buf, len);
}

/**
 * @brief Initialise a msgbuf instance to return errors as negative errno values
 *
 * @see pldm__msgbuf_init
 *
 * @param[out] ctx - pldm_msgbuf context for extractor
 * @param[in] minsize - The minimum required length of buffer `buf`
 * @param[in] buf - buffer to be extracted
 * @param[in] len - size of buffer
 *
 * @return 0 if the provided buffer region is sensible, otherwise -EINVAL if
 *         pointer parameters are invalid, or -EOVERFLOW if length constraints
 *         are violated.
 */
LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_init_errno(struct pldm_msgbuf *ctx,
						    size_t minsize,
						    const void *buf, size_t len)
{
	if (!ctx) {
		return -EINVAL;
	}

	ctx->mode = PLDM_MSGBUF_C_ERRNO;
	return pldm__msgbuf_init(ctx, minsize, buf, len);
}

/**
 * @brief Validate buffer overflow state
 *
 * @param[in] ctx - pldm_msgbuf context for extractor
 *
 * @return PLDM_SUCCESS if there are zero or more bytes of data that remain
 * unread from the buffer. Otherwise, PLDM_ERROR_INVALID_LENGTH indicates that a
 * prior accesses would have occurred beyond the bounds of the buffer, and
 * PLDM_ERROR_INVALID_DATA indicates that the provided context was not a valid
 * pointer.
 */
LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_validate(struct pldm_msgbuf *ctx)
{
	assert(ctx);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	return 0;
}

/**
 * @brief Test whether a message buffer has been exactly consumed
 *
 * @param[in] ctx - pldm_msgbuf context for extractor
 *
 * @return PLDM_SUCCESS iff there are zero bytes of data that remain unread from
 * the buffer and no overflow has occurred. Otherwise, PLDM_ERROR_INVALID_LENGTH
 * indicates that an incorrect sequence of accesses have occurred, and
 * PLDM_ERROR_INVALID_DATA indicates that the provided context was not a valid
 * pointer.
 */
LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_consumed(struct pldm_msgbuf *ctx)
{
	assert(ctx);
	if (ctx->remaining != 0) {
		return pldm_msgbuf_status(ctx, EBADMSG);
	}

	return 0;
}

/**
 * @brief Destroy the pldm buf
 *
 * @param[in] ctx - pldm_msgbuf context for extractor
 *
 * @return PLDM_SUCCESS if all buffer accesses were in-bounds,
 * PLDM_ERROR_INVALID_DATA if the ctx parameter is invalid, or
 * PLDM_ERROR_INVALID_LENGTH if prior accesses would have occurred beyond the
 * bounds of the buffer.
 */
LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_destroy(struct pldm_msgbuf *ctx)
{
	int valid;

	assert(ctx);
	valid = pldm_msgbuf_validate(ctx);

	ctx->cursor = NULL;
	ctx->remaining = 0;

	return valid;
}

/**
 * @brief Destroy the pldm_msgbuf instance, and check that the underlying buffer
 * has been completely consumed without overflow
 *
 * @param[in] ctx - pldm_msgbuf context
 *
 * @return PLDM_SUCCESS if all buffer access were in-bounds and completely
 * consume the underlying buffer. Otherwise, PLDM_ERROR_INVALID_DATA if the ctx
 * parameter is invalid, or PLDM_ERROR_INVALID_LENGTH if prior accesses would
 * have occurred byond the bounds of the buffer
 */
LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_destroy_consumed(struct pldm_msgbuf *ctx)
{
	int consumed;

	assert(ctx);
	consumed = pldm_msgbuf_consumed(ctx);

	ctx->cursor = NULL;
	ctx->remaining = 0;

	return consumed;
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
#ifdef __cplusplus
#define pldm_msgbuf_extract_typecheck(ty, fn, dst, ...)                        \
	pldm_msgbuf_typecheck_##ty<decltype(dst)>(__VA_ARGS__)
#else
#define pldm_msgbuf_extract_typecheck(ty, fn, dst, ...)                        \
	(pldm_require_obj_type(dst, ty), fn(__VA_ARGS__))
#endif

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
#define pldm_msgbuf_extract_uint8(ctx, dst)                                    \
	pldm_msgbuf_extract_typecheck(uint8_t, pldm__msgbuf_extract_uint8,     \
				      dst, ctx, dst)
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_uint8(struct pldm_msgbuf *ctx, void *dst)
{
	assert(ctx);

	if (!ctx->cursor || !dst) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	if (ctx->remaining == INTMAX_MIN) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= sizeof(uint8_t);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(dst, ctx->cursor, sizeof(uint8_t));

	ctx->cursor++;
	return 0;
}

#define pldm_msgbuf_extract_int8(ctx, dst)                                     \
	pldm_msgbuf_extract_typecheck(int8_t, pldm__msgbuf_extract_int8, dst,  \
				      ctx, dst)
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_int8(struct pldm_msgbuf *ctx, void *dst)
{
	assert(ctx);

	if (!ctx->cursor || !dst) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	if (ctx->remaining == INTMAX_MIN) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= sizeof(int8_t);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(dst, ctx->cursor, sizeof(int8_t));
	ctx->cursor++;
	return 0;
}

#define pldm_msgbuf_extract_uint16(ctx, dst)                                   \
	pldm_msgbuf_extract_typecheck(uint16_t, pldm__msgbuf_extract_uint16,   \
				      dst, ctx, dst)
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_uint16(struct pldm_msgbuf *ctx, void *dst)
{
	uint16_t ldst;

	assert(ctx);

	if (!ctx->cursor || !dst) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	// Check for underflow while tracking the magnitude of the buffer overflow
	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(ldst) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");
	if (ctx->remaining < INTMAX_MIN + (intmax_t)sizeof(ldst)) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	// Check for buffer overflow. If we overflow, account for the request as
	// negative values in ctx->remaining. This way we can debug how far
	// we've overflowed.
	ctx->remaining -= sizeof(ldst);

	// Prevent the access if it would overflow. First, assert so we blow up
	// the test suite right at the point of failure. However, cater to
	// -DNDEBUG by explicitly testing that the access is valid.
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	// Use memcpy() to have the compiler deal with any alignment
	// issues on the target architecture
	memcpy(&ldst, ctx->cursor, sizeof(ldst));

	// Only assign the target value once it's correctly decoded
	ldst = le16toh(ldst);

	// Allow storing to unaligned
	memcpy(dst, &ldst, sizeof(ldst));

	ctx->cursor += sizeof(ldst);

	return 0;
}

#define pldm_msgbuf_extract_int16(ctx, dst)                                    \
	pldm_msgbuf_extract_typecheck(int16_t, pldm__msgbuf_extract_int16,     \
				      dst, ctx, dst)
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_int16(struct pldm_msgbuf *ctx, void *dst)
{
	int16_t ldst;

	assert(ctx);

	if (!ctx->cursor || !dst) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(ldst) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");
	if (ctx->remaining < INTMAX_MIN + (intmax_t)sizeof(ldst)) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= sizeof(ldst);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(&ldst, ctx->cursor, sizeof(ldst));

	ldst = le16toh(ldst);
	memcpy(dst, &ldst, sizeof(ldst));
	ctx->cursor += sizeof(ldst);

	return 0;
}

#define pldm_msgbuf_extract_uint32(ctx, dst)                                   \
	pldm_msgbuf_extract_typecheck(uint32_t, pldm__msgbuf_extract_uint32,   \
				      dst, ctx, dst)
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_uint32(struct pldm_msgbuf *ctx, void *dst)
{
	uint32_t ldst;

	assert(ctx);

	if (!ctx->cursor || !dst) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(ldst) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");
	if (ctx->remaining < INTMAX_MIN + (intmax_t)sizeof(ldst)) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= sizeof(ldst);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(&ldst, ctx->cursor, sizeof(ldst));
	ldst = le32toh(ldst);
	memcpy(dst, &ldst, sizeof(ldst));
	ctx->cursor += sizeof(ldst);

	return 0;
}

#define pldm_msgbuf_extract_int32(ctx, dst)                                    \
	pldm_msgbuf_extract_typecheck(int32_t, pldm__msgbuf_extract_int32,     \
				      dst, ctx, dst)
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_int32(struct pldm_msgbuf *ctx, void *dst)
{
	int32_t ldst;

	assert(ctx);

	if (!ctx->cursor || !dst) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(ldst) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");
	if (ctx->remaining < INTMAX_MIN + (intmax_t)sizeof(ldst)) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= sizeof(ldst);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(&ldst, ctx->cursor, sizeof(ldst));
	ldst = le32toh(ldst);
	memcpy(dst, &ldst, sizeof(ldst));
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

#define pldm_msgbuf_extract_real32(ctx, dst)                                   \
	pldm_msgbuf_extract_typecheck(real32_t, pldm__msgbuf_extract_real32,   \
				      dst, ctx, dst)
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_real32(struct pldm_msgbuf *ctx, void *dst)
{
	uint32_t ldst;

	static_assert(sizeof(real32_t) == sizeof(ldst),
		      "Mismatched type sizes for dst and ldst");

	assert(ctx);

	if (!ctx->cursor || !dst) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(ldst) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");
	if (ctx->remaining < INTMAX_MIN + (intmax_t)sizeof(ldst)) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= sizeof(ldst);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(&ldst, ctx->cursor, sizeof(ldst));
	ldst = le32toh(ldst);
	memcpy(dst, &ldst, sizeof(ldst));
	ctx->cursor += sizeof(ldst);

	return 0;
}

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
		real32_t *: pldm__msgbuf_extract_real32)(ctx, dst)

LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_extract_array_void(struct pldm_msgbuf *ctx, void *dst,
				size_t count)
{
	assert(ctx);

	if (!ctx->cursor || !dst) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	if (!count) {
		return 0;
	}

#if INTMAX_MAX < SIZE_MAX
	if (count > INTMAX_MAX) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
#endif

	if (ctx->remaining < INTMAX_MIN + (intmax_t)count) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= (intmax_t)count;
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(dst, ctx->cursor, count);
	ctx->cursor += count;

	return 0;
}

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_extract_array_char(struct pldm_msgbuf *ctx, char *dst, size_t count)
{
	return pldm__msgbuf_extract_array_void(ctx, dst, count);
}

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_extract_array_uint8(struct pldm_msgbuf *ctx, uint8_t *dst,
				size_t count)
{
	return pldm__msgbuf_extract_array_void(ctx, dst, count);
}

#define pldm_msgbuf_extract_array(ctx, dst, count)                             \
	_Generic((*(dst)),                                                     \
		uint8_t: pldm_msgbuf_extract_array_uint8,                      \
		char: pldm_msgbuf_extract_array_char)(ctx, dst, count)

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_insert_uint32(struct pldm_msgbuf *ctx,
						       const uint32_t src)
{
	uint32_t val = htole32(src);

	assert(ctx);

	if (!ctx->cursor) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");
	if (ctx->remaining < INTMAX_MIN + (intmax_t)sizeof(src)) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= sizeof(src);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(ctx->cursor, &val, sizeof(val));
	ctx->cursor += sizeof(src);

	return 0;
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_insert_uint16(struct pldm_msgbuf *ctx,
						       const uint16_t src)
{
	uint16_t val = htole16(src);

	assert(ctx);

	if (!ctx->cursor) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");
	if (ctx->remaining < INTMAX_MIN + (intmax_t)sizeof(src)) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= sizeof(src);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(ctx->cursor, &val, sizeof(val));
	ctx->cursor += sizeof(src);

	return 0;
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_insert_uint8(struct pldm_msgbuf *ctx,
						      const uint8_t src)
{
	assert(ctx);

	if (!ctx->cursor) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");
	if (ctx->remaining < INTMAX_MIN + (intmax_t)sizeof(src)) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= sizeof(src);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(ctx->cursor, &src, sizeof(src));
	ctx->cursor += sizeof(src);

	return 0;
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_insert_int32(struct pldm_msgbuf *ctx,
						      const int32_t src)
{
	int32_t val = htole32(src);

	assert(ctx);

	if (!ctx->cursor) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");
	if (ctx->remaining < INTMAX_MIN + (intmax_t)sizeof(src)) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= sizeof(src);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(ctx->cursor, &val, sizeof(val));
	ctx->cursor += sizeof(src);

	return 0;
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_insert_int16(struct pldm_msgbuf *ctx,
						      const int16_t src)
{
	int16_t val = htole16(src);

	assert(ctx);

	if (!ctx->cursor) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");
	if (ctx->remaining < INTMAX_MIN + (intmax_t)sizeof(src)) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= sizeof(src);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(ctx->cursor, &val, sizeof(val));
	ctx->cursor += sizeof(src);

	return 0;
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_insert_int8(struct pldm_msgbuf *ctx,
						     const int8_t src)
{
	assert(ctx);

	if (!ctx->cursor) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	static_assert(
		// NOLINTNEXTLINE(bugprone-sizeof-expression)
		sizeof(src) < INTMAX_MAX,
		"The following addition may not uphold the runtime assertion");
	if (ctx->remaining < INTMAX_MIN + (intmax_t)sizeof(src)) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= sizeof(src);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(ctx->cursor, &src, sizeof(src));
	ctx->cursor += sizeof(src);

	return 0;
}

#define pldm_msgbuf_insert(dst, src)                                           \
	_Generic((src),                                                        \
		uint8_t: pldm_msgbuf_insert_uint8,                             \
		int8_t: pldm_msgbuf_insert_int8,                               \
		uint16_t: pldm_msgbuf_insert_uint16,                           \
		int16_t: pldm_msgbuf_insert_int16,                             \
		uint32_t: pldm_msgbuf_insert_uint32,                           \
		int32_t: pldm_msgbuf_insert_int32)(dst, src)

LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_insert_array_void(struct pldm_msgbuf *ctx, const void *src,
			       size_t count)
{
	assert(ctx);

	if (!ctx->cursor || !src) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	if (!count) {
		return 0;
	}

#if INTMAX_MAX < SIZE_MAX
	if (count > INTMAX_MAX) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
#endif

	if (ctx->remaining < INTMAX_MIN + (intmax_t)count) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= (intmax_t)count;
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	memcpy(ctx->cursor, src, count);
	ctx->cursor += count;

	return 0;
}

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_insert_array_char(struct pldm_msgbuf *ctx, const char *src,
			      size_t count)
{
	return pldm__msgbuf_insert_array_void(ctx, src, count);
}

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_insert_array_uint8(struct pldm_msgbuf *ctx, const uint8_t *src,
			       size_t count)
{
	return pldm__msgbuf_insert_array_void(ctx, src, count);
}

#define pldm_msgbuf_insert_array(dst, src, count)                              \
	_Generic((*(src)),                                                     \
		uint8_t: pldm_msgbuf_insert_array_uint8,                       \
		char: pldm_msgbuf_insert_array_char)(dst, src, count)

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_span_required(struct pldm_msgbuf *ctx,
						       size_t required,
						       void **cursor)
{
	assert(ctx);

	if (!ctx->cursor || !cursor || *cursor) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

#if INTMAX_MAX < SIZE_MAX
	if (required > INTMAX_MAX) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
#endif

	if (ctx->remaining < INTMAX_MIN + (intmax_t)required) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
	ctx->remaining -= (intmax_t)required;
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	*cursor = ctx->cursor;
	ctx->cursor += required;

	return 0;
}

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_span_string_ascii(struct pldm_msgbuf *ctx, void **cursor,
			      size_t *length)
{
	intmax_t measured;

	assert(ctx);

	if (!ctx->cursor || (cursor && *cursor)) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	if (ctx->remaining < 0) {
		/* Tracking the amount of overflow gets disturbed here */
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	measured = (intmax_t)strnlen((const char *)ctx->cursor, ctx->remaining);
	if (measured == ctx->remaining) {
		/*
		 * We have hit the end of the buffer prior to the NUL terminator.
		 * Optimistically, the NUL terminator was one-beyond-the-end. Setting
		 * ctx->remaining negative ensures the `pldm_msgbuf_destroy*()` APIs also
		 * return an error.
		 */
		ctx->remaining = -1;
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	/* Include the NUL terminator in the span length, as spans are opaque */
	measured++;

	if (ctx->remaining < INTMAX_MIN + measured) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	ctx->remaining -= measured;
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	if (cursor) {
		*cursor = ctx->cursor;
	}

	ctx->cursor += measured;

	if (length) {
		*length = measured;
	}

	return 0;
}

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_span_string_utf16(struct pldm_msgbuf *ctx, void **cursor,
			      size_t *length)
{
	static const char16_t term = 0;
	ptrdiff_t measured;
	void *end;

	assert(ctx);

	if (!ctx->cursor || (cursor && *cursor)) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	if (ctx->remaining < 0) {
		/* Tracking the amount of overflow gets disturbed here */
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	/*
	 * Avoid tripping up on UTF16-LE: We may have consecutive NUL _bytes_ that do
	 * not form a UTF16 NUL _code-point_ due to alignment with respect to the
	 * start of the string
	 */
	end = ctx->cursor;
	do {
		if (end != ctx->cursor) {
			/*
			 * If we've looped we've found a relatively-unaligned NUL code-point.
			 * Scan again from a relatively-aligned start point.
			 */
			end = (char *)end + 1;
		}
		measured = (char *)end - (char *)ctx->cursor;
		end = memmem(end, ctx->remaining - measured, &term,
			     sizeof(term));
	} while (end && ((uintptr_t)end & 1) != ((uintptr_t)ctx->cursor & 1));

	if (!end) {
		/*
		 * Optimistically, the last required pattern byte was one beyond the end of
		 * the buffer. Setting ctx->remaining negative ensures the
		 * `pldm_msgbuf_destroy*()` APIs also return an error.
		 */
		ctx->remaining = -1;
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	end = (char *)end + sizeof(char16_t);
	measured = (char *)end - (char *)ctx->cursor;

#if INTMAX_MAX < PTRDIFF_MAX
	if (measured >= INTMAX_MAX) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}
#endif

	if (ctx->remaining < INTMAX_MIN + (intmax_t)measured) {
		assert(ctx->remaining < 0);
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	ctx->remaining -= (intmax_t)measured;
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	if (cursor) {
		*cursor = ctx->cursor;
	}

	ctx->cursor += measured;

	if (length) {
		*length = (size_t)measured;
	}

	return 0;
}

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_span_remaining(struct pldm_msgbuf *ctx, void **cursor, size_t *len)
{
	assert(ctx);

	if (!ctx->cursor || !cursor || *cursor || !len) {
		return pldm_msgbuf_status(ctx, EINVAL);
	}

	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return pldm_msgbuf_status(ctx, EOVERFLOW);
	}

	*cursor = ctx->cursor;
	ctx->cursor += ctx->remaining;
	*len = ctx->remaining;
	ctx->remaining = 0;

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
LIBPLDM_CC_ALWAYS_INLINE int
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
pldm__msgbuf_copy(struct pldm_msgbuf *dst, struct pldm_msgbuf *src, size_t size,
		  const char *description)
{
	assert(src);
	assert(dst);
	assert(src->mode == dst->mode);

	if (!src->cursor || !dst->cursor || !description) {
		return pldm_msgbuf_status(dst, EINVAL);
	}

#if INTMAX_MAX < SIZE_MAX
	if (size > INTMAX_MAX) {
		return pldm_msgbuf_status(dst, EOVERFLOW);
	}
#endif

	if (src->remaining < INTMAX_MIN + (intmax_t)size) {
		return pldm_msgbuf_status(dst, EOVERFLOW);
	}

	if (dst->remaining < INTMAX_MIN + (intmax_t)size) {
		return pldm_msgbuf_status(dst, EOVERFLOW);
	}

	src->remaining -= (intmax_t)size;
	assert(src->remaining >= 0);
	if (src->remaining < 0) {
		return pldm_msgbuf_status(dst, EOVERFLOW);
	}

	dst->remaining -= (intmax_t)size;
	assert(dst->remaining >= 0);
	if (dst->remaining < 0) {
		return pldm_msgbuf_status(dst, EOVERFLOW);
	}

	memcpy(dst->cursor, src->cursor, size);
	src->cursor += size;
	dst->cursor += size;

	return 0;
}

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_copy_string_ascii(struct pldm_msgbuf *dst, struct pldm_msgbuf *src)
{
	void *ascii = NULL;
	size_t len = 0;
	int rc;

	rc = pldm_msgbuf_span_string_ascii(src, &ascii, &len);
	if (rc < 0) {
		return rc;
	}

	return pldm__msgbuf_insert_array_void(dst, ascii, len);
}

LIBPLDM_CC_ALWAYS_INLINE int
pldm_msgbuf_copy_string_utf16(struct pldm_msgbuf *dst, struct pldm_msgbuf *src)
{
	void *utf16 = NULL;
	size_t len = 0;
	int rc;

	rc = pldm_msgbuf_span_string_utf16(src, &utf16, &len);
	if (rc < 0) {
		return rc;
	}

	return pldm__msgbuf_insert_array_void(dst, utf16, len);
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <type_traits>

template <typename T>
static inline int pldm_msgbuf_typecheck_uint8_t(struct pldm_msgbuf *ctx,
						void *buf)
{
	static_assert(std::is_same<uint8_t *, T>::value);
	return pldm__msgbuf_extract_uint8(ctx, buf);
}

template <typename T>
static inline int pldm_msgbuf_typecheck_int8_t(struct pldm_msgbuf *ctx,
					       void *buf)
{
	static_assert(std::is_same<int8_t *, T>::value);
	return pldm__msgbuf_extract_int8(ctx, buf);
}

template <typename T>
static inline int pldm_msgbuf_typecheck_uint16_t(struct pldm_msgbuf *ctx,
						 void *buf)
{
	static_assert(std::is_same<uint16_t *, T>::value);
	return pldm__msgbuf_extract_uint16(ctx, buf);
}

template <typename T>
static inline int pldm_msgbuf_typecheck_int16_t(struct pldm_msgbuf *ctx,
						void *buf)
{
	static_assert(std::is_same<int16_t *, T>::value);
	return pldm__msgbuf_extract_int16(ctx, buf);
}

template <typename T>
static inline int pldm_msgbuf_typecheck_uint32_t(struct pldm_msgbuf *ctx,
						 void *buf)
{
	static_assert(std::is_same<uint32_t *, T>::value);
	return pldm__msgbuf_extract_uint32(ctx, buf);
}

template <typename T>
static inline int pldm_msgbuf_typecheck_int32_t(struct pldm_msgbuf *ctx,
						void *buf)
{
	static_assert(std::is_same<int32_t *, T>::value);
	return pldm__msgbuf_extract_int32(ctx, buf);
}

template <typename T>
static inline int pldm_msgbuf_typecheck_real32_t(struct pldm_msgbuf *ctx,
						 void *buf)
{
	static_assert(std::is_same<real32_t *, T>::value);
	return pldm__msgbuf_extract_real32(ctx, buf);
}
#endif

#endif /* BUF_H */
