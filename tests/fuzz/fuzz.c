#include <libpldm/api.h>
#include <libpldm/base.h>
#include <libpldm/file.h>
#include <libpldm/firmware_update.h>
#include <libpldm/fru.h>
#include <libpldm/pdr.h>
#include <libpldm/platform.h>
#include <libpldm/platform_pd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "msgbuf.h"

int pldm_edac_crc32_validate(uint32_t expected LIBPLDM_CC_UNUSED,
                             const void* data LIBPLDM_CC_UNUSED,
                             size_t size LIBPLDM_CC_UNUSED)
{
    return 0;
}

static int fuzz_decode_pldm_firmware_update_package(const uint8_t* data,
                                                    size_t size)
{
    static const uint8_t package_header_identifiers[][16] = {
        PLDM_PACKAGE_HEADER_IDENTIFIER_V1_0,
        PLDM_PACKAGE_HEADER_IDENTIFIER_V1_1,
        PLDM_PACKAGE_HEADER_IDENTIFIER_V1_2,
        PLDM_PACKAGE_HEADER_IDENTIFIER_V1_3,
    };

    struct pldm_package_downstream_device_id_record ddrec;
    struct pldm_package_component_image_information info;
    struct pldm_package_firmware_device_id_record fdrec;
    DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR04H(pin);
    pldm_package_header_information_pad hdr;
    struct pldm_package pkg = {0};
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    const uint8_t (*id)[16];
    uint8_t id_idx;
    void* package;
    int rc;

    if (size < sizeof(*id))
    {
        return -1;
    }

    package = calloc(size, 1);
    if (!package)
    {
        return 0;
    }

    rc = pldm_msgbuf_init_errno(buf, 1, data, size);
    if (rc)
    {
        rc = -1;
        goto cleanup_package;
    }

    pldm_msgbuf_extract(buf, id_idx);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    memcpy(package, data, size);

    id_idx %= ARRAY_SIZE(package_header_identifiers);
    id = &package_header_identifiers[id_idx];
    memcpy(package, package_header_identifiers[id_idx], sizeof(*id));

    rc =
        decode_pldm_firmware_update_package(package, size, &pin, &hdr, &pkg, 0);
    if (rc < 0)
    {
        rc = 0;
        goto cleanup_package;
    }

    foreach_pldm_package_firmware_device_id_record(pkg, fdrec, rc)
    {
        (void)fdrec;
    }
    if (rc < 0)
    {
        rc = 0;
        goto cleanup_package;
    }

    foreach_pldm_package_downstream_device_id_record(pkg, ddrec, rc)
    {
        struct pldm_descriptor desc;

        foreach_pldm_package_downstream_device_id_record_descriptor(pkg, ddrec,
                                                                    desc, rc)
        {
            (void)desc;
        }
        if (rc)
        {
            rc = 0;
            goto cleanup_package;
        }
    }
    if (rc)
    {
        rc = 0;
        goto cleanup_package;
    }

    foreach_pldm_package_component_image_information(pkg, info, rc)
    {
        (void)info;
    }

    rc = 0;
cleanup_package:
    free(package);
    return rc;
}

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

