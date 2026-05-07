#include <libpldm/fru.h>
#include <stdlib.h>

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
        return pldm_msgbuf_discard(buf, -1);
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

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    return fuzz_get_fru_record_by_option(data, size);
}
