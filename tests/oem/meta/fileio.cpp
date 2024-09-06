/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <endian.h>
#include <libpldm/oem/meta/file_io.h>

#include <cstdlib>
#include <new>

#include "msgbuf.h"

#include <gtest/gtest.h>

#ifdef LIBPLDM_API_TESTING
TEST(DecodeOemMetaFileIoWriteReq, testGoodDecodeRequest)
{
    constexpr const uint8_t postCode[4] = {0x93, 0xe0, 0x00, 0xea};
    struct pldm_msgbuf _ctx;
    struct pldm_msgbuf* ctx = &_ctx;
    int rc;

    constexpr size_t encodedPayloadLen =
        PLDM_OEM_META_FILE_IO_WRITE_REQ_MIN_LENGTH + sizeof(postCode);
    constexpr size_t encodedMsgLen = sizeof(pldm_msg_hdr) + encodedPayloadLen;
    alignas(pldm_msg) unsigned char encodedMsgBuf[encodedMsgLen] = {};
    auto* encodedMsg = new (encodedMsgBuf) pldm_msg;

    rc = pldm_msgbuf_init_errno(ctx, 0, encodedMsg->payload, encodedPayloadLen);
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(ctx, 0);
    pldm_msgbuf_insert_int32(ctx, sizeof(postCode));
    rc = pldm_msgbuf_insert_array_uint8(ctx, sizeof(postCode), postCode,
                                        sizeof(postCode));
    ASSERT_EQ(rc, 0);

    rc = pldm_msgbuf_destroy_consumed(ctx);
    ASSERT_EQ(rc, 0);

    constexpr size_t decodedReqLen =
        sizeof(struct pldm_oem_meta_file_io_write_req) + sizeof(postCode);
    alignas(pldm_oem_meta_file_io_write_req) unsigned char
        decodedReqBuf[decodedReqLen];
    auto* decodedReq = new (decodedReqBuf) pldm_oem_meta_file_io_write_req;
    auto* decodedReqData =
        static_cast<uint8_t*>(pldm_oem_meta_file_io_write_req_data(decodedReq));

    rc = decode_oem_meta_file_io_write_req(encodedMsg, encodedPayloadLen,
                                           decodedReq, decodedReqLen);
    ASSERT_EQ(rc, 0);

    EXPECT_EQ(decodedReq->handle, 0);
    ASSERT_EQ(decodedReq->length, sizeof(postCode));
    EXPECT_EQ(memcmp(decodedReqData, postCode, decodedReq->length), 0);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(DecodeOemMetaFileIoWriteReq, testInvalidFieldsDecodeRequest)
{
    struct pldm_msg msg = {};

    auto rc = decode_oem_meta_file_io_write_req(&msg, sizeof(msg), NULL, 0);
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(DecodeOemMetaFileIoWriteReq, testInvalidLengthDecodeRequest)
{
    struct pldm_oem_meta_file_io_write_req req = {};
    struct pldm_msg msg = {};

    auto rc = decode_oem_meta_file_io_write_req(&msg, 0, &req, sizeof(req));
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(DecodeOemMetaFileIoWriteReq, testInvalidDataRequest)
{
    struct pldm_oem_meta_file_io_write_req req = {};
    struct pldm_msg msg = {};
    int rc;

    rc = decode_oem_meta_file_io_write_req(
        &msg, PLDM_OEM_META_FILE_IO_WRITE_REQ_MIN_LENGTH - 1, &req,
        sizeof(req));
    EXPECT_EQ(rc, -EOVERFLOW);
}
#endif
