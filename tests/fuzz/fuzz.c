#include <libpldm/api.h>
#include <libpldm/base.h>
#include <libpldm/file.h>
#include <libpldm/firmware_update.h>
#include <libpldm/fru.h>
#include <libpldm/pdr.h>
#include <libpldm/platform.h>
#include <libpldm/rde.h>
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

static int fuzz_decode_pldm_rde_negotiate_redfish_parameters_req(
    const struct pldm_msg* msg, size_t payload_length)
{
    struct pldm_rde_negotiate_redfish_parameters_req req;

    decode_pldm_rde_negotiate_redfish_parameters_req(msg, payload_length, &req);

    return 0;
}

static int fuzz_decode_pldm_rde_negotiate_redfish_parameters_resp(
    const struct pldm_msg* msg, size_t payload_length)
{
    struct pldm_rde_negotiate_redfish_parameters_resp resp;

    decode_pldm_rde_negotiate_redfish_parameters_resp(msg, payload_length,
                                                      &resp);

    return 0;
}

static int fuzz_decode_pldm_rde_negotiate_medium_parameters_req(
    const struct pldm_msg* msg, size_t payload_length)
{
    struct pldm_rde_negotiate_medium_parameters_req req;

    decode_pldm_rde_negotiate_medium_parameters_req(msg, payload_length, &req);

    return 0;
}

static int fuzz_decode_pldm_rde_negotiate_medium_parameters_resp(
    const struct pldm_msg* msg, size_t payload_length)
{
    struct pldm_rde_negotiate_medium_parameters_resp resp;

    decode_pldm_rde_negotiate_medium_parameters_resp(msg, payload_length,
                                                     &resp);

    return 0;
}

static int
    fuzz_decode_pldm_rde_get_schema_dictionary_req(const struct pldm_msg* msg,
                                                   size_t payload_length)
{
    struct pldm_rde_get_schema_dictionary_req req;

    decode_pldm_rde_get_schema_dictionary_req(msg, payload_length, &req);

    return 0;
}

static int
    fuzz_decode_pldm_rde_get_schema_dictionary_resp(const struct pldm_msg* msg,
                                                    size_t payload_length)
{
    struct pldm_rde_get_schema_dictionary_resp resp;

    decode_pldm_rde_get_schema_dictionary_resp(msg, payload_length, &resp);

    return 0;
}

static int fuzz_decode_pldm_rde_get_schema_uri_req(const struct pldm_msg* msg,
                                                   size_t payload_length)
{
    struct pldm_rde_get_schema_uri_req req;

    decode_pldm_rde_get_schema_uri_req(msg, payload_length, &req);

    return 0;
}

static int fuzz_decode_pldm_rde_get_schema_uri_resp(const struct pldm_msg* msg,
                                                    size_t payload_length)
{
    struct pldm_rde_get_schema_uri_resp resp;
    struct pldm_rde_varstring frag;
    int rc;

    if (decode_pldm_rde_get_schema_uri_resp(msg, payload_length, &resp))
    {
        return 0;
    }

    foreach_pldm_rde_get_schema_uri_fragment(resp, frag, rc)
    {
        (void)frag;
    }
    (void)rc;

    return 0;
}

static int
    fuzz_decode_pldm_rde_get_resource_etag_req(const struct pldm_msg* msg,
                                               size_t payload_length)
{
    struct pldm_rde_get_resource_etag_req req;

    decode_pldm_rde_get_resource_etag_req(msg, payload_length, &req);

    return 0;
}

static int
    fuzz_decode_pldm_rde_get_resource_etag_resp(const struct pldm_msg* msg,
                                                size_t payload_length)
{
    struct pldm_rde_get_resource_etag_resp resp;

    decode_pldm_rde_get_resource_etag_resp(msg, payload_length, &resp);

    return 0;
}

static int fuzz_decode_pldm_rde_operation_init_req(const struct pldm_msg* msg,
                                                   size_t payload_length)
{
    struct pldm_rde_operation_init_req req;

    decode_pldm_rde_operation_init_req(msg, payload_length, &req);

    return 0;
}

