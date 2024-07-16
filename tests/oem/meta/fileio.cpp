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
    int rc;

    constexpr auto hdrSize = sizeof(pldm_msg_hdr);

    uint8_t buf[hdrSize + sizeof(uint8_t) + sizeof(int32_t) +
                (postCodeSize * sizeof(uint8_t))] = {};

    ASSERT_EQ(pldm_msgbuf_init_cc(ctx, 0, &buf[hdrSize], sizeof(buf) - hdrSize),
              0);

    pldm_msgbuf_insert_uint8(ctx, fileHandle);
    pldm_msgbuf_insert_int32(ctx, dataLengthLE);
    rc = pldm_msgbuf_insert_array_uint8(ctx, sizeof(postCode), postCode,
                                        sizeof(postCode));
    ASSERT_EQ(rc, PLDM_SUCCESS);

    uint8_t* request_buf[sizeof(struct pldm_oem_meta_file_io_write_req) +
                         (postCodeSize * sizeof(uint8_t))] = {};
    size_t request_msg_len = sizeof(struct pldm_oem_meta_file_io_write_req) +
                             (postCodeSize * sizeof(uint8_t));
    auto* request_msg =
        reinterpret_cast<struct pldm_oem_meta_file_io_write_req*>(request_buf);
    auto* retDataField = static_cast<uint8_t*>(
        pldm_oem_meta_file_io_write_req_data(request_msg));
    auto request = reinterpret_cast<pldm_msg*>(buf);

    rc = decode_oem_meta_file_io_write_req(request, sizeof(buf) - hdrSize,
                                           request_msg, request_msg_len);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(request_msg->handle, fileHandle);
    EXPECT_EQ(request_msg->length, dataLengthLE);
    EXPECT_EQ(retDataField[0], postCode[0]);
    EXPECT_EQ(retDataField[1], postCode[1]);
    EXPECT_EQ(retDataField[2], postCode[2]);
    EXPECT_EQ(retDataField[3], postCode[3]);
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
    int rc = 0;
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

    rc = decode_oem_meta_file_io_read_req(request, sizeof(buf) - hdrSize,
                                          &request_msg);

    ASSERT_EQ(rc, 0);
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