static int fuzz_decode_pldm_base_get_tid_resp(const struct pldm_msg* msg,
                                              size_t payload_length)
{
    struct pldm_base_get_tid_resp resp;

    decode_pldm_base_get_tid_resp(msg, payload_length, &resp);

    return 0;
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

static int fuzz_decode_pldm_file_df_heartbeat_req(const struct pldm_msg* msg,
                                                  size_t payload_length)
{
    struct pldm_file_df_heartbeat_req req;

    decode_pldm_file_df_heartbeat_req(msg, payload_length, &req);

    return 0;
}

static int
    fuzz_decode_pldm_base_multipart_receive_req(const struct pldm_msg* msg,
                                                size_t payload_length)
{
    struct pldm_base_multipart_receive_req req;

    decode_pldm_base_multipart_receive_req(msg, payload_length, &req);

    return 0;
}

static int (*const decode_pldm_msg_tests[])(const struct pldm_msg*, size_t) = {
    fuzz_decode_pldm_base_get_tid_resp,
    fuzz_decode_pldm_base_get_pldm_types_resp,
    fuzz_decode_pldm_platform_set_numeric_sensor_enable_resp,
    fuzz_decode_pldm_file_df_heartbeat_req,
    fuzz_decode_pldm_base_multipart_receive_req,
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

static int fuzz_encode_pldm_base_get_tid_resp(struct pldm_msg* msg,
                                              size_t payload_length,
                                              const uint8_t* data, size_t size)
{
    struct pldm_base_get_tid_resp resp;
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    uint8_t instance_id;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 1 + PLDM_BASE_GET_TID_RESP_BYTES, data,
                                size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, instance_id);
    pldm_msgbuf_extract(buf, resp.completion_code);
    pldm_msgbuf_extract(buf, resp.tid);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    encode_pldm_base_get_tid_resp(instance_id, &resp, msg, &payload_length);

    return 0;
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

static int fuzz_encode_pldm_file_df_heartbeat_resp(struct pldm_msg* msg,
                                                   size_t payload_length,
                                                   const uint8_t* data,
                                                   size_t size)
{
    struct pldm_file_df_heartbeat_resp resp;
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    uint8_t instance_id;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 0, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, instance_id);
    pldm_msgbuf_extract(buf, resp.completion_code);
    pldm_msgbuf_extract(buf, resp.responder_max_interval);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    encode_pldm_file_df_heartbeat_resp(instance_id, &resp, msg,
                                       &payload_length);

    return 0;
}

static int (*const encode_pldm_msg_tests[])(struct pldm_msg*, size_t,
                                            const uint8_t*, size_t) = {
    fuzz_encode_pldm_base_get_tid_resp,
    fuzz_encode_pldm_base_get_pldm_types_resp,
    fuzz_encode_pldm_platform_set_numeric_sensor_enable_req,
    fuzz_encode_pldm_file_df_heartbeat_resp,
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

static int fuzz_pldm_pdr_add(const uint8_t* data, size_t size)
{
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    uint16_t terminus_handle;
    const uint8_t* pdr_data;
    uint32_t record_handle;
    uint8_t is_remote;
    size_t pdr_size;
    pldm_pdr* repo;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 0, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, record_handle);
    pldm_msgbuf_extract(buf, terminus_handle);
    pldm_msgbuf_extract(buf, is_remote);
    pldm_msgbuf_span_remaining(buf, (const void**)&pdr_data, &pdr_size);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    repo = pldm_pdr_init();
    if (!repo)
    {
        return -1;
    }

    pldm_pdr_add(repo, pdr_data, pdr_size, is_remote & 1, terminus_handle,
                 &record_handle);

    pldm_pdr_destroy(repo);

    return 0;
}

static int fuzz_pldm_entity_association_pdr_extract(const uint8_t* data,
                                                    size_t size)
{
    pldm_entity* entities = NULL;
    size_t num_entities = 0;

    if (size > UINT16_MAX)
    {
        return -1;
    }

    pldm_entity_association_pdr_extract(data, (uint16_t)size, &num_entities,
                                        &entities);

    free(entities);

    return 0;
}

static uint8_t
    fuzz_pd_get_sensor_reading(void* ctx LIBPLDM_CC_UNUSED,
                               const struct pldm_numeric_sensor_value_pdr* pdr,
                               bool8_t rearm_event_state LIBPLDM_CC_UNUSED,
                               struct pldm_platform_pd_sensor_state* state)
{
    state->operational_state = PLDM_SENSOR_ENABLED;
    state->event_enable = PLDM_NO_EVENT_GENERATION;
    state->present_state = PLDM_SENSOR_NORMAL;
    state->previous_state = PLDM_SENSOR_NORMAL;
    state->event_state = PLDM_SENSOR_NORMAL;

    switch (pdr->sensor_data_size)
    {
        case PLDM_SENSOR_DATA_SIZE_UINT16:
        case PLDM_SENSOR_DATA_SIZE_SINT16:
            state->current_reading.value_u16 = 0;
            break;
        case PLDM_SENSOR_DATA_SIZE_UINT32:
        case PLDM_SENSOR_DATA_SIZE_SINT32:
            state->current_reading.value_u32 = 0;
            break;
        default:
            state->current_reading.value_u8 = 0;
            break;
    }

    return PLDM_SUCCESS;
}

/*
 * Numeric sensor PDR for sensor ID 1, uint8 data size.
 * Used to exercise the GET_SENSOR_READING path in the platform_pd responder.
 */
static const uint8_t fuzz_pd_sensor_pdr[] = {
    /* pldm_value_pdr_hdr */
    0x00,
    0x00,
    0x00,
    0x00,                    /* record_handle */
    0x01,                    /* version */
    PLDM_NUMERIC_SENSOR_PDR, /* type */
    0x00,
    0x00, /* record_change_num */
    0x45,
    0x00, /* length = PLDM_PDR_NUMERIC_SENSOR_PDR_MIN_LENGTH */
    /* body */
    0x00,
    0x00, /* terminus_handle */
    0x01,
    0x00, /* sensor_id = 1 */
    0x00,
    0x00, /* entity_type */
    0x01,
    0x00, /* entity_instance_num */
    0x00,
    0x00,                        /* container_id */
    PLDM_NO_INIT,                /* sensor_init */
    0x00,                        /* sensor_auxiliary_names_pdr */
    0x00,                        /* base_unit */
    0x00,                        /* unit_modifier */
    0x00,                        /* rate_unit */
    0x00,                        /* base_oem_unit_handle */
    0x00,                        /* aux_unit */
    0x00,                        /* aux_unit_modifier */
    0x00,                        /* aux_rate_unit */
    0x00,                        /* rel */
    0x00,                        /* aux_oem_unit_handle */
    0x01,                        /* is_linear */
    PLDM_SENSOR_DATA_SIZE_UINT8, /* sensor_data_size */
    0x00,
    0x00,
    0x80,
    0x3f, /* resolution = 1.0f */
    0x00,
    0x00,
    0x00,
    0x00, /* offset = 0.0f */
    0x00,
    0x00, /* accuracy */
    0x00, /* plus_tolerance */
    0x00, /* minus_tolerance */
    0x00, /* hysteresis */
    0x00, /* supported_thresholds */
    0x00, /* threshold_and_hysteresis_volatility */
    0x00,
    0x00,
    0x00,
    0x00, /* state_transition_interval */
    0x00,
    0x00,
    0x00,
    0x00,                          /* update_interval */
    0xff,                          /* max_readable */
    0x00,                          /* min_readable */
    PLDM_RANGE_FIELD_FORMAT_UINT8, /* range_field_format */
    0x00,                          /* range_field_support */
    0x00,
    0x00,
    0x00, /* nominal, normal_max, normal_min */
    0x00,
    0x00, /* warning_high, warning_low */
    0x00,
    0x00, /* critical_high, critical_low */
    0x00,
    0x00, /* fatal_high, fatal_low */
};

static int fuzz_platform_pd_handle_msg(const uint8_t* data, size_t size)
{
    uint8_t out[4096];
    struct pldm_platform_pd_ops ops = {
        .get_sensor_reading = fuzz_pd_get_sensor_reading,
        .ctx = NULL,
    };
    struct pldm_platform_pd* pd;
    uint32_t record_handle = 0;
    size_t out_len = sizeof(out);
    pldm_pdr* repo;
    int rc;

    repo = pldm_pdr_init();
    if (!repo)
    {
        return -1;
    }

    rc = pldm_pdr_add(repo, fuzz_pd_sensor_pdr, sizeof(fuzz_pd_sensor_pdr),
                      false, 0, &record_handle);
    if (rc)
    {
        pldm_pdr_destroy(repo);
        return -1;
    }

    pd = pldm_platform_pd_new(repo, NULL, &ops, sizeof(ops));
    if (!pd)
    {
        pldm_pdr_destroy(repo);
        return -1;
    }

    pldm_platform_pd_handle_msg(pd, data, size, out, &out_len);

    free(pd);
    pldm_pdr_destroy(repo);
    return 0;
}

static int (*const fuzz_tests[])(const uint8_t*, size_t) = {
    fuzz_decode_pldm_firmware_update_package,
    fuzz_get_fru_record_by_option,
    fuzz_pldm_pdr_add,
    fuzz_pldm_entity_association_pdr_extract,
    fuzz_pldm_state_effecter_pdr,
    fuzz_platform_pd_handle_msg,
    libpldm_decode_one_pldm_msg,
    libpldm_encode_one_pldm_msg,
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
