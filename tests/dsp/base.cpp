#include "msgbuf.hpp"

#include <libpldm/base.h>
#include <libpldm/pldm_types.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::ElementsAreArray;

constexpr auto hdrSize = sizeof(pldm_msg_hdr);

TEST(Ver2string, Ver2string)
{
    ver32_t version{0x61, 0x10, 0xf7, 0xf3};
    const char* vstr = "3.7.10a";
    char buffer[1024];
    auto rc = pldm_base_ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    version = {0x00, 0xf0, 0xf0, 0xf1};
    vstr = "1.0.0";
    rc = pldm_base_ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    version = {0x00, 0xf7, 0x01, 0x10};
    vstr = "10.01.7";
    rc = pldm_base_ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    version = {0x00, 0xff, 0xf1, 0xf3};
    vstr = "3.1";
    rc = pldm_base_ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    version = {0x61, 0xff, 0xf0, 0xf1};
    vstr = "1.0a";
    rc = pldm_base_ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    rc = pldm_base_ver2str(&version, buffer, 3);
    EXPECT_EQ(rc, 2);
    EXPECT_STREQ("1.", buffer);

    rc = pldm_base_ver2str(&version, buffer, 1);
    EXPECT_EQ(rc, 0);

    rc = pldm_base_ver2str(&version, buffer, 0);
    EXPECT_EQ(rc, -1);
}

TEST(PackPLDMMessage, BadPathTest)
{
    struct pldm_header_info hdr;
    struct pldm_header_info* hdr_ptr = NULL;
    pldm_msg_hdr msg{};

    // PLDM header information pointer is NULL
    auto rc = pack_pldm_header(hdr_ptr, &msg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // PLDM message pointer is NULL
    rc = pack_pldm_header(&hdr, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // PLDM header information pointer and PLDM message pointer is NULL
    rc = pack_pldm_header(hdr_ptr, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // RESERVED message type
    hdr.msg_type = PLDM_RESERVED;
    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // Instance ID out of range
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = 32;
    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // PLDM type out of range
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = 31;
    hdr.pldm_type = 64;
    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_PLDM_TYPE);
}

TEST(PackPLDMMessage, RequestMessageGoodPath)
{
    struct pldm_header_info hdr;
    pldm_msg_hdr msg{};

    // Message type is REQUEST and lower range of the field values
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = 0;
    hdr.pldm_type = 0;
    hdr.command = 0;

    auto rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 1);
    EXPECT_EQ(msg.datagram, 0);
    EXPECT_EQ(msg.instance_id, 0);
    EXPECT_EQ(msg.type, 0);
    EXPECT_EQ(msg.command, 0);

    // Message type is REQUEST and upper range of the field values
    hdr.instance = 31;
    hdr.pldm_type = 63;
    hdr.command = 255;

    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 1);
    EXPECT_EQ(msg.datagram, 0);
    EXPECT_EQ(msg.instance_id, 31);
    EXPECT_EQ(msg.type, 63);
    EXPECT_EQ(msg.command, 255);

    // Message type is PLDM_ASYNC_REQUEST_NOTIFY
    hdr.msg_type = PLDM_ASYNC_REQUEST_NOTIFY;

    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 1);
    EXPECT_EQ(msg.datagram, 1);
    EXPECT_EQ(msg.instance_id, 31);
    EXPECT_EQ(msg.type, 63);
    EXPECT_EQ(msg.command, 255);
}

TEST(PackPLDMMessage, ResponseMessageGoodPath)
{
    struct pldm_header_info hdr;
    pldm_msg_hdr msg{};

    // Message type is PLDM_RESPONSE and lower range of the field values
    hdr.msg_type = PLDM_RESPONSE;
    hdr.instance = 0;
    hdr.pldm_type = 0;
    hdr.command = 0;

    auto rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 0);
    EXPECT_EQ(msg.datagram, 0);
    EXPECT_EQ(msg.instance_id, 0);
    EXPECT_EQ(msg.type, 0);
    EXPECT_EQ(msg.command, 0);

    // Message type is PLDM_RESPONSE and upper range of the field values
    hdr.instance = 31;
    hdr.pldm_type = 63;
    hdr.command = 255;

    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 0);
    EXPECT_EQ(msg.datagram, 0);
    EXPECT_EQ(msg.instance_id, 31);
    EXPECT_EQ(msg.type, 63);
    EXPECT_EQ(msg.command, 255);
}

TEST(UnpackPLDMMessage, BadPathTest)
{
    struct pldm_header_info hdr;

    // PLDM message pointer is NULL
    auto rc = unpack_pldm_header(nullptr, &hdr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(UnpackPLDMMessage, RequestMessageGoodPath)
{
    struct pldm_header_info hdr;
    pldm_msg_hdr msg{};

    // Unpack PLDM request message and lower range of field values
    msg.request = 1;
    auto rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_REQUEST);
    EXPECT_EQ(hdr.instance, 0);
    EXPECT_EQ(hdr.pldm_type, 0);
    EXPECT_EQ(hdr.command, 0);

    // Unpack PLDM async request message and lower range of field values
    msg.datagram = 1;
    rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_ASYNC_REQUEST_NOTIFY);

    // Unpack PLDM request message and upper range of field values
    msg.datagram = 0;
    msg.instance_id = 31;
    msg.type = 63;
    msg.command = 255;
    rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_REQUEST);
    EXPECT_EQ(hdr.instance, 31);
    EXPECT_EQ(hdr.pldm_type, 63);
    EXPECT_EQ(hdr.command, 255);
}

TEST(UnpackPLDMMessage, ResponseMessageGoodPath)
{
    struct pldm_header_info hdr;
    pldm_msg_hdr msg{};

    // Unpack PLDM response message and lower range of field values
    auto rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_RESPONSE);
    EXPECT_EQ(hdr.instance, 0);
    EXPECT_EQ(hdr.pldm_type, 0);
    EXPECT_EQ(hdr.command, 0);

    // Unpack PLDM response message and upper range of field values
    msg.instance_id = 31;
    msg.type = 63;
    msg.command = 255;
    rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_RESPONSE);
    EXPECT_EQ(hdr.instance, 31);
    EXPECT_EQ(hdr.pldm_type, 63);
    EXPECT_EQ(hdr.command, 255);
}

TEST(GetPLDMCommands, testEncodeRequest)
{
    uint8_t pldmType = 0x05;
    ver32_t version{0xff, 0xff, 0xff, 0xff};
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_COMMANDS_REQ_BYTES>
        requestMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_get_commands_req(0, pldmType, version, request);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(0, memcmp(request->payload, &pldmType, sizeof(pldmType)));
    EXPECT_EQ(0, memcmp(request->payload + sizeof(pldmType), &version,
                        sizeof(version)));
}

TEST(GetPLDMCommands, testDecodeRequest)
{
    uint8_t pldmType = 0x05;
    ver32_t version{0xff, 0xff, 0xff, 0xff};
    uint8_t pldmTypeOut{};
    ver32_t versionOut{0xff, 0xff, 0xff, 0xff};
    std::array<uint8_t, hdrSize + PLDM_GET_COMMANDS_REQ_BYTES> requestMsg{};

    memcpy(requestMsg.data() + hdrSize, &pldmType, sizeof(pldmType));
    memcpy(requestMsg.data() + sizeof(pldmType) + hdrSize, &version,
           sizeof(version));

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc = decode_get_commands_req(request, requestMsg.size() - hdrSize,
                                      &pldmTypeOut, &versionOut);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(pldmTypeOut, pldmType);
    EXPECT_EQ(0, memcmp(&versionOut, &version, sizeof(version)));
}

