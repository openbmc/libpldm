/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef PLDM_MSGBUF_H
#define PLDM_MSGBUF_H

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
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

struct pldm_msgbuf {
	uint8_t *cursor;
	ssize_t remaining;
};

/**
 * @brief Initialize pldm buf struct for buf extractor
 *
 * @param[out] ctx - pldm_msgbuf context for extractor
 * @param[in] minsize - The minimum required length of buffer `buf`
 * @param[in] buf - buffer to be extracted
 * @param[in] len - size of buffer
 *
 * @return PLDM_SUCCESS if all buffer accesses were in-bounds,
 * PLDM_ERROR_INVALID_DATA if pointer parameters are invalid, or
 * PLDM_ERROR_INVALID_LENGTH if length constraints are violated.
 */
__attribute__((no_sanitize("pointer-overflow"))) static inline int
pldm_msgbuf_init(struct pldm_msgbuf *ctx, size_t minsize, const void *buf,
		 size_t len)
{
	uint8_t *end;

	if (!ctx || !buf) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if ((minsize > len) || (len > SSIZE_MAX)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	end = (uint8_t *)buf + len;
	if (end && end < (uint8_t *)buf) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	ctx->cursor = (uint8_t *)buf;
	ctx->remaining = (ssize_t)len;

	return PLDM_SUCCESS;
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
static inline int pldm_msgbuf_validate(struct pldm_msgbuf *ctx)
{
	if (!ctx) {
		return PLDM_ERROR_INVALID_DATA;
	}

	return ctx->remaining >= 0 ? PLDM_SUCCESS : PLDM_ERROR_INVALID_LENGTH;
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
static inline int pldm_msgbuf_consumed(struct pldm_msgbuf *ctx)
{
	if (!ctx) {
		return PLDM_ERROR_INVALID_DATA;
	}

	return ctx->remaining == 0 ? PLDM_SUCCESS : PLDM_ERROR_INVALID_LENGTH;
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
static inline int pldm_msgbuf_destroy(struct pldm_msgbuf *ctx)
{
	int valid;

	if (!ctx) {
		return PLDM_ERROR_INVALID_DATA;
	}

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
static inline int pldm_msgbuf_destroy_consumed(struct pldm_msgbuf *ctx)
{
	int consumed;

	if (!ctx) {
		return PLDM_ERROR_INVALID_DATA;
	}

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
 * location of the instantation, which gives it a great leg-up over the problems
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
 * @param[inout] ctx - pldm_msgbuf context for extractor
 * @param[out] dst - destination of extracted value
 *
 * @return PLDM_SUCCESS if buffer accesses were in-bounds,
 * PLDM_ERROR_INVALID_LENGTH otherwise.
 * PLDM_ERROR_INVALID_DATA if input a invalid ctx
 */
#define pldm_msgbuf_extract_uint8(ctx, dst)                                    \
	pldm_msgbuf_extract_typecheck(uint8_t, pldm__msgbuf_extract_uint8,     \
				      dst, ctx, dst)
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
static inline int pldm__msgbuf_extract_uint8(struct pldm_msgbuf *ctx, void *dst)
{
	if (!ctx || !ctx->cursor || !dst) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(uint8_t);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(dst, ctx->cursor, sizeof(uint8_t));

	ctx->cursor++;
	return PLDM_SUCCESS;
}

#define pldm_msgbuf_extract_int8(ctx, dst)                                     \
	pldm_msgbuf_extract_typecheck(int8_t, pldm__msgbuf_extract_int8, dst,  \
				      ctx, dst)
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
static inline int pldm__msgbuf_extract_int8(struct pldm_msgbuf *ctx, void *dst)
{
	if (!ctx || !ctx->cursor || !dst) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(int8_t);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(dst, ctx->cursor, sizeof(int8_t));
	ctx->cursor++;
	return PLDM_SUCCESS;
}

#define pldm_msgbuf_extract_uint16(ctx, dst)                                   \
	pldm_msgbuf_extract_typecheck(uint16_t, pldm__msgbuf_extract_uint16,   \
				      dst, ctx, dst)
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
static inline int pldm__msgbuf_extract_uint16(struct pldm_msgbuf *ctx,
					      void *dst)
{
	uint16_t ldst;

	if (!ctx || !ctx->cursor || !dst) {
		return PLDM_ERROR_INVALID_DATA;
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
		return PLDM_ERROR_INVALID_LENGTH;
	}

	// Use memcpy() to have the compiler deal with any alignment
	// issues on the target architecture
	memcpy(&ldst, ctx->cursor, sizeof(ldst));

	// Only assign the target value once it's correctly decoded
	ldst = le16toh(ldst);

	// Allow storing to unaligned
	memcpy(dst, &ldst, sizeof(ldst));

	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

#define pldm_msgbuf_extract_int16(ctx, dst)                                    \
	pldm_msgbuf_extract_typecheck(int16_t, pldm__msgbuf_extract_int16,     \
				      dst, ctx, dst)
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
static inline int pldm__msgbuf_extract_int16(struct pldm_msgbuf *ctx, void *dst)
{
	int16_t ldst;

	if (!ctx || !ctx->cursor || !dst) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(ldst);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(&ldst, ctx->cursor, sizeof(ldst));

	ldst = le16toh(ldst);
	memcpy(dst, &ldst, sizeof(ldst));
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

#define pldm_msgbuf_extract_uint32(ctx, dst)                                   \
	pldm_msgbuf_extract_typecheck(uint32_t, pldm__msgbuf_extract_uint32,   \
				      dst, ctx, dst)
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
static inline int pldm__msgbuf_extract_uint32(struct pldm_msgbuf *ctx,
					      void *dst)
{
	uint32_t ldst;

	if (!ctx || !ctx->cursor || !dst) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(ldst);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(&ldst, ctx->cursor, sizeof(ldst));

	ldst = le32toh(ldst);
	memcpy(dst, &ldst, sizeof(ldst));
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

#define pldm_msgbuf_extract_int32(ctx, dst)                                    \
	pldm_msgbuf_extract_typecheck(int32_t, pldm__msgbuf_extract_int32,     \
				      dst, ctx, dst)
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
static inline int pldm__msgbuf_extract_int32(struct pldm_msgbuf *ctx, void *dst)
{
	int32_t ldst;

	if (!ctx || !ctx->cursor || !dst) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(ldst);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
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
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
static inline int pldm__msgbuf_extract_real32(struct pldm_msgbuf *ctx,
					      void *dst)
{
	uint32_t ldst;

	_Static_assert(sizeof(real32_t) == sizeof(ldst),
		       "Mismatched type sizes for dst and ldst");

	if (!ctx || !ctx->cursor || !dst) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(ldst);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(&ldst, ctx->cursor, sizeof(ldst));
	ldst = le32toh(ldst);
	memcpy(dst, &ldst, sizeof(ldst));
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
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

static inline int pldm_msgbuf_extract_array_uint8(struct pldm_msgbuf *ctx,
						  uint8_t *dst, size_t count)
{
	if (!ctx || !ctx->cursor || !dst) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!count) {
		return PLDM_SUCCESS;
	}

	if (count >= SSIZE_MAX) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	ctx->remaining -= (ssize_t)count;
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(dst, ctx->cursor, count);
	ctx->cursor += count;

	return PLDM_SUCCESS;
}

#define pldm_msgbuf_extract_array(ctx, dst, count)                             \
	_Generic((*(dst)), uint8_t: pldm_msgbuf_extract_array_uint8)(ctx, dst, \
								     count)

static inline int pldm_msgbuf_insert_uint32(struct pldm_msgbuf *ctx,
					    const uint32_t src)
{
	uint32_t val = htole32(src);

	if (!ctx || !ctx->cursor) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(src);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(ctx->cursor, &val, sizeof(val));
	ctx->cursor += sizeof(src);

	return PLDM_SUCCESS;
}

static inline int pldm_msgbuf_insert_uint16(struct pldm_msgbuf *ctx,
					    const uint16_t src)
{
	uint16_t val = htole16(src);

	if (!ctx || !ctx->cursor) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(src);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(ctx->cursor, &val, sizeof(val));
	ctx->cursor += sizeof(src);

	return PLDM_SUCCESS;
}

static inline int pldm_msgbuf_insert_uint8(struct pldm_msgbuf *ctx,
					   const uint8_t src)
{
	if (!ctx || !ctx->cursor) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(src);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(ctx->cursor, &src, sizeof(src));
	ctx->cursor += sizeof(src);

	return PLDM_SUCCESS;
}

static inline int pldm_msgbuf_insert_int32(struct pldm_msgbuf *ctx,
					   const int32_t src)
{
	int32_t val = htole32(src);

	if (!ctx || !ctx->cursor) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(src);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(ctx->cursor, &val, sizeof(val));
	ctx->cursor += sizeof(src);

	return PLDM_SUCCESS;
}

static inline int pldm_msgbuf_insert_int16(struct pldm_msgbuf *ctx,
					   const int16_t src)
{
	int16_t val = htole16(src);

	if (!ctx || !ctx->cursor) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(src);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(ctx->cursor, &val, sizeof(val));
	ctx->cursor += sizeof(src);

	return PLDM_SUCCESS;
}

static inline int pldm_msgbuf_insert_int8(struct pldm_msgbuf *ctx,
					  const int8_t src)
{
	if (!ctx || !ctx->cursor) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(src);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(ctx->cursor, &src, sizeof(src));
	ctx->cursor += sizeof(src);

	return PLDM_SUCCESS;
}

#define pldm_msgbuf_insert(dst, src)                                           \
	_Generic((src),                                                        \
		uint8_t: pldm_msgbuf_insert_uint8,                             \
		int8_t: pldm_msgbuf_insert_int8,                               \
		uint16_t: pldm_msgbuf_insert_uint16,                           \
		int16_t: pldm_msgbuf_insert_int16,                             \
		uint32_t: pldm_msgbuf_insert_uint32,                           \
		int32_t: pldm_msgbuf_insert_int32)(dst, src)

static inline int pldm_msgbuf_insert_array_uint8(struct pldm_msgbuf *ctx,
						 const uint8_t *src,
						 size_t count)
{
	if (!ctx || !ctx->cursor || !src) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!count) {
		return PLDM_SUCCESS;
	}

	if (count >= SSIZE_MAX) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	ctx->remaining -= (ssize_t)count;
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(ctx->cursor, src, count);
	ctx->cursor += count;

	return PLDM_SUCCESS;
}

#define pldm_msgbuf_insert_array(dst, src, count)                              \
	_Generic((*(src)), uint8_t: pldm_msgbuf_insert_array_uint8)(dst, src,  \
								    count)

static inline int pldm_msgbuf_span_required(struct pldm_msgbuf *ctx,
					    size_t required, void **cursor)
{
	if (!ctx || !ctx->cursor || !cursor || *cursor) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (required > SSIZE_MAX) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	ctx->remaining -= (ssize_t)required;
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*cursor = ctx->cursor;
	ctx->cursor += required;

	return PLDM_SUCCESS;
}

static inline int pldm_msgbuf_span_remaining(struct pldm_msgbuf *ctx,
					     void **cursor, size_t *len)
{
	if (!ctx || !ctx->cursor || !cursor || *cursor || !len) {
		return PLDM_ERROR_INVALID_DATA;
	}

	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*cursor = ctx->cursor;
	ctx->cursor += ctx->remaining;
	*len = ctx->remaining;
	ctx->remaining = 0;

	return PLDM_SUCCESS;
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
