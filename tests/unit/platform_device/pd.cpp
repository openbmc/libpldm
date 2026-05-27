// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

#include <libpldm/base.h>
#include <libpldm/control.h>
#include <libpldm/pdr.h>
#include <libpldm/platform.h>
#include <libpldm/platform_device.h>
#include <libpldm/sizes.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <expected>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

static constexpr auto hdrSize = sizeof(pldm_msg_hdr);

/* Arbitrary instance ID for tests */
static constexpr uint8_t TEST_IID = 3;

/* ------------------------------------------------------------------
 * Test callbacks
 * ------------------------------------------------------------------ */

struct SensorState
{
    uint8_t op_state = PLDM_SENSOR_ENABLED;
    uint8_t event_enable = PLDM_NO_EVENT_GENERATION;
    uint8_t present_state = 0;
    uint8_t previous_state = 0;
    uint8_t event_state = 0;
    uint8_t reading[4] = {42, 0, 0, 0};
    uint8_t return_cc = PLDM_SUCCESS;
};

static SensorState g_sensor;

static uint8_t cb_get_sensor_reading(
    void* ctx, const struct pldm_numeric_sensor_value_pdr* /*pdr*/,
    bool8_t /*rearm_event_state*/, struct pldm_platform_pd_sensor_state* state)
{
    auto* s = static_cast<SensorState*>(ctx);
    state->operational_state = s->op_state;
    state->event_enable = s->event_enable;
    state->present_state = s->present_state;
    state->previous_state = s->previous_state;
    state->event_state = s->event_state;
    memcpy(&state->current_reading, s->reading, 4);
    return s->return_cc;
}

/* Helper: build a minimal request message */
static std::vector<uint8_t>
    make_request(uint8_t command, const uint8_t* payload, size_t payload_len)
{
    std::vector<uint8_t> msg(hdrSize + payload_len);
    struct pldm_header_info hdr{};
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = TEST_IID;
    hdr.pldm_type = PLDM_PLATFORM;
    hdr.command = command;
    pack_pldm_header(&hdr, reinterpret_cast<pldm_msg_hdr*>(msg.data()));
    if (payload_len > 0)
    {
        memcpy(msg.data() + hdrSize, payload, payload_len);
    }
    return msg;
}

/* Helper: call pldm_platform_pd_handle_msg and return response_bytes or errno
 */
static std::expected<std::vector<uint8_t>, int>
    dispatch(struct pldm_platform_pd* pd, const std::vector<uint8_t>& req,
             size_t out_capacity = 512)
{
    std::vector<uint8_t> out(out_capacity);
    size_t out_len = out.size();
    int rc = pldm_platform_pd_handle_msg(pd, req.data(), req.size(), out.data(),
                                         &out_len);
    if (rc != 0)
    {
        return std::unexpected(rc);
    }
    out.resize(out_len);
    return out;
}

/* Helper: extract completion code from response payload */
static uint8_t response_cc(const std::vector<uint8_t>& resp)
{
    if (resp.size() <= hdrSize)
    {
        return 0xff;
    }
    return resp[hdrSize];
}

/* ===================================================================
 * Fixture
 * =================================================================== */

class PdTest : public ::testing::Test
{
  protected:
    pldm_pdr* pdr = nullptr;
    struct pldm_platform_pd* pd = nullptr;

    void SetUp() override
    {
        pdr = pldm_pdr_init();
        ASSERT_NE(pdr, nullptr);
        g_sensor = SensorState{};
        pd = pldm_platform_pd_new(pdr, nullptr);
        ASSERT_NE(pd, nullptr);
        ASSERT_EQ(pldm_platform_pd_set_sensor_ops(pd, cb_get_sensor_reading,
                                                  &g_sensor),
                  0);
    }

    void TearDown() override
    {
        free(pd);
        pldm_pdr_destroy(pdr);
    }