TEST(GetPLDMCommands, testEncodeResponse)
{
    uint8_t completionCode = 0;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_COMMANDS_RESP_BYTES>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    std::array<bitfield8_t, PLDM_MAX_CMDS_PER_TYPE / 8> commands{};
    commands[0].byte = 1;
    commands[1].byte = 2;
    commands[2].byte = 3;

    auto rc =
        encode_get_commands_resp(0, PLDM_SUCCESS, commands.data(), response);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    uint8_t* payload_ptr = response->payload;
    EXPECT_EQ(completionCode, payload_ptr[0]);
    EXPECT_EQ(1, payload_ptr[sizeof(completionCode)]);
    EXPECT_EQ(2,
              payload_ptr[sizeof(completionCode) + sizeof(commands[0].byte)]);
    EXPECT_EQ(3, payload_ptr[sizeof(completionCode) + sizeof(commands[0].byte) +
                             sizeof(commands[1].byte)]);
}

TEST(GetPLDMTypes, testEncodeResponse)
{
    uint8_t completionCode = 0;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_TYPES_RESP_BYTES>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    std::array<bitfield8_t, PLDM_MAX_TYPES / 8> types{};
    types[0].byte = 1;
    types[1].byte = 2;
    types[2].byte = 3;

    auto rc = encode_get_types_resp(0, PLDM_SUCCESS, types.data(), response);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    uint8_t* payload_ptr = response->payload;
    EXPECT_EQ(completionCode, payload_ptr[0]);
    EXPECT_EQ(1, payload_ptr[sizeof(completionCode)]);
    EXPECT_EQ(2, payload_ptr[sizeof(completionCode) + sizeof(types[0].byte)]);
    EXPECT_EQ(3, payload_ptr[sizeof(completionCode) + sizeof(types[0].byte) +
                             sizeof(types[1].byte)]);
}

TEST(GetPLDMTypes, testGoodDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_TYPES_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;
    responseMsg[2 + hdrSize] = 2;
    responseMsg[3 + hdrSize] = 3;
    std::array<bitfield8_t, PLDM_MAX_TYPES / 8> outTypes{};

    uint8_t completion_code;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_types_resp(response, responseMsg.size() - hdrSize,
                                    &completion_code, outTypes.data());

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completion_code, PLDM_SUCCESS);
    EXPECT_EQ(responseMsg[1 + hdrSize], outTypes[0].byte);
    EXPECT_EQ(responseMsg[2 + hdrSize], outTypes[1].byte);
    EXPECT_EQ(responseMsg[3 + hdrSize], outTypes[2].byte);
}

TEST(GetPLDMTypes, testBadDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_TYPES_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;
    responseMsg[2 + hdrSize] = 2;
    responseMsg[3 + hdrSize] = 3;
    std::array<bitfield8_t, PLDM_MAX_TYPES / 8> outTypes{};

    uint8_t retcompletion_code = 0;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_types_resp(response, responseMsg.size() - hdrSize - 1,
                                    &retcompletion_code, outTypes.data());

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetPLDMCommands, testGoodDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_COMMANDS_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;
    responseMsg[2 + hdrSize] = 2;
    responseMsg[3 + hdrSize] = 3;
    std::array<bitfield8_t, PLDM_MAX_CMDS_PER_TYPE / 8> outTypes{};

    uint8_t completion_code;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_commands_resp(response, responseMsg.size() - hdrSize,
                                       &completion_code, outTypes.data());

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completion_code, PLDM_SUCCESS);
    EXPECT_EQ(responseMsg[1 + hdrSize], outTypes[0].byte);
    EXPECT_EQ(responseMsg[2 + hdrSize], outTypes[1].byte);
    EXPECT_EQ(responseMsg[3 + hdrSize], outTypes[2].byte);
}

TEST(GetPLDMCommands, testBadDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_COMMANDS_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;
    responseMsg[2 + hdrSize] = 2;
    responseMsg[3 + hdrSize] = 3;
    std::array<bitfield8_t, PLDM_MAX_CMDS_PER_TYPE / 8> outTypes{};

    uint8_t retcompletion_code = 0;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc =
        decode_get_commands_resp(response, responseMsg.size() - hdrSize - 1,
                                 &retcompletion_code, outTypes.data());

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetPLDMVersion, testGoodEncodeRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_VERSION_REQ_BYTES>
        requestMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    uint8_t pldmType = 0x03;
    uint32_t transferHandle = 0x0;
    uint8_t opFlag = 0x01;

    auto rc =
        encode_get_version_req(0, transferHandle, opFlag, pldmType, request);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(
        0, memcmp(request->payload, &transferHandle, sizeof(transferHandle)));
    EXPECT_EQ(0, memcmp(request->payload + sizeof(transferHandle), &opFlag,
                        sizeof(opFlag)));
    EXPECT_EQ(0,
              memcmp(request->payload + sizeof(transferHandle) + sizeof(opFlag),
                     &pldmType, sizeof(pldmType)));
}

TEST(GetPLDMVersion, testBadEncodeRequest)
{
    uint8_t pldmType = 0x03;
    uint32_t transferHandle = 0x0;
    uint8_t opFlag = 0x01;

    auto rc =
        encode_get_version_req(0, transferHandle, opFlag, pldmType, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetPLDMVersion, testEncodeResponse)
{
    uint8_t completionCode = 0;
    uint32_t transferHandle = 0;
    uint8_t flag = PLDM_START_AND_END;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_VERSION_RESP_BYTES>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    ver32_t version = {0xff, 0xff, 0xff, 0xff};

    auto rc = encode_get_version_resp(0, PLDM_SUCCESS, 0, PLDM_START_AND_END,
                                      &version, sizeof(ver32_t), response);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, response->payload[0]);
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]),
                        &transferHandle, sizeof(transferHandle)));
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(transferHandle),
                        &flag, sizeof(flag)));
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(transferHandle) + sizeof(flag),
                        &version, sizeof(version)));
}

TEST(GetPLDMVersion, testDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_VERSION_REQ_BYTES> requestMsg{};
    uint32_t transferHandle = 0x0;
    uint32_t retTransferHandle = 0x0;
    uint8_t flag = PLDM_GET_FIRSTPART;
    uint8_t retFlag = PLDM_GET_FIRSTPART;
    uint8_t pldmType = PLDM_BASE;
    uint8_t retType = PLDM_BASE;

    memcpy(requestMsg.data() + hdrSize, &transferHandle,
           sizeof(transferHandle));
    memcpy(requestMsg.data() + sizeof(transferHandle) + hdrSize, &flag,
           sizeof(flag));
    memcpy(requestMsg.data() + sizeof(transferHandle) + sizeof(flag) + hdrSize,
           &pldmType, sizeof(pldmType));

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = decode_get_version_req(request, requestMsg.size() - hdrSize,
                                     &retTransferHandle, &retFlag, &retType);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(transferHandle, retTransferHandle);
    EXPECT_EQ(flag, retFlag);
    EXPECT_EQ(pldmType, retType);
}

