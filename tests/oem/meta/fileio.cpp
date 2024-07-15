/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <endian.h>
#include <libpldm/oem/meta/file_io.h>

#include "msgbuf.h"

#include <gtest/gtest.h>

static constexpr size_t oemMetaDecodeWriteFileIoReqBytes = 9;
static constexpr size_t postCodeSize = 4;
static constexpr size_t invalidDataSize = 0;

TEST(DecodeOemMetaFileIoReq, testGoodDecodeRequest)
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

    std::array<uint8_t, oemMetaDecodeWriteFileIoReqBytes> retDataField{};

    uint8_t retfileHandle = 0;
    uint32_t retFileDataCnt = 0;

    auto request = reinterpret_cast<pldm_msg*>(buf);

    auto rc = decode_oem_meta_write_file_io_req(request, sizeof(buf) - hdrSize,
                                                &retfileHandle, &retFileDataCnt,
                                                retDataField.data());

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(retfileHandle, fileHandle);
    EXPECT_EQ(retFileDataCnt, dataLengthLE);
    EXPECT_EQ(retDataField[0], postCode[0]);
    EXPECT_EQ(retDataField[1], postCode[1]);
    EXPECT_EQ(retDataField[2], postCode[2]);
    EXPECT_EQ(retDataField[3], postCode[3]);
}

TEST(DecodeOemMetaFileIoReq, testInvalidFieldsDecodeRequest)
{
    std::array<uint8_t, oemMetaDecodeWriteFileIoReqBytes> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = decode_oem_meta_write_file_io_req(request, requestMsg.size(),
                                                NULL, NULL, NULL);

    EXPECT_EQ(rc, -EINVAL);
}

TEST(DecodeOemMetaFileIoReq, testInvalidLengthDecodeRequest)
{
    uint8_t retfileHandle = 0;
    uint32_t retFileDataCnt = 0;
    uint8_t buf[1] = {};
    std::array<uint8_t, oemMetaDecodeWriteFileIoReqBytes> retDataField{};

    auto request = reinterpret_cast<pldm_msg*>(buf);

    auto rc = decode_oem_meta_write_file_io_req(
        request, 0, &retfileHandle, &retFileDataCnt, retDataField.data());

    EXPECT_EQ(rc, -EOVERFLOW);
}

TEST(DecodeOemMetaFileIoReq, testInvalidDataRequest)
{
    uint8_t buf[1] = {};
    uint8_t retfileHandle = 0;
    uint32_t retFileDataCnt = 0;

    std::array<uint8_t, invalidDataSize> retDataField{};

    auto request = reinterpret_cast<pldm_msg*>(buf);

    auto rc =
        decode_oem_meta_write_file_io_req(request, sizeof(buf), &retfileHandle,
                                          &retFileDataCnt, retDataField.data());

    EXPECT_EQ(rc, -EINVAL);
}