    /* Add a dummy PDR record; returns the assigned record handle */
    uint32_t add_dummy_pdr(size_t extra_bytes = 0)
    {
        std::vector<uint8_t> data(sizeof(pldm_pdr_hdr) + extra_bytes, 0);
        uint32_t handle = 0;
        int rc = pldm_pdr_add(pdr, data.data(), data.size(), false, 0, &handle);
        EXPECT_EQ(rc, 0);
        return handle;
    }

    /* Build and register a minimal PLDM_NUMERIC_SENSOR_PDR for sensor_id */
    void add_numeric_sensor_pdr(
        uint16_t sensor_id,
        uint8_t sensor_data_size = PLDM_SENSOR_DATA_SIZE_UINT8)
    {
        uint8_t vbytes;
        if (sensor_data_size == PLDM_SENSOR_DATA_SIZE_UINT16 ||
            sensor_data_size == PLDM_SENSOR_DATA_SIZE_SINT16)
        {
            vbytes = 2;
        }
        else if (sensor_data_size == PLDM_SENSOR_DATA_SIZE_UINT32 ||
                 sensor_data_size == PLDM_SENSOR_DATA_SIZE_SINT32)
        {
            vbytes = 4;
        }
        else
        {
            vbytes = 1;
        }

        /* Length field set to total record size (consistent with existing
         * UINT8 record where length == PLDM_PDR_NUMERIC_SENSOR_PDR_MIN_LENGTH)
         */
        uint16_t rec_len = static_cast<uint16_t>(
            PLDM_PDR_NUMERIC_SENSOR_PDR_MIN_LENGTH + (vbytes - 1) * 3);

        // clang-format off
        std::vector<uint8_t> rec = {
            0x00, 0x00, 0x00, 0x00,                          // record_handle
            0x01,                                            // version
            PLDM_NUMERIC_SENSOR_PDR,                         // type
            0x00, 0x00,                                      // record_change_num
            static_cast<uint8_t>(rec_len),                   // length low
            static_cast<uint8_t>(rec_len >> 8),              // length high
            0x00, 0x00,                                      // terminus_handle
            static_cast<uint8_t>(sensor_id & 0xff),          // sensor_id low byte
            static_cast<uint8_t>(sensor_id >> 8),            // sensor_id high byte
            0x00, 0x00,                                      // entity_type
            0x01, 0x00,                                      // entity_instance_num
            0x00, 0x00,                                      // container_id
            PLDM_NO_INIT,                                    // sensor_init
            0x00,                                            // sensor_auxiliary_names_pdr
            0x00,                                            // base_unit
            0x00,                                            // unit_modifier
            0x00,                                            // rate_unit
            0x00,                                            // base_oem_unit_handle
            0x00,                                            // aux_unit
            0x00,                                            // aux_unit_modifier
            0x00,                                            // aux_rate_unit
            0x00,                                            // rel
            0x00,                                            // aux_oem_unit_handle
            0x01,                                            // is_linear
            sensor_data_size,                                // sensor_data_size
            0x00, 0x00, 0x00, 0x00,                          // resolution
            0x00, 0x00, 0x00, 0x00,                          // offset
            0x00, 0x00,                                      // accuracy
            0x00,                                            // plus_tolerance
            0x00,                                            // minus_tolerance
        };
        for (uint8_t i = 0; i < vbytes; ++i) { rec.push_back(0x00); } // hysteresis
        rec.insert(rec.end(), {
            0x00,                                            // supported_thresholds
            0x00,                                            // threshold_and_hysteresis_volatility
            0x00, 0x00, 0x00, 0x00,                          // state_transition_interval
            0x00, 0x00, 0x00, 0x00,                          // update_interval
        });
        for (uint8_t i = 0; i < vbytes; ++i) { rec.push_back(0xff); } // max_readable
        for (uint8_t i = 0; i < vbytes; ++i) { rec.push_back(0x00); } // min_readable
        rec.insert(rec.end(), {
            PLDM_RANGE_FIELD_FORMAT_UINT8,                   // range_field_format
            0x00,                                            // range_field_support
            0x00, 0x00, 0x00, 0x00, 0x00,                    // nominal/normal range
            0x00, 0x00, 0x00, 0x00,                          // warning range
            0x00, 0x00,                                      // critical range
            0x00, 0x00,                                      // fatal range
        });
        // clang-format on
        uint32_t handle = 0;
        ASSERT_EQ(pldm_pdr_add(pdr, rec.data(), rec.size(), false, 0, &handle),
                  0);
    }
};