TEST(GetPLDMVersion, testDecodeResponse)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_VERSION_RESP_BYTES>
        responseMsg{};
    uint32_t transferHandle = 0x0;
    uint32_t retTransferHandle = 0x0;
    uint8_t flag = PLDM_START_AND_END;
    uint8_t retFlag = PLDM_START_AND_END;
    uint8_t completionCode = 0;
    ver32_t version = {0xff, 0xff, 0xff, 0xff};
    ver32_t versionOut;
    uint8_t completion_code;

    memcpy(responseMsg.data() + sizeof(completionCode) + hdrSize,
           &transferHandle, sizeof(transferHandle));
    memcpy(responseMsg.data() + sizeof(completionCode) +
               sizeof(transferHandle) + hdrSize,
           &flag, sizeof(flag));
    memcpy(responseMsg.data() + sizeof(completionCode) +
               sizeof(transferHandle) + sizeof(flag) + hdrSize,
           &version, sizeof(version));

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_version_resp(response, responseMsg.size() - hdrSize,
                                      &completion_code, &retTransferHandle,
                                      &retFlag, &versionOut);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(transferHandle, retTransferHandle);
    EXPECT_EQ(flag, retFlag);

    EXPECT_EQ(versionOut.major, version.major);
    EXPECT_EQ(versionOut.minor, version.minor);
    EXPECT_EQ(versionOut.update, version.update);
    EXPECT_EQ(versionOut.alpha, version.alpha);
}

TEST(GetTID, testEncodeRequest)
{
    pldm_msg request{};

    auto rc = encode_get_tid_req(0, &request);
    ASSERT_EQ(rc, PLDM_SUCCESS);
}

TEST(GetTID, testEncodeResponse)
{
    uint8_t completionCode = 0;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_TID_RESP_BYTES>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    uint8_t tid = 1;

    auto rc = encode_get_tid_resp(0, PLDM_SUCCESS, tid, response);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    uint8_t* payload = response->payload;
    EXPECT_EQ(completionCode, payload[0]);
    EXPECT_EQ(1, payload[sizeof(completionCode)]);
}

TEST(GetTID, testDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_TID_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;

    uint8_t tid;
    uint8_t completion_code;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_tid_resp(response, responseMsg.size() - hdrSize,
                                  &completion_code, &tid);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completion_code, PLDM_SUCCESS);
    EXPECT_EQ(tid, 1);
}

