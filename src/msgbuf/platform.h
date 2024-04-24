/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef PLDM_MSGBUF_PLATFORM_H
#define PLDM_MSGBUF_PLATFORM_H

#include "../msgbuf.h"
#include <libpldm/base.h>
#include <libpldm/platform.h>

static inline int
pldm_msgbuf_extract_value_pdr_hdr(struct pldm_msgbuf *ctx,
				  struct pldm_value_pdr_hdr *hdr)
{
	pldm_msgbuf_extract(ctx, &hdr->record_handle);
	pldm_msgbuf_extract(ctx, &hdr->version);
	pldm_msgbuf_extract(ctx, &hdr->type);
	pldm_msgbuf_extract(ctx, &hdr->record_change_num);
	pldm_msgbuf_extract(ctx, &hdr->length);

	return pldm_msgbuf_validate(ctx);
}

/*
 * We use __attribute__((always_inline)) below so the compiler has visibility of
 * the switch() at the call site. It is often the case that the size of multiple
 * fields depends on the tag. Inlining thus gives the compiler visibility to
 * hoist one tag-based code-path condition to cover all invocations.
 */

__attribute__((always_inline)) static inline int
pldm_msgbuf_extract_sensor_data(struct pldm_msgbuf *ctx,
				enum pldm_sensor_readings_data_type tag,
				union_sensor_data_size *dst)
{
	switch (tag) {
	case PLDM_SENSOR_DATA_SIZE_UINT8:
		return pldm_msgbuf_extract(ctx, &dst->value_u8);
	case PLDM_SENSOR_DATA_SIZE_SINT8:
		return pldm_msgbuf_extract(ctx, &dst->value_s8);
	case PLDM_SENSOR_DATA_SIZE_UINT16:
		return pldm_msgbuf_extract(ctx, &dst->value_u16);
	case PLDM_SENSOR_DATA_SIZE_SINT16:
		return pldm_msgbuf_extract(ctx, &dst->value_s16);
	case PLDM_SENSOR_DATA_SIZE_UINT32:
		return pldm_msgbuf_extract(ctx, &dst->value_u32);
	case PLDM_SENSOR_DATA_SIZE_SINT32:
		return pldm_msgbuf_extract(ctx, &dst->value_s32);
	}

	return -PLDM_ERROR_INVALID_DATA;
}

/*
 * This API is bad, but it's because the caller's APIs are also bad. They should
 * have used the approach used by callers of pldm_msgbuf_extract_sensor_data()
 * above
 */
__attribute__((always_inline)) static inline int
pldm_msgbuf_extract_sensor_value(struct pldm_msgbuf *ctx,
				 enum pldm_sensor_readings_data_type tag,
				 uint8_t *val)
{
	switch (tag) {
	case PLDM_SENSOR_DATA_SIZE_UINT8:
		return pldm_msgbuf_extract_uint8(ctx, (uint8_t *)val);
	case PLDM_SENSOR_DATA_SIZE_SINT8:
		return pldm_msgbuf_extract_int8(ctx, (int8_t *)val);
	case PLDM_SENSOR_DATA_SIZE_UINT16:
		return pldm_msgbuf_extract_uint16(ctx, (uint16_t *)val);
	case PLDM_SENSOR_DATA_SIZE_SINT16:
		return pldm_msgbuf_extract_int16(ctx, (int16_t *)val);
	case PLDM_SENSOR_DATA_SIZE_UINT32:
		return pldm_msgbuf_extract_uint32(ctx, (uint32_t *)val);
	case PLDM_SENSOR_DATA_SIZE_SINT32:
		return pldm_msgbuf_extract_int32(ctx, (int32_t *)val);
	}

	return -PLDM_ERROR_INVALID_DATA;
}

__attribute__((always_inline)) static inline int
pldm_msgbuf_extract_range_field_format(struct pldm_msgbuf *ctx,
				       enum pldm_range_field_format tag,
				       void *dst)
{
	int ret;
	uint8_t value_u8;
	int8_t value_i8;
	uint16_t value_u16;
	int16_t value_i16;
	uint32_t value_u32;
	int32_t value_i32;
	real32_t value_r32;
	switch (tag) {
	case PLDM_RANGE_FIELD_FORMAT_UINT8:
		ret = pldm_msgbuf_extract(ctx, &value_u8);
		memcpy(dst, &value_u8, sizeof(uint8_t));
		return ret;
	case PLDM_RANGE_FIELD_FORMAT_SINT8:
		ret = pldm_msgbuf_extract(ctx, &value_i8);
		memcpy(dst, &value_i8, sizeof(int8_t));
		return ret;
	case PLDM_RANGE_FIELD_FORMAT_UINT16:
		ret = pldm_msgbuf_extract(ctx, &value_u16);
		memcpy(dst, &value_u16, sizeof(uint16_t));
		return ret;
	case PLDM_RANGE_FIELD_FORMAT_SINT16:
		ret = pldm_msgbuf_extract(ctx, &value_i16);
		memcpy(dst, &value_i16, sizeof(int16_t));
		return ret;
	case PLDM_RANGE_FIELD_FORMAT_UINT32:
		ret = pldm_msgbuf_extract(ctx, &value_u32);
		memcpy(dst, &value_u32, sizeof(uint32_t));
		return ret;
	case PLDM_RANGE_FIELD_FORMAT_SINT32:
		ret = pldm_msgbuf_extract(ctx, &value_i32);
		memcpy(dst, &value_i32, sizeof(int32_t));
		return ret;
	case PLDM_RANGE_FIELD_FORMAT_REAL32:
		ret = pldm_msgbuf_extract(ctx, &value_r32);
		memcpy(dst, &value_r32, sizeof(real32_t));
		return ret;
	}

	return -PLDM_ERROR_INVALID_DATA;
}

/* This API is bad, but it's because the caller's APIs are also bad */
__attribute__((always_inline)) static inline int
pldm_msgbuf_extract_effecter_value(struct pldm_msgbuf *ctx,
				   enum pldm_effecter_data_size tag,
				   uint8_t *val)
{
	switch (tag) {
	case PLDM_EFFECTER_DATA_SIZE_UINT8:
		return pldm_msgbuf_extract_uint8(ctx, (uint8_t *)val);
	case PLDM_EFFECTER_DATA_SIZE_SINT8:
		return pldm_msgbuf_extract_int8(ctx, (int8_t *)val);
	case PLDM_EFFECTER_DATA_SIZE_UINT16:
		return pldm_msgbuf_extract_uint16(ctx, (uint16_t *)val);
	case PLDM_EFFECTER_DATA_SIZE_SINT16:
		return pldm_msgbuf_extract_int16(ctx, (int16_t *)val);
	case PLDM_EFFECTER_DATA_SIZE_UINT32:
		return pldm_msgbuf_extract_uint32(ctx, (uint32_t *)val);
	case PLDM_EFFECTER_DATA_SIZE_SINT32:
		return pldm_msgbuf_extract_int32(ctx, (int32_t *)val);
	}

	return -PLDM_ERROR_INVALID_DATA;
}

#endif
