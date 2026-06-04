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

static int fuzz_decode_pldm_base_get_pldm_types_resp(const struct pldm_msg* msg,
                                                     size_t payload_length)
{
    struct pldm_base_get_pldm_types_resp resp;

    decode_pldm_base_get_pldm_types_resp(msg, payload_length, &resp);

    return 0;
}

static int fuzz_decode_pldm_platform_set_numeric_sensor_enable_resp(
    const struct pldm_msg* msg, size_t payload_length)
{
    uint8_t completion_code;

    decode_pldm_platform_set_numeric_sensor_enable_resp(msg, payload_length,
                                                        &completion_code);

    return 0;
}

static int (*const decode_pldm_msg_tests[])(const struct pldm_msg*, size_t) = {
    fuzz_decode_pldm_base_get_pldm_types_resp,
    fuzz_decode_pldm_platform_set_numeric_sensor_enable_resp,
};

static int libpldm_decode_one_pldm_msg(const uint8_t* data, size_t size)
{
    int rc = 0;

    if (size < sizeof(struct pldm_msg))
    {
        return -1;
    }

    for (size_t i = 0; i < ARRAY_SIZE(decode_pldm_msg_tests); i++)
    {
        rc += decode_pldm_msg_tests[i](
            (const void*)data, size - offsetof(struct pldm_msg, payload));
    }

    return -rc == ARRAY_SIZE(decode_pldm_msg_tests) ? -1 : 0;
}

static int fuzz_encode_pldm_base_get_pldm_types_resp(struct pldm_msg* msg,
                                                     size_t payload_length,
                                                     const uint8_t* data,
                                                     size_t size)
{
    struct pldm_base_get_pldm_types_resp resp;
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    uint8_t instance_id;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 1 + PLDM_BASE_GET_PLDM_TYPES_RESP_BYTES,
                                data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, instance_id);
    pldm_msgbuf_extract(buf, resp.completion_code);
    for (size_t i = 0; i < ARRAY_SIZE(resp.pldm_types); i++)
    {
        pldm_msgbuf_extract(buf, resp.pldm_types[i].byte);
    }

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    encode_pldm_base_get_pldm_types_resp(instance_id, &resp, msg,
                                         &payload_length);

    return 0;
}

static int fuzz_encode_pldm_platform_set_numeric_sensor_enable_req(
    struct pldm_msg* msg, size_t payload_length, const uint8_t* data,
    size_t size)
{
    struct pldm_platform_set_numeric_sensor_enable_req req;
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    uint8_t instance_id;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 0, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, instance_id);
    pldm_msgbuf_extract(buf, req.sensor_id);
    pldm_msgbuf_extract(buf, req.sensor_operational_state);
    pldm_msgbuf_extract(buf, req.sensor_event_message_enable);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    encode_pldm_platform_set_numeric_sensor_enable_req(instance_id, &req, msg,
                                                       &payload_length);

    return 0;
}

#if HAVE_LIBPLDM_API_TESTING
static int
    fuzz_encode_pldm_platform_numeric_sensor_value_pdr(const uint8_t* data,
                                                       size_t size)
{
    struct pldm_numeric_sensor_value_pdr pdr;
    uint8_t buf[PLDM_PDR_NUMERIC_SENSOR_PDR_FIXED_LENGTH +
                3 * sizeof(uint32_t) + 9 * sizeof(uint32_t)];
    size_t buf_len = sizeof(buf);

    if (size < sizeof(pdr))
    {
        return -1;
    }

    memcpy(&pdr, data, sizeof(pdr));
    encode_pldm_platform_numeric_sensor_value_pdr(&pdr, buf, &buf_len);

    return 0;
}
#endif

static int (*const encode_pldm_msg_tests[])(struct pldm_msg*, size_t,
                                            const uint8_t*, size_t) = {
    fuzz_encode_pldm_base_get_pldm_types_resp,
    fuzz_encode_pldm_platform_set_numeric_sensor_enable_req,
};

static int libpldm_encode_one_pldm_msg(const uint8_t* data, size_t size)
{
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    struct pldm_msg* msg;
    const uint8_t* mat;
    size_t mat_len;
    size_t msg_len;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 0, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract_uint16_to_size(buf, msg_len);
    pldm_msgbuf_span_remaining(buf, (const void**)&mat, &mat_len);

    rc = pldm_msgbuf_complete_consumed(buf);
    if (rc)
    {
        return -1;
    }

    if (msg_len < sizeof(*msg))
    {
        return -1;
    }

    msg = malloc(msg_len);
    rc = 0;

    for (size_t i = 0; i < ARRAY_SIZE(encode_pldm_msg_tests); i++)
    {
        rc += encode_pldm_msg_tests[i](
            msg, msg_len - offsetof(struct pldm_msg, payload), mat, mat_len);
    }

    free(msg);

    return -rc == ARRAY_SIZE(encode_pldm_msg_tests) ? -1 : 0;
}

static int (*const fuzz_tests[])(const uint8_t*, size_t) = {
    fuzz_get_fru_record_by_option,
    fuzz_pldm_state_effecter_pdr,
    libpldm_decode_one_pldm_msg,
    libpldm_encode_one_pldm_msg,
#if HAVE_LIBPLDM_API_TESTING
    fuzz_encode_pldm_platform_numeric_sensor_value_pdr,
#endif
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
