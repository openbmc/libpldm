#include <libpldm/firmware_update.h>
#include <libpldm/fru.h>
#include <libpldm/platform.h>
#include <stdlib.h>

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

    foreach_pldm_package_downstream_device_id_record(pkg, ddrec, rc)
    {
        struct pldm_descriptor desc;

        foreach_pldm_package_downstream_device_id_record_descriptor(pkg, ddrec,
                                                                    desc, rc)
        {
            (void)desc;
        }
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

static int fuzz_decode_pldm_platform_set_numeric_sensor_enable_resp(
    const uint8_t* data, size_t size)
{
    uint8_t completion_code;

    if (size < sizeof(struct pldm_msg))
    {
        return -1;
    }

    decode_pldm_platform_set_numeric_sensor_enable_resp((const void*)data, size,
                                                        &completion_code);

    return 0;
}

static int
    fuzz_encode_pldm_platform_set_numeric_sensor_enable_req(const uint8_t* data,
                                                            size_t size)
{
    PLDM_MSG_BUFFER(
        _msg, PLDM_MSG_SIZE(PLDM_PLATFORM_SET_NUMERIC_SENSOR_ENABLE_REQ_BYTES));
    struct pldm_msg* msg = (void*)&_msg;
    struct pldm_set_numeric_sensor_enable_req req;
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
    pldm_msgbuf_extract(buf, req.op_state);
    pldm_msgbuf_extract(buf, req.event_enable);

    rc = pldm_msgbuf_complete(buf);
    if (rc)
    {
        return -1;
    }

    encode_pldm_platform_set_numeric_sensor_enable_req(
        instance_id, &req, msg,
        PLDM_PLATFORM_SET_NUMERIC_SENSOR_ENABLE_REQ_BYTES);

    return 0;
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

static int (*const fuzz_tests[])(const uint8_t*, size_t) = {
    fuzz_decode_pldm_firmware_update_package,
    fuzz_decode_pldm_platform_set_numeric_sensor_enable_resp,
    fuzz_encode_pldm_platform_set_numeric_sensor_enable_req,
    fuzz_get_fru_record_by_option,
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
