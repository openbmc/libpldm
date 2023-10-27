#include <endian.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "base.h"
#include "oem/meta/libpldm/file_io.h"

#include <gtest/gtest.h>

constexpr auto hdrSize = sizeof(pldm_msg_hdr);

#define OEM_META_DECODE_WRITE_FILE_IO_REQ_BYTES 10
TEST(DecodeWritFileIoReqOemMeta, testGoodDecodeRequest)
{
    std::array<uint8_t, hdrSize + OEM_META_DECODE_WRITE_FILE_IO_REQ_BYTES>
        requestMsg{};

    uint8_t fileHandle = 0x00;
    uint16_t dataLength = 0x04;
    uint32_t dataLengthLE = le32toh(dataLength);

    std::array<uint8_t, 4> postCode = {0x93, 0xE0, 0x00, 0xEA};

    uint8_t retfileHandle = 0;
    uint32_t retFileDataCnt = 0;

    std::array<uint8_t, OEM_META_DECODE_WRITE_FILE_IO_REQ_BYTES> retDataField{};

    memcpy(requestMsg.data() + hdrSize, &fileHandle, sizeof(fileHandle));
    memcpy(requestMsg.data() + sizeof(fileHandle) + hdrSize, &dataLengthLE,
           sizeof(dataLengthLE));
    memcpy(requestMsg.data() + sizeof(fileHandle) + sizeof(dataLengthLE) +
               hdrSize,
           &postCode, sizeof(postCode));

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = decode_write_file_io_req_oem_meta(
        request, requestMsg.size() - hdrSize, &retfileHandle, &retFileDataCnt,
        retDataField.data());

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retfileHandle, fileHandle);
    EXPECT_EQ(retFileDataCnt, dataLength);
    EXPECT_EQ(retDataField[0], postCode[0]);
    EXPECT_EQ(retDataField[1], postCode[1]);
    EXPECT_EQ(retDataField[2], postCode[2]);
    EXPECT_EQ(retDataField[3], postCode[3]);
}

TEST(DecodeWritFileIoReqOemMeta, testBadDecodeRequest)
{
    std::array<uint8_t, OEM_META_DECODE_WRITE_FILE_IO_REQ_BYTES> requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = decode_write_file_io_req_oem_meta(request, requestMsg.size(),
                                                NULL, NULL, NULL);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}
