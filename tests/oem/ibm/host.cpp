#include <endian.h>
#include <libpldm/base.h>
#include <libpldm/oem/ibm/host.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <new>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

constexpr auto hdrSize = sizeof(pldm_msg_hdr);

TEST(GetAlertStatus, testGoodEncodeRequest)
{
    PLDM_MSG_BUFFER(requestMsg, PLDM_GET_ALERT_STATUS_REQ_BYTES);
    auto* request = new (requestMsg) pldm_msg;

    uint8_t versionId = 0x0;

    auto rc = encode_get_alert_status_req(0, versionId, request,
                                          PLDM_GET_ALERT_STATUS_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(versionId, request->payload[0]);
}

TEST(GetAlertStatus, testBadEncodeRequest)
{
    PLDM_MSG_BUFFER(requestMsg, PLDM_GET_ALERT_STATUS_REQ_BYTES);
    auto* request = new (requestMsg) pldm_msg;
    auto rc = encode_get_alert_status_req(0, 0x0, request,
                                          PLDM_GET_ALERT_STATUS_REQ_BYTES + 1);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetAlertStatus, testGoodDecodeResponse)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint32_t rack_entry = 0xff000030;
    uint32_t pri_cec_node = 0x00008030;

    PLDM_MSG_BUFFER(responseMsg, PLDM_GET_ALERT_STATUS_RESP_BYTES);
    auto* response = new (responseMsg) pldm_msg;
    auto* resp = new (response->payload) pldm_get_alert_status_resp;
    resp->completion_code = completionCode;
    resp->rack_entry = htole32(rack_entry);
    resp->pri_cec_node = htole32(pri_cec_node);

    uint8_t retCompletionCode = 0;
    uint32_t retRack_entry = 0;
    uint32_t retPri_cec_node = 0;

    auto rc = decode_get_alert_status_resp(
        response, sizeof(responseMsg) - hdrSize, &retCompletionCode,
        &retRack_entry, &retPri_cec_node);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retCompletionCode, completionCode);
    EXPECT_EQ(retRack_entry, rack_entry);
    EXPECT_EQ(retPri_cec_node, pri_cec_node);
}

TEST(GetAlertStatus, testBadDecodeResponse)
{
    auto rc = decode_get_alert_status_resp(NULL, 0, NULL, NULL, NULL);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    uint8_t completionCode = PLDM_SUCCESS;
    uint32_t rack_entry = 0xff000030;
    uint32_t pri_cec_node = 0x00008030;

    PLDM_MSG_BUFFER(responseMsg, PLDM_GET_ALERT_STATUS_RESP_BYTES);
    auto* response = new (responseMsg) pldm_msg;
    auto* resp = new (response->payload) pldm_get_alert_status_resp;
    resp->completion_code = completionCode;
    resp->rack_entry = htole32(rack_entry);
    resp->pri_cec_node = htole32(pri_cec_node);

    uint8_t retCompletionCode = 0;
    uint32_t retRack_entry = 0;
    uint32_t retPri_cec_node = 0;

    rc = decode_get_alert_status_resp(
        response, sizeof(responseMsg) - hdrSize + 1, &retCompletionCode,
        &retRack_entry, &retPri_cec_node);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetAlertStatus, testGoodEncodeResponse)
{
    uint32_t rack_entry = 0xff000030;
    uint32_t pri_cec_node = 0x00008030;

    PLDM_MSG_BUFFER(responseMsg, PLDM_GET_ALERT_STATUS_RESP_BYTES);
    auto* response = new (responseMsg) pldm_msg;

    auto rc = encode_get_alert_status_resp(
        0, PLDM_SUCCESS, rack_entry, pri_cec_node, response,
        sizeof(responseMsg) - sizeof(pldm_msg_hdr));

    ASSERT_EQ(rc, PLDM_SUCCESS);
    EXPECT_THAT(responseMsg, testing::ElementsAreArray(
                                 {0x00, 0x3f, 0xf0, 0x00, 0x30, 0x00, 0x00,
                                  0xff, 0x30, 0x80, 0x00, 0x00}));
}

TEST(GetAlertStatus, testBadEncodeResponse)
{
    uint32_t rack_entry = 0xff000030;
    uint32_t pri_cec_node = 0x00008030;

    PLDM_MSG_BUFFER(responseMsg, PLDM_GET_ALERT_STATUS_RESP_BYTES);
    auto* response = new (responseMsg) pldm_msg;

    auto rc = encode_get_alert_status_resp(0, PLDM_SUCCESS, rack_entry,
                                           pri_cec_node, response,
                                           sizeof(responseMsg) - hdrSize + 1);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetAlertStatus, testGoodDecodeRequest)
{
    uint8_t versionId = 0x0;
    uint8_t retVersionId;

    PLDM_MSG_BUFFER(requestMsg, PLDM_GET_ALERT_STATUS_REQ_BYTES);
    auto* req = new (requestMsg) pldm_msg;

    req->payload[0] = versionId;

    auto rc = decode_get_alert_status_req(
        req, sizeof(requestMsg) - sizeof(pldm_msg_hdr), &retVersionId);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retVersionId, versionId);
}

TEST(GetAlertStatus, testBadDecodeRequest)
{
    uint8_t versionId = 0x0;
    uint8_t retVersionId;

    PLDM_MSG_BUFFER(requestMsg, PLDM_GET_ALERT_STATUS_REQ_BYTES);
    auto* req = new (requestMsg) pldm_msg;

    req->payload[0] = versionId;

    auto rc = decode_get_alert_status_req(
        req, sizeof(requestMsg) - sizeof(pldm_msg_hdr) + 1, &retVersionId);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}
