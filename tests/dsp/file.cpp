#include "msgbuf.hpp"

#include <libpldm/base.h>
#include <libpldm/file.h>
#include <libpldm/pldm_types.h>

#include <array>
#include <cstdint>
#include <cstring>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfOpenReq, GoodTest)
{
    uint8_t instance_id = 0;
    uint16_t file_identifier = 0x0100;
    bitfield16_t file_attribute;
    file_attribute.value = 0x0400;

    static constexpr const size_t requestMsgLength = PLDM_DF_OPEN_REQ_BYTES;
    std::array<uint8_t, requestMsgLength> requestMsg = {0x00, 0x01, 0x00, 0x04};

    const struct pldm_file_df_open_req req_data = {file_identifier,
                                                   file_attribute};

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = requestMsgLength;
    auto rc = encode_pldm_file_df_open_req(instance_id, &req_data, requestPtr,
                                           &payload_length);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(
        0, memcmp(requestPtr->payload, requestMsg.data(), sizeof(requestMsg)));
    EXPECT_EQ(payload_length, requestMsgLength);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfOpenReq, BadTestUnAllocatedPtrParams)
{
    uint8_t instance_id = 0;
    uint16_t file_identifier = 0x0100;
    bitfield16_t file_attribute;
    file_attribute.value = 0x0400;
    int rc;

    static constexpr const size_t requestMsgLength = PLDM_DF_OPEN_REQ_BYTES;

    const struct pldm_file_df_open_req req_data = {file_identifier,
                                                   file_attribute};

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = requestMsgLength;
    rc = encode_pldm_file_df_open_req(instance_id, &req_data, nullptr,
                                      &payload_length);
    EXPECT_EQ(rc, -EINVAL);

    rc = encode_pldm_file_df_open_req(instance_id, nullptr, requestPtr,
                                      &payload_length);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfOpenReq, BadTestInvalidExpectedOutputMsgLength)
{
    uint8_t instance_id = 0;
    uint16_t file_identifier = 0x0100;
    bitfield16_t file_attribute;
    file_attribute.value = 0x0400;
    int rc;

    static constexpr const size_t requestMsgLength = PLDM_DF_OPEN_REQ_BYTES;

    const struct pldm_file_df_open_req req_data = {file_identifier,
                                                   file_attribute};

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = 1;
    rc = encode_pldm_file_df_open_req(instance_id, &req_data, requestPtr,
                                      &payload_length);
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfOpenResp, GoodTest)
{
    uint8_t completion_code = PLDM_SUCCESS;
    uint16_t file_descriptor = 20;

    struct pldm_file_df_open_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_OPEN_RESP_BYTES;

    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completion_code);
    pldm_msgbuf_insert_uint16(buf, file_descriptor);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_file_df_open_resp(responseMsg, payload_length, &resp_data);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(resp_data.completion_code, completion_code);
    EXPECT_EQ(resp_data.file_descriptor, file_descriptor);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfOpenResp, BadTestUnAllocatedPtrParams)
{
    uint8_t completion_code = PLDM_SUCCESS;
    uint16_t file_descriptor = 20;

    struct pldm_file_df_open_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_OPEN_RESP_BYTES;

    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completion_code);
    pldm_msgbuf_insert_uint16(buf, file_descriptor);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_file_df_open_resp(nullptr, payload_length, &resp_data);
    EXPECT_EQ(rc, -EINVAL);

    rc = decode_pldm_file_df_open_resp(responseMsg, payload_length, nullptr);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfOpenResp, BadTestInvalidExpectedInputMsgLength)
{
    uint8_t completion_code = PLDM_SUCCESS;
    uint16_t file_descriptor = 20;

    struct pldm_file_df_open_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_OPEN_RESP_BYTES;

    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completion_code);
    pldm_msgbuf_insert_uint16(buf, file_descriptor);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_file_df_open_resp(responseMsg, 0, &resp_data);
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfCloseReq, GoodTest)
{
    uint8_t instance_id = 0;
    uint16_t file_descriptor = 0x0200;
    bitfield16_t df_close_options;
    df_close_options.value = 0x0100;

    static constexpr const size_t requestMsgLength = PLDM_DF_OPEN_REQ_BYTES;
    std::array<uint8_t, requestMsgLength> requestMsg = {0x00, 0x02, 0x00, 0x01};

    const struct pldm_file_df_close_req req_data = {file_descriptor,
                                                    df_close_options};

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = requestMsgLength;
    auto rc = encode_pldm_file_df_close_req(instance_id, &req_data, requestPtr,
                                            &payload_length);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(
        0, memcmp(requestPtr->payload, requestMsg.data(), sizeof(requestMsg)));
    EXPECT_EQ(payload_length, requestMsgLength);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfCloseReq, BadTestUnAllocatedPtrParams)
{
    uint8_t instance_id = 0;
    uint16_t file_descriptor = 0x0200;
    bitfield16_t df_close_options;
    df_close_options.value = 0x0100;
    int rc;

    static constexpr const size_t requestMsgLength = PLDM_DF_OPEN_REQ_BYTES;

    const struct pldm_file_df_close_req req_data = {file_descriptor,
                                                    df_close_options};

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = requestMsgLength;
    rc = encode_pldm_file_df_close_req(instance_id, &req_data, nullptr,
                                       &payload_length);
    EXPECT_EQ(rc, -EINVAL);

    rc = encode_pldm_file_df_close_req(instance_id, nullptr, requestPtr,
                                       &payload_length);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfCloseReq, BadTestInvalidExpectedOutputMsgLength)
{
    uint8_t instance_id = 0;
    uint16_t file_descriptor = 0x0200;
    bitfield16_t df_close_options;
    df_close_options.value = 0x0100;
    int rc;

    static constexpr const size_t requestMsgLength = PLDM_DF_OPEN_REQ_BYTES;

    const struct pldm_file_df_close_req req_data = {file_descriptor,
                                                    df_close_options};

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = 1;
    rc = encode_pldm_file_df_close_req(instance_id, &req_data, requestPtr,
                                       &payload_length);
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfCloseResp, GoodTest)
{
    uint8_t completion_code = PLDM_SUCCESS;

    struct pldm_file_df_close_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_CLOSE_RESP_BYTES;

    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completion_code);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc =
        decode_pldm_file_df_close_resp(responseMsg, payload_length, &resp_data);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(resp_data.completion_code, completion_code);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfCloseResp, BadTestUnAllocatedPtrParams)
{
    uint8_t completion_code = PLDM_SUCCESS;

    struct pldm_file_df_close_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_CLOSE_RESP_BYTES;

    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completion_code);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_file_df_close_resp(nullptr, payload_length, &resp_data);
    EXPECT_EQ(rc, -EINVAL);

    rc = decode_pldm_file_df_close_resp(responseMsg, payload_length, nullptr);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfHeartbeatReq, GoodTest)
{
    uint8_t instance_id = 0;
    uint16_t file_descriptor = 0x0200;
    uint32_t requester_max_interval = 0x88130000; // 5000 ms

    static constexpr const size_t requestMsgLength =
        PLDM_DF_HEARTBEAT_REQ_BYTES;
    std::array<uint8_t, requestMsgLength> requestMsg = {0x00, 0x02, 0x00,
                                                        0x00, 0x13, 0x88};

    const struct pldm_file_df_heartbeat_req req_data = {file_descriptor,
                                                        requester_max_interval};

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = requestMsgLength;
    auto rc = encode_pldm_file_df_heartbeat_req(instance_id, &req_data,
                                                requestPtr, &payload_length);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(
        0, memcmp(requestPtr->payload, requestMsg.data(), sizeof(requestMsg)));
    EXPECT_EQ(payload_length, requestMsgLength);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfHeartbeatReq, BadTestUnAllocatedPtrParams)
{
    uint8_t instance_id = 0;
    uint16_t file_descriptor = 0x0200;
    uint32_t requester_max_interval = 0x88130000; // 5000 ms
    int rc;

    static constexpr const size_t requestMsgLength =
        PLDM_DF_HEARTBEAT_REQ_BYTES;

    const struct pldm_file_df_heartbeat_req req_data = {file_descriptor,
                                                        requester_max_interval};

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = requestMsgLength;
    rc = encode_pldm_file_df_heartbeat_req(instance_id, &req_data, nullptr,
                                           &payload_length);
    EXPECT_EQ(rc, -EINVAL);

    rc = encode_pldm_file_df_heartbeat_req(instance_id, nullptr, requestPtr,
                                           &payload_length);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfHeartbeatReq, BadTestInvalidExpectedOutputMsgLength)
{
    uint8_t instance_id = 0;
    uint16_t file_descriptor = 0x0200;
    uint32_t requester_max_interval = 0x88130000; // 5000 ms
    int rc;

    static constexpr const size_t requestMsgLength =
        PLDM_DF_HEARTBEAT_REQ_BYTES;

    const struct pldm_file_df_heartbeat_req req_data = {file_descriptor,
                                                        requester_max_interval};

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = 1;
    rc = encode_pldm_file_df_heartbeat_req(instance_id, &req_data, requestPtr,
                                           &payload_length);
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfHeartbeatResp, GoodTest)
{
    uint8_t completion_code = PLDM_SUCCESS;
    uint32_t responder_max_interval = 0xa00f0000; // 4000 ms

    struct pldm_file_df_heartbeat_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_HEARTBEAT_RESP_BYTES;

    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completion_code);
    pldm_msgbuf_insert_uint32(buf, responder_max_interval);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_file_df_heartbeat_resp(responseMsg, payload_length,
                                            &resp_data);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(resp_data.completion_code, completion_code);
    EXPECT_EQ(resp_data.responder_max_interval, responder_max_interval);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfHeartbeatResp, BadTestUnAllocatedPtrParams)
{
    uint8_t completion_code = PLDM_SUCCESS;
    uint32_t responder_max_interval = 0xa00f0000; // 4000 ms

    struct pldm_file_df_heartbeat_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_HEARTBEAT_RESP_BYTES;

    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completion_code);
    pldm_msgbuf_insert_uint32(buf, responder_max_interval);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc =
        decode_pldm_file_df_heartbeat_resp(nullptr, payload_length, &resp_data);
    EXPECT_EQ(rc, -EINVAL);

    rc = decode_pldm_file_df_heartbeat_resp(responseMsg, payload_length,
                                            nullptr);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfHeartbeatResp, BadTestInvalidExpectedInputMsgLength)
{
    uint8_t completion_code = PLDM_SUCCESS;
    uint32_t responder_max_interval = 0xa00f0000; // 4000 ms

    struct pldm_file_df_heartbeat_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_HEARTBEAT_RESP_BYTES;

    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completion_code);
    pldm_msgbuf_insert_uint32(buf, responder_max_interval);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_file_df_heartbeat_resp(responseMsg, 0, &resp_data);
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfOpenResp, GoodTest)
{
    uint8_t instance_id = 0;
    uint8_t completion_code = PLDM_SUCCESS;
    uint16_t file_descriptor = 0x0100;
    size_t payload_length = PLDM_DF_OPEN_RESP_BYTES;
    std::array<uint8_t, PLDM_DF_OPEN_RESP_BYTES> responseMsg = {0x00, 0x00,
                                                                0x01};

    const struct pldm_file_df_open_resp resp_data = {completion_code,
                                                     file_descriptor};

    PLDM_MSG_DEFINE_P(responsePtr, PLDM_DF_OPEN_RESP_BYTES);
    auto rc = encode_pldm_file_df_open_resp(instance_id, &resp_data,
                                            responsePtr, &payload_length);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(0, memcmp(responsePtr->payload, responseMsg.data(),
                        sizeof(responseMsg)));
    EXPECT_EQ(payload_length, PLDM_DF_OPEN_RESP_BYTES);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfOpenResp, BadTestUnAllocatedPtrParams)
{
    uint8_t instance_id = 0;
    uint8_t completion_code = PLDM_SUCCESS;
    uint16_t file_descriptor = 0x0100;
    int rc;
    size_t payload_length = PLDM_DF_OPEN_RESP_BYTES;
    const struct pldm_file_df_open_resp resp_data = {completion_code,
                                                     file_descriptor};

    PLDM_MSG_DEFINE_P(responsePtr, PLDM_DF_OPEN_RESP_BYTES);
    rc = encode_pldm_file_df_open_resp(instance_id, &resp_data, nullptr,
                                       &payload_length);
    EXPECT_EQ(rc, -EINVAL);

    rc = encode_pldm_file_df_open_req(instance_id, nullptr, responsePtr,
                                      &payload_length);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfOpenResp, BadTestInvalidExpectedOutputMsgLength)
{
    uint8_t instance_id = 0;
    uint8_t completion_code = PLDM_SUCCESS;
    uint16_t file_descriptor = 0x0100;
    size_t payload_length;

    const struct pldm_file_df_open_resp resp_data = {completion_code,
                                                     file_descriptor};

    PLDM_MSG_DEFINE_P(responsePtr, PLDM_DF_OPEN_RESP_BYTES);
    payload_length = 1;
    auto rc = encode_pldm_file_df_open_resp(instance_id, &resp_data,
                                            responsePtr, &payload_length);
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfOpenResp, GoodTestCompletionCode)
{
    uint8_t instance_id = 0;
    uint8_t completion_code = PLDM_FILE_CC_INVALID_FILE_DESCRIPTOR;
    uint16_t file_descriptor = 0x0100;
    size_t payload_length = PLDM_DF_OPEN_RESP_BYTES;

    const struct pldm_file_df_open_resp resp_data = {completion_code,
                                                     file_descriptor};

    PLDM_MSG_DEFINE_P(responsePtr, PLDM_DF_OPEN_RESP_BYTES);
    auto rc = encode_pldm_file_df_open_resp(instance_id, &resp_data,
                                            responsePtr, &payload_length);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(payload_length, 1ul); // Completion code only
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfOpenReq, GoodTest)
{
    uint16_t file_identifier = 0x0100;
    bitfield16_t file_attribute;
    file_attribute.value = 0x0400;

    struct pldm_file_df_open_req req_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_OPEN_REQ_BYTES;

    PLDM_MSG_DEFINE_P(requestMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, requestMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint16(buf, file_identifier);
    pldm_msgbuf_insert_uint16(buf, file_attribute.value);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_file_df_open_req(requestMsg, payload_length, &req_data);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(req_data.file_identifier, file_identifier);
    EXPECT_EQ(req_data.file_attribute.value, file_attribute.value);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfOpenReq, BadTestUnAllocatedPtrParams)
{
    uint16_t file_identifier = 0x0100;
    bitfield16_t file_attribute;
    file_attribute.value = 0x0400;

    struct pldm_file_df_open_req req_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_OPEN_REQ_BYTES;

    PLDM_MSG_DEFINE_P(requestMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, requestMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint16(buf, file_identifier);
    pldm_msgbuf_insert_uint16(buf, file_attribute.value);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_file_df_open_req(nullptr, payload_length, &req_data);
    EXPECT_EQ(rc, -EINVAL);

    rc = decode_pldm_file_df_open_req(requestMsg, payload_length, nullptr);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfOpenReq, BadTestInvalidExpectedInputMsgLength)
{
    uint16_t file_identifier = 0x0100;
    bitfield16_t file_attribute;
    file_attribute.value = 0x0400;

    struct pldm_file_df_open_req req_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_OPEN_REQ_BYTES;

    PLDM_MSG_DEFINE_P(requestMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, requestMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint16(buf, file_identifier);
    pldm_msgbuf_insert_uint16(buf, file_attribute.value);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_file_df_open_req(requestMsg, 0, &req_data);
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfCloseResp, GoodTest)
{
    uint8_t instance_id = 0;
    size_t payload_length = PLDM_DF_CLOSE_RESP_BYTES;
    std::array<uint8_t, PLDM_DF_CLOSE_RESP_BYTES> responseMsg = {PLDM_SUCCESS};

    const struct pldm_file_df_close_resp resp_data = {PLDM_SUCCESS};

    PLDM_MSG_DEFINE_P(responsePtr, PLDM_DF_CLOSE_RESP_BYTES);

    auto rc = encode_pldm_file_df_close_resp(instance_id, &resp_data,
                                             responsePtr, &payload_length);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(0, memcmp(responsePtr->payload, responseMsg.data(),
                        sizeof(responseMsg)));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeDfCloseResp, BadTestUnAllocatedPtrParams)
{
    uint8_t instance_id = 0;
    size_t payload_length = PLDM_DF_CLOSE_RESP_BYTES;
    const struct pldm_file_df_close_resp resp_data = {PLDM_SUCCESS};
    int rc;

    PLDM_MSG_DEFINE_P(responsePtr, PLDM_DF_CLOSE_RESP_BYTES);

    rc = encode_pldm_file_df_close_resp(instance_id, &resp_data, nullptr,
                                        &payload_length);
    EXPECT_EQ(rc, -EINVAL);

    rc = encode_pldm_file_df_close_resp(instance_id, nullptr, responsePtr,
                                        &payload_length);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfCloseReq, GoodTest)
{
    uint16_t file_descriptor = 0x1000;
    bitfield16_t df_close_options;
    df_close_options.value = 0x4000;

    struct pldm_file_df_close_req req_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_CLOSE_REQ_BYTES;

    PLDM_MSG_DEFINE_P(requestMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, requestMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint16(buf, file_descriptor);
    pldm_msgbuf_insert_uint16(buf, df_close_options.value);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_file_df_close_req(requestMsg, payload_length, &req_data);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(req_data.file_descriptor, file_descriptor);
    EXPECT_EQ(req_data.df_close_options.value, df_close_options.value);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeDfCloseReq, BadTestUnAllocatedPtrParams)
{
    uint16_t file_descriptor = 0x1000;
    bitfield16_t df_close_options;
    df_close_options.value = 0x4000;

    struct pldm_file_df_close_req req_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_CLOSE_REQ_BYTES;

    PLDM_MSG_DEFINE_P(requestMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, requestMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint16(buf, file_descriptor);
    pldm_msgbuf_insert_uint16(buf, df_close_options.value);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_file_df_close_req(nullptr, payload_length, &req_data);
    EXPECT_EQ(rc, -EINVAL);

    rc = decode_pldm_file_df_close_req(requestMsg, payload_length, nullptr);
    EXPECT_EQ(rc, -EINVAL);
}
#endif