TEST(DecodeMultipartReceiveRequest, testDecodeRequestPass)
{
    constexpr uint8_t kPldmType = PLDM_BASE;
    constexpr uint8_t kFlag = PLDM_XFER_FIRST_PART;
    constexpr uint32_t kTransferCtx = 0x01;
    constexpr uint32_t kTransferHandle = 0x10;
    constexpr uint32_t kSectionOffset = 0x0;
    constexpr uint32_t kSectionLength = 0x10;

    PLDM_MSG_DEFINE_P(msg, PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    // Header values don't matter for this test.
    rc = pldm_msgbuf_init_errno(buf, PLDM_MULTIPART_RECEIVE_REQ_BYTES,
                                msg->payload, PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    ASSERT_EQ(rc, 0);
    pldm_msgbuf_insert_uint8(buf, kPldmType);
    pldm_msgbuf_insert_uint8(buf, kFlag);
    pldm_msgbuf_insert_uint32(buf, kTransferCtx);
    pldm_msgbuf_insert_uint32(buf, kTransferHandle);
    pldm_msgbuf_insert_uint32(buf, kSectionOffset);
    pldm_msgbuf_insert_uint32(buf, kSectionLength);
    rc = pldm_msgbuf_complete(buf);
    ASSERT_EQ(rc, 0);

    uint8_t pldm_type;
    uint8_t flag;
    uint32_t transfer_ctx;
    uint32_t transfer_handle;
    uint32_t section_offset;
    uint32_t section_length;
    rc = decode_multipart_receive_req(
        msg, PLDM_MULTIPART_RECEIVE_REQ_BYTES, &pldm_type, &flag, &transfer_ctx,
        &transfer_handle, &section_offset, &section_length);

    ASSERT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(pldm_type, kPldmType);
    EXPECT_EQ(flag, kFlag);
    EXPECT_EQ(transfer_ctx, kTransferCtx);
    EXPECT_EQ(transfer_handle, kTransferHandle);
    EXPECT_EQ(section_offset, kSectionOffset);
    EXPECT_EQ(section_length, kSectionLength);
}

TEST(DecodeMultipartReceiveRequest, testDecodeRequestFailNullData)
{
    EXPECT_EQ(decode_multipart_receive_req(NULL, 0, NULL, NULL, NULL, NULL,
                                           NULL, NULL),
              PLDM_ERROR_INVALID_DATA);
}

TEST(DecodeMultipartReceiveRequest, testDecodeRequestFailBadLength)
{
    PLDM_MSG_DEFINE_P(msg, PLDM_MULTIPART_RECEIVE_REQ_BYTES + 1);
    uint8_t pldm_type;
    uint8_t flag;
    uint32_t transfer_ctx;
    uint32_t transfer_handle;
    uint32_t section_offset;
    uint32_t section_length;

    memset(msg, 0, PLDM_MSG_SIZE(PLDM_MULTIPART_RECEIVE_REQ_BYTES + 1));
    EXPECT_EQ(decode_multipart_receive_req(
                  msg, PLDM_MULTIPART_RECEIVE_REQ_BYTES + 1, &pldm_type, &flag,
                  &transfer_ctx, &transfer_handle, &section_offset,
                  &section_length),
              PLDM_ERROR_INVALID_DATA);
}

TEST(DecodeMultipartReceiveRequest, testDecodeRequestFailBadPldmType)
{
    constexpr uint8_t kPldmType = 0xff;
    constexpr uint8_t kFlag = PLDM_XFER_FIRST_PART;

    PLDM_MSG_DEFINE_P(msg, PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    // Header values don't matter for this test.
    rc = pldm_msgbuf_init_errno(buf, PLDM_MULTIPART_RECEIVE_REQ_BYTES,
                                msg->payload, PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    ASSERT_EQ(rc, 0);
    pldm_msgbuf_insert_uint8(buf, kPldmType);
    pldm_msgbuf_insert_uint8(buf, kFlag);
    rc = pldm_msgbuf_complete(buf);
    ASSERT_EQ(rc, 0);

    uint8_t pldm_type;
    uint8_t flag;
    uint32_t transfer_ctx;
    uint32_t transfer_handle;
    uint32_t section_offset;
    uint32_t section_length;

    EXPECT_EQ(decode_multipart_receive_req(
                  msg, PLDM_MULTIPART_RECEIVE_REQ_BYTES, &pldm_type, &flag,
                  &transfer_ctx, &transfer_handle, &section_offset,
                  &section_length),
              PLDM_ERROR_INVALID_PLDM_TYPE);
}

TEST(DecodeMultipartReceiveRequest, testDecodeRequestFailBadTransferFlag)
{
    constexpr uint8_t kPldmType = PLDM_BASE;
    constexpr uint8_t kFlag = PLDM_XFER_CURRENT_PART + 0x10;

    PLDM_MSG_DEFINE_P(msg, PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    // Header values don't matter for this test.
    rc = pldm_msgbuf_init_errno(buf, PLDM_MULTIPART_RECEIVE_REQ_BYTES,
                                msg->payload, PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    ASSERT_EQ(rc, 0);
    pldm_msgbuf_insert_uint8(buf, kPldmType);
    pldm_msgbuf_insert_uint8(buf, kFlag);
    rc = pldm_msgbuf_complete(buf);
    ASSERT_EQ(rc, 0);

    uint8_t pldm_type;
    uint8_t flag;
    uint32_t transfer_ctx;
    uint32_t transfer_handle;
    uint32_t section_offset;
    uint32_t section_length;

    EXPECT_EQ(decode_multipart_receive_req(
                  msg, PLDM_MULTIPART_RECEIVE_REQ_BYTES, &pldm_type, &flag,
                  &transfer_ctx, &transfer_handle, &section_offset,
                  &section_length),
              PLDM_ERROR_UNEXPECTED_TRANSFER_FLAG_OPERATION);
}

TEST(DecodeMultipartReceiveRequest, testDecodeRequestFailBadHandle)
{
    constexpr uint8_t kPldmType = PLDM_BASE;
    constexpr uint8_t kFlag = PLDM_XFER_NEXT_PART;
    constexpr uint32_t kTransferCtx = 0x01;
    constexpr uint32_t kTransferHandle = 0x0;
    constexpr uint32_t kSectionOffset = 0x100;

    PLDM_MSG_DEFINE_P(msg, PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    // Header values don't matter for this test.
    rc = pldm_msgbuf_init_errno(buf, PLDM_MULTIPART_RECEIVE_REQ_BYTES,
                                msg->payload, PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    ASSERT_EQ(rc, 0);
    pldm_msgbuf_insert_uint8(buf, kPldmType);
    pldm_msgbuf_insert_uint8(buf, kFlag);
    pldm_msgbuf_insert_uint32(buf, kTransferCtx);
    pldm_msgbuf_insert_uint32(buf, kTransferHandle);
    pldm_msgbuf_insert_uint32(buf, kSectionOffset);
    rc = pldm_msgbuf_complete(buf);
    ASSERT_EQ(rc, 0);

    uint8_t pldm_type;
    uint8_t flag;
    uint32_t transfer_ctx;
    uint32_t transfer_handle;
    uint32_t section_offset;
    uint32_t section_length;
    EXPECT_EQ(decode_multipart_receive_req(
                  msg, PLDM_MULTIPART_RECEIVE_REQ_BYTES, &pldm_type, &flag,
                  &transfer_ctx, &transfer_handle, &section_offset,
                  &section_length),
              PLDM_ERROR_INVALID_DATA);
}

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeMultipartReceiveRequest, GoodTest)
{
    uint8_t instance_id = 0;

    const struct pldm_base_multipart_receive_req req_data = {
        PLDM_BASE, PLDM_XFER_FIRST_PART, 0x01, 0x10, 0x00, 0x10};

    static constexpr const size_t requestMsgLength =
        PLDM_MULTIPART_RECEIVE_REQ_BYTES;

    std::array<uint8_t, requestMsgLength> requestMsg = {
        PLDM_BASE, PLDM_XFER_FIRST_PART,
        0x01,      0x00,
        0x00,      0x00,
        0x10,      0x00,
        0x00,      0x00,
        0x00,      0x00,
        0x00,      0x00,
        0x10,      0x00,
        0x00,      0x00};

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = requestMsgLength;
    auto rc = encode_pldm_base_multipart_receive_req(
        instance_id, &req_data, requestPtr, &payload_length);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(
        0, memcmp(requestPtr->payload, requestMsg.data(), sizeof(requestMsg)));
    EXPECT_EQ(payload_length, requestMsgLength);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeMultipartReceiveRequest, BadTestUnAllocatedPtrParams)
{
    uint8_t instance_id = 0;
    int rc;

    const struct pldm_base_multipart_receive_req req_data = {
        PLDM_BASE, PLDM_XFER_FIRST_PART, 0x01, 0x10, 0x00, 0x10};

    static constexpr const size_t requestMsgLength =
        PLDM_MULTIPART_RECEIVE_REQ_BYTES;

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = requestMsgLength;
    rc = encode_pldm_base_multipart_receive_req(instance_id, nullptr,
                                                requestPtr, &payload_length);
    EXPECT_EQ(rc, -EINVAL);

    rc = encode_pldm_base_multipart_receive_req(instance_id, &req_data, nullptr,
                                                &payload_length);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeMultipartReceiveRequest, BadTestInvalidExpectedOutputMsgLength)
{
    uint8_t instance_id = 0;
    int rc;

    const struct pldm_base_multipart_receive_req req_data = {
        PLDM_BASE, PLDM_XFER_FIRST_PART, 0x01, 0x10, 0x00, 0x10};

    static constexpr const size_t requestMsgLength =
        PLDM_MULTIPART_RECEIVE_REQ_BYTES;

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = 1;
    rc = encode_pldm_base_multipart_receive_req(instance_id, &req_data,
                                                requestPtr, &payload_length);
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeMultipartReceiveResponse, GoodTest)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transferFlag = PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_END;
    uint32_t nextDataTransferHandle = 0x15;
    static constexpr const uint32_t dataLength = 9;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint32_t dataIntegrityChecksum = 0x3C;

    struct pldm_base_multipart_receive_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length =
        PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES + dataLength +
        sizeof(dataIntegrityChecksum);
    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completionCode);
    pldm_msgbuf_insert_uint8(buf, transferFlag);
    pldm_msgbuf_insert_uint32(buf, nextDataTransferHandle);
    pldm_msgbuf_insert_uint32(buf, dataLength);
    rc = pldm_msgbuf_insert_array_uint8(buf, dataLength, data.data(),
                                        dataLength);
    EXPECT_EQ(rc, 0);
    pldm_msgbuf_insert_uint32(buf, dataIntegrityChecksum);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    uint32_t respDataIntegrityChecksum = 0;

    rc = decode_pldm_base_multipart_receive_resp(
        responseMsg, payload_length, &resp_data, &respDataIntegrityChecksum);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(resp_data.completion_code, completionCode);
    EXPECT_EQ(resp_data.transfer_flag, transferFlag);
    EXPECT_EQ(resp_data.next_transfer_handle, nextDataTransferHandle);
    EXPECT_EQ(resp_data.data.length, dataLength);
    EXPECT_EQ(0,
              memcmp(data.data(), resp_data.data.ptr, resp_data.data.length));
    EXPECT_EQ(respDataIntegrityChecksum, dataIntegrityChecksum);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeMultipartReceiveResponse, BadTestUnAllocatedPtrParams)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transferFlag = PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_END;
    uint32_t nextDataTransferHandle = 0x15;
    static constexpr const uint32_t dataLength = 9;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint32_t dataIntegrityChecksum = 0x3C;

    struct pldm_base_multipart_receive_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length =
        PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES + dataLength +
        sizeof(dataIntegrityChecksum);
    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completionCode);
    pldm_msgbuf_insert_uint8(buf, transferFlag);
    pldm_msgbuf_insert_uint32(buf, nextDataTransferHandle);
    pldm_msgbuf_insert_uint32(buf, dataLength);
    rc = pldm_msgbuf_insert_array_uint8(buf, dataLength, data.data(),
                                        dataLength);
    EXPECT_EQ(rc, 0);
    pldm_msgbuf_insert_uint32(buf, dataIntegrityChecksum);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    uint32_t respDataIntegrityChecksum = 0;

    rc = decode_pldm_base_multipart_receive_resp(
        nullptr, payload_length, &resp_data, &respDataIntegrityChecksum);

    EXPECT_EQ(rc, -EINVAL);

    rc = decode_pldm_base_multipart_receive_resp(
        responseMsg, payload_length, nullptr, &respDataIntegrityChecksum);

    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeMultipartReceiveResponse, BadTestInvalidExpectedInputMsgLength)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transferFlag = PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_END;
    uint32_t nextDataTransferHandle = 0x15;
    static constexpr const uint32_t dataLength = 9;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint32_t dataIntegrityChecksum = 0x3C;

    struct pldm_base_multipart_receive_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length =
        PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES + dataLength +
        sizeof(dataIntegrityChecksum);
    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completionCode);
    pldm_msgbuf_insert_uint8(buf, transferFlag);
    pldm_msgbuf_insert_uint32(buf, nextDataTransferHandle);
    pldm_msgbuf_insert_uint32(buf, dataLength);
    rc = pldm_msgbuf_insert_array_uint8(buf, dataLength, data.data(),
                                        dataLength);
    EXPECT_EQ(rc, 0);
    pldm_msgbuf_insert_uint32(buf, dataIntegrityChecksum);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    uint32_t respDataIntegrityChecksum = 0;

    rc = decode_pldm_base_multipart_receive_resp(responseMsg, 0, &resp_data,
                                                 &respDataIntegrityChecksum);

    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeMultipartReceiveResponse, BadTestRedundantCheckSum)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transferFlag =
        PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_ACK_COMPLETION;
    uint32_t nextDataTransferHandle = 0x00;
    static constexpr const uint32_t dataLength = 0;
    uint32_t dataIntegrityChecksum = 0x2c;

    struct pldm_base_multipart_receive_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    /*
     * Data field is omitted in a response with ACKNOWLEDGE_COMPLETION
     * TransferFlag. Intentionally insert a DataIntegrityChecksum field to the
     * response message
     */

    static constexpr const size_t payload_length =
        PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES + dataLength +
        sizeof(dataIntegrityChecksum);
    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completionCode);
    pldm_msgbuf_insert_uint8(buf, transferFlag);
    pldm_msgbuf_insert_uint32(buf, nextDataTransferHandle);
    pldm_msgbuf_insert_uint32(buf, dataLength);
    pldm_msgbuf_insert_uint32(buf, dataIntegrityChecksum);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    uint32_t respDataIntegrityChecksum = 0;

    rc = decode_pldm_base_multipart_receive_resp(
        responseMsg, payload_length, &resp_data, &respDataIntegrityChecksum);

    /*
     * pldm_msgbuf_complete_consumed() returns -EBADMSG as
     * decode_pldm_base_multipart_receive_resp() didn't consume all the input
     * message buffer.
     */
    EXPECT_EQ(rc, -EBADMSG);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeMultipartReceiveResponse, BadTestMissingCheckSum)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transferFlag = PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_END;
    uint32_t nextDataTransferHandle = 0x00;
    static constexpr const uint32_t dataLength = 9;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    struct pldm_base_multipart_receive_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    /*
     * Intentionally do not insert DataIntegrityChecksum field to the response
     * message
     */
    static constexpr const size_t payload_length =
        PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES + dataLength;
    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completionCode);
    pldm_msgbuf_insert_uint8(buf, transferFlag);
    pldm_msgbuf_insert_uint32(buf, nextDataTransferHandle);
    pldm_msgbuf_insert_uint32(buf, dataLength);
    rc = pldm_msgbuf_insert_array_uint8(buf, dataLength, data.data(),
                                        dataLength);
    EXPECT_EQ(rc, 0);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    uint32_t respDataIntegrityChecksum = 0;

    rc = decode_pldm_base_multipart_receive_resp(
        responseMsg, payload_length, &resp_data, &respDataIntegrityChecksum);

    /*
     * pldm_msgbuf_extract_p() returns -EOVERFLOW as
     * decode_pldm_base_multipart_receive_resp() tried to consume more than
     * the expected input message buffer.
     */
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeMultipartReceiveResponse, GoodTestWithChecksum)
{
    uint8_t instance_id = 0;
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transferFlag =
        PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_START_AND_END;
    uint32_t nextDataTransferHandle = 0x15;
    static constexpr const uint32_t dataLength = 9;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint32_t dataIntegrityChecksum = 0x3C;
    static constexpr const size_t responseMsgLength =
        PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES + dataLength +
        sizeof(dataIntegrityChecksum);
    size_t payload_length = responseMsgLength;

    struct variable_field payload = {data.data(), dataLength};
    struct pldm_base_multipart_receive_resp resp_data = {
        completionCode, transferFlag, nextDataTransferHandle, payload};
    std::array<uint8_t, responseMsgLength> responseMsg = {
        completionCode,
        transferFlag,
        0x15, // nextDataTransferHandle
        0x00,
        0x00,
        0x00,
        0x09, // dataLength
        0x00,
        0x00,
        0x00,
        0x1, // data
        0x2,
        0x3,
        0x4,
        0x5,
        0x6,
        0x7,
        0x8,
        0x9,
        0x3c, // dataIntegrityChecksum
        0x00,
        0x00,
        0x00};

    PLDM_MSG_DEFINE_P(responsePtr, responseMsgLength);
    int rc;

    rc = encode_base_multipart_receive_resp(instance_id, &resp_data,
                                            dataIntegrityChecksum, responsePtr,
                                            &payload_length);

    ASSERT_EQ(0, rc);
    EXPECT_EQ(0, memcmp(responsePtr->payload, responseMsg.data(),
                        sizeof(responseMsg)));
    EXPECT_EQ(payload_length, responseMsgLength);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeMultipartReceiveResponse, GoodTestWithoutChecksum)
{
    uint8_t instance_id = 0;
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transferFlag =
        PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_ACK_COMPLETION;
    uint32_t nextDataTransferHandle = 0;
    static constexpr const uint32_t dataLength = 0;
    static constexpr const size_t responseMsgLength =
        PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES + dataLength;
    size_t payload_length = responseMsgLength;

    struct variable_field payload = {nullptr, dataLength};
    struct pldm_base_multipart_receive_resp resp_data = {
        completionCode, transferFlag, nextDataTransferHandle, payload};
    std::array<uint8_t, responseMsgLength> responseMsg = {
        completionCode, transferFlag,
        0x00, // nextDataTransferHandle
        0x00,           0x00,         0x00,
        0x00, // dataLength
        0x00,           0x00,         0x00};

    PLDM_MSG_DEFINE_P(responsePtr, responseMsgLength);
    int rc;

    rc = encode_base_multipart_receive_resp(instance_id, &resp_data, 0,
                                            responsePtr, &payload_length);

    ASSERT_EQ(0, rc);
    EXPECT_EQ(0, memcmp(responsePtr->payload, responseMsg.data(),
                        sizeof(responseMsg)));
    EXPECT_EQ(payload_length, responseMsgLength);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeMultipartReceiveResponse, GoodTestCompletionCode)
{
    uint8_t instance_id = 0;
    uint8_t completionCode = PLDM_MULTIPART_RECEIVE_NEGOTIATION_INCOMPLETE;
    uint8_t transferFlag = PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_START;
    uint32_t nextDataTransferHandle = 0x16;
    static constexpr const uint32_t dataLength = 9;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    static constexpr const size_t responseMsgLength =
        PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES + dataLength;
    size_t payload_length = responseMsgLength;

    struct variable_field payload = {data.data(), dataLength};
    struct pldm_base_multipart_receive_resp resp_data = {
        completionCode, transferFlag, nextDataTransferHandle, payload};
    std::array<uint8_t, 1> responseMsg = {completionCode};

    PLDM_MSG_DEFINE_P(responsePtr, responseMsgLength);
    int rc;

    rc = encode_base_multipart_receive_resp(instance_id, &resp_data, 0,
                                            responsePtr, &payload_length);

    ASSERT_EQ(0, rc);
    EXPECT_EQ(0, memcmp(responsePtr->payload, responseMsg.data(),
                        sizeof(responseMsg)));
    EXPECT_EQ(payload_length, 1ul);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeMultipartReceiveResponse, BadTestUnAllocatedParams)
{
    uint8_t instance_id = 0;
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transferFlag =
        PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_START_AND_END;
    uint32_t nextDataTransferHandle = 0x15;
    static constexpr const uint32_t dataLength = 9;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint32_t dataIntegrityChecksum = 0x3C;
    static constexpr const size_t responseMsgLength =
        PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES + dataLength +
        sizeof(dataIntegrityChecksum);
    size_t payload_length = responseMsgLength;

    struct variable_field payload = {data.data(), dataLength};
    struct pldm_base_multipart_receive_resp resp_data = {
        completionCode, transferFlag, nextDataTransferHandle, payload};

    PLDM_MSG_DEFINE_P(responsePtr, responseMsgLength);
    int rc;

    rc = encode_base_multipart_receive_resp(instance_id, nullptr,
                                            dataIntegrityChecksum, responsePtr,
                                            &payload_length);
    EXPECT_EQ(rc, -EINVAL);

    rc = encode_base_multipart_receive_resp(instance_id, &resp_data,
                                            dataIntegrityChecksum, nullptr,
                                            &payload_length);
    EXPECT_EQ(rc, -EINVAL);

    rc = encode_base_multipart_receive_resp(
        instance_id, &resp_data, dataIntegrityChecksum, responsePtr, nullptr);
    EXPECT_EQ(rc, -EINVAL);

    resp_data.data.ptr = nullptr;
    rc = encode_base_multipart_receive_resp(instance_id, &resp_data,
                                            dataIntegrityChecksum, responsePtr,
                                            &payload_length);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeMultipartReceiveResponse, BadTestInvalidExpectedOutputMsgLength)
{
    uint8_t instance_id = 0;
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t transferFlag = PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_START;
    uint32_t nextDataTransferHandle = 0x16;
    static constexpr const uint32_t dataLength = 9;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    static constexpr const size_t responseMsgLength =
        PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES;
    size_t payload_length = responseMsgLength;

    struct variable_field payload = {data.data(), dataLength};
    struct pldm_base_multipart_receive_resp resp_data = {
        completionCode, transferFlag, nextDataTransferHandle, payload};

    PLDM_MSG_DEFINE_P(responsePtr, responseMsgLength);
    int rc;

    rc = encode_base_multipart_receive_resp(instance_id, &resp_data, 0,
                                            responsePtr, &payload_length);
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

TEST(CcOnlyResponse, testEncode)
{
    struct pldm_msg responseMsg;

    auto rc =
        encode_cc_only_resp(0 /*instance id*/, 1 /*pldm type*/, 2 /*command*/,
                            3 /*completion code*/, &responseMsg);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto p = reinterpret_cast<uint8_t*>(&responseMsg);
    EXPECT_THAT(std::vector<uint8_t>(p, p + sizeof(responseMsg)),
                ElementsAreArray({0, 1, 2, 3}));

    rc = encode_cc_only_resp(PLDM_INSTANCE_MAX + 1, 1, 2, 3, &responseMsg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_cc_only_resp(0, 1, 2, 3, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(SetTID, testGoodEncodeRequest)
{
    uint8_t instanceId = 0;
    uint8_t tid = 0x01;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + sizeof(tid)> requestMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_set_tid_req(instanceId, tid, request);
    ASSERT_EQ(rc, PLDM_SUCCESS);

    EXPECT_EQ(request->hdr.command, PLDM_SET_TID);
    EXPECT_EQ(request->hdr.type, PLDM_BASE);
    EXPECT_EQ(request->hdr.request, 1);
    EXPECT_EQ(request->hdr.datagram, 0);
    EXPECT_EQ(request->hdr.instance_id, instanceId);
    EXPECT_EQ(0, memcmp(request->payload, &tid, sizeof(tid)));
}

TEST(SetTID, testBadEncodeRequest)
{
    uint8_t tid = 0x01;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + sizeof(tid)> requestMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_set_tid_req(0, tid, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_set_tid_req(0, 0, request);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_set_tid_req(0, 0xff, request);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

#if HAVE_LIBPLDM_API_TESTING
TEST(SetTID, testGoodDecodeRequest)
{
    uint8_t tid = 0x01;
    uint8_t tidOut = 0x00;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + sizeof(tid)> requestMsg{};

    requestMsg[sizeof(pldm_msg_hdr)] = tid;

    pldm_msg* request = new (requestMsg.data()) pldm_msg;
    auto rc = decode_set_tid_req(
        request, requestMsg.size() - sizeof(pldm_msg_hdr), &tidOut);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(tid, tidOut);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(SetTID, testBadDecodeRequestMsg)
{
    uint8_t tid = 0x01;
    std::array<uint8_t, hdrSize + PLDM_SET_TID_REQ_BYTES> requestMsg{};

    auto rc = decode_set_tid_req(
        nullptr, requestMsg.size() - sizeof(pldm_msg_hdr), &tid);

    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(SetTID, testBadDecodeRequestTid)
{
    std::array<uint8_t, hdrSize + PLDM_SET_TID_REQ_BYTES> requestMsg{};
    pldm_msg* request = new (requestMsg.data()) pldm_msg;

    auto rc = decode_set_tid_req(
        request, requestMsg.size() - sizeof(pldm_msg_hdr), nullptr);

    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(SetTID, testBadDecodeRequestMsgSize)
{
    std::array<uint8_t, hdrSize + PLDM_SET_TID_REQ_BYTES> requestMsg{};
    pldm_msg* request = new (requestMsg.data()) pldm_msg;

    auto rc = decode_set_tid_req(request, -1, nullptr);

    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PldmMsgHdr, correlateSuccess)
{
    static const struct pldm_msg_hdr req = {
        .instance_id = 0,
        .reserved = 0,
        .datagram = 0,
        .request = 1,
        .type = 0,
        .header_ver = 1,
        .command = 0x01,
    };
    static const struct pldm_msg_hdr resp = {
        .instance_id = 0,
        .reserved = 0,
        .datagram = 0,
        .request = 0,
        .type = 0,
        .header_ver = 1,
        .command = 0x01,
    };

    ASSERT_EQ(pldm_msg_hdr_correlate_response(&req, &resp), true);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PldmMsgHdr, correlateFailInstanceID)
{
    static const struct pldm_msg_hdr req = {
        .instance_id = 0,
        .reserved = 0,
        .datagram = 0,
        .request = 1,
        .type = 0,
        .header_ver = 1,
        .command = 0x01,
    };
    static const struct pldm_msg_hdr resp = {
        .instance_id = 1,
        .reserved = 0,
        .datagram = 0,
        .request = 0,
        .type = 0,
        .header_ver = 1,
        .command = 0x01,
    };

    ASSERT_EQ(pldm_msg_hdr_correlate_response(&req, &resp), false);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PldmMsgHdr, correlateFailRequest)
{
    static const struct pldm_msg_hdr req = {
        .instance_id = 0,
        .reserved = 0,
        .datagram = 0,
        .request = 1,
        .type = 0,
        .header_ver = 1,
        .command = 0x01,
    };
    static const struct pldm_msg_hdr resp = {
        .instance_id = 0,
        .reserved = 0,
        .datagram = 0,
        .request = 1,
        .type = 0,
        .header_ver = 1,
        .command = 0x01,
    };

    ASSERT_EQ(pldm_msg_hdr_correlate_response(&req, &resp), false);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PldmMsgHdr, correlateFailType)
{
    static const struct pldm_msg_hdr req = {
        .instance_id = 0,
        .reserved = 0,
        .datagram = 0,
        .request = 1,
        .type = 0,
        .header_ver = 1,
        .command = 0x01,
    };
    static const struct pldm_msg_hdr resp = {
        .instance_id = 0,
        .reserved = 0,
        .datagram = 0,
        .request = 0,
        .type = 1,
        .header_ver = 1,
        .command = 0x01,
    };

    ASSERT_EQ(pldm_msg_hdr_correlate_response(&req, &resp), false);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PldmMsgHdr, correlateFailCommand)
{
    static const struct pldm_msg_hdr req = {
        .instance_id = 0,
        .reserved = 0,
        .datagram = 0,
        .request = 1,
        .type = 0,
        .header_ver = 1,
        .command = 0x01,
    };
    static const struct pldm_msg_hdr resp = {
        .instance_id = 0,
        .reserved = 0,
        .datagram = 0,
        .request = 0,
        .type = 0,
        .header_ver = 1,
        .command = 0x02,
    };

    ASSERT_EQ(pldm_msg_hdr_correlate_response(&req, &resp), false);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PldmMsgHdr, correlateFailRequestIsResponse)
{
    static const struct pldm_msg_hdr req = {
        .instance_id = 0,
        .reserved = 0,
        .datagram = 0,
        .request = 0,
        .type = 0,
        .header_ver = 1,
        .command = 0x01,
    };
    static const struct pldm_msg_hdr resp = {
        .instance_id = 0,
        .reserved = 0,
        .datagram = 0,
        .request = 0,
        .type = 0,
        .header_ver = 1,
        .command = 0x02,
    };

    ASSERT_EQ(pldm_msg_hdr_correlate_response(&req, &resp), false);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeNegotiateTransferParamsRequest, GoodTest)
{
    uint8_t instance_id = 0;

    const struct pldm_base_negotiate_transfer_params_req req_data = {
        0x0001, // BE 256
        {{0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x81}}};

    static constexpr const size_t requestMsgLength =
        PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES;

    std::array<uint8_t, requestMsgLength> requestMsg = {
        0x01, 0x00, // requester_part_size = 256
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x81}; // requester_protocol_support =
                                 // PLDM_BASE & PLDM_FILE

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = requestMsgLength;
    auto rc = encode_pldm_base_negotiate_transfer_params_req(
        instance_id, &req_data, requestPtr, &payload_length);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(
        0, memcmp(requestPtr->payload, requestMsg.data(), sizeof(requestMsg)));
    EXPECT_EQ(payload_length, requestMsgLength);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeNegotiateTransferParamsRequest, BadTestUnAllocatedPtrParams)
{
    int rc;
    uint8_t instance_id = 0;
    const struct pldm_base_negotiate_transfer_params_req req_data = {
        0x0001, // BE 256
        {{0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x81}}};

    static constexpr const size_t requestMsgLength =
        PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES;

    PLDM_MSG_DEFINE_P(requestPtr, requestMsgLength);
    size_t payload_length = requestMsgLength;
    rc = encode_pldm_base_negotiate_transfer_params_req(
        instance_id, nullptr, requestPtr, &payload_length);
    EXPECT_EQ(rc, -EINVAL);

    rc = encode_pldm_base_negotiate_transfer_params_req(
        instance_id, &req_data, nullptr, &payload_length);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(EncodeNegotiateTransferParamsRequest,
     BadTestInvalidExpectedOutputMsgLength)
{
    int rc;
    uint8_t instance_id = 0;
    const struct pldm_base_negotiate_transfer_params_req req_data = {
        0x0001, // BE 256
        {{0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x81}}};

    PLDM_MSG_DEFINE_P(requestPtr,
                      PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES);

    size_t payload_length = 1;
    rc = encode_pldm_base_negotiate_transfer_params_req(
        instance_id, &req_data, requestPtr, &payload_length);
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeNegotiateTransferParamsResponse, GoodTest)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint16_t responderPartSize = 128;
    std::array<uint8_t, 8> responderProtocolSupport = {0x00, 0x00, 0x00, 0x00,
                                                       0x00, 0x00, 0x00, 0x81};

    struct pldm_base_negotiate_transfer_params_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length =
        PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES;
    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completionCode);
    pldm_msgbuf_insert_uint16(buf, responderPartSize);
    rc = pldm_msgbuf_insert_array_uint8(
        buf, sizeof(resp_data.responder_protocol_support),
        responderProtocolSupport.data(),
        sizeof(resp_data.responder_protocol_support));
    EXPECT_EQ(rc, 0);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_base_negotiate_transfer_params_resp(
        responseMsg, payload_length, &resp_data);

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(resp_data.completion_code, completionCode);
    EXPECT_EQ(resp_data.responder_part_size, responderPartSize);
    EXPECT_EQ(0, memcmp(responderProtocolSupport.data(),
                        resp_data.responder_protocol_support,
                        sizeof(resp_data.responder_protocol_support)));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeNegotiateTransferParamsResponse, BadTestUnAllocatedPtrParams)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint16_t responderPartSize = 128;
    std::array<uint8_t, 8> responderProtocolSupport = {0x00, 0x00, 0x00, 0x00,
                                                       0x00, 0x00, 0x00, 0x81};

    struct pldm_base_negotiate_transfer_params_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length =
        PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES;
    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completionCode);
    pldm_msgbuf_insert_uint16(buf, responderPartSize);
    rc = pldm_msgbuf_insert_array_uint8(
        buf, sizeof(resp_data.responder_protocol_support),
        responderProtocolSupport.data(),
        sizeof(resp_data.responder_protocol_support));
    EXPECT_EQ(rc, 0);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_base_negotiate_transfer_params_resp(
        nullptr, payload_length, &resp_data);

    EXPECT_EQ(rc, -EINVAL);

    rc = decode_pldm_base_negotiate_transfer_params_resp(
        responseMsg, payload_length, nullptr);

    EXPECT_EQ(rc, -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(DecodeNegotiateTransferParamsResponse,
     BadTestInvalidExpectedInputMsgLength)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint16_t responderPartSize = 128;
    std::array<uint8_t, 8> responderProtocolSupport = {0x00, 0x00, 0x00, 0x00,
                                                       0x00, 0x00, 0x00, 0x81};

    struct pldm_base_negotiate_transfer_params_resp resp_data = {};

    PLDM_MSGBUF_RW_DEFINE_P(buf);
    int rc;

    static constexpr const size_t payload_length =
        PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES;
    PLDM_MSG_DEFINE_P(responseMsg, payload_length);

    rc = pldm_msgbuf_init_errno(buf, 0, responseMsg->payload, payload_length);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, completionCode);
    pldm_msgbuf_insert_uint16(buf, responderPartSize);
    rc = pldm_msgbuf_insert_array_uint8(
        buf, sizeof(resp_data.responder_protocol_support),
        responderProtocolSupport.data(),
        sizeof(resp_data.responder_protocol_support));
    EXPECT_EQ(rc, 0);

    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    rc = decode_pldm_base_negotiate_transfer_params_resp(responseMsg, 0,
                                                         &resp_data);

    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateTransferParams, TestDecodeNegotiateTransferParamsReqPass)
{
    // Prepare a sample request message
    uint8_t instance_id = 0x0A;
    uint16_t requester_part_size = 1024; // 0x0400
    bitfield8_t requester_protocol_support[8];
    for (int i = 0; i < 8; ++i)
    {
        requester_protocol_support[i].byte = static_cast<uint8_t>(i + 1);
    }

    PLDM_MSG_DEFINE_P(request,
                      PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES);

    // Build the request using the an appropriate API
    struct pldm_base_negotiate_transfer_params_req req;
    req.requester_part_size = requester_part_size;
    memcpy(req.requester_protocol_support, requester_protocol_support,
           sizeof(requester_protocol_support));

    size_t req_payload_len = PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES;
    ASSERT_EQ(encode_pldm_base_negotiate_transfer_params_req(
                  instance_id, &req, request, &req_payload_len),
              0);

    struct pldm_base_negotiate_transfer_params_req decoded_req;
    size_t payload_len = PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES;

    // Successful decode
    int rc = decode_pldm_base_negotiate_transfer_params_req(
        request, payload_len, &decoded_req);
    ASSERT_EQ(rc, 0);
    EXPECT_EQ(decoded_req.requester_part_size, requester_part_size);
    for (int i = 0; i < 8; ++i)
    {
        EXPECT_EQ(decoded_req.requester_protocol_support[i].byte,
                  requester_protocol_support[i].byte);
    }
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateTransferParams, TestDecodeNegotiateTransferParamsReqFail)
{
    // Prepare a sample request message
    uint8_t instance_id = 0x0A;
    uint16_t requester_part_size = 1024; // 0x0400
    bitfield8_t requester_protocol_support[8];
    for (int i = 0; i < 8; ++i)
    {
        requester_protocol_support[i].byte = static_cast<uint8_t>(i + 1);
    }

    PLDM_MSG_DEFINE_P(request,
                      PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES);

    // Build the request using the an appropriate API
    struct pldm_base_negotiate_transfer_params_req req;
    req.requester_part_size = requester_part_size;
    memcpy(req.requester_protocol_support, requester_protocol_support,
           sizeof(requester_protocol_support));

    size_t req_payload_len = PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES;
    ASSERT_EQ(encode_pldm_base_negotiate_transfer_params_req(
                  instance_id, &req, request, &req_payload_len),
              0);

    struct pldm_base_negotiate_transfer_params_req decoded_req;
    size_t payload_len = PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES;
    int rc = 0;

    // Null arguments
    rc = decode_pldm_base_negotiate_transfer_params_req(nullptr, payload_len,
                                                        &decoded_req);
    EXPECT_EQ(rc, -EINVAL);
    rc = decode_pldm_base_negotiate_transfer_params_req(request, payload_len,
                                                        nullptr);
    EXPECT_EQ(rc, -EINVAL);

    // Incorrect payload length - too short
    rc = decode_pldm_base_negotiate_transfer_params_req(
        request, payload_len - 1, &decoded_req);
    EXPECT_EQ(rc, -EOVERFLOW);

    // Incorrect payload length - too long
    rc = decode_pldm_base_negotiate_transfer_params_req(
        request, payload_len + 1, &decoded_req);
    EXPECT_EQ(rc, -EBADMSG);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateTransferParams, TestEncodeNegotiateTransferParamsRespPass)
{
    // Prepare encode parameters for a successful response
    uint8_t instance_id = 0x0B;
    uint8_t completion_code_success = PLDM_SUCCESS;
    uint16_t responder_part_size = 2048; // 0x0800
    bitfield8_t responder_protocol_support[8];
    for (int i = 0; i < 8; ++i)
    {
        responder_protocol_support[i].byte = static_cast<uint8_t>(0xA0 + i);
    }

    struct pldm_base_negotiate_transfer_params_resp resp_params_success;
    resp_params_success.completion_code = completion_code_success;
    resp_params_success.responder_part_size = responder_part_size;
    memcpy(resp_params_success.responder_protocol_support,
           responder_protocol_support, sizeof(responder_protocol_support));

    PLDM_MSG_DEFINE_P(response,
                      PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES);
    size_t payload_len = PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES;

    // Encode success case
    int rc = encode_pldm_base_negotiate_transfer_params_resp(
        instance_id, &resp_params_success, response, &payload_len);
    ASSERT_EQ(rc, 0);
    EXPECT_EQ(response->hdr.request, PLDM_RESPONSE);
    EXPECT_EQ(response->hdr.instance_id, instance_id);
    EXPECT_EQ(response->hdr.type, PLDM_BASE);
    EXPECT_EQ(response->hdr.command, PLDM_NEGOTIATE_TRANSFER_PARAMETERS);

    // Verify the encoded response using the decode function
    struct pldm_base_negotiate_transfer_params_resp decoded_resp;
    payload_len = PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES;
    rc = decode_pldm_base_negotiate_transfer_params_resp(response, payload_len,
                                                         &decoded_resp);
    ASSERT_EQ(rc, 0);
    EXPECT_EQ(decoded_resp.completion_code, completion_code_success);
    EXPECT_EQ(decoded_resp.responder_part_size, responder_part_size);
    for (int i = 0; i < 8; ++i)
    {
        EXPECT_EQ(decoded_resp.responder_protocol_support[i].byte,
                  responder_protocol_support[i].byte);
    }
}

TEST(NegotiateTransferParams, TestEncodeNegotiateTransferParamsRespFail)
{
    // Prepare encode parameters
    uint8_t instance_id = 0x0B;
    uint8_t completion_code_success = PLDM_SUCCESS;
    uint16_t responder_part_size = 2048; // 0x0800
    bitfield8_t responder_protocol_support[8];
    for (int i = 0; i < 8; ++i)
    {
        responder_protocol_support[i].byte = static_cast<uint8_t>(0xA0 + i);
    }

    struct pldm_base_negotiate_transfer_params_resp resp_params_success;
    resp_params_success.completion_code = completion_code_success;
    resp_params_success.responder_part_size = responder_part_size;
    memcpy(resp_params_success.responder_protocol_support,
           responder_protocol_support, sizeof(responder_protocol_support));

    PLDM_MSG_DEFINE_P(response,
                      PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES);
    size_t payload_len = PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES;
    int rc = 0;

    // Null arguments
    rc = encode_pldm_base_negotiate_transfer_params_resp(
        instance_id, nullptr, response, &payload_len);
    EXPECT_EQ(rc, -EINVAL);

    payload_len = PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES;
    rc = encode_pldm_base_negotiate_transfer_params_resp(
        instance_id, &resp_params_success, nullptr, &payload_len);
    EXPECT_EQ(rc, -EINVAL);

    // Incorrect payload length
    payload_len = PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES - 1;
    rc = encode_pldm_base_negotiate_transfer_params_resp(
        instance_id, &resp_params_success, response, &payload_len);
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif
