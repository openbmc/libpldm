#include <libpldm/fru.h>
#include <libpldm/platform.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "msgbuf.h"

static int fuzz_get_fru_record_by_option(const uint8_t* data, size_t size)
{
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    size_t record_size;
    size_t table_size;
    uint8_t* record;
    uint8_t* table;
    uint16_t rsi;
    uint8_t rt;
    uint8_t ft;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 8, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract_uint16_to_size(buf, table_size);
    pldm_msgbuf_extract_uint16_to_size(buf, record_size);
    pldm_msgbuf_extract(buf, rsi);
    pldm_msgbuf_extract(buf, rt);
    pldm_msgbuf_extract(buf, ft);

    rc = pldm_msgbuf_span_required(buf, table_size, (const void**)&table);
    if (rc)
    {
        return pldm_msgbuf_discard(buf, -1);
    }

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    record = malloc(record_size);
    if (!record)
    {
        return -1;
    }

    get_fru_record_by_option(table, table_size, record, &record_size, rsi, rt,
                             ft);

    free(record);

    return 0;
}

static int fuzz_pldm_state_effecter_pdr(const uint8_t* data, size_t size)
{
    struct state_effecter_possible_states states;
    struct pldm_state_effecter_pdr* pdr;
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    const uint8_t* tail;
    size_t header_size;
    size_t tail_size;
    int rc;

    header_size =
        sizeof(struct pldm_state_effecter_pdr) - sizeof(pdr->possible_states);

    pdr = malloc(size);
    if (!pdr)
    {
        return -1;
    }

    /*
     * Ideally we would use decode_pldm_state_effecter_pdr(), but it is not
     * currently exposed and needs some rework before we make it so.
     *
     * TODO: rework struct pldm_state_effecter_pdr so that it doesn't require
     * __attribute__((packed)).
     *
     * TODO: clean up the function prototype for
     * decode_pldm_state_effecter_pdr() - make it use sensible types
     */
    rc = pldm_msgbuf_init_errno(buf, header_size, data, size);
    if (rc)
    {
        rc = -1;
        goto cleanup_pdr;
    }
    pldm_msgbuf_extract(buf, pdr->hdr.record_handle);
    pldm_msgbuf_extract(buf, pdr->hdr.version);
    pldm_msgbuf_extract(buf, pdr->hdr.type);
    pldm_msgbuf_extract(buf, pdr->hdr.record_change_num);
    pldm_msgbuf_extract(buf, pdr->hdr.length);
    pldm_msgbuf_extract(buf, pdr->terminus_handle);
    pldm_msgbuf_extract(buf, pdr->effecter_id);
    pldm_msgbuf_extract(buf, pdr->entity_type);
    pldm_msgbuf_extract(buf, pdr->entity_instance);
    pldm_msgbuf_extract(buf, pdr->container_id);
    pldm_msgbuf_extract(buf, pdr->effecter_semantic_id);
    pldm_msgbuf_extract(buf, pdr->effecter_init);
    pldm_msgbuf_extract(buf, pdr->has_description_pdr);
    pldm_msgbuf_extract(buf, pdr->composite_effecter_count);
    pldm_msgbuf_span_remaining(buf, (const void**)&tail, &tail_size);
    if (pldm_msgbuf_complete(buf))
    {
        rc = -1;
        goto cleanup_pdr;
    }
    memcpy(pdr->possible_states, tail, tail_size);

    foreach_pldm_platform_state_effecter_pdr_possible_states(pdr, size, states,
                                                             rc)
    {
        bitfield8_t bitfield;

        foreach_pldm_platform_state_effecter_pdr_states(states, bitfield, rc)
        {
            (void)bitfield.byte;
        }
        if (rc)
        {
            break;
        }
    }

cleanup_pdr:
    free(pdr);

    return rc;
}

static int (*const fuzz_tests[])(const uint8_t*, size_t) = {
    fuzz_get_fru_record_by_option,
    fuzz_pldm_state_effecter_pdr,
};

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    int rc = 0;
    for (size_t i = 0; i < ARRAY_SIZE(fuzz_tests); i++)
    {
        rc += fuzz_tests[i](data, size);
    }
    return -rc == ARRAY_SIZE(fuzz_tests) ? -1 : 0;
}