static int fuzz_decode_pldm_rde_operation_init_resp(const struct pldm_msg* msg,
                                                    size_t payload_length)
{
    struct pldm_rde_operation_init_resp resp;

    decode_pldm_rde_operation_init_resp(msg, payload_length, &resp);

    return 0;
}

static int
    fuzz_decode_pldm_rde_operation_complete_req(const struct pldm_msg* msg,
                                                size_t payload_length)
{
    struct pldm_rde_operation_complete_req req;

    decode_pldm_rde_operation_complete_req(msg, payload_length, &req);

    return 0;
}

static int
    fuzz_decode_pldm_rde_operation_complete_resp(const struct pldm_msg* msg,
                                                 size_t payload_length)
{
    struct pldm_rde_operation_complete_resp resp;

    decode_pldm_rde_operation_complete_resp(msg, payload_length, &resp);

    return 0;
}

static int fuzz_decode_pldm_rde_operation_status_req(const struct pldm_msg* msg,
                                                     size_t payload_length)
{
    struct pldm_rde_operation_status_req req;

    decode_pldm_rde_operation_status_req(msg, payload_length, &req);

    return 0;
}

static int
    fuzz_decode_pldm_rde_operation_status_resp(const struct pldm_msg* msg,
                                               size_t payload_length)
{
    struct pldm_rde_operation_status_resp resp;

    decode_pldm_rde_operation_status_resp(msg, payload_length, &resp);

    return 0;
}

static int
    fuzz_decode_pldm_rde_operation_enumerate_req(const struct pldm_msg* msg,
                                                 size_t payload_length)
{
    decode_pldm_rde_operation_enumerate_req(msg, payload_length);

    return 0;
}

static int
    fuzz_decode_pldm_rde_operation_enumerate_resp(const struct pldm_msg* msg,
                                                  size_t payload_length)
{
    struct pldm_rde_operation_enumerate_resp resp;
    struct pldm_rde_op_entry entry;
    int rc;

    if (decode_pldm_rde_operation_enumerate_resp(msg, payload_length, &resp))
    {
        return 0;
    }

    foreach_pldm_rde_op_entry(resp, entry, rc)
    {
        (void)entry;
    }
    (void)rc;

    return 0;
}

static int (*const decode_pldm_msg_tests[])(const struct pldm_msg*, size_t) = {
    fuzz_decode_pldm_rde_operation_enumerate_req,
    fuzz_decode_pldm_rde_operation_enumerate_resp,
    fuzz_decode_pldm_rde_operation_status_req,
    fuzz_decode_pldm_rde_operation_status_resp,
    fuzz_decode_pldm_rde_operation_complete_req,
    fuzz_decode_pldm_rde_operation_complete_resp,
    fuzz_decode_pldm_rde_operation_init_req,
    fuzz_decode_pldm_rde_operation_init_resp,
    fuzz_decode_pldm_rde_get_resource_etag_req,
    fuzz_decode_pldm_rde_get_resource_etag_resp,
    fuzz_decode_pldm_rde_get_schema_uri_req,
    fuzz_decode_pldm_rde_get_schema_uri_resp,
    fuzz_decode_pldm_rde_get_schema_dictionary_req,
    fuzz_decode_pldm_rde_get_schema_dictionary_resp,
    fuzz_decode_pldm_rde_negotiate_medium_parameters_req,
    fuzz_decode_pldm_rde_negotiate_medium_parameters_resp,
    fuzz_decode_pldm_rde_negotiate_redfish_parameters_req,
    fuzz_decode_pldm_rde_negotiate_redfish_parameters_resp,
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

static int fuzz_encode_pldm_rde_negotiate_redfish_parameters_req(
    struct pldm_msg* msg, size_t payload_length, const uint8_t* data,
    size_t size)
{
    struct pldm_rde_negotiate_redfish_parameters_req req;
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    uint8_t instance_id;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 0, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, instance_id);
    pldm_msgbuf_extract(buf, req.mc_concurrency_support);
    pldm_msgbuf_extract(buf, req.mc_feature_support.value);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    encode_pldm_rde_negotiate_redfish_parameters_req(instance_id, &req, msg,
                                                     &payload_length);

    return 0;
}

