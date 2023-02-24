#ifndef PLDM_MSGBUF_H
#define PLDM_MSGBUF_H

#include "base.h"
#include "pldm_types.h"

#include <assert.h>
#include <endian.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pldm_msgbuf {
	const uint8_t *cursor;
	ssize_t remaining;
};

/**
 * @brief Initialize pldm buf struct for buf extractor
 *
 * @param[in] ctx - pldm_msgbuf context for extractor
 * @param[in] buf - buffer to be extracted
 * @param[in] len - size of buffer
 *
 * @return PLDM_SUCCESS if all buffer accesses were in-bounds,
 * PLDM_ERROR_INVALID_LENGTH otherwise.
 */
static inline int pldm_msgbuf_init(struct pldm_msgbuf *ctx, size_t minsize,
				   const uint8_t *buf, size_t len)
{
	uint8_t *end;

	if (!ctx || !buf) {
		return -PLDM_ERROR_INVALID_DATA;
	}

	if (minsize > len) {
		return -PLDM_ERROR_INVALID_LENGTH;
	}

	if (len > SSIZE_MAX) {
		return -PLDM_ERROR_INVALID_LENGTH;
	}

	end = (uint8_t *)buf + len;
	if (end && end < (uint8_t *)buf) {
		return -PLDM_ERROR_INVALID_LENGTH;
	}

	ctx->cursor = buf;
	ctx->remaining = len;

	return PLDM_SUCCESS;
}

/**
 * Validate buffer overflow state
 *
 * @param[in] ctx - pldm_msgbuf context for extractor
 *
 * @return A positive value if there's data remaining in the buffer, 0 if the
 * buffer has been completely consumed, or a negative value if an overflow has
 * occurred
 */
static inline ssize_t pldm_msgbuf_validate(struct pldm_msgbuf *ctx)
{
	return ctx->remaining;
}

/**
 * @brief Destroy the pldm buf
 *
 * @param[in] ctx - pldm_msgbuf context for extractor
 *
 * @return PLDM_SUCCESS if all buffer accesses were in-bounds,
 * PLDM_ERROR_INVALID_LENGTH otherwise.
 */
static inline int pldm_msgbuf_destroy(struct pldm_msgbuf *ctx)
{
	if (!ctx) {
		return -PLDM_ERROR_INVALID_DATA;
	}

	ssize_t valid = pldm_msgbuf_validate(ctx);

	ctx->cursor = NULL;

	return valid >= 0 ? PLDM_SUCCESS : -PLDM_ERROR_INVALID_LENGTH;
}

/**
 * pldm_msgbuf extractors
 *
 * @param[in] ctx - pldm_msgbuf context for extractor
 * @param[out] dst - destination of extracted value
 *
 * @return PLDM_SUCCESS if buffer accesses were in-bounds,
 * PLDM_ERROR_INVALID_LENGTH otherwise.
 * PLDM_ERROR_INVALID_DATA if input a invalid ctx
 */
static inline int pldm_msgbuf_extract_uint8(struct pldm_msgbuf *ctx,
					    uint8_t *dst)
{
	if (!ctx) {
		return -PLDM_ERROR_INVALID_DATA;
	}

	if (!dst) {
		return -PLDM_ERROR_INVALID_DATA;
	}

	if (ctx->remaining <= 0) {
		return -PLDM_ERROR_INVALID_LENGTH;
	}

	*dst = *((uint8_t *)(ctx->cursor));
	ctx->cursor++;
	ctx->remaining--;
	return PLDM_SUCCESS;
}

static inline int pldm_msgbuf_extract_int8(struct pldm_msgbuf *ctx, int8_t *dst)
{
	if (!ctx) {
		return -PLDM_ERROR_INVALID_DATA;
	}

	if (!dst) {
		return -PLDM_ERROR_INVALID_DATA;
	}

	if (ctx->remaining <= 0) {
		return -PLDM_ERROR_INVALID_LENGTH;
	}

	*dst = *((int8_t *)(ctx->cursor));
	ctx->cursor++;
	ctx->remaining--;
	return PLDM_SUCCESS;
}

static inline int pldm_msgbuf_extract_uint16(struct pldm_msgbuf *ctx,
					     uint16_t *dst)
{
	uint16_t ldst;

	if (!ctx || !ctx->cursor || !dst) {
		return -PLDM_ERROR_INVALID_DATA;
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
		return -PLDM_ERROR_INVALID_LENGTH;
	}

	// Use memcpy() to have the compiler deal with any alignment
	// issues on the target architecture
	memcpy(&ldst, ctx->cursor, sizeof(ldst));

	// Only assign the target value once it's correctly decoded
	*dst = le16toh(ldst);
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

static inline int pldm_msgbuf_extract_int16(struct pldm_msgbuf *ctx,
					    int16_t *dst)
{
	int16_t ldst;

	if (!ctx || !ctx->cursor || !dst) {
		return -PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(ldst);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return -PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(&ldst, ctx->cursor, sizeof(ldst));

	*dst = le16toh(ldst);
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

static inline int pldm_msgbuf_extract_uint32(struct pldm_msgbuf *ctx,
					     uint32_t *dst)
{
	uint32_t ldst;

	if (!ctx || !ctx->cursor || !dst) {
		return -PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(ldst);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return -PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(&ldst, ctx->cursor, sizeof(ldst));

	*dst = le32toh(ldst);
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

static inline int pldm_msgbuf_extract_int32(struct pldm_msgbuf *ctx,
					    int32_t *dst)
{
	int32_t ldst;

	if (!ctx || !ctx->cursor || !dst) {
		return -PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(ldst);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return -PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(&ldst, ctx->cursor, sizeof(ldst));

	*dst = le32toh(ldst);
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

static inline int pldm_msgbuf_extract_real32(struct pldm_msgbuf *ctx,
					     real32_t *dst)
{
	uint32_t ldst;

	if (!ctx || !ctx->cursor || !dst) {
		return -PLDM_ERROR_INVALID_DATA;
	}

	ctx->remaining -= sizeof(ldst);
	assert(ctx->remaining >= 0);
	if (ctx->remaining < 0) {
		return -PLDM_ERROR_INVALID_LENGTH;
	}

	memcpy(&ldst, ctx->cursor, sizeof(ldst));

	ldst = le32toh(ldst);
	memcpy(dst, &ldst, sizeof(ldst));
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

#define pldm_msgbuf_extract(ctx, dst)                                          \
	_Generic((*(dst)), uint8_t                                             \
		 : pldm_msgbuf_extract_uint8, int8_t                           \
		 : pldm_msgbuf_extract_int8, uint16_t                          \
		 : pldm_msgbuf_extract_uint16, int16_t                         \
		 : pldm_msgbuf_extract_int16, uint32_t                         \
		 : pldm_msgbuf_extract_uint32, int32_t                         \
		 : pldm_msgbuf_extract_int32, real32_t                         \
		 : pldm_msgbuf_extract_real32)(ctx, dst)

#ifdef __cplusplus
}
#endif

#endif /* BUF_H */