/* ===================================================================
 * 1. pldm_platform_pd_setup validation
 * =================================================================== */

TEST(PdSetup, NullArgs)
{
    alignas(PLDM_ALIGNOF_PLDM_PLATFORM_PD)
        uint8_t buf[PLDM_SIZEOF_PLDM_PLATFORM_PD];

    /* NULL pd */
    EXPECT_EQ(pldm_platform_pd_setup(nullptr, PLDM_SIZEOF_PLDM_PLATFORM_PD,
                                     reinterpret_cast<pldm_pdr*>(1), nullptr),
              -EINVAL);

    /* NULL pdr */
    EXPECT_EQ(
        pldm_platform_pd_setup(reinterpret_cast<struct pldm_platform_pd*>(buf),
                               PLDM_SIZEOF_PLDM_PLATFORM_PD, nullptr, nullptr),
        -EINVAL);
}

TEST(PdSetup, SizeTooSmall)
{
    alignas(PLDM_ALIGNOF_PLDM_PLATFORM_PD)
        uint8_t buf[PLDM_SIZEOF_PLDM_PLATFORM_PD];
    pldm_pdr* pdr = pldm_pdr_init();
    ASSERT_NE(pdr, nullptr);

    EXPECT_EQ(
        pldm_platform_pd_setup(reinterpret_cast<struct pldm_platform_pd*>(buf),
                               PLDM_SIZEOF_PLDM_PLATFORM_PD - 1, pdr, nullptr),
        -EINVAL);

    pldm_pdr_destroy(pdr);
}

TEST(PdSetup, OptionalCallbacksMayBeNull)
{
    alignas(PLDM_ALIGNOF_PLDM_PLATFORM_PD)
        uint8_t buf[PLDM_SIZEOF_PLDM_PLATFORM_PD];
    pldm_pdr* pdr = pldm_pdr_init();
    ASSERT_NE(pdr, nullptr);

    int rc =
        pldm_platform_pd_setup(reinterpret_cast<struct pldm_platform_pd*>(buf),
                               PLDM_SIZEOF_PLDM_PLATFORM_PD, pdr, nullptr);
    EXPECT_EQ(rc, 0);

    pldm_pdr_destroy(pdr);
}

/* ===================================================================
 * 2. GET_PDR_REPOSITORY_INFO
 * =================================================================== */

TEST_F(PdTest, GetPdrRepositoryInfoEmptyRepo)
{
    auto req = make_request(PLDM_GET_PDR_REPOSITORY_INFO, nullptr, 0);
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    ASSERT_GE(resp->size(), hdrSize + PLDM_GET_PDR_REPOSITORY_INFO_RESP_BYTES);
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);

    /* record count should be 0 */
    auto* r = reinterpret_cast<const pldm_pdr_repository_info_resp*>(
        resp->data() + hdrSize);
    EXPECT_EQ(le32toh(r->record_count), 0u);
    EXPECT_EQ(le32toh(r->repository_size), 0u);
    EXPECT_EQ(le32toh(r->largest_record_size), 0u);
}