static int fuzz_encode_pldm_rde_negotiate_redfish_parameters_resp(
    struct pldm_msg* msg, size_t payload_length, const uint8_t* data,
    size_t size)
{
    struct pldm_rde_negotiate_redfish_parameters_resp resp = {0};
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    const uint8_t* name;
    size_t name_len;
    uint8_t instance_id;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 0, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, instance_id);
    pldm_msgbuf_extract(buf, resp.completion_code);
    pldm_msgbuf_extract(buf, resp.device_concurrency_support);
    pldm_msgbuf_extract(buf, resp.device_capabilities_flags.byte);
    pldm_msgbuf_extract(buf, resp.device_feature_support.value);
    pldm_msgbuf_extract(buf, resp.device_configuration_signature);
    pldm_msgbuf_extract(buf, resp.provider_name.string_format);
    pldm_msgbuf_span_remaining(buf, (const void**)&name, &name_len);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    resp.provider_name.string_data.ptr = name;
    resp.provider_name.string_data.length = name_len;
    encode_pldm_rde_negotiate_redfish_parameters_resp(instance_id, &resp, msg,
                                                      &payload_length);

    return 0;
}

static int fuzz_encode_pldm_rde_negotiate_medium_parameters_req(
    struct pldm_msg* msg, size_t payload_length, const uint8_t* data,
    size_t size)
{
    struct pldm_rde_negotiate_medium_parameters_req req;
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    uint8_t instance_id;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 0, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, instance_id);
    pldm_msgbuf_extract(buf, req.mc_maximum_transfer_chunk_size_bytes);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    encode_pldm_rde_negotiate_medium_parameters_req(instance_id, &req, msg,
                                                    &payload_length);

    return 0;
}

static int fuzz_encode_pldm_rde_negotiate_medium_parameters_resp(
    struct pldm_msg* msg, size_t payload_length, const uint8_t* data,
    size_t size)
{
    struct pldm_rde_negotiate_medium_parameters_resp resp;
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
    pldm_msgbuf_extract(buf, resp.device_maximum_transfer_chunk_size_bytes);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    encode_pldm_rde_negotiate_medium_parameters_resp(instance_id, &resp, msg,
                                                     &payload_length);

    return 0;
}

static int fuzz_encode_pldm_rde_get_schema_dictionary_req(struct pldm_msg* msg,
                                                          size_t payload_length,
                                                          const uint8_t* data,
                                                          size_t size)
{
    struct pldm_rde_get_schema_dictionary_req req;
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    uint8_t instance_id;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 0, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, instance_id);
    pldm_msgbuf_extract(buf, req.resource_id);
    pldm_msgbuf_extract(buf, req.requested_schema_class);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    encode_pldm_rde_get_schema_dictionary_req(instance_id, &req, msg,
                                              &payload_length);

    return 0;
}

static int fuzz_encode_pldm_rde_get_schema_dictionary_resp(
    struct pldm_msg* msg, size_t payload_length, const uint8_t* data,
    size_t size)
{
    struct pldm_rde_get_schema_dictionary_resp resp;
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
    pldm_msgbuf_extract(buf, resp.dictionary_format);
    pldm_msgbuf_extract(buf, resp.transfer_handle);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    encode_pldm_rde_get_schema_dictionary_resp(instance_id, &resp, msg,
                                               &payload_length);

    return 0;
}

static int fuzz_encode_pldm_rde_get_schema_uri_req(struct pldm_msg* msg,
                                                   size_t payload_length,
                                                   const uint8_t* data,
                                                   size_t size)
{
    struct pldm_rde_get_schema_uri_req req;
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    uint8_t instance_id;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 0, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, instance_id);
    pldm_msgbuf_extract(buf, req.resource_id);
    pldm_msgbuf_extract(buf, req.requested_schema_class);
    pldm_msgbuf_extract(buf, req.oem_extension_number);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    encode_pldm_rde_get_schema_uri_req(instance_id, &req, msg, &payload_length);

    return 0;
}

