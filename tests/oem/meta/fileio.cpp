/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <endian.h>
#include <libpldm/oem/meta/file_io.h>

#include <cstdlib>

#include "msgbuf.h"

#include <gtest/gtest.h>

static constexpr size_t oemMetaDecodeWriteFileIoReqBytes = 9;
static constexpr size_t postCodeSize = 4;

TEST(DecodeOemMetaFileIoWriteReq, testGoodDecodeRequest)
{
    struct pldm_msgbuf _ctx;
    struct pldm_msgbuf* ctx = &_ctx;
    uint8_t fileHandle = 0x00;
    int32_t dataLengthLE = 0x04;
    uint8_t postCode[4] = {0x93, 0xe0, 0x00, 0xea};

    constexpr auto hdrSize = sizeof(pldm_msg_hdr);

    uint8_t buf[hdrSize + sizeof(uint8_t) + sizeof(int32_t) +
                (postCodeSize * sizeof(uint8_t))] = {};

    ASSERT_EQ(pldm_msgbuf_init_cc(ctx, 0, &buf[hdrSize], sizeof(buf) - hdrSize),
              0);

    pldm_msgbuf_insert_uint8(ctx, fileHandle);
    pldm_msgbuf_insert_int32(ctx, dataLengthLE);
    pldm_msgbuf_insert_array_uint8(ctx, postCode, sizeof(postCode));

    size_t request_msg_len =
        sizeof(struct pldm_oem_meta_file_io_write_req) + sizeof(postCode);
    auto* request_msg = static_cast<struct pldm_oem_meta_file_io_write_req*>(
        malloc(request_msg_len));
    auto* retDataField = static_cast<uint8_t*>(
        pldm_oem_meta_file_io_write_req_data(request_msg));
    auto request = reinterpret_cast<pldm_msg*>(buf);

    auto rc = decode_oem_meta_file_io_write_req(request, sizeof(buf) - hdrSize,
                                                request_msg, request_msg_len);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(request_msg->handle, fileHandle);
    EXPECT_EQ(request_msg->length, dataLengthLE);
    EXPECT_EQ(retDataField[0], postCode[0]);
    EXPECT_EQ(retDataField[1], postCode[1]);
    EXPECT_EQ(retDataField[2], postCode[2]);
    EXPECT_EQ(retDataField[3], postCode[3]);

    free(request_msg);
}

TEST(DecodeOemMetaFileIoWriteReq, testInvalidFieldsDecodeRequest)
{
    std::array<uint8_t, oemMetaDecodeWriteFileIoReqBytes> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc =
        decode_oem_meta_file_io_write_req(request, requestMsg.size(), NULL, 0);

    EXPECT_EQ(rc, -EINVAL);
}

TEST(DecodeOemMetaFileIoWriteReq, testInvalidLengthDecodeRequest)
{
    struct pldm_oem_meta_file_io_write_req request_msg;
    uint8_t buf[1] = {};

    auto request = reinterpret_cast<pldm_msg*>(buf);
    auto rc = decode_oem_meta_file_io_write_req(request, 0, &request_msg,
                                                sizeof(request_msg));

    EXPECT_EQ(rc, -EOVERFLOW);
}

TEST(DecodeOemMetaFileIoWriteReq, testInvalidDataRequest)
{
    struct pldm_oem_meta_file_io_write_req request_msg;
    uint8_t buf[1] = {};

    auto request = reinterpret_cast<pldm_msg*>(buf);
    auto rc = decode_oem_meta_file_io_write_req(
        request, sizeof(buf), &request_msg, sizeof(request_msg));

    EXPECT_EQ(rc, -EOVERFLOW);
}

TEST(DecodeOemMetaFileIoReadReq, testGoodDecodeRequest)
{
    struct pldm_msgbuf _ctx;
    struct pldm_msgbuf* ctx = &_ctx;
    uint8_t fileHandle = 0x00;
    uint8_t option = PLDM_OEM_META_FILE_IO_READ_DATA;
    uint8_t length = 0x31;
    struct pldm_oem_meta_file_io_read_data_info data_info;
    data_info.transferFlag = 0x01;
    data_info.highOffset = 0x12;
    data_info.lowOffset = 0x23;

    constexpr auto hdrSize = sizeof(pldm_msg_hdr);

    uint8_t buf[hdrSize + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) +
                sizeof(struct pldm_oem_meta_file_io_read_data_info)] = {};

    ASSERT_EQ(
        pldm_msgbuf_init_errno(ctx, 0, &buf[hdrSize], sizeof(buf) - hdrSize),
        0);

    pldm_msgbuf_insert_uint8(ctx, fileHandle);
    pldm_msgbuf_insert_uint8(ctx, option);
    pldm_msgbuf_insert_uint8(ctx, length);
    pldm_msgbuf_insert_uint8(ctx, data_info.transferFlag);
    pldm_msgbuf_insert_uint8(ctx, data_info.highOffset);
    pldm_msgbuf_insert_uint8(ctx, data_info.lowOffset);

    struct pldm_oem_meta_file_io_read_req request_msg;
    auto request = reinterpret_cast<pldm_msg*>(buf);

    auto rc = decode_oem_meta_file_io_read_req(request, sizeof(buf) - hdrSize,
                                               &request_msg);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(request_msg.handle, fileHandle);
    EXPECT_EQ(request_msg.option, option);
    EXPECT_EQ(request_msg.length, length);
    EXPECT_EQ(request_msg.info.data.transferFlag, data_info.transferFlag);
    EXPECT_EQ(request_msg.info.data.highOffset, data_info.highOffset);
    EXPECT_EQ(request_msg.info.data.lowOffset, data_info.lowOffset);
}