TEST_F(PdTest, GetPdrRepositoryInfoOneRecord)
{
    add_dummy_pdr(4);

    auto req = make_request(PLDM_GET_PDR_REPOSITORY_INFO, nullptr, 0);
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    ASSERT_GE(resp->size(), hdrSize + PLDM_GET_PDR_REPOSITORY_INFO_RESP_BYTES);

    auto* r = reinterpret_cast<const pldm_pdr_repository_info_resp*>(
        resp->data() + hdrSize);
    EXPECT_EQ(le32toh(r->record_count), 1u);
    EXPECT_EQ(le32toh(r->largest_record_size), sizeof(pldm_pdr_hdr) + 4);
}

TEST_F(PdTest, GetPdrRepositoryInfoMultipleRecords)
{
    add_dummy_pdr(2);
    add_dummy_pdr(6);
    add_dummy_pdr(2);

    auto req = make_request(PLDM_GET_PDR_REPOSITORY_INFO, nullptr, 0);
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    auto* r = reinterpret_cast<const pldm_pdr_repository_info_resp*>(
        resp->data() + hdrSize);
    EXPECT_EQ(le32toh(r->record_count), 3u);
    EXPECT_EQ(le32toh(r->largest_record_size), sizeof(pldm_pdr_hdr) + 6);
}

TEST_F(PdTest, GetPdrRepositoryInfoRejectsPayload)
{
    uint8_t extra = 0x00;
    auto req = make_request(PLDM_GET_PDR_REPOSITORY_INFO, &extra, 1);
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_ERROR_INVALID_LENGTH);
}

/* ===================================================================
 * 3. GET_PDR
 * =================================================================== */

TEST_F(PdTest, GetPdrInvalidHandle)
{
    struct pldm_get_pdr_req pdr_req{};
    pdr_req.record_handle = htole32(9999u);
    pdr_req.data_transfer_handle = 0;
    pdr_req.transfer_op_flag = PLDM_GET_FIRSTPART;
    pdr_req.request_count = htole16(512);
    pdr_req.record_change_number = 0;

    auto req =
        make_request(PLDM_GET_PDR, reinterpret_cast<const uint8_t*>(&pdr_req),
                     sizeof(pdr_req));
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_PLATFORM_INVALID_RECORD_HANDLE);
}

TEST_F(PdTest, GetPdrSingleRecordRoundtrip)
{
    uint32_t handle = add_dummy_pdr(4);

    /* pldm_pdr_add writes the record handle into the first four bytes of the
     * stored PDR. Retrieve the actual stored bytes. */
    uint8_t* stored_data = nullptr;
    uint32_t stored_size = 0, next_h = 0;
    ASSERT_TRUE(
        pldm_pdr_find_record(pdr, handle, &stored_data, &stored_size, &next_h));

    struct pldm_get_pdr_req pdr_req{};
    pdr_req.record_handle = htole32(handle);
    pdr_req.data_transfer_handle = 0;
    pdr_req.transfer_op_flag = PLDM_GET_FIRSTPART;
    pdr_req.request_count = htole16(512);
    pdr_req.record_change_number = 0;

    auto req =
        make_request(PLDM_GET_PDR, reinterpret_cast<const uint8_t*>(&pdr_req),
                     sizeof(pdr_req));
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    ASSERT_GE(resp->size(),
              hdrSize + PLDM_GET_PDR_MIN_RESP_BYTES + stored_size);
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);

    auto* r =
        reinterpret_cast<const pldm_get_pdr_resp*>(resp->data() + hdrSize);
    EXPECT_EQ(r->transfer_flag, PLDM_PLATFORM_TRANSFER_START_AND_END);
    uint16_t resp_count = le16toh(r->response_count);
    EXPECT_EQ(resp_count, stored_size);
    EXPECT_EQ(memcmp(r->record_data, stored_data, stored_size), 0);
}