static int fuzz_encode_pldm_rde_get_schema_uri_resp(struct pldm_msg* msg,
                                                    size_t payload_length,
                                                    const uint8_t* data,
                                                    size_t size)
{
    struct pldm_rde_get_schema_uri_resp resp = {0};
    struct pldm_rde_varstring uris[8];
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    const uint8_t* frags;
    size_t frags_len;
    uint8_t instance_id;
    uint8_t count;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 0, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, instance_id);
    pldm_msgbuf_extract(buf, resp.completion_code);
    pldm_msgbuf_extract(buf, count);
    pldm_msgbuf_span_remaining(buf, (const void**)&frags, &frags_len);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    count %= (uint8_t)(ARRAY_SIZE(uris) + 1);
    resp.string_fragment_count = count;
    for (uint8_t i = 0; i < count; i++)
    {
        size_t off = frags_len ? (i % frags_len) : 0;

        uris[i].string_format = frags_len ? frags[off] : 0;
        uris[i].string_data.ptr = frags + off;
        uris[i].string_data.length = frags_len - off;
    }

    encode_pldm_rde_get_schema_uri_resp(instance_id, &resp, uris, msg,
                                        &payload_length);

    return 0;
}

static int fuzz_encode_pldm_rde_get_resource_etag_req(struct pldm_msg* msg,
                                                      size_t payload_length,
                                                      const uint8_t* data,
                                                      size_t size)
{
    struct pldm_rde_get_resource_etag_req req;
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    uint8_t instance_id;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 0, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, instance_id);
    pldm_msgbuf_extract(buf, req.resource_id);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    encode_pldm_rde_get_resource_etag_req(instance_id, &req, msg,
                                          &payload_length);

    return 0;
}

static int fuzz_encode_pldm_rde_get_resource_etag_resp(struct pldm_msg* msg,
                                                       size_t payload_length,
                                                       const uint8_t* data,
                                                       size_t size)
{
    struct pldm_rde_get_resource_etag_resp resp = {0};
    PLDM_MSGBUF_RO_DEFINE_P(buf);
    const uint8_t* etag;
    size_t etag_len;
    uint8_t instance_id;
    int rc;

    rc = pldm_msgbuf_init_errno(buf, 0, data, size);
    if (rc)
    {
        return -1;
    }

    pldm_msgbuf_extract(buf, instance_id);
    pldm_msgbuf_extract(buf, resp.completion_code);
    pldm_msgbuf_extract(buf, resp.etag.string_format);
    pldm_msgbuf_span_remaining(buf, (const void**)&etag, &etag_len);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    resp.etag.string_data.ptr = etag;
    resp.etag.string_data.length = etag_len;
    encode_pldm_rde_get_resource_etag_resp(instance_id, &resp, msg,
                                           &payload_length);

    return 0;
}

static int (*const encode_pldm_msg_tests[])(struct pldm_msg*, size_t,
                                            const uint8_t*, size_t) = {
    fuzz_encode_pldm_rde_get_resource_etag_req,
    fuzz_encode_pldm_rde_get_resource_etag_resp,
    fuzz_encode_pldm_rde_get_schema_uri_req,
    fuzz_encode_pldm_rde_get_schema_uri_resp,
    fuzz_encode_pldm_rde_get_schema_dictionary_req,
    fuzz_encode_pldm_rde_get_schema_dictionary_resp,
    fuzz_encode_pldm_rde_negotiate_medium_parameters_req,
    fuzz_encode_pldm_rde_negotiate_medium_parameters_resp,
    fuzz_encode_pldm_rde_negotiate_redfish_parameters_req,
    fuzz_encode_pldm_rde_negotiate_redfish_parameters_resp,
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

static int (*const fuzz_tests[])(const uint8_t*, size_t) = {
    fuzz_decode_pldm_firmware_update_package,
    fuzz_get_fru_record_by_option,
    fuzz_pldm_pdr_add,
    fuzz_pldm_entity_association_pdr_extract,
    fuzz_pldm_state_effecter_pdr,
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
