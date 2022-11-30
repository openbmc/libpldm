#ifndef BUF_H
#define BUF_H

#include "pldm_types.h"
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pldm_buf {
	const uint8_t *cursor;
	ssize_t remaining;
};

/**
 * @brief Initialize pldm buf struct for buf extractor
 *
 * @param[in] ctx - pldm_buf context for extractor
 * @param[in] buf - buffer to be extracted
 * @param[in] len - size of buffer
 *
 * @return PLDM_SUCCESS if all buffer accesses were in-bounds,
 * PLDM_ERROR_INVALID_LENGTH otherwise.
 */
int pldm_buf_init(struct pldm_buf *ctx, const uint8_t *buf, size_t len);

/**
 * @brief Destroy the pldm buf
 *
 * @param[in] ctx - pldm_buf context for extractor
 *
 * @return PLDM_SUCCESS if all buffer accesses were in-bounds,
 * PLDM_ERROR_INVALID_LENGTH otherwise.
 */
int pldm_buf_destroy(struct pldm_buf *ctx);

/**
 * Validate buffer overflow state
 *
 * @param[in] ctx - pldm_buf context for extractor
 *
 * @return A positive value if there's data remaining in the buffer, 0 if the
 * buffer has been completely consumed, or a negative value if an overflow has
 * occurred
 */
ssize_t pldm_buf_validate(struct pldm_buf *ctx);

/**
 * pldm_buf extractors
 *
 * @param[in] ctx - pldm_buf context for extractor
 * @param[out] dst - destination of extracted value
 *
 * @return PLDM_SUCCESS if buffer accesses were in-bounds,
 * PLDM_ERROR_INVALID_LENGTH otherwise.
 * PLDM_ERROR_INVALID_DATA if input a invalid ctx
 */
int pldm_buf_extract_uint8(struct pldm_buf *ctx, uint8_t *dst);
int pldm_buf_extract_int8(struct pldm_buf *ctx, int8_t *dst);
int pldm_buf_extract_uint16(struct pldm_buf *ctx, uint16_t *dst);
int pldm_buf_extract_int16(struct pldm_buf *ctx, int16_t *dst);
int pldm_buf_extract_uint32(struct pldm_buf *ctx, uint32_t *dst);
int pldm_buf_extract_int32(struct pldm_buf *ctx, int32_t *dst);
int pldm_buf_extract_real32(struct pldm_buf *ctx, real32_t *dst);

#ifdef __cplusplus
}
#endif

#endif /* BUF_H */