TEST_F(PdTest, GetPdrTruncatedByRequestCount)
{
    uint32_t handle = add_dummy_pdr(16);

    struct pldm_get_pdr_req pdr_req{};
    pdr_req.record_handle = htole32(handle);
    pdr_req.data_transfer_handle = 0;
    pdr_req.transfer_op_flag = PLDM_GET_FIRSTPART;
    pdr_req.request_count = htole16(4);
    pdr_req.record_change_number = 0;

    auto req =
        make_request(PLDM_GET_PDR, reinterpret_cast<const uint8_t*>(&pdr_req),
                     sizeof(pdr_req));
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);
    auto* r =
        reinterpret_cast<const pldm_get_pdr_resp*>(resp->data() + hdrSize);
    EXPECT_EQ(le16toh(r->response_count), 4u);
}

TEST_F(PdTest, GetPdrInvalidTransferOpFlag)
{
    uint32_t handle = add_dummy_pdr();

    struct pldm_get_pdr_req pdr_req{};
    pdr_req.record_handle = htole32(handle);
    pdr_req.data_transfer_handle = 0;
    pdr_req.transfer_op_flag = PLDM_GET_NEXTPART; /* not FIRSTPART */
    pdr_req.request_count = htole16(512);
    pdr_req.record_change_number = 0;

    auto req =
        make_request(PLDM_GET_PDR, reinterpret_cast<const uint8_t*>(&pdr_req),
                     sizeof(pdr_req));
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp),
              PLDM_PLATFORM_INVALID_TRANSFER_OPERATION_FLAG);
}

/* ===================================================================
 * 4. GET_SENSOR_READING
 * =================================================================== */

TEST_F(PdTest, GetSensorReadingSuccess)
{
    add_numeric_sensor_pdr(1);
    g_sensor.reading[0] = 99;

    struct pldm_get_sensor_reading_req sreq{};
    sreq.sensor_id = htole16(1);
    sreq.rearm_event_state = 0;

    auto req =
        make_request(PLDM_GET_SENSOR_READING,
                     reinterpret_cast<const uint8_t*>(&sreq), sizeof(sreq));
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);
    ASSERT_GE(resp->size(), hdrSize + PLDM_GET_SENSOR_READING_MIN_RESP_BYTES);

    auto* r = reinterpret_cast<const pldm_get_sensor_reading_resp*>(
        resp->data() + hdrSize);
    EXPECT_EQ(r->sensor_data_size, PLDM_SENSOR_DATA_SIZE_UINT8);
    EXPECT_EQ(r->present_reading[0], 99);
}

TEST_F(PdTest, GetSensorReadingInvalidSensorId)
{
    /* No PDR registered for sensor_id 999 → responder returns INVALID_SENSOR_ID
     */
    struct pldm_get_sensor_reading_req sreq{};
    sreq.sensor_id = htole16(999);
    sreq.rearm_event_state = 0;

    auto req =
        make_request(PLDM_GET_SENSOR_READING,
                     reinterpret_cast<const uint8_t*>(&sreq), sizeof(sreq));
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_PLATFORM_INVALID_SENSOR_ID);
}

TEST_F(PdTest, GetSensorReadingShortPayload)
{
    /* payload smaller than required */
    auto req = make_request(PLDM_GET_SENSOR_READING, nullptr, 0);
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_ERROR_INVALID_LENGTH);
}

TEST_F(PdTest, GetSensorReadingUint32)
{
    add_numeric_sensor_pdr(2, PLDM_SENSOR_DATA_SIZE_UINT32);
    g_sensor.reading[0] = 1;
    g_sensor.reading[1] = 2;
    g_sensor.reading[2] = 3;
    g_sensor.reading[3] = 4;

    struct pldm_get_sensor_reading_req sreq{};
    sreq.sensor_id = htole16(2);
    sreq.rearm_event_state = 0;

    auto req =
        make_request(PLDM_GET_SENSOR_READING,
                     reinterpret_cast<const uint8_t*>(&sreq), sizeof(sreq));
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);
    /* UINT32 response is min+3 bytes */
    EXPECT_EQ(resp->size(),
              hdrSize + PLDM_GET_SENSOR_READING_MIN_RESP_BYTES + 3);

    auto* r = reinterpret_cast<const pldm_get_sensor_reading_resp*>(
        resp->data() + hdrSize);
    EXPECT_EQ(r->present_reading[0], 1);
    EXPECT_EQ(r->present_reading[1], 2);
    EXPECT_EQ(r->present_reading[2], 3);
    EXPECT_EQ(r->present_reading[3], 4);
}

