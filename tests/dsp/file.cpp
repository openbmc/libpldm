#include <libpldm/file.h>
#include <libpldm/pldm_types.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "msgbuf.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifdef LIBPLDM_API_TESTING
TEST(EncodeDfOpenReq, GoodTest)
{
    uint8_t instance_id = 0;
    uint16_t file_identifier = 0x01;
    bitfield16_t file_attribute;
    file_attribute.value = 0x04;

    const struct pldm_df_open_req req_data = {file_identifier, file_attribute};

    PLDM_MSG_DEFINE_P(requestPtr, PLDM_DF_OPEN_REQ_BYTES);
    auto rc = encode_df_open_req(instance_id, &req_data, requestPtr,
                                 PLDM_DF_OPEN_REQ_BYTES);

    struct pldm_df_open_req* request =
        (struct pldm_df_open_req*)requestPtr->payload;
    ASSERT_EQ(rc, 0);
    EXPECT_EQ(request->file_identifier, file_identifier);
    EXPECT_EQ(request->file_attribute.value, file_attribute.value);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(EncodeDfOpenReq, BadTest)
{
    uint8_t instance_id = 0;
    uint16_t file_identifier = 0x01;
    bitfield16_t file_attribute;
    file_attribute.value = 0x04;

    const struct pldm_df_open_req req_data = {file_identifier, file_attribute};

    PLDM_MSG_DEFINE_P(requestPtr, PLDM_DF_OPEN_REQ_BYTES);
    auto rc = encode_df_open_req(instance_id, &req_data, nullptr,
                                 PLDM_DF_OPEN_REQ_BYTES);
    EXPECT_EQ(rc, -EINVAL);
    rc = encode_df_open_req(instance_id, &req_data, requestPtr, 1);
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(DecodeDfOpenResp, GoodTest)
{
    uint8_t completion_code = PLDM_SUCCESS;
    uint16_t file_descriptor = 20;

    struct pldm_df_open_resp resp_data = {};

    struct pldm_msgbuf _buf;
    struct pldm_msgbuf* buf = &_buf;
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_OPEN_RESP_BYTES;

    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    EXPECT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completion_code);
    pldm_msgbuf_insert_uint16(buf, file_descriptor);

    ASSERT_EQ(pldm_msgbuf_destroy_consumed(buf), 0);

    rc = decode_df_open_resp(responseMsg, payload_length, &resp_data);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(resp_data.completion_code, completion_code);
    EXPECT_EQ(resp_data.file_descriptor, file_descriptor);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(DecodeDfOpenResp, BadTest)
{
    uint8_t completion_code = PLDM_SUCCESS;
    uint16_t file_descriptor = 20;

    struct pldm_df_open_resp resp_data = {};

    struct pldm_msgbuf _buf;
    struct pldm_msgbuf* buf = &_buf;
    int rc;

    static constexpr const size_t payload_length = PLDM_DF_OPEN_RESP_BYTES;

    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    EXPECT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completion_code);
    pldm_msgbuf_insert_uint16(buf, file_descriptor);

    rc = decode_df_open_resp(nullptr, payload_length, &resp_data);
    EXPECT_EQ(rc, -EINVAL);
    rc = decode_df_open_resp(responseMsg, payload_length, nullptr);
    EXPECT_EQ(rc, -EINVAL);
    rc = decode_df_open_resp(responseMsg, 0, &resp_data);
    EXPECT_EQ(rc, -EOVERFLOW);

    ASSERT_EQ(pldm_msgbuf_destroy_consumed(buf), 0);
}
#endif