TEST(DecodeOemMetaFileIoReadReq, testInvalidFieldsDecodeRequest)
{
    uint8_t buf[sizeof(struct pldm_oem_meta_file_io_read_req)] = {};
    auto request = reinterpret_cast<pldm_msg*>(buf);

    auto rc = decode_oem_meta_file_io_read_req(request, sizeof(buf), NULL);

    EXPECT_EQ(rc, -EINVAL);
}

TEST(DecodeOemMetaFileIoReadReq, testInvalidLengthDecodeRequest)
{
    struct pldm_oem_meta_file_io_read_req request_msg;
    uint8_t buf[1] = {};
    auto request = reinterpret_cast<pldm_msg*>(buf);

    auto rc = decode_oem_meta_file_io_read_req(request, 0, &request_msg);

    EXPECT_EQ(rc, -EOVERFLOW);
}

TEST(DecodeOemMetaFileIoReadReq, testInvalidDataRequest)
{
    struct pldm_oem_meta_file_io_read_req request_msg;
    uint8_t buf[1] = {};
    auto request = reinterpret_cast<pldm_msg*>(buf);

    auto rc =
        decode_oem_meta_file_io_read_req(request, sizeof(buf), &request_msg);

    EXPECT_EQ(rc, -EOVERFLOW);
}

TEST(EncodeOemMetaFileIoReadResp, testGoodEncodeResponse)
{
    uint8_t instance_id = 0x00;
    uint8_t crc32[4] = {0x32, 0x54, 0x71, 0xab};
    struct pldm_oem_meta_file_io_read_resp* resp =
        static_cast<struct pldm_oem_meta_file_io_read_resp*>(malloc(
            sizeof(struct pldm_oem_meta_file_io_read_resp) + sizeof(crc32)));
    resp->completion_code = PLDM_SUCCESS;
    resp->handle = 0x01;
    resp->option = PLDM_OEM_META_FILE_IO_READ_ATTR;
    resp->length = 0x04;
    uint8_t* datafield =
        static_cast<uint8_t*>(pldm_oem_meta_file_io_read_resp_data(resp));

    EXPECT_NE(datafield, nullptr);

    datafield[0] = crc32[0];
    datafield[1] = crc32[1];
    datafield[2] = crc32[2];
    datafield[3] = crc32[3];

    constexpr auto hdrSize = sizeof(pldm_msg_hdr);

    uint8_t buf[hdrSize + sizeof(struct pldm_oem_meta_file_io_read_resp) +
                sizeof(crc32)] = {};

    auto response_msg = reinterpret_cast<pldm_msg*>(buf);
    auto* response_buf =
        reinterpret_cast<struct pldm_oem_meta_file_io_read_resp*>(
            response_msg->payload);
    auto* retdatafield = static_cast<uint8_t*>(
        pldm_oem_meta_file_io_read_resp_data(response_buf));

    EXPECT_NE(retdatafield, nullptr);

    auto rc = encode_oem_meta_file_io_read_resp(
        instance_id, resp, sizeof(buf) - hdrSize, response_msg);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(response_msg->hdr.instance_id, instance_id);
    EXPECT_EQ(response_buf->completion_code, resp->completion_code);
    EXPECT_EQ(response_buf->handle, resp->handle);
    EXPECT_EQ(response_buf->option, resp->option);
    EXPECT_EQ(response_buf->length, resp->length);
    EXPECT_EQ(retdatafield[0], datafield[0]);
    EXPECT_EQ(retdatafield[1], datafield[1]);
    EXPECT_EQ(retdatafield[2], datafield[2]);
    EXPECT_EQ(retdatafield[3], datafield[3]);

    free(resp);
}

TEST(EncodeOemMetaFileIoReadResp, testInvalidFieldsEncodeResponse)
{
    uint8_t instance_id = 0x00;
    uint8_t buf[sizeof(struct pldm_oem_meta_file_io_read_resp)] = {};
    auto response_msg = reinterpret_cast<pldm_msg*>(buf);

    auto rc = encode_oem_meta_file_io_read_resp(instance_id, NULL, sizeof(buf),
                                                response_msg);

    EXPECT_EQ(rc, -EINVAL);
}

TEST(EncodeOemMetaFileIoReadResp, testInvalidLengthEncodeResponse)
{
    uint8_t instance_id = 0x00;
    struct pldm_oem_meta_file_io_read_resp resp;
    uint8_t buf[1] = {};
    auto response_msg = reinterpret_cast<pldm_msg*>(buf);

    auto rc =
        encode_oem_meta_file_io_read_resp(instance_id, &resp, 0, response_msg);

    EXPECT_EQ(rc, -EOVERFLOW);
}

TEST(EncodeOemMetaFileIoReadResp, testInvalidDataEncodeResponse)
{
    uint8_t instance_id = 0x00;
    struct pldm_oem_meta_file_io_read_resp resp;
    uint8_t buf[1] = {};
    auto response_msg = reinterpret_cast<pldm_msg*>(buf);

    auto rc = encode_oem_meta_file_io_read_resp(instance_id, &resp, sizeof(buf),
                                                response_msg);

    EXPECT_EQ(rc, -EOVERFLOW);
}