TEST_F(PdTest, GetSensorReadingSint8)
{
    add_numeric_sensor_pdr(1, PLDM_SENSOR_DATA_SIZE_SINT8);
    g_sensor.reading[0] = 0xfe; /* -2 as int8 */

    struct pldm_get_sensor_reading_req sreq{};
    sreq.sensor_id = htole16(1);
    sreq.rearm_event_state = 0;

    auto req =
        make_request(PLDM_GET_SENSOR_READING,
                     reinterpret_cast<const uint8_t*>(&sreq), sizeof(sreq));
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);
    EXPECT_EQ(resp->size(), hdrSize + PLDM_GET_SENSOR_READING_MIN_RESP_BYTES);
    auto* r = reinterpret_cast<const pldm_get_sensor_reading_resp*>(
        resp->data() + hdrSize);
    EXPECT_EQ(r->sensor_data_size, PLDM_SENSOR_DATA_SIZE_SINT8);
    EXPECT_EQ(r->present_reading[0], 0xfe);
}

TEST_F(PdTest, GetSensorReadingUint16)
{
    add_numeric_sensor_pdr(1, PLDM_SENSOR_DATA_SIZE_UINT16);
    g_sensor.reading[0] = 0x34;
    g_sensor.reading[1] = 0x12;

    struct pldm_get_sensor_reading_req sreq{};
    sreq.sensor_id = htole16(1);
    sreq.rearm_event_state = 0;

    auto req =
        make_request(PLDM_GET_SENSOR_READING,
                     reinterpret_cast<const uint8_t*>(&sreq), sizeof(sreq));
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);
    EXPECT_EQ(resp->size(),
              hdrSize + PLDM_GET_SENSOR_READING_MIN_RESP_BYTES + 1);
    auto* r = reinterpret_cast<const pldm_get_sensor_reading_resp*>(
        resp->data() + hdrSize);
    EXPECT_EQ(r->sensor_data_size, PLDM_SENSOR_DATA_SIZE_UINT16);
    EXPECT_EQ(r->present_reading[0], 0x34);
    EXPECT_EQ(r->present_reading[1], 0x12);
}

TEST_F(PdTest, GetSensorReadingSint32)
{
    add_numeric_sensor_pdr(1, PLDM_SENSOR_DATA_SIZE_SINT32);
    g_sensor.reading[0] = 0xff;
    g_sensor.reading[1] = 0xff;
    g_sensor.reading[2] = 0xff;
    g_sensor.reading[3] = 0xff; /* -1 as int32 LE */

    struct pldm_get_sensor_reading_req sreq{};
    sreq.sensor_id = htole16(1);
    sreq.rearm_event_state = 0;

    auto req =
        make_request(PLDM_GET_SENSOR_READING,
                     reinterpret_cast<const uint8_t*>(&sreq), sizeof(sreq));
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);
    EXPECT_EQ(resp->size(),
              hdrSize + PLDM_GET_SENSOR_READING_MIN_RESP_BYTES + 3);
    auto* r = reinterpret_cast<const pldm_get_sensor_reading_resp*>(
        resp->data() + hdrSize);
    EXPECT_EQ(r->sensor_data_size, PLDM_SENSOR_DATA_SIZE_SINT32);
}

/* ===================================================================
 * 5. pldm_platform_pd_set_sensor_ops validation
 * =================================================================== */

TEST(PdSetSensorOps, NullPd)
{
    EXPECT_EQ(pldm_platform_pd_set_sensor_ops(nullptr, cb_get_sensor_reading,
                                              nullptr),
              -EINVAL);
}

