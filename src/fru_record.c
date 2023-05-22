#include <assert.h>
#include <endian.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fru.h"

struct fru_record_tlv_iter {
	const uint8_t *start;
	size_t current_pos;
	size_t field_index;
	size_t nubmer_of_fields;
	const uint8_t *end_sentinel;
};

struct fru_record_data_iter {
	const uint8_t *start;
	const uint8_t *end_sentinel;
	size_t current_pos;
};

static struct fru_record_tlv_iter* _fru_record_tlv_iter_create(const struct pldm_fru_record_data_format *rdf,
		const void *end_sentinel)
{
	struct fru_record_tlv_iter *iter = malloc(sizeof(*iter));
	assert(iter != NULL);

	iter->start = (uint8_t *)rdf;
	iter->current_pos = sizeof(struct pldm_fru_record_data_format) - sizeof(struct pldm_fru_record_tlv);
	iter->field_index = 0;
	iter->nubmer_of_fields = rdf->num_fru_fields;
	iter->end_sentinel = end_sentinel;

	return iter;
}

void fru_record_tlv_iter_free(struct fru_record_tlv_iter *iter)
{
	free(iter);
}

bool fru_record_tlv_iter_is_end(const struct fru_record_tlv_iter *iter)
{
	if (iter->field_index >= iter->nubmer_of_fields) {
		return true;
	}

	if(iter->end_sentinel == NULL){
		return false;
	}

	if ((iter->start + iter->current_pos) >= iter->end_sentinel) {
		return true;
	}
	return false;
}

struct pldm_fru_record_tlv* fru_record_tlv_iter_value(const struct fru_record_tlv_iter *iter)
{
	if (fru_record_tlv_iter_is_end(iter)) {
		return NULL;
	}

	return (struct pldm_fru_record_tlv *)(iter->start + iter->current_pos);
}


void fru_record_tlv_iter_next(struct fru_record_tlv_iter *iter)
{
	const struct pldm_fru_record_tlv *tlv = (struct pldm_fru_record_tlv *)(iter->start + iter->current_pos);
	iter->current_pos += tlv->length;
}

size_t fru_record_tlv_iter_offset(const struct fru_record_tlv_iter *iter)
{
	return iter->current_pos + 1;
}

struct fru_record_data_iter *fru_record_data_iter_create(const uint8_t *table, size_t table_size)
{
	struct fru_record_data_iter *iter = malloc(sizeof(*iter));
	assert(iter != NULL);

	iter->start = table;
	iter->end_sentinel = (uint8_t *)(table + table_size);
	iter->current_pos = 0;

	return iter;
}

void fru_record_data_iter_free(struct fru_record_data_iter *iter)
{
	free(iter);
}

bool fru_record_data_iter_is_end(const struct fru_record_data_iter *iter)
{
	if ((iter->start + iter->current_pos) >= iter->end_sentinel) {
		return true;
	}
	return false;
}

struct pldm_fru_record_data_format *fru_record_data_iter_value(const struct fru_record_data_iter *iter)
{
	if(fru_record_data_iter_is_end(iter)){
		return NULL;
	}
	return (struct pldm_fru_record_data_format *)(iter->start + iter->current_pos);
}

struct fru_record_tlv_iter *fru_record_tlv_iter_create(const struct fru_record_data_iter *iter)
{
	struct pldm_fru_record_data_format *rdf = fru_record_data_iter_value(iter);
	if(rdf == NULL){
		return NULL;
	}

	return _fru_record_tlv_iter_create(rdf, iter->end_sentinel);
}

void fru_record_data_iter_skip(struct fru_record_data_iter *iter, size_t count)
{
	iter->current_pos += count;
}

void fru_record_data_iter_next(struct fru_record_data_iter *iter)
{
	struct fru_record_tlv_iter *tlv_iter = fru_record_tlv_iter_create(iter);
	while(!fru_record_tlv_iter_is_end(tlv_iter)){
		fru_record_tlv_iter_next(tlv_iter);
	}
	size_t offset = fru_record_tlv_iter_offset(tlv_iter);
	fru_record_tlv_iter_free(tlv_iter);
	fru_record_data_iter_skip(iter, offset);
}

struct buffer{
	uint8_t *start;
	size_t capacity;
	size_t current_pos;
};

static void buffer_init(struct buffer *b, void *start, size_t capacity)
{
	b->start = start;
	b->capacity = capacity;
	b->current_pos = 0;
}

static void buffer_append(struct buffer *b, void *data, size_t length)
{
	assert((b->current_pos + length) < b->capacity);

	memcpy(b->start + b->current_pos, data, length);
	b->current_pos += length;
}

static size_t buffer_size(struct buffer *b)
{
	return b->current_pos;
}

static void* buffer_value(struct buffer *b)
{
	return b->start + b->current_pos;
}

void get_fru_record_by_option1(const uint8_t *table, size_t table_size,
		uint8_t *record_table, size_t *record_size,
		uint16_t rsi, uint8_t rt, uint8_t ft)
{
	struct fru_record_data_iter *rd_iter = fru_record_data_iter_create(table, table_size);
	struct pldm_fru_record_data_format *rdf;

	struct buffer buf;
	buffer_init(&buf, record_table, *record_size);

	while( !fru_record_data_iter_is_end( rd_iter)) {
		rdf = fru_record_data_iter_value(rd_iter);
		if ((rdf->record_set_id != htobe16(rsi)
					&& rsi != 0) || (rdf->record_type != rt && rt != 0)){
			fru_record_data_iter_next(rd_iter);
			continue;
		}
		struct pldm_fru_record_data_format *rdf1 = buffer_value(&buf);
		struct fru_record_tlv_iter *tlv_iter = fru_record_tlv_iter_create(rd_iter);
		buffer_append(&buf, rdf, fru_record_tlv_iter_offset(tlv_iter));
		rdf1->num_fru_fields = 0;

		while(!fru_record_tlv_iter_is_end(tlv_iter)){
			struct pldm_fru_record_tlv *tlv = fru_record_tlv_iter_value(tlv_iter);
			if(tlv->type == ft || ft == 0){
				buffer_append(&buf, tlv, tlv->length);
				rdf1->num_fru_fields++;
			}
			fru_record_tlv_iter_next(tlv_iter);

		}
		size_t offset = fru_record_tlv_iter_offset(tlv_iter);
		fru_record_tlv_iter_free(tlv_iter);
		fru_record_data_iter_skip(rd_iter, offset);
	}
	fru_record_data_iter_free(rd_iter);
	*record_size = buffer_size(&buf);
}
