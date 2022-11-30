#include "buf.h"
#include "base.h"
#include <assert.h>
#include <endian.h>
#include <stdbool.h>
#include <string.h>

int pldm_buf_init(struct pldm_buf *ctx, const uint8_t *buf, size_t len)
{
	uint8_t *end;

	if (!ctx || !buf) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!len) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	end = (uint8_t *)buf + len;
	if (end && end < (uint8_t *)buf) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	ctx->cursor = buf;
	ctx->remaining = len;

	return PLDM_SUCCESS;
}

int pldm_buf_destroy(struct pldm_buf *ctx)
{
	if (!ctx) {
		return PLDM_ERROR_INVALID_DATA;
	}

	ssize_t valid = pldm_buf_validate(ctx);

	ctx->cursor = NULL;

	return valid >= 0 ? PLDM_SUCCESS : PLDM_ERROR_INVALID_LENGTH;
}

ssize_t pldm_buf_validate(struct pldm_buf *ctx) { return ctx->remaining; }

int pldm_buf_extract_uint8(struct pldm_buf *ctx, uint8_t *dst)
{
	if (!ctx) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!dst) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!ctx->remaining) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*dst = *((uint8_t *)(ctx->cursor));
	ctx->cursor++;
	ctx->remaining--;
	return PLDM_SUCCESS;
}

int pldm_buf_extract_int8(struct pldm_buf *ctx, int8_t *dst)
{
	if (!ctx) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!dst) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!ctx->remaining) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*dst = *((int8_t *)(ctx->cursor));
	ctx->cursor++;
	ctx->remaining--;
	return PLDM_SUCCESS;
}

int pldm_buf_extract_uint16(struct pldm_buf *ctx, uint16_t *dst)
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
	*dst = le16toh(ldst);
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

int pldm_buf_extract_int16(struct pldm_buf *ctx, int16_t *dst)
{
	int16_t ldst;

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
	*dst = le16toh(ldst);
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

int pldm_buf_extract_uint32(struct pldm_buf *ctx, uint32_t *dst)
{
	uint32_t ldst;

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
	*dst = le32toh(ldst);
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

int pldm_buf_extract_int32(struct pldm_buf *ctx, int32_t *dst)
{
	int32_t ldst;

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
	*dst = le32toh(ldst);
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}

int pldm_buf_extract_real32(struct pldm_buf *ctx, real32_t *dst)
{
	uint32_t ldst;

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
	ldst = le32toh(ldst);
	memcpy(dst, &ldst, sizeof(ldst));
	ctx->cursor += sizeof(ldst);

	return PLDM_SUCCESS;
}