TEST(PdSetSensorOps, NullOps)
{
    alignas(PLDM_ALIGNOF_PLDM_PLATFORM_PD)
        uint8_t buf[PLDM_SIZEOF_PLDM_PLATFORM_PD];
    pldm_pdr* pdr = pldm_pdr_init();
    ASSERT_NE(pdr, nullptr);
    ASSERT_EQ(
        pldm_platform_pd_setup(reinterpret_cast<struct pldm_platform_pd*>(buf),
                               PLDM_SIZEOF_PLDM_PLATFORM_PD, pdr, nullptr),
        0);
    EXPECT_EQ(
        pldm_platform_pd_set_sensor_ops(
            reinterpret_cast<struct pldm_platform_pd*>(buf), nullptr, nullptr),
        -EINVAL);
    pldm_pdr_destroy(pdr);
}

TEST(PdSensorOps, GetSensorReadingNoOpsReturnsUnsupportedCmd)
{
    pldm_pdr* pdr = pldm_pdr_init();
    ASSERT_NE(pdr, nullptr);

    struct pldm_platform_pd* pd = pldm_platform_pd_new(pdr, nullptr);
    ASSERT_NE(pd, nullptr);
    /* deliberately do NOT call pldm_platform_pd_set_sensor_ops */

    struct pldm_get_sensor_reading_req sreq{};
    sreq.sensor_id = htole16(1);
    sreq.rearm_event_state = 0;

    auto req =
        make_request(PLDM_GET_SENSOR_READING,
                     reinterpret_cast<const uint8_t*>(&sreq), sizeof(sreq));
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_ERROR_UNSUPPORTED_PLDM_CMD);

    free(pd);
    pldm_pdr_destroy(pdr);
}

/* ===================================================================
 * 9. Dispatch edge cases
 * =================================================================== */

TEST_F(PdTest, UnknownCommand)
{
    auto req = make_request(0xfe, nullptr, 0);
    auto resp = dispatch(pd, req);

    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_ERROR_UNSUPPORTED_PLDM_CMD);
}

TEST_F(PdTest, WrongPldmType)
{
    std::vector<uint8_t> msg(hdrSize);
    struct pldm_header_info hdr{};
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = TEST_IID;
    hdr.pldm_type = PLDM_BASE; /* wrong type */
    hdr.command = PLDM_GET_PDR;
    pack_pldm_header(&hdr, reinterpret_cast<pldm_msg_hdr*>(msg.data()));

    std::vector<uint8_t> out(512);
    size_t out_len = out.size();
    int rc = pldm_platform_pd_handle_msg(pd, msg.data(), msg.size(), out.data(),
                                         &out_len);
    EXPECT_EQ(rc, -ENOMSG);
}

TEST_F(PdTest, ResponseMessageRejected)
{
    std::vector<uint8_t> msg(hdrSize);
    struct pldm_header_info hdr{};
    hdr.msg_type = PLDM_RESPONSE;
    hdr.instance = TEST_IID;
    hdr.pldm_type = PLDM_PLATFORM;
    hdr.command = PLDM_GET_PDR;
    pack_pldm_header(&hdr, reinterpret_cast<pldm_msg_hdr*>(msg.data()));

    std::vector<uint8_t> out(512);
    size_t out_len = out.size();
    int rc = pldm_platform_pd_handle_msg(pd, msg.data(), msg.size(), out.data(),
                                         &out_len);
    EXPECT_EQ(rc, -EPROTO);
}

TEST_F(PdTest, NullPointers)
{
    std::vector<uint8_t> buf(512);
    size_t out_len = buf.size();

    EXPECT_EQ(pldm_platform_pd_handle_msg(nullptr, buf.data(), 1, buf.data(),
                                          &out_len),
              -EINVAL);
    EXPECT_EQ(pldm_platform_pd_handle_msg(pd, nullptr, 1, buf.data(), &out_len),
              -EINVAL);
    EXPECT_EQ(pldm_platform_pd_handle_msg(pd, buf.data(), 1, nullptr, &out_len),
              -EINVAL);
    EXPECT_EQ(
        pldm_platform_pd_handle_msg(pd, buf.data(), 1, buf.data(), nullptr),
        -EINVAL);
}

TEST_F(PdTest, ShortInput)
{
    std::vector<uint8_t> buf(512);
    size_t out_len = buf.size();
    /* one byte short of a complete header */
    EXPECT_EQ(pldm_platform_pd_handle_msg(pd, buf.data(), hdrSize - 1,
                                          buf.data(), &out_len),
              -EOVERFLOW);
}

TEST_F(PdTest, TinyOutputBuffer)
{
    auto req = make_request(PLDM_GET_PDR_REPOSITORY_INFO, nullptr, 0);
    std::vector<uint8_t> out(hdrSize); /* room for header but no cc byte */
    size_t out_len = out.size();
    EXPECT_EQ(pldm_platform_pd_handle_msg(pd, req.data(), req.size(),
                                          out.data(), &out_len),
              -EOVERFLOW);
}

TEST_F(PdTest, AsyncNotifyRejected)
{
    std::vector<uint8_t> msg(hdrSize);
    struct pldm_header_info hdr{};
    hdr.msg_type = PLDM_ASYNC_REQUEST_NOTIFY;
    hdr.instance = TEST_IID;
    hdr.pldm_type = PLDM_PLATFORM;
    hdr.command = PLDM_GET_PDR;
    pack_pldm_header(&hdr, reinterpret_cast<pldm_msg_hdr*>(msg.data()));

    std::vector<uint8_t> out(512);
    size_t out_len = out.size();
    int rc = pldm_platform_pd_handle_msg(pd, msg.data(), msg.size(), out.data(),
                                         &out_len);
    EXPECT_EQ(rc, -EPROTO);
}

/* ===================================================================
 * 10. pldm_control integration
 * =================================================================== */

TEST(PdControl, RegistersPlatformType)
{
    alignas(8) uint8_t ctrl_buf[PLDM_SIZEOF_PLDM_CONTROL];
    auto* control = reinterpret_cast<struct pldm_control*>(ctrl_buf);
    ASSERT_EQ(pldm_control_setup(control, PLDM_SIZEOF_PLDM_CONTROL), 0);

    pldm_pdr* pdr = pldm_pdr_init();
    ASSERT_NE(pdr, nullptr);

    struct pldm_platform_pd* pd = pldm_platform_pd_new(pdr, control);
    ASSERT_NE(pd, nullptr);
    ASSERT_EQ(
        pldm_platform_pd_set_sensor_ops(pd, cb_get_sensor_reading, &g_sensor),
        0);

    /* Build a GET_PLDM_TYPES request and dispatch through pldm_control */
    std::vector<uint8_t> req(hdrSize);
    struct pldm_header_info hdr{};
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = 0;
    hdr.pldm_type = PLDM_BASE;
    hdr.command = PLDM_GET_PLDM_TYPES;
    pack_pldm_header(&hdr, reinterpret_cast<pldm_msg_hdr*>(req.data()));

    std::vector<uint8_t> resp(512);
    size_t resp_len = resp.size();
    int rc = pldm_control_handle_msg(control, req.data(), req.size(),
                                     resp.data(), &resp_len);
    EXPECT_EQ(rc, 0);
    EXPECT_GT(resp_len, hdrSize + 1u);

    uint8_t cc = resp[hdrSize];
    EXPECT_EQ(cc, PLDM_SUCCESS);

    /* The types bitfield follows the completion code.
     * PLDM_PLATFORM == 0x02 → byte 0 bit 2 */
    uint8_t types_byte0 = resp[hdrSize + 1];
    EXPECT_NE(types_byte0 & (1u << PLDM_PLATFORM), 0u);

    free(pd);
    pldm_pdr_destroy(pdr);
}
