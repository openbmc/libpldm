#include <endian.h>
#include <libpldm/base.h>
#include <libpldm/entity.h>
#include <libpldm/platform.h>
#include <libpldm/pldm_types.h>

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "msgbuf.h"

#include <gtest/gtest.h>

constexpr auto hdrSize = sizeof(pldm_msg_hdr);

TEST(StateEffecterPdr, testIncorrectInvocations)
{
    struct state_effecter_possible_states possible_states{};
    struct pldm_state_effecter_pdr effecter{};
    size_t actual_size;
    int rc;

    /* effecter can't be NULL */
    rc = encode_state_effecter_pdr(NULL, 0, &possible_states, 1, &actual_size);
    EXPECT_EQ(rc, PLDM_ERROR);

    /* possible states size can't be NULL */
    rc = encode_state_effecter_pdr(&effecter, sizeof(effecter), NULL, 0,
                                   &actual_size);
    EXPECT_EQ(rc, PLDM_ERROR);

    /* possible states size can't be too large */
    rc = encode_state_effecter_pdr(&effecter, sizeof(effecter),
                                   &possible_states, SIZE_MAX, &actual_size);
    EXPECT_EQ(rc, PLDM_ERROR);

    /* actual size can't be NULL */
    rc = encode_state_effecter_pdr(&effecter, sizeof(effecter),
                                   &possible_states, 0, NULL);
    EXPECT_EQ(rc, PLDM_ERROR);

    /* allocation size can't be less than effecter size with possible states */
    rc = encode_state_effecter_pdr(&effecter, 0, &possible_states, 1,
                                   &actual_size);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    /* disallow mismatches between recorded possible state size and provided
     * possible state size */
    effecter.composite_effecter_count = 1;
    rc = encode_state_effecter_pdr(&effecter, sizeof(effecter),
                                   &possible_states, 1, &actual_size);
    EXPECT_EQ(rc, PLDM_ERROR);
    EXPECT_EQ(actual_size, 0);
}

TEST(StateEffecterPdr, testReasonableInvocations)
{
    struct state_effecter_possible_states possible_states{};
    struct pldm_state_effecter_pdr effecter{};
    size_t actual_size;
    int rc;

    /* Accept 0 possible states */
    rc = encode_state_effecter_pdr(&effecter, sizeof(effecter),
                                   &possible_states, 0, &actual_size);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(actual_size, sizeof(effecter) - sizeof(effecter.possible_states));
}

TEST(SetStateEffecterStates, testEncodeResponse)
{
    std::array<uint8_t,
               sizeof(pldm_msg_hdr) + PLDM_SET_STATE_EFFECTER_STATES_RESP_BYTES>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    uint8_t completionCode = 0;

    auto rc = encode_set_state_effecter_states_resp(0, PLDM_SUCCESS, response);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, response->payload[0]);
}

TEST(SetStateEffecterStates, testEncodeRequest)
{
    std::array<uint8_t,
               sizeof(pldm_msg_hdr) + PLDM_SET_STATE_EFFECTER_STATES_REQ_BYTES>
        requestMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    uint16_t effecterId = 0x0a;
    uint8_t compEffecterCnt = 0x2;
    std::array<set_effecter_state_field, 8> stateField{};
    stateField[0] = {PLDM_REQUEST_SET, 2};
    stateField[1] = {PLDM_REQUEST_SET, 3};

    auto rc = encode_set_state_effecter_states_req(
        0, effecterId, compEffecterCnt, stateField.data(), request);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(effecterId, request->payload[0]);
    EXPECT_EQ(compEffecterCnt, request->payload[sizeof(effecterId)]);
    EXPECT_EQ(stateField[0].set_request,
              request->payload[sizeof(effecterId) + sizeof(compEffecterCnt)]);
    EXPECT_EQ(stateField[0].effecter_state,
              request->payload[sizeof(effecterId) + sizeof(compEffecterCnt) +
                               sizeof(stateField[0].set_request)]);
    EXPECT_EQ(stateField[1].set_request,
              request->payload[sizeof(effecterId) + sizeof(compEffecterCnt) +
                               sizeof(stateField[0])]);
    EXPECT_EQ(stateField[1].effecter_state,
              request->payload[sizeof(effecterId) + sizeof(compEffecterCnt) +
                               sizeof(stateField[0]) +
                               sizeof(stateField[1].set_request)]);
}

TEST(SetStateEffecterStates, testGoodDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_SET_STATE_EFFECTER_STATES_RESP_BYTES>
        responseMsg{};

    uint8_t retcompletion_code = 0;

    responseMsg[hdrSize] = PLDM_SUCCESS;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_set_state_effecter_states_resp(
        response, responseMsg.size() - hdrSize, &retcompletion_code);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(PLDM_SUCCESS, retcompletion_code);
}

TEST(SetStateEffecterStates, testGoodDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_SET_STATE_EFFECTER_STATES_REQ_BYTES>
        requestMsg{};

    uint16_t effecterId = 0x32;
    uint16_t effecterIdLE = htole16(effecterId);
    uint8_t compEffecterCnt = 0x2;

    std::array<set_effecter_state_field, 8> stateField{};
    stateField[0] = {PLDM_REQUEST_SET, 3};
    stateField[1] = {PLDM_REQUEST_SET, 4};

    uint16_t retEffecterId = 0;
    uint8_t retCompEffecterCnt = 0;

    std::array<set_effecter_state_field, 8> retStateField{};

    memcpy(requestMsg.data() + hdrSize, &effecterIdLE, sizeof(effecterIdLE));
    memcpy(requestMsg.data() + sizeof(effecterIdLE) + hdrSize, &compEffecterCnt,
           sizeof(compEffecterCnt));
    memcpy(requestMsg.data() + sizeof(effecterIdLE) + sizeof(compEffecterCnt) +
               hdrSize,
           &stateField, sizeof(stateField));

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = decode_set_state_effecter_states_req(
        request, requestMsg.size() - hdrSize, &retEffecterId,
        &retCompEffecterCnt, retStateField.data());

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(effecterId, retEffecterId);
    EXPECT_EQ(retCompEffecterCnt, compEffecterCnt);
    EXPECT_EQ(retStateField[0].set_request, stateField[0].set_request);
    EXPECT_EQ(retStateField[0].effecter_state, stateField[0].effecter_state);
    EXPECT_EQ(retStateField[1].set_request, stateField[1].set_request);
    EXPECT_EQ(retStateField[1].effecter_state, stateField[1].effecter_state);
}

TEST(SetStateEffecterStates, testBadDecodeRequest)
{
    const struct pldm_msg* msg = NULL;

    auto rc = decode_set_state_effecter_states_req(msg, sizeof(*msg), NULL,
                                                   NULL, NULL);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(SetStateEffecterStates, testBadDecodeResponse)
{
    std::array<uint8_t, PLDM_SET_STATE_EFFECTER_STATES_RESP_BYTES>
        responseMsg{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_set_state_effecter_states_resp(response,
                                                    responseMsg.size(), NULL);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetPDR, testGoodEncodeResponse)
{
    uint8_t completionCode = 0;
    uint32_t nextRecordHndl = 0x12;
    uint32_t nextDataTransferHndl = 0x13;
    uint8_t transferFlag = PLDM_END;
    uint16_t respCnt = 0x5;
    std::vector<uint8_t> recordData{1, 2, 3, 4, 5};
    uint8_t transferCRC = 6;

    // + size of record data and transfer CRC
    std::vector<uint8_t> responseMsg(hdrSize + PLDM_GET_PDR_MIN_RESP_BYTES +
                                     recordData.size() + 1);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_get_pdr_resp(0, PLDM_SUCCESS, nextRecordHndl,
                                  nextDataTransferHndl, transferFlag, respCnt,
                                  recordData.data(), transferCRC, response);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    struct pldm_get_pdr_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_pdr_resp*>(response->payload);

    EXPECT_EQ(completionCode, resp->completion_code);
    EXPECT_EQ(nextRecordHndl, le32toh(resp->next_record_handle));
    EXPECT_EQ(nextDataTransferHndl, le32toh(resp->next_data_transfer_handle));
    EXPECT_EQ(transferFlag, resp->transfer_flag);
    EXPECT_EQ(respCnt, le16toh(resp->response_count));
    EXPECT_EQ(0,
              memcmp(recordData.data(), resp->record_data, recordData.size()));
    EXPECT_EQ(*(response->payload + sizeof(pldm_get_pdr_resp) - 1 +
                recordData.size()),
              transferCRC);

    transferFlag = PLDM_START_AND_END; // No CRC in this case
    responseMsg.resize(responseMsg.size() - sizeof(transferCRC));
    rc = encode_get_pdr_resp(0, PLDM_SUCCESS, nextRecordHndl,
                             nextDataTransferHndl, transferFlag, respCnt,
                             recordData.data(), transferCRC, response);
    EXPECT_EQ(rc, PLDM_SUCCESS);
}

TEST(GetPDR, testBadEncodeResponse)
{
    uint32_t nextRecordHndl = 0x12;
    uint32_t nextDataTransferHndl = 0x13;
    uint8_t transferFlag = PLDM_START_AND_END;
    uint16_t respCnt = 0x5;
    std::vector<uint8_t> recordData{1, 2, 3, 4, 5};
    uint8_t transferCRC = 0;

    auto rc = encode_get_pdr_resp(0, PLDM_SUCCESS, nextRecordHndl,
                                  nextDataTransferHndl, transferFlag, respCnt,
                                  recordData.data(), transferCRC, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetPDR, testGoodDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_PDR_REQ_BYTES> requestMsg{};

    uint32_t recordHndl = 0x32;
    uint32_t dataTransferHndl = 0x11;
    uint8_t transferOpFlag = PLDM_GET_FIRSTPART;
    uint16_t requestCnt = 0x5;
    uint16_t recordChangeNum = 0x01;

    uint32_t retRecordHndl = 0;
    uint32_t retDataTransferHndl = 0;
    uint8_t retTransferOpFlag = 0;
    uint16_t retRequestCnt = 0;
    uint16_t retRecordChangeNum = 0;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_get_pdr_req* request =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_pdr_req*>(req->payload);

    request->record_handle = htole32(recordHndl);
    request->data_transfer_handle = htole32(dataTransferHndl);
    request->transfer_op_flag = transferOpFlag;
    request->request_count = htole16(requestCnt);
    request->record_change_number = htole16(recordChangeNum);

    auto rc = decode_get_pdr_req(
        req, requestMsg.size() - hdrSize, &retRecordHndl, &retDataTransferHndl,
        &retTransferOpFlag, &retRequestCnt, &retRecordChangeNum);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retRecordHndl, recordHndl);
    EXPECT_EQ(retDataTransferHndl, dataTransferHndl);
    EXPECT_EQ(retTransferOpFlag, transferOpFlag);
    EXPECT_EQ(retRequestCnt, requestCnt);
    EXPECT_EQ(retRecordChangeNum, recordChangeNum);
}

TEST(GetPDR, testBadDecodeRequest)
{
    std::array<uint8_t, PLDM_GET_PDR_REQ_BYTES> requestMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = decode_get_pdr_req(req, requestMsg.size(), NULL, NULL, NULL, NULL,
                                 NULL);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetPDR, testGoodEncodeRequest)
{
    uint32_t record_hndl = 0;
    uint32_t data_transfer_hndl = 0;
    uint8_t transfer_op_flag = PLDM_GET_FIRSTPART;
    uint16_t request_cnt = 20;
    uint16_t record_chg_num = 0;

    std::vector<uint8_t> requestMsg(hdrSize + PLDM_GET_PDR_REQ_BYTES);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_get_pdr_req(0, record_hndl, data_transfer_hndl,
                                 transfer_op_flag, request_cnt, record_chg_num,
                                 request, PLDM_GET_PDR_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    struct pldm_get_pdr_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_pdr_req*>(request->payload);
    EXPECT_EQ(record_hndl, le32toh(req->record_handle));
    EXPECT_EQ(data_transfer_hndl, le32toh(req->data_transfer_handle));
    EXPECT_EQ(transfer_op_flag, req->transfer_op_flag);
    EXPECT_EQ(request_cnt, le16toh(req->request_count));
    EXPECT_EQ(record_chg_num, le16toh(req->record_change_number));
}

TEST(GetPDR, testBadEncodeRequest)
{
    uint32_t record_hndl = 0;
    uint32_t data_transfer_hndl = 0;
    uint8_t transfer_op_flag = PLDM_GET_FIRSTPART;
    uint16_t request_cnt = 32;
    uint16_t record_chg_num = 0;

    std::vector<uint8_t> requestMsg(hdrSize + PLDM_GET_PDR_REQ_BYTES);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_get_pdr_req(0, record_hndl, data_transfer_hndl,
                                 transfer_op_flag, request_cnt, record_chg_num,
                                 nullptr, PLDM_GET_PDR_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_get_pdr_req(0, record_hndl, data_transfer_hndl,
                            transfer_op_flag, request_cnt, record_chg_num,
                            request, PLDM_GET_PDR_REQ_BYTES + 1);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetPDR, testGoodDecodeResponse)
{
    const char* recordData = "123456789";
    uint8_t completionCode = PLDM_SUCCESS;
    uint32_t nextRecordHndl = 0;
    uint32_t nextDataTransferHndl = 0;
    uint8_t transferFlag = PLDM_END;
    constexpr uint16_t respCnt = 9;
    uint8_t transferCRC = 96;
    size_t recordDataLength = 32;
    std::array<uint8_t, hdrSize + PLDM_GET_PDR_MIN_RESP_BYTES + respCnt +
                            sizeof(transferCRC)>
        responseMsg{};

    uint8_t retCompletionCode = 0;
    uint8_t retRecordData[32] = {0};
    uint32_t retNextRecordHndl = 0;
    uint32_t retNextDataTransferHndl = 0;
    uint8_t retTransferFlag = 0;
    uint16_t retRespCnt = 0;
    uint8_t retTransferCRC = 0;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_get_pdr_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_pdr_resp*>(response->payload);
    resp->completion_code = completionCode;
    resp->next_record_handle = htole32(nextRecordHndl);
    resp->next_data_transfer_handle = htole32(nextDataTransferHndl);
    resp->transfer_flag = transferFlag;
    resp->response_count = htole16(respCnt);
    memcpy(resp->record_data, recordData, respCnt);
    response->payload[PLDM_GET_PDR_MIN_RESP_BYTES + respCnt] = transferCRC;

    auto rc = decode_get_pdr_resp(
        response, responseMsg.size() - hdrSize, &retCompletionCode,
        &retNextRecordHndl, &retNextDataTransferHndl, &retTransferFlag,
        &retRespCnt, retRecordData, recordDataLength, &retTransferCRC);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retCompletionCode, completionCode);
    EXPECT_EQ(retNextRecordHndl, nextRecordHndl);
    EXPECT_EQ(retNextDataTransferHndl, nextDataTransferHndl);
    EXPECT_EQ(retTransferFlag, transferFlag);
    EXPECT_EQ(retRespCnt, respCnt);
    EXPECT_EQ(retTransferCRC, transferCRC);
    EXPECT_EQ(0, memcmp(recordData, resp->record_data, respCnt));
}

TEST(GetPDR, testBadDecodeResponse)
{
    const char* recordData = "123456789";
    uint8_t completionCode = PLDM_SUCCESS;
    uint32_t nextRecordHndl = 0;
    uint32_t nextDataTransferHndl = 0;
    uint8_t transferFlag = PLDM_END;
    constexpr uint16_t respCnt = 9;
    uint8_t transferCRC = 96;
    size_t recordDataLength = respCnt - 1;
    std::array<uint8_t, hdrSize + PLDM_GET_PDR_MIN_RESP_BYTES + respCnt +
                            sizeof(transferCRC)>
        responseMsg{};

    uint8_t retCompletionCode = 0;
    uint8_t retRecordData[32] = {0};
    uint32_t retNextRecordHndl = 0;
    uint32_t retNextDataTransferHndl = 0;
    uint8_t retTransferFlag = 0;
    uint16_t retRespCnt = 0;
    uint8_t retTransferCRC = 0;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_get_pdr_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_pdr_resp*>(response->payload);
    resp->completion_code = completionCode;
    resp->next_record_handle = htole32(nextRecordHndl);
    resp->next_data_transfer_handle = htole32(nextDataTransferHndl);
    resp->transfer_flag = transferFlag;
    resp->response_count = htole16(respCnt);
    memcpy(resp->record_data, recordData, respCnt);
    response->payload[PLDM_GET_PDR_MIN_RESP_BYTES + respCnt] = transferCRC;

    auto rc = decode_get_pdr_resp(response, responseMsg.size() - hdrSize, NULL,
                                  NULL, NULL, NULL, NULL, NULL, 0, NULL);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_get_pdr_resp(
        response, responseMsg.size() - hdrSize - 1, &retCompletionCode,
        &retNextRecordHndl, &retNextDataTransferHndl, &retTransferFlag,
        &retRespCnt, retRecordData, recordDataLength, &retTransferCRC);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

#ifdef LIBPLDM_API_TESTING
TEST(GetPDR, testGoodDecodeResponseSafe)
{
    static const char recordData[] = "123456789";

    alignas(pldm_msg) unsigned char data[sizeof(pldm_msg_hdr) +
                                         PLDM_GET_PDR_MIN_RESP_BYTES +
                                         sizeof(recordData) - 1 + 1];
    PLDM_MSGBUF_DEFINE_P(buf);
    int rc;

    pldm_msg* msg = new (data) pldm_msg;

    rc = pldm_msgbuf_init_errno(buf, PLDM_GET_PDR_MIN_RESP_BYTES, msg->payload,
                                sizeof(data) - sizeof(msg->hdr));
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_insert_uint8(buf, PLDM_SUCCESS);
    pldm_msgbuf_insert_uint32(buf, 0);
    pldm_msgbuf_insert_uint32(buf, 0);
    pldm_msgbuf_insert_uint8(buf, PLDM_END);
    pldm_msgbuf_insert_uint16(buf, sizeof(recordData) - 1);
    rc = pldm_msgbuf_insert_array_char(buf, sizeof(recordData) - 1, recordData,
                                       sizeof(recordData) - 1);
    ASSERT_EQ(rc, 0);
    pldm_msgbuf_insert_uint8(buf, 96);
    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    alignas(pldm_get_pdr_resp) unsigned char
        resp_data[sizeof(pldm_get_pdr_resp) + sizeof(recordData) - 1];
    pldm_get_pdr_resp* resp = new (resp_data) pldm_get_pdr_resp;
    uint8_t crc;
    rc = decode_get_pdr_resp_safe(msg, sizeof(data) - sizeof(msg->hdr), resp,
                                  sizeof(resp_data) - sizeof(*resp), &crc);
    ASSERT_EQ(rc, 0);
    EXPECT_EQ(resp->completion_code, PLDM_SUCCESS);
    EXPECT_EQ(resp->next_record_handle, 0);
    EXPECT_EQ(resp->next_data_transfer_handle, 0);
    EXPECT_EQ(resp->transfer_flag, PLDM_END);
    ASSERT_EQ(resp->response_count, sizeof(recordData) - 1);
    EXPECT_EQ(crc, 96);
    EXPECT_EQ(0, memcmp(recordData, resp->record_data, resp->response_count));
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(GetPDR, testBadDecodeResponseSafeTrivial)
{
    pldm_get_pdr_resp resp;
    uint8_t crc;
    int rc;

    rc = decode_get_pdr_resp_safe(nullptr, PLDM_GET_PDR_MIN_RESP_BYTES, &resp,
                                  sizeof(resp), &crc);
    EXPECT_EQ(rc, -EINVAL);

    alignas(pldm_msg) unsigned char
        msg_data[sizeof(pldm_msg_hdr) + PLDM_GET_PDR_MIN_RESP_BYTES];
    pldm_msg* msg = new (msg_data) pldm_msg;
    rc = decode_get_pdr_resp_safe(msg, PLDM_GET_PDR_MIN_RESP_BYTES, nullptr,
                                  sizeof(resp), &crc);
    EXPECT_EQ(rc, -EINVAL);

    rc = decode_get_pdr_resp_safe(msg, PLDM_GET_PDR_MIN_RESP_BYTES, &resp,
                                  sizeof(resp), nullptr);
    EXPECT_EQ(rc, -EINVAL);

    msg->payload[0] = PLDM_ERROR_INVALID_DATA;
    rc = decode_get_pdr_resp_safe(msg, 1, &resp, sizeof(resp), &crc);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(resp.completion_code, PLDM_ERROR_INVALID_DATA);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(GetPDRRepositoryInfo, testGoodEncodeRequest)
{
    pldm_msg request{};

    auto rc = encode_get_pdr_repository_info_req(0, &request,
                                                 sizeof(struct pldm_msg));
    ASSERT_EQ(rc, PLDM_SUCCESS);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(GetPDRRepositoryInfo, testBadEncodeRequest)
{
    auto rc =
        encode_get_pdr_repository_info_req(0, nullptr, sizeof(struct pldm_msg));
    EXPECT_EQ(rc, -EINVAL);
}
#endif

TEST(GetPDRRepositoryInfo, testGoodEncodeResponse)
{
    uint8_t completionCode = 0;
    uint8_t repositoryState = PLDM_AVAILABLE;
    uint8_t updateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint8_t oemUpdateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint32_t recordCount = 100;
    uint32_t repositorySize = 100;
    uint32_t largestRecordSize = UINT32_MAX;
    uint8_t dataTransferHandleTimeout = PLDM_NO_TIMEOUT;

    std::vector<uint8_t> responseMsg(hdrSize +
                                     PLDM_GET_PDR_REPOSITORY_INFO_RESP_BYTES);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_get_pdr_repository_info_resp(
        0, PLDM_SUCCESS, repositoryState, updateTime, oemUpdateTime,
        recordCount, repositorySize, largestRecordSize,
        dataTransferHandleTimeout, response);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    struct pldm_pdr_repository_info_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_pdr_repository_info_resp*>(
            response->payload);

    EXPECT_EQ(completionCode, resp->completion_code);
    EXPECT_EQ(repositoryState, resp->repository_state);
    EXPECT_EQ(0, memcmp(updateTime, resp->update_time, PLDM_TIMESTAMP104_SIZE));
    EXPECT_EQ(0, memcmp(oemUpdateTime, resp->oem_update_time,
                        PLDM_TIMESTAMP104_SIZE));
    EXPECT_EQ(recordCount, le32toh(resp->record_count));
    EXPECT_EQ(repositorySize, le32toh(resp->repository_size));
    EXPECT_EQ(largestRecordSize, le32toh(resp->largest_record_size));
    EXPECT_EQ(dataTransferHandleTimeout, resp->data_transfer_handle_timeout);
}

TEST(GetPDRRepositoryInfo, testBadEncodeResponse)
{
    uint8_t repositoryState = PLDM_AVAILABLE;
    uint8_t updateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint8_t oemUpdateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint32_t recordCount = 100;
    uint32_t repositorySize = 100;
    uint32_t largestRecordSize = UINT32_MAX;
    uint8_t dataTransferHandleTimeout = PLDM_NO_TIMEOUT;

    auto rc = encode_get_pdr_repository_info_resp(
        0, PLDM_SUCCESS, repositoryState, updateTime, oemUpdateTime,
        recordCount, repositorySize, largestRecordSize,
        dataTransferHandleTimeout, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetPDRRepositoryInfo, testGoodDecodeResponse)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t repositoryState = PLDM_AVAILABLE;
    uint8_t updateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint8_t oemUpdateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint32_t recordCount = 100;
    uint32_t repositorySize = 100;
    uint32_t largestRecordSize = UINT32_MAX;
    uint8_t dataTransferHandleTimeout = PLDM_NO_TIMEOUT;

    std::array<uint8_t, hdrSize + PLDM_GET_PDR_REPOSITORY_INFO_RESP_BYTES>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_pdr_repository_info_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_pdr_repository_info_resp*>(
            response->payload);
    resp->completion_code = completionCode;
    resp->repository_state = repositoryState;
    memcpy(resp->update_time, updateTime, PLDM_TIMESTAMP104_SIZE);
    memcpy(resp->oem_update_time, oemUpdateTime, PLDM_TIMESTAMP104_SIZE);
    resp->record_count = htole32(recordCount);
    resp->repository_size = htole32(repositorySize);
    resp->largest_record_size = htole32(largestRecordSize);
    resp->data_transfer_handle_timeout = dataTransferHandleTimeout;

    uint8_t retCompletionCode = 0;
    uint8_t retRepositoryState = 0;
    uint8_t retUpdateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint8_t retOemUpdateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint32_t retRecordCount = 0;
    uint32_t retRepositorySize = 0;
    uint32_t retLargestRecordSize = 0;
    uint8_t retDataTransferHandleTimeout = 0;

    auto rc = decode_get_pdr_repository_info_resp(
        response, responseMsg.size() - hdrSize, &retCompletionCode,
        &retRepositoryState, retUpdateTime, retOemUpdateTime, &retRecordCount,
        &retRepositorySize, &retLargestRecordSize,
        &retDataTransferHandleTimeout);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, retCompletionCode);
    EXPECT_EQ(repositoryState, retRepositoryState);
    EXPECT_EQ(0, memcmp(updateTime, retUpdateTime, PLDM_TIMESTAMP104_SIZE));
    EXPECT_EQ(0,
              memcmp(oemUpdateTime, retOemUpdateTime, PLDM_TIMESTAMP104_SIZE));
    EXPECT_EQ(recordCount, recordCount);
    EXPECT_EQ(repositorySize, repositorySize);
    EXPECT_EQ(largestRecordSize, largestRecordSize);
    EXPECT_EQ(dataTransferHandleTimeout, dataTransferHandleTimeout);
}

TEST(GetPDRRepositoryInfo, testBadDecodeResponse)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t repositoryState = PLDM_AVAILABLE;
    uint8_t updateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint8_t oemUpdateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint32_t recordCount = htole32(100);
    uint32_t repositorySize = htole32(100);
    uint32_t largestRecordSize = htole32(UINT32_MAX);
    uint8_t dataTransferHandleTimeout = PLDM_NO_TIMEOUT;

    std::array<uint8_t, hdrSize + PLDM_GET_PDR_REPOSITORY_INFO_RESP_BYTES>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_pdr_repository_info_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_pdr_repository_info_resp*>(
            response->payload);
    resp->completion_code = completionCode;
    resp->repository_state = repositoryState;
    memcpy(resp->update_time, updateTime, PLDM_TIMESTAMP104_SIZE);
    memcpy(resp->oem_update_time, oemUpdateTime, PLDM_TIMESTAMP104_SIZE);
    resp->record_count = recordCount;
    resp->repository_size = repositorySize;
    resp->largest_record_size = largestRecordSize;
    resp->data_transfer_handle_timeout = dataTransferHandleTimeout;

    uint8_t retCompletionCode = 0;
    uint8_t retRepositoryState = 0;
    uint8_t retUpdateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint8_t retOemUpdateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint32_t retRecordCount = 0;
    uint32_t retRepositorySize = 0;
    uint32_t retLargestRecordSize = 0;
    uint8_t retDataTransferHandleTimeout = 0;

    auto rc = decode_get_pdr_repository_info_resp(
        response, responseMsg.size() - hdrSize, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_get_pdr_repository_info_resp(
        response, responseMsg.size() - hdrSize - 1, &retCompletionCode,
        &retRepositoryState, retUpdateTime, retOemUpdateTime, &retRecordCount,
        &retRepositorySize, &retLargestRecordSize,
        &retDataTransferHandleTimeout);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    resp->repository_state = PLDM_FAILED + 1;
    rc = decode_get_pdr_repository_info_resp(
        response, responseMsg.size() - hdrSize, &retCompletionCode,
        &retRepositoryState, retUpdateTime, retOemUpdateTime, &retRecordCount,
        &retRepositorySize, &retLargestRecordSize,
        &retDataTransferHandleTimeout);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

#ifdef LIBPLDM_API_TESTING
TEST(GetPDRRepositoryInfo, testGoodDecodeResponseSafe)
{
    alignas(pldm_msg) unsigned char
        data[sizeof(pldm_msg_hdr) + PLDM_GET_PDR_REPOSITORY_INFO_RESP_BYTES];
    uint8_t updateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    uint8_t oemUpdateTime[PLDM_TIMESTAMP104_SIZE] = {0};
    PLDM_MSGBUF_DEFINE_P(buf);
    int rc;

    pldm_msg* msg = new (data) pldm_msg;

    rc = pldm_msgbuf_init_errno(buf, PLDM_GET_PDR_REPOSITORY_INFO_RESP_BYTES,
                                msg->payload, sizeof(data) - sizeof(msg->hdr));
    ASSERT_EQ(rc, 0);
    pldm_msgbuf_insert_uint8(buf, PLDM_SUCCESS);
    pldm_msgbuf_insert_uint8(buf, PLDM_AVAILABLE);
    rc = pldm_msgbuf_insert_array_uint8(buf, PLDM_TIMESTAMP104_SIZE, updateTime,
                                        sizeof(updateTime));
    ASSERT_EQ(rc, 0);
    rc = pldm_msgbuf_insert_array_uint8(buf, PLDM_TIMESTAMP104_SIZE,
                                        oemUpdateTime, sizeof(oemUpdateTime));
    ASSERT_EQ(rc, 0);
    pldm_msgbuf_insert_uint32(buf, 100);
    pldm_msgbuf_insert_uint32(buf, 100);
    pldm_msgbuf_insert_uint32(buf, UINT32_MAX);
    pldm_msgbuf_insert_uint8(buf, PLDM_NO_TIMEOUT);
    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    struct pldm_pdr_repository_info_resp resp;
    rc = decode_get_pdr_repository_info_resp_safe(
        msg, sizeof(data) - sizeof(msg->hdr), &resp);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(PLDM_SUCCESS, resp.completion_code);
    EXPECT_EQ(PLDM_AVAILABLE, resp.repository_state);
    EXPECT_EQ(0,
              memcmp(updateTime, resp.update_time, sizeof(resp.update_time)));
    EXPECT_EQ(0, memcmp(oemUpdateTime, resp.oem_update_time,
                        sizeof(resp.oem_update_time)));
    EXPECT_EQ(100, resp.record_count);
    EXPECT_EQ(100, resp.repository_size);
    EXPECT_EQ(UINT32_MAX, resp.largest_record_size);
    EXPECT_EQ(PLDM_NO_TIMEOUT, resp.data_transfer_handle_timeout);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(GetPDRRepositoryInfo, testBadDecodeResponseSafeTrivial)
{
    struct pldm_pdr_repository_info_resp resp;
    int rc;

    rc = decode_get_pdr_repository_info_resp_safe(
        nullptr, PLDM_GET_PDR_REPOSITORY_INFO_RESP_BYTES, &resp);
    EXPECT_EQ(rc, -EINVAL);

    alignas(pldm_msg) unsigned char
        msg_data[sizeof(pldm_msg) - 1 +
                 PLDM_GET_PDR_REPOSITORY_INFO_RESP_BYTES];
    pldm_msg* msg = new (msg_data) pldm_msg;
    rc = decode_get_pdr_repository_info_resp_safe(msg, 0, &resp);
    EXPECT_EQ(rc, -EOVERFLOW);

    rc = decode_get_pdr_repository_info_resp_safe(
        msg, PLDM_GET_PDR_REPOSITORY_INFO_RESP_BYTES, nullptr);
    EXPECT_EQ(rc, -EINVAL);

    msg->payload[0] = PLDM_ERROR_INVALID_DATA;
    rc = decode_get_pdr_repository_info_resp_safe(msg, 1, &resp);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(resp.completion_code, PLDM_ERROR_INVALID_DATA);
}
#endif

TEST(SetNumericEffecterValue, testGoodDecodeRequest)
{
    std::array<uint8_t,
               hdrSize + PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 3>
        requestMsg{};

    uint16_t effecter_id = 32768;
    uint8_t effecter_data_size = PLDM_EFFECTER_DATA_SIZE_UINT32;
    uint32_t effecter_value = 123456789;

    uint16_t reteffecter_id;
    uint8_t reteffecter_data_size;
    uint8_t reteffecter_value[4];

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_set_numeric_effecter_value_req* request =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_set_numeric_effecter_value_req*>(
            req->payload);

    request->effecter_id = htole16(effecter_id);
    request->effecter_data_size = effecter_data_size;
    uint32_t effecter_value_le = htole32(effecter_value);
    memcpy(request->effecter_value, &effecter_value_le,
           sizeof(effecter_value_le));

    auto rc = decode_set_numeric_effecter_value_req(
        req, requestMsg.size() - hdrSize, &reteffecter_id,
        &reteffecter_data_size, reteffecter_value);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    uint32_t value = *(reinterpret_cast<uint32_t*>(reteffecter_value));
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(reteffecter_id, effecter_id);
    EXPECT_EQ(reteffecter_data_size, effecter_data_size);
    EXPECT_EQ(value, effecter_value);
}

TEST(SetNumericEffecterValue, testBadDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES>
        requestMsg{};

    auto rc = decode_set_numeric_effecter_value_req(
        NULL, requestMsg.size() - hdrSize, NULL, NULL, NULL);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    uint16_t effecter_id = 0x10;
    uint8_t effecter_data_size = PLDM_EFFECTER_DATA_SIZE_UINT8;
    uint8_t effecter_value = 1;

    uint16_t reteffecter_id;
    uint8_t reteffecter_data_size;
    uint8_t reteffecter_value[4];

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_set_numeric_effecter_value_req* request =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_set_numeric_effecter_value_req*>(
            req->payload);

    request->effecter_id = effecter_id;
    request->effecter_data_size = effecter_data_size;
    memcpy(request->effecter_value, &effecter_value, sizeof(effecter_value));

    rc = decode_set_numeric_effecter_value_req(
        req, requestMsg.size() - hdrSize - 1, &reteffecter_id,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        &reteffecter_data_size, reinterpret_cast<uint8_t*>(&reteffecter_value));
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(SetNumericEffecterValue, testGoodEncodeRequest)
{
    uint16_t effecter_id = 0;
    uint8_t effecter_data_size = PLDM_EFFECTER_DATA_SIZE_UINT16;
    uint16_t effecter_value = 65534;

    std::vector<uint8_t> requestMsg(
        hdrSize + PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 1);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_set_numeric_effecter_value_req(
        0, effecter_id, effecter_data_size,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(&effecter_value), request,
        PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 1);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    struct pldm_set_numeric_effecter_value_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_set_numeric_effecter_value_req*>(
            request->payload);
    EXPECT_EQ(effecter_id, req->effecter_id);
    EXPECT_EQ(effecter_data_size, req->effecter_data_size);
    uint16_t* val = (uint16_t*)req->effecter_value;
    *val = le16toh(*val);
    EXPECT_EQ(effecter_value, *val);
}

TEST(SetNumericEffecterValue, testBadEncodeRequest)
{
    std::vector<uint8_t> requestMsg(
        hdrSize + PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_set_numeric_effecter_value_req(
        0, 0, 0, NULL, NULL, PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    uint16_t effecter_value;
    rc = encode_set_numeric_effecter_value_req(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        0, 0, 6, reinterpret_cast<uint8_t*>(&effecter_value), request,
        PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(SetNumericEffecterValue, testGoodDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_SET_NUMERIC_EFFECTER_VALUE_RESP_BYTES>
        responseMsg{};

    uint8_t completion_code = 0xa0;

    uint8_t retcompletion_code;

    memcpy(responseMsg.data() + hdrSize, &completion_code,
           sizeof(completion_code));

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_set_numeric_effecter_value_resp(
        response, responseMsg.size() - hdrSize, &retcompletion_code);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completion_code, retcompletion_code);
}

TEST(SetNumericEffecterValue, testBadDecodeResponse)
{
    std::array<uint8_t, PLDM_SET_NUMERIC_EFFECTER_VALUE_RESP_BYTES>
        responseMsg{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_set_numeric_effecter_value_resp(response,
                                                     responseMsg.size(), NULL);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(SetNumericEffecterValue, testGoodEncodeResponse)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) +
                            PLDM_SET_NUMERIC_EFFECTER_VALUE_RESP_BYTES>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    uint8_t completionCode = 0;

    auto rc = encode_set_numeric_effecter_value_resp(
        0, PLDM_SUCCESS, response, PLDM_SET_NUMERIC_EFFECTER_VALUE_RESP_BYTES);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, response->payload[0]);
}

TEST(SetNumericEffecterValue, testBadEncodeResponse)
{
    auto rc = encode_set_numeric_effecter_value_resp(
        0, PLDM_SUCCESS, NULL, PLDM_SET_NUMERIC_EFFECTER_VALUE_RESP_BYTES);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetStateSensorReadings, testGoodEncodeResponse)
{
    std::array<uint8_t, hdrSize +
                            PLDM_GET_STATE_SENSOR_READINGS_MIN_RESP_BYTES +
                            sizeof(get_sensor_state_field) * 2>
        responseMsg{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    uint8_t completionCode = 0;
    uint8_t comp_sensorCnt = 0x2;

    std::array<get_sensor_state_field, 2> stateField{};
    stateField[0] = {PLDM_SENSOR_ENABLED, PLDM_SENSOR_NORMAL,
                     PLDM_SENSOR_WARNING, PLDM_SENSOR_UNKNOWN};
    stateField[1] = {PLDM_SENSOR_FAILED, PLDM_SENSOR_UPPERFATAL,
                     PLDM_SENSOR_UPPERCRITICAL, PLDM_SENSOR_FATAL};

    auto rc = encode_get_state_sensor_readings_resp(
        0, PLDM_SUCCESS, comp_sensorCnt, stateField.data(), response);

    struct pldm_get_state_sensor_readings_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_state_sensor_readings_resp*>(
            response->payload);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, resp->completion_code);
    EXPECT_EQ(comp_sensorCnt, resp->comp_sensor_count);
    EXPECT_EQ(stateField[0].sensor_op_state, resp->field->sensor_op_state);
    EXPECT_EQ(stateField[0].present_state, resp->field->present_state);
    EXPECT_EQ(stateField[0].previous_state, resp->field->previous_state);
    EXPECT_EQ(stateField[0].event_state, resp->field->event_state);
    EXPECT_EQ(stateField[1].sensor_op_state, resp->field[1].sensor_op_state);
    EXPECT_EQ(stateField[1].present_state, resp->field[1].present_state);
    EXPECT_EQ(stateField[1].previous_state, resp->field[1].previous_state);
    EXPECT_EQ(stateField[1].event_state, resp->field[1].event_state);
}

TEST(GetStateSensorReadings, testBadEncodeResponse)
{
    auto rc = encode_get_state_sensor_readings_resp(0, PLDM_SUCCESS, 0, nullptr,
                                                    nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetStateSensorReadings, testGoodDecodeResponse)
{
    std::array<uint8_t, hdrSize +
                            PLDM_GET_STATE_SENSOR_READINGS_MIN_RESP_BYTES +
                            sizeof(get_sensor_state_field) * 2>
        responseMsg{};

    uint8_t completionCode = 0;
    uint8_t comp_sensorCnt = 2;

    std::array<get_sensor_state_field, 2> stateField{};
    stateField[0] = {PLDM_SENSOR_DISABLED, PLDM_SENSOR_UNKNOWN,
                     PLDM_SENSOR_UNKNOWN, PLDM_SENSOR_UNKNOWN};
    stateField[1] = {PLDM_SENSOR_ENABLED, PLDM_SENSOR_LOWERFATAL,
                     PLDM_SENSOR_LOWERCRITICAL, PLDM_SENSOR_WARNING};

    uint8_t retcompletion_code = 0;
    uint8_t retcomp_sensorCnt = 0;
    std::array<get_sensor_state_field, 2> retstateField{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_get_state_sensor_readings_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_state_sensor_readings_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->comp_sensor_count = comp_sensorCnt;
    memcpy(resp->field, &stateField,
           (sizeof(get_sensor_state_field) * comp_sensorCnt));

    auto rc = decode_get_state_sensor_readings_resp(
        response, responseMsg.size() - hdrSize, &retcompletion_code,
        &retcomp_sensorCnt, retstateField.data());

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, retcompletion_code);
    EXPECT_EQ(comp_sensorCnt, retcomp_sensorCnt);
    EXPECT_EQ(stateField[0].sensor_op_state, retstateField[0].sensor_op_state);
    EXPECT_EQ(stateField[0].present_state, retstateField[0].present_state);
    EXPECT_EQ(stateField[0].previous_state, retstateField[0].previous_state);
    EXPECT_EQ(stateField[0].event_state, retstateField[0].event_state);
    EXPECT_EQ(stateField[1].sensor_op_state, retstateField[1].sensor_op_state);
    EXPECT_EQ(stateField[1].present_state, retstateField[1].present_state);
    EXPECT_EQ(stateField[1].previous_state, retstateField[1].previous_state);
    EXPECT_EQ(stateField[1].event_state, retstateField[1].event_state);
}

TEST(GetStateSensorReadings, testBadDecodeResponse)
{
    std::array<uint8_t, hdrSize +
                            PLDM_GET_STATE_SENSOR_READINGS_MIN_RESP_BYTES +
                            sizeof(get_sensor_state_field) * 2>
        responseMsg{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_state_sensor_readings_resp(
        response, responseMsg.size() - hdrSize, nullptr, nullptr, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    uint8_t completionCode = 0;
    uint8_t comp_sensorCnt = 1;

    std::array<get_sensor_state_field, 1> stateField{};
    stateField[0] = {PLDM_SENSOR_ENABLED, PLDM_SENSOR_UPPERFATAL,
                     PLDM_SENSOR_UPPERCRITICAL, PLDM_SENSOR_WARNING};

    uint8_t retcompletion_code = 0;
    uint8_t retcomp_sensorCnt = 0;
    std::array<get_sensor_state_field, 1> retstateField{};

    struct pldm_get_state_sensor_readings_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_state_sensor_readings_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->comp_sensor_count = comp_sensorCnt;
    memcpy(resp->field, &stateField,
           (sizeof(get_sensor_state_field) * comp_sensorCnt));

    rc = decode_get_state_sensor_readings_resp(
        response, responseMsg.size() - hdrSize, &retcompletion_code,
        &retcomp_sensorCnt, retstateField.data());

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetStateSensorReadings, testGoodEncodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_STATE_SENSOR_READINGS_REQ_BYTES>
        requestMsg{};

    uint16_t sensorId = 0xab;
    bitfield8_t sensorRearm;
    sensorRearm.byte = 0x03;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc = encode_get_state_sensor_readings_req(0, sensorId, sensorRearm, 0,
                                                   request);

    struct pldm_get_state_sensor_readings_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_state_sensor_readings_req*>(
            request->payload);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(sensorId, le16toh(req->sensor_id));
    EXPECT_EQ(sensorRearm.byte, req->sensor_rearm.byte);
}

TEST(GetStateSensorReadings, testBadEncodeRequest)
{
    bitfield8_t sensorRearm;
    sensorRearm.byte = 0x0;

    auto rc =
        encode_get_state_sensor_readings_req(0, 0, sensorRearm, 0, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetStateSensorReadings, testGoodDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_STATE_SENSOR_READINGS_REQ_BYTES>
        requestMsg{};

    uint16_t sensorId = 0xcd;
    bitfield8_t sensorRearm;
    sensorRearm.byte = 0x10;

    uint16_t retsensorId;
    bitfield8_t retsensorRearm;
    uint8_t retreserved;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    struct pldm_get_state_sensor_readings_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_state_sensor_readings_req*>(
            request->payload);

    req->sensor_id = htole16(sensorId);
    req->sensor_rearm.byte = sensorRearm.byte;

    auto rc = decode_get_state_sensor_readings_req(
        request, requestMsg.size() - hdrSize, &retsensorId, &retsensorRearm,
        &retreserved);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(sensorId, retsensorId);
    EXPECT_EQ(sensorRearm.byte, retsensorRearm.byte);
    EXPECT_EQ(0, retreserved);
}

TEST(GetStateSensorReadings, testBadDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_STATE_SENSOR_READINGS_REQ_BYTES>
        requestMsg{};

    auto rc = decode_get_state_sensor_readings_req(
        nullptr, requestMsg.size() - hdrSize, nullptr, nullptr, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
    uint16_t sensorId = 0x11;
    bitfield8_t sensorRearm;
    sensorRearm.byte = 0x04;

    uint16_t retsensorId;
    bitfield8_t retsensorRearm;
    uint8_t retreserved;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    struct pldm_get_state_sensor_readings_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_state_sensor_readings_req*>(
            request->payload);

    req->sensor_id = htole16(sensorId);
    req->sensor_rearm.byte = sensorRearm.byte;

    rc = decode_get_state_sensor_readings_req(
        request, requestMsg.size() - hdrSize - 1, &retsensorId, &retsensorRearm,
        &retreserved);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(EventMessageBufferSize, testGoodEventMessageBufferSizeRequest)
{
    uint8_t eventBufferSize = 32;

    std::array<uint8_t, hdrSize + PLDM_EVENT_MESSAGE_BUFFER_SIZE_REQ_BYTES>
        requestMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_event_message_buffer_size_req(0, eventBufferSize, request);

    EXPECT_EQ(rc, PLDM_SUCCESS);
}

TEST(EventMessageBufferSize, testGoodEventMessageBufferSizeResponse)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint16_t terminusMaxBufferSize = 256;

    std::array<uint8_t, hdrSize + PLDM_EVENT_MESSAGE_BUFFER_SIZE_RESP_BYTES>
        responseMsg{};

    uint8_t retCompletionCode;
    uint16_t retMaxBufferSize = 0;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_event_message_buffer_size_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_event_message_buffer_size_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->terminus_max_buffer_size = terminusMaxBufferSize;

    auto rc = decode_event_message_buffer_size_resp(
        response, responseMsg.size() - hdrSize, &retCompletionCode,
        &retMaxBufferSize);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retCompletionCode, completionCode);
    EXPECT_EQ(terminusMaxBufferSize, retMaxBufferSize);
}

TEST(EventMessageBufferSize, testBadEventMessageBufferSizeResponse)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint16_t terminusMaxBufferSize = 256;

    std::array<uint8_t, hdrSize + PLDM_EVENT_MESSAGE_BUFFER_SIZE_RESP_BYTES>
        responseMsg{};

    uint8_t retCompletionCode;
    uint16_t retMaxBufferSize = 0;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_event_message_buffer_size_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_event_message_buffer_size_resp*>(
            response->payload);
    resp->completion_code = completionCode;
    resp->terminus_max_buffer_size = terminusMaxBufferSize;

    auto rc =
        decode_event_message_buffer_size_resp(response, 0, nullptr, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_event_message_buffer_size_resp(
        response, responseMsg.size(), &retCompletionCode, &retMaxBufferSize);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(PlatformEventMessageSupported, testGoodEncodeRequest)
{
    uint8_t formatVersion = 0x01;

    std::array<uint8_t, hdrSize + PLDM_EVENT_MESSAGE_SUPPORTED_REQ_BYTES>
        requestMsg{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_event_message_supported_req(0, formatVersion, request);

    struct pldm_event_message_supported_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_event_message_supported_req*>(
            request->payload);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(formatVersion, req->format_version);
}

TEST(PlatformEventMessageSupported, testBadEncodeRequest)
{
    uint8_t eventData = 34;
    uint8_t formatVersion = 0x0;

    std::array<uint8_t, hdrSize + PLDM_EVENT_MESSAGE_SUPPORTED_REQ_BYTES +
                            sizeof(eventData)>
        requestMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_event_message_supported_req(0, formatVersion, request);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_event_message_supported_req(0, formatVersion, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(PlatformEventMessageSupported, testGoodDecodeRespond)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t synchConfiguration = PLDM_MESSAGE_TYPE_SYNCHRONOUS;
    bitfield8_t synchConfigSupported;
    synchConfigSupported.byte = 0xe;
    uint8_t numberEventClassReturned = 0x3;
    std::vector<uint8_t> eventClass{0x0, 0x5, 0xfa};
    constexpr uint8_t eventClassCount = 3;

    std::array<uint8_t, hdrSize + PLDM_EVENT_MESSAGE_SUPPORTED_MIN_RESP_BYTES +
                            eventClassCount>
        responseMsg{};

    uint8_t retCompletionCode;
    uint8_t retSynchConfig = 0;
    uint8_t retNumberEventClass = 0;
    bitfield8_t retSynchConfigSupport;
    uint8_t retEventClass[eventClassCount] = {0};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_event_message_supported_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_event_message_supported_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->synchrony_configuration = synchConfiguration;
    resp->synchrony_configuration_supported.byte = synchConfigSupported.byte;
    resp->number_event_class_returned = numberEventClassReturned;
    memcpy(resp->event_class, eventClass.data(), numberEventClassReturned);

    auto rc = decode_event_message_supported_resp(
        response, responseMsg.size() - hdrSize, &retCompletionCode,
        &retSynchConfig, &retSynchConfigSupport, &retNumberEventClass,
        retEventClass, eventClassCount);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retCompletionCode, completionCode);
    EXPECT_EQ(retSynchConfig, synchConfiguration);
    EXPECT_EQ(retNumberEventClass, numberEventClassReturned);
    EXPECT_EQ(retSynchConfigSupport.byte, synchConfigSupported.byte);
    EXPECT_EQ(0, memcmp(eventClass.data(), resp->event_class,
                        numberEventClassReturned));
}

TEST(PlatformEventMessageSupported, testBadSynchConfiguration)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t synchConfiguration = 0x4;
    bitfield8_t synchConfigSupported;
    synchConfigSupported.byte = 0xe;
    uint8_t numberEventClassReturned = 0x3;
    std::vector<uint8_t> eventClass{0x0, 0x5, 0xfa};
    constexpr uint8_t eventClassCount = 3;

    std::array<uint8_t, hdrSize + PLDM_EVENT_MESSAGE_SUPPORTED_MIN_RESP_BYTES +
                            eventClassCount>
        responseMsg{};

    uint8_t retCompletionCode;
    uint8_t retSynchConfig = 0;
    uint8_t retNumberEventClass = 0;
    bitfield8_t retSynchConfigSupport;
    uint8_t retEventClass[eventClassCount] = {0};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_event_message_supported_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_event_message_supported_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->synchrony_configuration = synchConfiguration;
    resp->synchrony_configuration_supported.byte = synchConfigSupported.byte;
    resp->number_event_class_returned = numberEventClassReturned;
    memcpy(resp->event_class, eventClass.data(), numberEventClassReturned);

    auto rc = decode_event_message_supported_resp(
        response, responseMsg.size() - hdrSize, &retCompletionCode,
        &retSynchConfig, &retSynchConfigSupport, &retNumberEventClass,
        retEventClass, eventClassCount);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(PlatformEventMessageSupported, testBadDecodeRespond)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t synchConfiguration = PLDM_MESSAGE_TYPE_SYNCHRONOUS;
    bitfield8_t synchConfigSupported;
    synchConfigSupported.byte = 0xe;
    uint8_t numberEventClassReturned = 0x3;
    std::vector<uint8_t> eventClass{0x0, 0x5, 0xfa};
    constexpr uint8_t eventClassCount = 3;

    std::array<uint8_t, hdrSize + PLDM_EVENT_MESSAGE_SUPPORTED_MIN_RESP_BYTES +
                            eventClassCount>
        responseMsg{};

    uint8_t retCompletionCode;
    uint8_t retSynchConfig = 0;
    uint8_t retNumberEventClass = 0;
    bitfield8_t retSynchConfigSupport;
    uint8_t retEventClass[eventClassCount] = {0};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_event_message_supported_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_event_message_supported_resp*>(
            response->payload);
    resp->completion_code = completionCode;
    resp->synchrony_configuration = synchConfiguration;
    resp->synchrony_configuration_supported.byte = synchConfigSupported.byte;
    resp->number_event_class_returned = numberEventClassReturned;
    memcpy(resp->event_class, eventClass.data(), numberEventClassReturned);

    auto rc = decode_event_message_supported_resp(response, 0, nullptr, nullptr,
                                                  nullptr, nullptr, nullptr, 0);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_event_message_supported_resp(
        response, PLDM_EVENT_MESSAGE_SUPPORTED_MIN_RESP_BYTES - 1,
        &retCompletionCode, &retSynchConfig, &retSynchConfigSupport,
        &retNumberEventClass, retEventClass, eventClassCount);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    rc = decode_event_message_supported_resp(
        response, responseMsg.size() - hdrSize, &retCompletionCode,
        &retSynchConfig, &retSynchConfigSupport, &retNumberEventClass,
        retEventClass, 1);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(PollForPlatformEventMessage, testGoodEncodeRequestFirstPart)
{
    uint8_t formatVersion = 0x01;
    uint8_t transferOperationFlag = PLDM_GET_FIRSTPART;
    uint32_t dataTransferHandle = 0xaabbccdd;
    uint16_t eventIdToAcknowledge = PLDM_PLATFORM_EVENT_ID_NULL;

    PLDM_MSG_DEFINE_P(request, PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);

    auto rc = encode_poll_for_platform_event_message_req(
        0, formatVersion, transferOperationFlag, dataTransferHandle,
        eventIdToAcknowledge, request,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    PLDM_MSGBUF_DEFINE_P(buf);
    rc = pldm_msgbuf_init_errno(
        buf, PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES, request->payload,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    ASSERT_EQ(rc, 0);

    uint8_t retFormatVersion;
    uint8_t retTransferOperationFlag;
    uint32_t retDataTransferHandle;
    uint16_t retEventIdToAcknowledge;

    pldm_msgbuf_extract_uint8(buf, retFormatVersion);
    pldm_msgbuf_extract_uint8(buf, retTransferOperationFlag);
    pldm_msgbuf_extract_uint32(buf, retDataTransferHandle);
    pldm_msgbuf_extract_uint16(buf, retEventIdToAcknowledge);
    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    EXPECT_EQ(retFormatVersion, formatVersion);
    EXPECT_EQ(retTransferOperationFlag, transferOperationFlag);
    EXPECT_EQ(retDataTransferHandle, dataTransferHandle);
    EXPECT_EQ(retEventIdToAcknowledge, eventIdToAcknowledge);
}

TEST(PollForPlatformEventMessage, testGoodEncodeRequestNextPart)
{
    uint8_t formatVersion = 0x01;
    uint8_t transferOperationFlag = PLDM_GET_NEXTPART;
    uint32_t dataTransferHandle = 0xaabbccdd;
    uint16_t eventIdToAcknowledge = PLDM_PLATFORM_EVENT_ID_FRAGMENT;

    PLDM_MSG_DEFINE_P(request, PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);

    auto rc = encode_poll_for_platform_event_message_req(
        0, formatVersion, transferOperationFlag, dataTransferHandle,
        eventIdToAcknowledge, request,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    PLDM_MSGBUF_DEFINE_P(buf);
    rc = pldm_msgbuf_init_errno(
        buf, PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES, request->payload,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    ASSERT_EQ(rc, 0);

    uint8_t retFormatVersion;
    uint8_t retTransferOperationFlag;
    uint32_t retDataTransferHandle;
    uint16_t retEventIdToAcknowledge;

    pldm_msgbuf_extract_uint8(buf, retFormatVersion);
    pldm_msgbuf_extract_uint8(buf, retTransferOperationFlag);
    pldm_msgbuf_extract_uint32(buf, retDataTransferHandle);
    pldm_msgbuf_extract_uint16(buf, retEventIdToAcknowledge);
    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    EXPECT_EQ(retFormatVersion, formatVersion);
    EXPECT_EQ(retTransferOperationFlag, transferOperationFlag);
    EXPECT_EQ(retDataTransferHandle, dataTransferHandle);
    EXPECT_EQ(retEventIdToAcknowledge, eventIdToAcknowledge);
}

TEST(PollForPlatformEventMessage, testGoodEncodeRequestAckOnly)
{
    uint8_t formatVersion = 0x01;
    uint8_t transferOperationFlag = PLDM_ACKNOWLEDGEMENT_ONLY;
    uint32_t dataTransferHandle = 0xaabbccdd;
    uint16_t eventIdToAcknowledge = 0x1234;

    PLDM_MSG_DEFINE_P(request, PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);

    auto rc = encode_poll_for_platform_event_message_req(
        0, formatVersion, transferOperationFlag, dataTransferHandle,
        eventIdToAcknowledge, request,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    PLDM_MSGBUF_DEFINE_P(buf);
    rc = pldm_msgbuf_init_errno(
        buf, PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES, request->payload,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    ASSERT_EQ(rc, 0);

    uint8_t retFormatVersion;
    uint8_t retTransferOperationFlag;
    uint32_t retDataTransferHandle;
    uint16_t retEventIdToAcknowledge;

    pldm_msgbuf_extract_uint8(buf, retFormatVersion);
    pldm_msgbuf_extract_uint8(buf, retTransferOperationFlag);
    pldm_msgbuf_extract_uint32(buf, retDataTransferHandle);
    pldm_msgbuf_extract_uint16(buf, retEventIdToAcknowledge);
    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    EXPECT_EQ(retFormatVersion, formatVersion);
    EXPECT_EQ(retTransferOperationFlag, transferOperationFlag);
    EXPECT_EQ(retDataTransferHandle, dataTransferHandle);
    EXPECT_EQ(retEventIdToAcknowledge, eventIdToAcknowledge);
}

TEST(PollForPlatformEventMessage, testBadEncodeRequest)
{
    uint8_t formatVersion = 0x01;
    uint8_t transferOperationFlag = PLDM_GET_FIRSTPART;
    uint32_t dataTransferHandle = 0xaabbccdd;
    uint16_t eventIdToAcknowledge = 0x1234;

    PLDM_MSG_DEFINE_P(request, PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);

    auto rc = encode_poll_for_platform_event_message_req(
        0, formatVersion, transferOperationFlag, dataTransferHandle,
        eventIdToAcknowledge, nullptr,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    transferOperationFlag = PLDM_GET_FIRSTPART;
    eventIdToAcknowledge = PLDM_PLATFORM_EVENT_ID_FRAGMENT;
    rc = encode_poll_for_platform_event_message_req(
        0, formatVersion, transferOperationFlag, dataTransferHandle,
        eventIdToAcknowledge, request,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    transferOperationFlag = PLDM_GET_NEXTPART;
    eventIdToAcknowledge = 0x1234;
    rc = encode_poll_for_platform_event_message_req(
        0, formatVersion, transferOperationFlag, dataTransferHandle,
        eventIdToAcknowledge, request,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    transferOperationFlag = PLDM_GET_NEXTPART;
    eventIdToAcknowledge = PLDM_PLATFORM_EVENT_ID_NULL;
    rc = encode_poll_for_platform_event_message_req(
        0, formatVersion, transferOperationFlag, dataTransferHandle,
        eventIdToAcknowledge, request,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    transferOperationFlag = PLDM_ACKNOWLEDGEMENT_ONLY;
    eventIdToAcknowledge = PLDM_PLATFORM_EVENT_ID_NULL;
    rc = encode_poll_for_platform_event_message_req(
        0, formatVersion, transferOperationFlag, dataTransferHandle,
        eventIdToAcknowledge, request,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    transferOperationFlag = PLDM_ACKNOWLEDGEMENT_ONLY;
    eventIdToAcknowledge = PLDM_PLATFORM_EVENT_ID_FRAGMENT;
    rc = encode_poll_for_platform_event_message_req(
        0, formatVersion, transferOperationFlag, dataTransferHandle,
        eventIdToAcknowledge, request,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    transferOperationFlag = PLDM_ACKNOWLEDGEMENT_ONLY + 1;
    eventIdToAcknowledge = PLDM_PLATFORM_EVENT_ID_FRAGMENT;
    rc = encode_poll_for_platform_event_message_req(
        0, formatVersion, transferOperationFlag, dataTransferHandle,
        eventIdToAcknowledge, request,
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(PollForPlatformEventMessage, testGoodDecodeRespond)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t tId = 0x9;
    uint16_t eventId = 159;
    uint32_t nextDataTransferHandle = 0x11223344;
    uint8_t transferFlag = PLDM_START_AND_END;
    uint8_t eventClass = 0x5;
    uint8_t eventData[5] = {0x55, 0x44, 0x33, 0x22, 0x11};
    constexpr uint32_t eventDataSize = 0x00000005;
    uint32_t eventDataIntegrityChecksum = 0x66778899;

    std::vector<uint8_t> responseMsg{
        0x1,
        0x0,
        0x0,
        PLDM_SUCCESS,
        0x9, // tid
        159,
        0x0, // event id
        0x44,
        0x33,
        0x22,
        0x11,               // next_data_transfer_handle
        PLDM_START_AND_END, // transfer_flag
        0x05,               // event class
        0x05,
        0x00,
        0x00,
        0x00, // event_data_size
        0x55,
        0x44,
        0x33,
        0x22,
        0x11, // event_data[5]
        0x99,
        0x88,
        0x77,
        0x66 // event_data_integrity_checksum
    };
    const uint32_t respMsgLen = 23;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    uint8_t retCompletionCode;
    uint8_t retTid = 0;
    uint16_t retEventId = 0;
    uint32_t retNextDataTransferHandle = 0;
    uint8_t retTransferFlag = 0;
    uint8_t retEventClass = 0;
    uint32_t retEventDataSize = 0;
    uint8_t* retEventData = nullptr;
    uint32_t retEventDataIntegrityChecksum = 0;

    auto rc = decode_poll_for_platform_event_message_resp(
        response, respMsgLen, &retCompletionCode, &retTid, &retEventId,
        &retNextDataTransferHandle, &retTransferFlag, &retEventClass,
        &retEventDataSize, (void**)&retEventData,
        &retEventDataIntegrityChecksum);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retCompletionCode, completionCode);
    EXPECT_EQ(retTid, tId);
    EXPECT_EQ(retEventId, eventId);
    EXPECT_EQ(retNextDataTransferHandle, nextDataTransferHandle);
    EXPECT_EQ(retTransferFlag, transferFlag);
    EXPECT_EQ(retEventClass, eventClass);
    EXPECT_EQ(retEventDataSize, eventDataSize);
    EXPECT_EQ(retEventDataIntegrityChecksum, eventDataIntegrityChecksum);
    EXPECT_EQ(0, memcmp(eventData, retEventData, eventDataSize));
}

TEST(PollForPlatformEventMessage, testGoodDecodeAckOnlyRespond)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t tId = 0x9;
    uint16_t eventId = 0xffff;

    std::vector<uint8_t> responseMsg{
        0x1,  0x0, 0x0, PLDM_SUCCESS,
        0x9, // tid
        0xff,
        0xff // event id
    };
    const uint32_t respMsgLen = 4;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    uint8_t retCompletionCode;
    uint8_t retTid = 0;
    uint16_t retEventId = 0;
    uint32_t retNextDataTransferHandle = 0;
    uint8_t retTransferFlag = 0;
    uint8_t retEventClass = 0;
    uint32_t retEventDataSize = 0;
    uint8_t* retEventData = nullptr;
    uint32_t retEventDataIntegrityChecksum = 0;

    auto rc = decode_poll_for_platform_event_message_resp(
        response, respMsgLen, &retCompletionCode, &retTid, &retEventId,
        &retNextDataTransferHandle, &retTransferFlag, &retEventClass,
        &retEventDataSize, (void**)&retEventData,
        &retEventDataIntegrityChecksum);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retCompletionCode, completionCode);
    EXPECT_EQ(retTid, tId);
    EXPECT_EQ(retEventId, eventId);

    eventId = 0x0000;
    responseMsg[5] = 0x00;
    responseMsg[6] = 0x00;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    rc = decode_poll_for_platform_event_message_resp(
        response, respMsgLen, &retCompletionCode, &retTid, &retEventId,
        &retNextDataTransferHandle, &retTransferFlag, &retEventClass,
        &retEventDataSize, (void**)&retEventData,
        &retEventDataIntegrityChecksum);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retCompletionCode, completionCode);
    EXPECT_EQ(retTid, tId);
    EXPECT_EQ(retEventId, eventId);
}

TEST(PollForPlatformEventMessage, testBadDecodeRespond)
{
    std::vector<uint8_t> responseMsg{
        0x1,
        0x0,
        0x0,
        PLDM_SUCCESS,
        0x9, // tid
        159,
        0x0, // event id
        0x44,
        0x33,
        0x22,
        0x11,               // next_data_transfer_handle
        PLDM_START_AND_END, // transfer_flag
        0x05,               // event class
        0x05,
        0x00,
        0x00,
        0x00, // event_data_size
        0x55,
        0x44,
        0x33,
        0x22,
        0x11, // event_data[5]
        0x99,
        0x88,
        0x77,
        0x66 // event_data_integrity_checksum
    };
    // const uint32_t respMsgLen = 23;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_poll_for_platform_event_message_resp(
        nullptr, 0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    uint8_t retCompletionCode;
    uint8_t retTid = 0;
    uint16_t retEventId = 0;
    uint32_t retNextDataTransferHandle = 0;
    uint8_t retTransferFlag = 0;
    uint8_t retEventClass = 0;
    uint32_t retEventDataSize = 0;
    uint8_t* retEventData = nullptr;
    uint32_t retEventDataIntegrityChecksum = 0;

    rc = decode_poll_for_platform_event_message_resp(
        response, PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_MIN_RESP_BYTES - 1,
        &retCompletionCode, &retTid, &retEventId, &retNextDataTransferHandle,
        &retTransferFlag, &retEventClass, &retEventDataSize,
        (void**)&retEventData, &retEventDataIntegrityChecksum);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(PollForPlatformEventMessage, testGoodDecodeRequestFirstPart)
{
    uint8_t formatVersion = 0x1;
    uint8_t transferOperationFlag = PLDM_GET_FIRSTPART;
    uint32_t dataTransferHandle = 0x11223344;
    uint16_t eventIdToAcknowledge = PLDM_PLATFORM_EVENT_ID_NULL;
    std::vector<uint8_t> requestMsg{0x1,  0x0,  0x0,  0x1,  PLDM_GET_FIRSTPART,
                                    0x44, 0x33, 0x22, 0x11, 0x00,
                                    0x00};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    uint8_t retFormatVersion;
    uint8_t retTransferOperationFlag;
    uint32_t retDataTransferHandle;
    uint16_t retEventIdToAcknowledge;

    auto rc = decode_poll_for_platform_event_message_req(
        request, requestMsg.size() - hdrSize, &retFormatVersion,
        &retTransferOperationFlag, &retDataTransferHandle,
        &retEventIdToAcknowledge);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retFormatVersion, formatVersion);
    EXPECT_EQ(retTransferOperationFlag, transferOperationFlag);
    EXPECT_EQ(retDataTransferHandle, dataTransferHandle);
    EXPECT_EQ(retEventIdToAcknowledge, eventIdToAcknowledge);
}

TEST(PollForPlatformEventMessage, testGoodDecodeRequestNextPart)
{
    uint8_t formatVersion = 0x1;
    uint8_t transferOperationFlag = PLDM_GET_NEXTPART;
    uint32_t dataTransferHandle = 0x11223344;
    uint16_t eventIdToAcknowledge = PLDM_PLATFORM_EVENT_ID_FRAGMENT;
    std::vector<uint8_t> requestMsg{0x1,  0x0,  0x0,  0x1,  PLDM_GET_NEXTPART,
                                    0x44, 0x33, 0x22, 0x11, 0xff,
                                    0xff};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    uint8_t retFormatVersion;
    uint8_t retTransferOperationFlag;
    uint32_t retDataTransferHandle;
    uint16_t retEventIdToAcknowledge;

    auto rc = decode_poll_for_platform_event_message_req(
        request, requestMsg.size() - hdrSize, &retFormatVersion,
        &retTransferOperationFlag, &retDataTransferHandle,
        &retEventIdToAcknowledge);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retFormatVersion, formatVersion);
    EXPECT_EQ(retTransferOperationFlag, transferOperationFlag);
    EXPECT_EQ(retDataTransferHandle, dataTransferHandle);
    EXPECT_EQ(retEventIdToAcknowledge, eventIdToAcknowledge);
}

TEST(PollForPlatformEventMessage, testGoodDecodeRequestAck)
{
    uint8_t formatVersion = 0x1;
    uint8_t transferOperationFlag = PLDM_ACKNOWLEDGEMENT_ONLY;
    uint32_t dataTransferHandle = 0x11223344;
    uint16_t eventIdToAcknowledge = 0x1234;
    std::vector<uint8_t> requestMsg{
        0x1,  0x0,  0x0,  0x1, PLDM_ACKNOWLEDGEMENT_ONLY, 0x44, 0x33,
        0x22, 0x11, 0x34, 0x12};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    uint8_t retFormatVersion;
    uint8_t retTransferOperationFlag;
    uint32_t retDataTransferHandle;
    uint16_t retEventIdToAcknowledge;

    auto rc = decode_poll_for_platform_event_message_req(
        request, requestMsg.size() - hdrSize, &retFormatVersion,
        &retTransferOperationFlag, &retDataTransferHandle,
        &retEventIdToAcknowledge);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retFormatVersion, formatVersion);
    EXPECT_EQ(retTransferOperationFlag, transferOperationFlag);
    EXPECT_EQ(retDataTransferHandle, dataTransferHandle);
    EXPECT_EQ(retEventIdToAcknowledge, eventIdToAcknowledge);
}

TEST(PollForPlatformEventMessage, testBadDecodeRequest)
{
    /*
     * transfer_operation_flag is PLDM_GET_FIRSTPART and
     * event_id_to_acknowledge is not PLDM_PLATFORM_EVENT_ID_NULL
     */
    std::vector<uint8_t> requestMsg{0x1,  0x0,  0x0,  0x1,  PLDM_GET_FIRSTPART,
                                    0x44, 0x33, 0x22, 0x11, 0x66,
                                    0x55};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    uint8_t retFormatVersion;
    uint8_t retTransferOperationFlag;
    uint32_t retDataTransferHandle;
    uint16_t retEventIdToAcknowledge;

    auto rc = decode_poll_for_platform_event_message_req(
        NULL, requestMsg.size() - hdrSize, &retFormatVersion,
        &retTransferOperationFlag, &retDataTransferHandle,
        &retEventIdToAcknowledge);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    /*
     * transfer_operation_flag is not PLDM_GET_FIRSTPART or PLDM_GET_NEXTPART or
     * PLDM_ACKNOWLEDGEMENT_ONLY
     */

    requestMsg[4] = PLDM_ACKNOWLEDGEMENT_ONLY + 1;

    rc = decode_poll_for_platform_event_message_req(
        request, requestMsg.size() - hdrSize, &retFormatVersion,
        &retTransferOperationFlag, &retDataTransferHandle,
        &retEventIdToAcknowledge);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    /*
     * transfer_operation_flag is PLDM_GET_NEXTPART and
     * event_id_to_acknowledge is not PLDM_PLATFORM_EVENT_ID_FRAGMENT
     */
    requestMsg[4] = PLDM_GET_NEXTPART;
    requestMsg[9] = 0x11;
    requestMsg[10] = 0x22;

    rc = decode_poll_for_platform_event_message_req(
        request, requestMsg.size() - hdrSize, &retFormatVersion,
        &retTransferOperationFlag, &retDataTransferHandle,
        &retEventIdToAcknowledge);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    /*
     * transfer_operation_flag is PLDM_ACKNOWLEDGEMENT_ONLY and
     * event_id_to_acknowledge is PLDM_PLATFORM_EVENT_ID_FRAGMENT
     */
    requestMsg[4] = PLDM_ACKNOWLEDGEMENT_ONLY;
    requestMsg[9] = 0xff;
    requestMsg[10] = 0xff;

    rc = decode_poll_for_platform_event_message_req(
        request, requestMsg.size() - hdrSize, &retFormatVersion,
        &retTransferOperationFlag, &retDataTransferHandle,
        &retEventIdToAcknowledge);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    /*
     * transfer_operation_flag is PLDM_ACKNOWLEDGEMENT_ONLY and
     * event_id_to_acknowledge is PLDM_PLATFORM_EVENT_ID_NULL
     */
    requestMsg[4] = PLDM_ACKNOWLEDGEMENT_ONLY;
    requestMsg[9] = 0x00;
    requestMsg[10] = 0x00;

    rc = decode_poll_for_platform_event_message_req(
        request, requestMsg.size() - hdrSize, &retFormatVersion,
        &retTransferOperationFlag, &retDataTransferHandle,
        &retEventIdToAcknowledge);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(PollForPlatformEventMessage, testGoodEncodeResposeP1)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t instance_id = 0;
    uint8_t tId = 0x9;
    uint16_t eventId = 0x1;
    uint32_t nextDataTransferHandle = 0xffff;
    uint8_t transferFlag = PLDM_END;
    uint8_t eventClass = 0x5;
    constexpr uint32_t eventDataSize = 9;
    uint8_t pEventData[eventDataSize] = {0x31, 0x32, 0x33, 0x34, 0x35,
                                         0x36, 0x37, 0x38, 0x39};
    uint32_t eventDataIntegrityChecksum = 0x11223344;
    constexpr size_t payloadLength =
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_RESP_BYTES + eventDataSize + 4;

    std::array<uint8_t, hdrSize + payloadLength> responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_poll_for_platform_event_message_resp(
        instance_id, completionCode, tId, eventId, nextDataTransferHandle,
        transferFlag, eventClass, eventDataSize, pEventData,
        eventDataIntegrityChecksum, response, payloadLength);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    PLDM_MSGBUF_DEFINE_P(buf);
    rc = pldm_msgbuf_init_errno(
        buf, PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_MIN_RESP_BYTES,
        response->payload, payloadLength);
    ASSERT_EQ(rc, 0);

    uint8_t retCompletionCode;
    uint8_t retTid = 0;
    uint16_t retEventId = 0;
    uint32_t retNextDataTransferHandle = 0;
    uint8_t retTransferFlag = 0;
    uint8_t retEventClass = 0;
    uint32_t retEventDataSize = 0;
    uint8_t retEventData[payloadLength] = {0};
    uint32_t retEventDataIntegrityChecksum = 0;

    pldm_msgbuf_extract_uint8(buf, retCompletionCode);
    pldm_msgbuf_extract_uint8(buf, retTid);
    pldm_msgbuf_extract_uint16(buf, retEventId);
    pldm_msgbuf_extract_uint32(buf, retNextDataTransferHandle);
    pldm_msgbuf_extract_uint8(buf, retTransferFlag);
    pldm_msgbuf_extract_uint8(buf, retEventClass);
    pldm_msgbuf_extract_uint32(buf, retEventDataSize);
    rc = pldm_msgbuf_extract_array_uint8(buf, retEventDataSize, retEventData,
                                         sizeof(retEventData));
    ASSERT_EQ(rc, 0);
    pldm_msgbuf_extract_uint32(buf, retEventDataIntegrityChecksum);
    ASSERT_EQ(pldm_msgbuf_complete(buf), 0);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retCompletionCode, completionCode);
    EXPECT_EQ(retTid, tId);
    EXPECT_EQ(retEventId, eventId);
    EXPECT_EQ(retNextDataTransferHandle, nextDataTransferHandle);
    EXPECT_EQ(retTransferFlag, transferFlag);
    EXPECT_EQ(retEventClass, eventClass);
    EXPECT_EQ(retEventDataSize, eventDataSize);
    EXPECT_EQ(retEventDataIntegrityChecksum, eventDataIntegrityChecksum);
    EXPECT_EQ(0, memcmp(pEventData, retEventData, eventDataSize));
}

TEST(PollForPlatformEventMessage, testGoodEncodeResposeP2)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t instance_id = 0;
    uint8_t tId = 0x9;
    uint16_t eventId = 0x0000;
    constexpr size_t payloadLength =
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_MIN_RESP_BYTES;

    std::array<uint8_t, hdrSize + payloadLength> responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_poll_for_platform_event_message_resp(
        instance_id, completionCode, tId, eventId, 0, 0, 0, 0, NULL, 0,
        response, payloadLength);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    PLDM_MSGBUF_DEFINE_P(buf);
    rc = pldm_msgbuf_init_errno(
        buf, PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_MIN_RESP_BYTES,
        response->payload, payloadLength);
    ASSERT_EQ(rc, 0);

    uint8_t retCompletionCode;
    uint8_t retTid = 0;
    uint16_t retEventId = 0;

    pldm_msgbuf_extract_uint8(buf, retCompletionCode);
    pldm_msgbuf_extract_uint8(buf, retTid);
    pldm_msgbuf_extract_uint16(buf, retEventId);
    ASSERT_EQ(pldm_msgbuf_complete(buf), 0);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retCompletionCode, completionCode);
    EXPECT_EQ(retTid, tId);
    EXPECT_EQ(retEventId, eventId);
}

TEST(PollForPlatformEventMessage, testGoodEncodeResposeP3)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t instance_id = 0;
    uint8_t tId = 0x9;
    uint16_t eventId = 0xffff;
    constexpr size_t payloadLength =
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_MIN_RESP_BYTES;

    std::array<uint8_t, hdrSize + payloadLength> responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_poll_for_platform_event_message_resp(
        instance_id, completionCode, tId, eventId, 0, 0, 0, 0, NULL, 0,
        response, payloadLength);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    PLDM_MSGBUF_DEFINE_P(buf);
    rc = pldm_msgbuf_init_errno(
        buf, PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_MIN_RESP_BYTES,
        response->payload, payloadLength);
    ASSERT_EQ(rc, 0);

    uint8_t retCompletionCode;
    uint8_t retTid = 0;
    uint16_t retEventId = 0;

    pldm_msgbuf_extract_uint8(buf, retCompletionCode);
    pldm_msgbuf_extract_uint8(buf, retTid);
    pldm_msgbuf_extract_uint16(buf, retEventId);
    ASSERT_EQ(pldm_msgbuf_complete(buf), PLDM_SUCCESS);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retCompletionCode, completionCode);
    EXPECT_EQ(retTid, tId);
    EXPECT_EQ(retEventId, eventId);
}

TEST(PollForPlatformEventMessage, testGoodEncodeResposeP4)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t instance_id = 0;
    uint8_t tId = 0x9;
    uint16_t eventId = 0x1;
    uint32_t nextDataTransferHandle = 0xffff;
    uint8_t transferFlag = PLDM_END;
    uint8_t eventClass = 0x5;
    constexpr uint32_t eventDataSize = 0;
    uint32_t eventDataIntegrityChecksum = 0x11223344;
    size_t payloadLength =
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_RESP_BYTES + eventDataSize + 4;

    std::array<uint8_t, hdrSize +
                            PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_RESP_BYTES +
                            eventDataSize + 4>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_poll_for_platform_event_message_resp(
        instance_id, completionCode, tId, eventId, nextDataTransferHandle,
        transferFlag, eventClass, eventDataSize, NULL,
        eventDataIntegrityChecksum, response, payloadLength);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    PLDM_MSGBUF_DEFINE_P(buf);
    rc = pldm_msgbuf_init_errno(
        buf, PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_MIN_RESP_BYTES,
        response->payload, payloadLength);
    ASSERT_EQ(rc, 0);

    uint8_t retCompletionCode;
    uint8_t retTid = 0;
    uint16_t retEventId = 0;
    uint32_t retNextDataTransferHandle = 0;
    uint8_t retTransferFlag = 0;
    uint8_t retEventClass = 0;
    uint32_t retEventDataSize = 0;
    uint32_t retEventDataIntegrityChecksum = 0;

    pldm_msgbuf_extract_uint8(buf, retCompletionCode);
    pldm_msgbuf_extract_uint8(buf, retTid);
    pldm_msgbuf_extract_uint16(buf, retEventId);
    pldm_msgbuf_extract_uint32(buf, retNextDataTransferHandle);
    pldm_msgbuf_extract_uint8(buf, retTransferFlag);
    pldm_msgbuf_extract_uint8(buf, retEventClass);
    pldm_msgbuf_extract_uint32(buf, retEventDataSize);
    pldm_msgbuf_extract_uint32(buf, retEventDataIntegrityChecksum);
    EXPECT_EQ(pldm_msgbuf_complete(buf), 0);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retCompletionCode, completionCode);
    EXPECT_EQ(retTid, tId);
    EXPECT_EQ(retEventId, eventId);
    EXPECT_EQ(retNextDataTransferHandle, nextDataTransferHandle);
    EXPECT_EQ(retTransferFlag, transferFlag);
    EXPECT_EQ(retEventClass, eventClass);
    EXPECT_EQ(retEventDataSize, eventDataSize);
    EXPECT_EQ(retEventDataIntegrityChecksum, eventDataIntegrityChecksum);
}

TEST(PollForPlatformEventMessage, testBadEncodeResponse)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t instance_id = 0;
    uint8_t tId = 0x9;
    uint16_t eventId = 0x1;
    uint32_t nextDataTransferHandle = 0xffff;
    uint8_t transferFlag = 0x0;
    uint8_t eventClass = 0x5;
    const uint32_t eventDataSize = 0;
    uint32_t eventDataIntegrityChecksum = 0x11223344;
    constexpr size_t payloadLength =
        PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_RESP_BYTES + eventDataSize + 4;

    std::array<uint8_t, hdrSize + payloadLength> responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_poll_for_platform_event_message_resp(
        instance_id, completionCode, tId, eventId, nextDataTransferHandle,
        transferFlag, eventClass, eventDataSize, NULL,
        eventDataIntegrityChecksum, NULL, payloadLength);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_poll_for_platform_event_message_resp(
        instance_id, completionCode, tId, eventId, nextDataTransferHandle,
        transferFlag, eventClass, 1, NULL, eventDataIntegrityChecksum, response,
        payloadLength);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(PlatformEventMessage, testGoodStateSensorDecodeRequest)
{
    std::array<uint8_t,
               hdrSize + PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES +
                   PLDM_PLATFORM_EVENT_MESSAGE_STATE_SENSOR_STATE_REQ_BYTES>
        requestMsg{};

    uint8_t retFormatVersion = 0;
    uint8_t retTid = 0;
    uint8_t retEventClass = 0;
    size_t retEventDataOffset = 0;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_platform_event_message_req* request =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_platform_event_message_req*>(req->payload);

    uint8_t formatVersion = 0x01;
    uint8_t tid = 0x02;
    // Sensor Event
    uint8_t eventClass = 0x00;

    request->format_version = formatVersion;
    request->tid = tid;
    request->event_class = eventClass;
    size_t eventDataOffset =
        sizeof(formatVersion) + sizeof(tid) + sizeof(eventClass);

    auto rc = decode_platform_event_message_req(
        req, requestMsg.size() - hdrSize, &retFormatVersion, &retTid,
        &retEventClass, &retEventDataOffset);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retFormatVersion, formatVersion);
    EXPECT_EQ(retTid, tid);
    EXPECT_EQ(retEventClass, eventClass);
    EXPECT_EQ(retEventDataOffset, eventDataOffset);
}

TEST(PlatformEventMessage, testBadDecodeRequest)
{
    const struct pldm_msg* msg = NULL;
    std::array<uint8_t,
               hdrSize + PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES +
                   PLDM_PLATFORM_EVENT_MESSAGE_STATE_SENSOR_STATE_REQ_BYTES - 1>
        requestMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());
    uint8_t retFormatVersion;
    uint8_t retTid = 0;
    uint8_t retEventClass = 0;
    size_t retEventDataOffset;

    auto rc = decode_platform_event_message_req(msg, sizeof(*msg), NULL, NULL,
                                                NULL, NULL);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_platform_event_message_req(
        req,
        requestMsg.size() - hdrSize -
            PLDM_PLATFORM_EVENT_MESSAGE_STATE_SENSOR_STATE_REQ_BYTES,
        &retFormatVersion, &retTid, &retEventClass, &retEventDataOffset);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(PlatformEventMessage, testGoodEncodeResponse)
{
    std::array<uint8_t,
               hdrSize + PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES +
                   PLDM_PLATFORM_EVENT_MESSAGE_STATE_SENSOR_STATE_REQ_BYTES - 1>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    uint8_t completionCode = 0;
    uint8_t instanceId = 0x01;
    uint8_t platformEventStatus = 0x01;

    auto rc = encode_platform_event_message_resp(instanceId, PLDM_SUCCESS,
                                                 platformEventStatus, response);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, response->payload[0]);
    EXPECT_EQ(platformEventStatus, response->payload[1]);
}

TEST(PlatformEventMessage, testBadEncodeResponse)
{
    auto rc = encode_platform_event_message_resp(0, PLDM_SUCCESS, 1, NULL);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(PlatformEventMessage, testGoodEncodeRequest)
{
    static constexpr const uint8_t formatVersion = 0x01;
    static constexpr const uint8_t eventClass = 0x00;
    static constexpr const uint8_t eventData = 34;
    static constexpr const uint8_t Tid = 0x03;
    struct pldm_platform_event_message_req req;
    PLDM_MSGBUF_DEFINE_P(buf);
    size_t len;
    void* data;

    PLDM_MSG_DEFINE_P(request, PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES +
                                   sizeof(eventData));

    /* Test with the minimum event type value */
    auto rc = encode_platform_event_message_req(
        0, formatVersion, Tid, eventClass, &eventData, sizeof(eventData),
        request, sizeof(eventData) + PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES);
    ASSERT_EQ(rc, PLDM_SUCCESS);

    rc = pldm_msgbuf_init_errno(
        buf, PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES, request->payload,
        PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES + sizeof(eventData));
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_extract_uint8(buf, req.format_version);
    pldm_msgbuf_extract_uint8(buf, req.tid);
    pldm_msgbuf_extract_uint8(buf, req.event_class);
    data = nullptr;
    pldm_msgbuf_span_remaining(buf, &data, &len);
    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    EXPECT_EQ(formatVersion, req.format_version);
    EXPECT_EQ(Tid, req.tid);
    EXPECT_EQ(eventClass, req.event_class);
    ASSERT_EQ(sizeof(eventData), len);
    EXPECT_EQ(0, memcmp(&eventData, data, len));

    /* Test with the maximum event type value */
    rc = encode_platform_event_message_req(
        0, formatVersion, Tid, PLDM_CPER_EVENT, &eventData, sizeof(eventData),
        request, sizeof(eventData) + PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES);
    ASSERT_EQ(rc, PLDM_SUCCESS);

    rc = pldm_msgbuf_init_errno(
        buf, PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES, request->payload,
        PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES + sizeof(eventData));
    ASSERT_EQ(rc, 0);

    pldm_msgbuf_extract_uint8(buf, req.format_version);
    pldm_msgbuf_extract_uint8(buf, req.tid);
    pldm_msgbuf_extract_uint8(buf, req.event_class);

    data = nullptr;
    pldm_msgbuf_span_remaining(buf, &data, &len);
    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), 0);

    EXPECT_EQ(formatVersion, req.format_version);
    EXPECT_EQ(Tid, req.tid);
    EXPECT_EQ(PLDM_CPER_EVENT, req.event_class);
    ASSERT_EQ(sizeof(eventData), len);
    EXPECT_EQ(0, memcmp(&eventData, data, len));
}

TEST(PlatformEventMessage, testBadEncodeRequest)
{
    uint8_t Tid = 0x03;
    uint8_t eventClass = 0x00;
    uint8_t eventData = 34;
    uint8_t formatVersion = 0x01;
    static constexpr const size_t payloadLen =
        PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES + sizeof(eventData);

    PLDM_MSG_DEFINE_P(request, payloadLen);

    auto rc = encode_platform_event_message_req(
        0, formatVersion, Tid, eventClass, &eventData, sizeof(eventData),
        nullptr, payloadLen);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_platform_event_message_req(0, 0, Tid, eventClass, &eventData,
                                           sizeof(eventData), request,
                                           payloadLen);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_platform_event_message_req(0, formatVersion, Tid, eventClass,
                                           nullptr, 0, request, payloadLen);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_platform_event_message_req(0, formatVersion, Tid, eventClass,
                                           &eventData, sizeof(eventData),
                                           request, 0);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    rc = encode_platform_event_message_req(
        0, formatVersion, Tid, PLDM_CPER_EVENT + 1, &eventData,
        sizeof(eventData), request, payloadLen);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(PlatformEventMessage, testGoodDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_PLATFORM_EVENT_MESSAGE_RESP_BYTES>
        responseMsg{};

    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t platformEventStatus = 0x01;

    uint8_t retcompletionCode;
    uint8_t retplatformEventStatus;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_platform_event_message_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_platform_event_message_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->platform_event_status = platformEventStatus;

    auto rc = decode_platform_event_message_resp(
        response, responseMsg.size() - hdrSize, &retcompletionCode,
        &retplatformEventStatus);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, retcompletionCode);
    EXPECT_EQ(platformEventStatus, retplatformEventStatus);
}

TEST(PlatformEventMessage, testBadDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_PLATFORM_EVENT_MESSAGE_RESP_BYTES>
        responseMsg{};

    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t platformEventStatus = 0x01;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_platform_event_message_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_platform_event_message_resp*>(
            response->payload);
    resp->completion_code = completionCode;
    resp->platform_event_status = platformEventStatus;

    auto rc = decode_platform_event_message_resp(
        nullptr, responseMsg.size() - hdrSize, nullptr, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_platform_event_message_resp(
        response, responseMsg.size() - hdrSize - 1, &completionCode,
        &platformEventStatus);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(PlatformEventMessage, testGoodSensorEventDataDecodeRequest)
{
    std::array<uint8_t, PLDM_SENSOR_EVENT_SENSOR_OP_STATE_DATA_LENGTH +
                            PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES>
        eventDataArr{};
    uint16_t sensorId = 0x1234;
    uint8_t sensorEventClassType = PLDM_SENSOR_OP_STATE;

    struct pldm_sensor_event_data* eventData =
        (struct pldm_sensor_event_data*)eventDataArr.data();
    eventData->sensor_id = sensorId;
    eventData->sensor_event_class_type = sensorEventClassType;

    size_t retSensorOpDataOffset;
    uint16_t retSensorId = 0;
    uint8_t retSensorEventClassType;
    size_t sensorOpDataOffset = sizeof(sensorId) + sizeof(sensorEventClassType);
    auto rc = decode_sensor_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventData), eventDataArr.size(),
        &retSensorId, &retSensorEventClassType, &retSensorOpDataOffset);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retSensorId, sensorId);
    EXPECT_EQ(retSensorEventClassType, sensorEventClassType);
    EXPECT_EQ(retSensorOpDataOffset, sensorOpDataOffset);
}

TEST(PlatformEventMessage, testBadSensorEventDataDecodeRequest)
{

    std::array<uint8_t, PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_MAX_DATA_LENGTH +
                            PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES>
        eventDataArr{};

    struct pldm_sensor_event_data* eventData =
        (struct pldm_sensor_event_data*)eventDataArr.data();

    size_t retSensorOpDataOffset;
    uint16_t retSensorId = 0;
    uint8_t retSensorEventClassType;
    auto rc = decode_sensor_event_data(NULL, eventDataArr.size(), &retSensorId,
                                       &retSensorEventClassType,
                                       &retSensorOpDataOffset);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_sensor_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventDataArr.data()),
        eventDataArr.size() -
            PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_MAX_DATA_LENGTH,
        &retSensorId, &retSensorEventClassType, &retSensorOpDataOffset);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    eventData->sensor_event_class_type = PLDM_SENSOR_OP_STATE;

    rc = decode_sensor_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventDataArr.data()), eventDataArr.size(),
        &retSensorId, &retSensorEventClassType, &retSensorOpDataOffset);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    eventData->sensor_event_class_type = PLDM_STATE_SENSOR_STATE;
    rc = decode_sensor_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventDataArr.data()), eventDataArr.size(),
        &retSensorId, &retSensorEventClassType, &retSensorOpDataOffset);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    eventData->sensor_event_class_type = PLDM_NUMERIC_SENSOR_STATE;
    rc = decode_sensor_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventDataArr.data()),
        eventDataArr.size() + 1, &retSensorId, &retSensorEventClassType,
        &retSensorOpDataOffset);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(PlatformEventMessage, testGoodPldmMsgPollEventDataDecodeRequest)
{
    std::array<uint8_t, PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION +
                            PLDM_PLATFORM_EVENT_MESSAGE_EVENT_ID +
                            PLDM_PLATFORM_EVENT_MESSAGE_TRANFER_HANDLE>
        eventData{
            0x1,                   // version
            0x88, 0x77,            // Event Id
            0x44, 0x33, 0x22, 0x11 // Transfer Handle
        };

    uint8_t formatVersion = 0x01;
    uint16_t eventID = 0x7788;
    uint32_t dataTransferHandle = 0x11223344;

    struct pldm_message_poll_event poll_event = {};

    auto rc = decode_pldm_message_poll_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventData.data()), eventData.size(),
        &poll_event);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(poll_event.format_version, formatVersion);
    EXPECT_EQ(poll_event.event_id, eventID);
    EXPECT_EQ(poll_event.data_transfer_handle, dataTransferHandle);
}

TEST(PlatformEventMessage, testBadPldmMsgPollEventDataDecodeRequest)
{

    std::array<uint8_t, PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION +
                            PLDM_PLATFORM_EVENT_MESSAGE_EVENT_ID +
                            PLDM_PLATFORM_EVENT_MESSAGE_TRANFER_HANDLE>
        eventData{
            0x1,                   // version
            0x88, 0x77,            // Event Id
            0x44, 0x33, 0x22, 0x11 // Transfer Handle
        };

    struct pldm_message_poll_event poll_event = {};

    auto rc = decode_pldm_message_poll_event_data(NULL, eventData.size(),
                                                  &poll_event);
    EXPECT_EQ(rc, -EINVAL);

    rc = decode_pldm_message_poll_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventData.data()), eventData.size(), NULL);
    EXPECT_EQ(rc, -EINVAL);

    rc = decode_pldm_message_poll_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventData.data()), eventData.size() - 1,
        &poll_event);
    EXPECT_EQ(rc, -EOVERFLOW);

    // Event id is 0x0000
    eventData[1] = 0x00;
    eventData[2] = 0x00;
    rc = decode_pldm_message_poll_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventData.data()), eventData.size(),
        &poll_event);

    EXPECT_EQ(rc, -EPROTO);

    // Event id is 0xffff
    eventData[1] = 0xff;
    eventData[2] = 0xff;
    rc = decode_pldm_message_poll_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventData.data()), eventData.size(),
        &poll_event);

    EXPECT_EQ(rc, -EPROTO);
}

#ifdef LIBPLDM_API_TESTING
TEST(PlatformEventMessage, testGoodPldmMsgPollEventDataEncode)
{
    std::array<uint8_t, PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION +
                            PLDM_PLATFORM_EVENT_MESSAGE_EVENT_ID +
                            PLDM_PLATFORM_EVENT_MESSAGE_TRANFER_HANDLE>
        eventData{};

    struct pldm_message_poll_event poll_event = {};
    poll_event.format_version = 0x01;
    poll_event.event_id = 0x7788;
    poll_event.data_transfer_handle = 0x11223344;

    int rc = encode_pldm_message_poll_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        &poll_event, reinterpret_cast<uint8_t*>(eventData.data()),
        eventData.size());

    EXPECT_EQ(rc, PLDM_SUCCESS);

    PLDM_MSGBUF_DEFINE_P(buf);

    rc = pldm_msgbuf_init_errno(
        buf, PLDM_MSG_POLL_EVENT_LENGTH,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventData.data()), eventData.size());
    ASSERT_EQ(rc, 0);

    uint8_t retFormatVersion;
    uint16_t reteventID;
    uint32_t retDataTransferHandle;

    pldm_msgbuf_extract_uint8(buf, retFormatVersion);
    pldm_msgbuf_extract_uint16(buf, reteventID);
    pldm_msgbuf_extract_uint32(buf, retDataTransferHandle);
    ASSERT_EQ(pldm_msgbuf_complete_consumed(buf), PLDM_SUCCESS);

    EXPECT_EQ(retFormatVersion, poll_event.format_version);
    EXPECT_EQ(reteventID, poll_event.event_id);
    EXPECT_EQ(retDataTransferHandle, poll_event.data_transfer_handle);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(PlatformEventMessage, testBadPldmMsgPollEventDataEncode)
{
    std::array<uint8_t, PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION +
                            PLDM_PLATFORM_EVENT_MESSAGE_EVENT_ID +
                            PLDM_PLATFORM_EVENT_MESSAGE_TRANFER_HANDLE>
        eventData{};

    struct pldm_message_poll_event poll_event = {};
    poll_event.format_version = 0x01;
    poll_event.event_id = 0x7788;
    poll_event.data_transfer_handle = 0x11223344;

    int rc = encode_pldm_message_poll_event_data(&poll_event, NULL,
                                                 eventData.size());
    EXPECT_EQ(rc, -EINVAL);

    poll_event.event_id = 0x0000;
    rc = encode_pldm_message_poll_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        &poll_event, reinterpret_cast<uint8_t*>(eventData.data()),
        eventData.size());
    EXPECT_EQ(rc, -EPROTO);

    poll_event.event_id = 0xffff;
    rc = encode_pldm_message_poll_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        &poll_event, reinterpret_cast<uint8_t*>(eventData.data()),
        eventData.size());
    EXPECT_EQ(rc, -EPROTO);
}
#endif

TEST(PlatformEventMessage, testGoodSensorOpEventDataDecodeRequest)
{
    std::array<uint8_t, PLDM_SENSOR_EVENT_SENSOR_OP_STATE_DATA_LENGTH>
        eventDataArr{};

    struct pldm_sensor_event_sensor_op_state* sensorData =
        (struct pldm_sensor_event_sensor_op_state*)eventDataArr.data();
    uint8_t presentState = PLDM_SENSOR_ENABLED;
    uint8_t previousState = PLDM_SENSOR_INITIALIZING;
    sensorData->present_op_state = presentState;
    sensorData->previous_op_state = previousState;

    uint8_t retPresentState;
    uint8_t retPreviousState;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto rc = decode_sensor_op_data(reinterpret_cast<uint8_t*>(sensorData),
                                    eventDataArr.size(), &retPresentState,
                                    &retPreviousState);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retPresentState, presentState);
    EXPECT_EQ(retPreviousState, previousState);
}

TEST(PlatformEventMessage, testBadSensorOpEventDataDecodeRequest)
{
    uint8_t presentOpState;
    uint8_t previousOpState;
    size_t sensorDataLength = PLDM_SENSOR_EVENT_SENSOR_OP_STATE_DATA_LENGTH;
    auto rc = decode_sensor_op_data(NULL, sensorDataLength, &presentOpState,
                                    &previousOpState);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, PLDM_SENSOR_EVENT_SENSOR_OP_STATE_DATA_LENGTH>
        sensorData{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    rc = decode_sensor_op_data(reinterpret_cast<uint8_t*>(sensorData.data()),
                               sensorDataLength + 1, &presentOpState,
                               &previousOpState);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    rc = decode_sensor_op_data(reinterpret_cast<uint8_t*>(sensorData.data()),
                               sensorDataLength, nullptr, &previousOpState);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(PlatformEventMessage, testGoodSensorStateEventDataDecodeRequest)
{
    std::array<uint8_t, PLDM_SENSOR_EVENT_STATE_SENSOR_STATE_DATA_LENGTH>
        eventDataArr{};

    struct pldm_sensor_event_state_sensor_state* sensorData =
        (struct pldm_sensor_event_state_sensor_state*)eventDataArr.data();
    uint8_t sensorOffset = 0x02;
    uint8_t eventState = PLDM_SENSOR_SHUTTINGDOWN;
    uint8_t previousEventState = PLDM_SENSOR_INTEST;
    sensorData->sensor_offset = sensorOffset;
    sensorData->event_state = eventState;
    sensorData->previous_event_state = previousEventState;

    uint8_t retSensorOffset;
    uint8_t retEventState;
    uint8_t retPreviousState;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto rc = decode_state_sensor_data(reinterpret_cast<uint8_t*>(sensorData),
                                       eventDataArr.size(), &retSensorOffset,
                                       &retEventState, &retPreviousState);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retSensorOffset, sensorOffset);
    EXPECT_EQ(retEventState, eventState);
    EXPECT_EQ(retPreviousState, previousEventState);
}

TEST(PlatformEventMessage, testBadStateSensorEventDataDecodeRequest)
{
    uint8_t sensorOffset;
    uint8_t eventState;
    uint8_t previousEventState;
    size_t sensorDataLength = PLDM_SENSOR_EVENT_STATE_SENSOR_STATE_DATA_LENGTH;
    auto rc = decode_state_sensor_data(NULL, sensorDataLength, &sensorOffset,
                                       &eventState, &previousEventState);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, PLDM_SENSOR_EVENT_STATE_SENSOR_STATE_DATA_LENGTH>
        sensorData{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    rc = decode_state_sensor_data(reinterpret_cast<uint8_t*>(sensorData.data()),
                                  sensorDataLength - 1, &sensorOffset,
                                  &eventState, &previousEventState);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    rc = decode_state_sensor_data(reinterpret_cast<uint8_t*>(sensorData.data()),
                                  sensorDataLength, &sensorOffset, nullptr,
                                  &previousEventState);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(PlatformEventMessage, testGoodNumericSensorEventDataDecodeRequest)
{
    std::array<uint8_t, PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_MAX_DATA_LENGTH>
        eventDataArr{};
    struct pldm_sensor_event_numeric_sensor_state* sensorData =
        (struct pldm_sensor_event_numeric_sensor_state*)eventDataArr.data();

    size_t sensorDataLength =
        PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_32BIT_DATA_LENGTH;
    uint8_t eventState = PLDM_SENSOR_SHUTTINGDOWN;
    uint8_t previousEventState = PLDM_SENSOR_INTEST;
    uint8_t sensorDataSize = PLDM_SENSOR_DATA_SIZE_UINT32;
    uint32_t presentReading = 305441741;
    sensorData->event_state = eventState;
    sensorData->previous_event_state = previousEventState;
    sensorData->sensor_data_size = sensorDataSize;
    {
        uint32_t presentReadingLE = htole32(presentReading);
        memcpy(&sensorData->present_reading, &presentReadingLE,
               sizeof(presentReadingLE));
    }

    uint8_t retEventState;
    uint8_t retPreviousEventState;
    uint8_t retSensorDataSize;
    uint32_t retPresentReading;

    auto rc = decode_numeric_sensor_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(sensorData), sensorDataLength,
        &retEventState, &retPreviousEventState, &retSensorDataSize,
        &retPresentReading);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retEventState, eventState);
    EXPECT_EQ(retPreviousEventState, previousEventState);
    EXPECT_EQ(retSensorDataSize, sensorDataSize);
    EXPECT_EQ(retPresentReading, presentReading);

    int16_t presentReadingNew = -31432;
    {
        int16_t presentReadingNewLE = htole16(presentReadingNew);
        memcpy(&sensorData->present_reading, &presentReadingNewLE,
               sizeof(presentReadingNewLE));
    }
    sensorDataSize = PLDM_SENSOR_DATA_SIZE_SINT16;
    sensorData->sensor_data_size = sensorDataSize;
    sensorDataLength = PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_16BIT_DATA_LENGTH;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    rc = decode_numeric_sensor_data(reinterpret_cast<uint8_t*>(sensorData),
                                    sensorDataLength, &retEventState,
                                    &retPreviousEventState, &retSensorDataSize,
                                    &retPresentReading);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retEventState, eventState);
    EXPECT_EQ(retPreviousEventState, previousEventState);
    EXPECT_EQ(retSensorDataSize, sensorDataSize);
    EXPECT_EQ(static_cast<int16_t>(retPresentReading), presentReadingNew);
}

TEST(PlatformEventMessage, testBadNumericSensorEventDataDecodeRequest)
{
    uint8_t eventState;
    uint8_t previousEventState;
    uint8_t sensorDataSize;
    uint32_t presentReading;
    size_t sensorDataLength =
        PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_MAX_DATA_LENGTH;
    auto rc = decode_numeric_sensor_data(NULL, sensorDataLength, &eventState,
                                         &previousEventState, &sensorDataSize,
                                         &presentReading);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, PLDM_SENSOR_EVENT_NUMERIC_SENSOR_STATE_MAX_DATA_LENGTH>
        sensorData{};
    rc = decode_numeric_sensor_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(sensorData.data()), sensorDataLength - 1,
        &eventState, &previousEventState, &sensorDataSize, &presentReading);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    struct pldm_sensor_event_numeric_sensor_state* numericSensorData =
        (struct pldm_sensor_event_numeric_sensor_state*)sensorData.data();
    numericSensorData->sensor_data_size = PLDM_SENSOR_DATA_SIZE_UINT8;
    rc = decode_numeric_sensor_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(sensorData.data()), sensorDataLength,
        &eventState, &previousEventState, &sensorDataSize, &presentReading);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    numericSensorData->sensor_data_size = PLDM_SENSOR_DATA_SIZE_UINT16;
    rc = decode_numeric_sensor_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(sensorData.data()), sensorDataLength,
        &eventState, &previousEventState, &sensorDataSize, &presentReading);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetNumericEffecterValue, testGoodEncodeRequest)
{
    std::vector<uint8_t> requestMsg(hdrSize +
                                    PLDM_GET_NUMERIC_EFFECTER_VALUE_REQ_BYTES);

    uint16_t effecter_id = 0xab01;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_get_numeric_effecter_value_req(0, effecter_id, request);

    struct pldm_get_numeric_effecter_value_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_numeric_effecter_value_req*>(
            request->payload);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(effecter_id, le16toh(req->effecter_id));
}

TEST(GetNumericEffecterValue, testBadEncodeRequest)
{
    std::vector<uint8_t> requestMsg(
        hdrSize + PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES);

    auto rc = encode_get_numeric_effecter_value_req(0, 0, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetNumericEffecterValue, testGoodDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_NUMERIC_EFFECTER_VALUE_REQ_BYTES>
        requestMsg{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_get_numeric_effecter_value_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_numeric_effecter_value_req*>(
            request->payload);

    uint16_t effecter_id = 0x12ab;
    req->effecter_id = htole16(effecter_id);

    uint16_t reteffecter_id;

    auto rc = decode_get_numeric_effecter_value_req(
        request, requestMsg.size() - hdrSize, &reteffecter_id);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(effecter_id, reteffecter_id);
}

TEST(GetNumericEffecterValue, testBadDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_NUMERIC_EFFECTER_VALUE_REQ_BYTES>
        requestMsg{};

    auto rc = decode_get_numeric_effecter_value_req(
        nullptr, requestMsg.size() - hdrSize, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_set_numeric_effecter_value_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_set_numeric_effecter_value_req*>(
            request->payload);

    uint16_t effecter_id = 0x1a;
    req->effecter_id = htole16(effecter_id);
    uint16_t reteffecter_id;

    rc = decode_get_numeric_effecter_value_req(
        request, requestMsg.size() - hdrSize - 1, &reteffecter_id);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetNumericEffecterValue, testGoodEncodeResponse)
{
    uint8_t completionCode = 0;
    uint8_t effecter_dataSize = PLDM_EFFECTER_DATA_SIZE_UINT32;
    uint8_t effecter_operState = EFFECTER_OPER_STATE_ENABLED_NOUPDATEPENDING;
    uint32_t pendingValue = 0x12345678;
    uint32_t presentValue = 0xabcdef11;
    uint32_t val_pending;
    uint32_t val_present;

    std::array<uint8_t,
               hdrSize + PLDM_GET_NUMERIC_EFFECTER_VALUE_MIN_RESP_BYTES + 6>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_get_numeric_effecter_value_resp(
        0, completionCode, effecter_dataSize, effecter_operState,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(&pendingValue),
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(&presentValue), response,
        responseMsg.size() - hdrSize);

    struct pldm_get_numeric_effecter_value_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_numeric_effecter_value_resp*>(
            response->payload);

    memcpy(&val_pending, &resp->pending_and_present_values[0],
           sizeof(val_pending));
    val_pending = le32toh(val_pending);
    memcpy(&val_present, &resp->pending_and_present_values[4],
           sizeof(val_present));
    val_present = le32toh(val_present);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(effecter_dataSize, resp->effecter_data_size);
    EXPECT_EQ(effecter_operState, resp->effecter_oper_state);
    EXPECT_EQ(pendingValue, val_pending);
    EXPECT_EQ(presentValue, val_present);
}

TEST(GetNumericEffecterValue, testBadEncodeResponse)
{
    std::array<uint8_t,
               hdrSize + PLDM_GET_NUMERIC_EFFECTER_VALUE_MIN_RESP_BYTES + 2>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    uint8_t pendingValue = 0x01;
    uint8_t presentValue = 0x02;

    auto rc = encode_get_numeric_effecter_value_resp(
        0, PLDM_SUCCESS, 0, 0, nullptr, nullptr, nullptr,
        responseMsg.size() - hdrSize);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_get_numeric_effecter_value_resp(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        0, PLDM_SUCCESS, 6, 9, reinterpret_cast<uint8_t*>(&pendingValue),
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(&presentValue), response,
        responseMsg.size() - hdrSize);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    uint8_t effecter_dataSize = PLDM_EFFECTER_DATA_SIZE_UINT8;
    uint8_t effecter_operState = EFFECTER_OPER_STATE_FAILED;

    rc = encode_get_numeric_effecter_value_resp(
        0, PLDM_SUCCESS, effecter_dataSize, effecter_operState,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(&pendingValue),
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(&presentValue), response,
        responseMsg.size() - hdrSize);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetNumericEffecterValue, testGoodDecodeResponse)
{
    std::array<uint8_t,
               hdrSize + PLDM_GET_NUMERIC_EFFECTER_VALUE_MIN_RESP_BYTES + 2>
        responseMsg{};

    uint8_t completionCode = 0;
    uint8_t effecter_dataSize = PLDM_EFFECTER_DATA_SIZE_UINT16;
    uint8_t effecter_operState = EFFECTER_OPER_STATE_ENABLED_NOUPDATEPENDING;
    uint16_t pendingValue = 0x4321;
    uint16_t presentValue = 0xdcba;

    uint8_t retcompletionCode;
    uint8_t reteffecter_dataSize;
    uint8_t reteffecter_operState;
    uint8_t retpendingValue[2];
    uint8_t retpresentValue[2];

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_get_numeric_effecter_value_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_numeric_effecter_value_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->effecter_data_size = effecter_dataSize;
    resp->effecter_oper_state = effecter_operState;

    uint16_t pendingValue_le = htole16(pendingValue);
    memcpy(resp->pending_and_present_values, &pendingValue_le,
           sizeof(pendingValue_le));
    uint16_t presentValue_le = htole16(presentValue);
    memcpy(&resp->pending_and_present_values[2], &presentValue_le,
           sizeof(presentValue_le));

    auto rc = decode_get_numeric_effecter_value_resp(
        response, responseMsg.size() - hdrSize, &retcompletionCode,
        &reteffecter_dataSize, &reteffecter_operState, retpendingValue,
        retpresentValue);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, retcompletionCode);
    EXPECT_EQ(effecter_dataSize, reteffecter_dataSize);
    EXPECT_EQ(effecter_operState, reteffecter_operState);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    EXPECT_EQ(pendingValue, *(reinterpret_cast<uint16_t*>(retpendingValue)));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    EXPECT_EQ(presentValue, *(reinterpret_cast<uint16_t*>(retpresentValue)));
}

TEST(GetNumericEffecterValue, testBadDecodeResponse)
{
    std::array<uint8_t,
               hdrSize + PLDM_GET_NUMERIC_EFFECTER_VALUE_MIN_RESP_BYTES + 6>
        responseMsg{};

    auto rc = decode_get_numeric_effecter_value_resp(
        nullptr, responseMsg.size() - hdrSize, nullptr, nullptr, nullptr,
        nullptr, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    uint8_t completionCode = 0;
    uint8_t effecter_dataSize = PLDM_EFFECTER_DATA_SIZE_SINT16;
    uint8_t effecter_operState = EFFECTER_OPER_STATE_DISABLED;
    uint16_t pendingValue = 0x5678;
    uint16_t presentValue = 0xcdef;

    uint8_t retcompletionCode;
    uint8_t reteffecter_dataSize;
    uint8_t reteffecter_operState;
    uint8_t retpendingValue[2];
    uint8_t retpresentValue[2];

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_get_numeric_effecter_value_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_numeric_effecter_value_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->effecter_data_size = effecter_dataSize;
    resp->effecter_oper_state = effecter_operState;

    uint16_t pendingValue_le = htole16(pendingValue);
    memcpy(resp->pending_and_present_values, &pendingValue_le,
           sizeof(pendingValue_le));
    uint16_t presentValue_le = htole16(presentValue);
    memcpy(&resp->pending_and_present_values[2], &presentValue_le,
           sizeof(presentValue_le));

    rc = decode_get_numeric_effecter_value_resp(
        response, responseMsg.size() - hdrSize, &retcompletionCode,
        &reteffecter_dataSize, &reteffecter_operState, retpendingValue,
        retpresentValue);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(PldmPDRRepositoryChgEventEvent, testGoodDecodeRequest)
{
    const uint8_t eventDataFormat = FORMAT_IS_PDR_HANDLES;
    const uint8_t numberOfChangeRecords = 2;
    uint8_t eventDataOperation1 = PLDM_RECORDS_DELETED;
    const uint8_t numberOfChangeEntries1 = 2;
    std::array<uint32_t, numberOfChangeEntries1> changeRecordArr1{
        {0x00000000, 0x12345678}};
    uint8_t eventDataOperation2 = PLDM_RECORDS_ADDED;
    const uint8_t numberOfChangeEntries2 = 5;
    std::array<uint32_t, numberOfChangeEntries2> changeRecordArr2{
        {0x01234567, 0x11223344, 0x45678901, 0x21222324, 0x98765432}};
    std::array<uint8_t, PLDM_PDR_REPOSITORY_CHG_EVENT_MIN_LENGTH +
                            PLDM_PDR_REPOSITORY_CHANGE_RECORD_MIN_LENGTH *
                                numberOfChangeRecords +
                            (numberOfChangeEntries1 + numberOfChangeEntries2) *
                                sizeof(uint32_t)>
        eventDataArr{};

    struct pldm_pdr_repository_chg_event_data* eventData =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_pdr_repository_chg_event_data*>(
            eventDataArr.data());
    eventData->event_data_format = eventDataFormat;
    eventData->number_of_change_records = numberOfChangeRecords;
    struct pldm_pdr_repository_change_record_data* changeRecord1 =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_pdr_repository_change_record_data*>(
            eventData->change_records);
    changeRecord1->event_data_operation = eventDataOperation1;
    changeRecord1->number_of_change_entries = numberOfChangeEntries1;
    memcpy(changeRecord1->change_entry, &changeRecordArr1[0],
           changeRecordArr1.size() * sizeof(uint32_t));
    struct pldm_pdr_repository_change_record_data* changeRecord2 =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_pdr_repository_change_record_data*>(
            eventData->change_records +
            PLDM_PDR_REPOSITORY_CHANGE_RECORD_MIN_LENGTH +
            (changeRecordArr1.size() * sizeof(uint32_t)));
    changeRecord2->event_data_operation = eventDataOperation2;
    changeRecord2->number_of_change_entries = numberOfChangeEntries2;
    memcpy(changeRecord2->change_entry, &changeRecordArr2[0],
           changeRecordArr2.size() * sizeof(uint32_t));

    uint8_t retEventDataFormat{};
    uint8_t retNumberOfChangeRecords{};
    size_t retChangeRecordDataOffset{0};
    auto rc = decode_pldm_pdr_repository_chg_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t*>(eventData), eventDataArr.size(),
        &retEventDataFormat, &retNumberOfChangeRecords,
        &retChangeRecordDataOffset);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retEventDataFormat, FORMAT_IS_PDR_HANDLES);
    EXPECT_EQ(retNumberOfChangeRecords, numberOfChangeRecords);

    const uint8_t* changeRecordData =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t*>(changeRecord1);
    size_t changeRecordDataSize =
        eventDataArr.size() - PLDM_PDR_REPOSITORY_CHG_EVENT_MIN_LENGTH;
    uint8_t retEventDataOperation;
    uint8_t retNumberOfChangeEntries;
    size_t retChangeEntryDataOffset;

    rc = decode_pldm_pdr_repository_change_record_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t*>(changeRecordData),
        changeRecordDataSize, &retEventDataOperation, &retNumberOfChangeEntries,
        &retChangeEntryDataOffset);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retEventDataOperation, eventDataOperation1);
    EXPECT_EQ(retNumberOfChangeEntries, numberOfChangeEntries1);
    changeRecordData += retChangeEntryDataOffset;
    EXPECT_EQ(0, memcmp(changeRecordData, &changeRecordArr1[0],
                        sizeof(uint32_t) * retNumberOfChangeEntries));

    changeRecordData += sizeof(uint32_t) * retNumberOfChangeEntries;
    changeRecordDataSize -= sizeof(uint32_t) * retNumberOfChangeEntries -
                            PLDM_PDR_REPOSITORY_CHANGE_RECORD_MIN_LENGTH;
    rc = decode_pldm_pdr_repository_change_record_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t*>(changeRecordData),
        changeRecordDataSize, &retEventDataOperation, &retNumberOfChangeEntries,
        &retChangeEntryDataOffset);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retEventDataOperation, eventDataOperation2);
    EXPECT_EQ(retNumberOfChangeEntries, numberOfChangeEntries2);
    changeRecordData += retChangeEntryDataOffset;
    EXPECT_EQ(0, memcmp(changeRecordData, &changeRecordArr2[0],
                        sizeof(uint32_t) * retNumberOfChangeEntries));
}

TEST(PldmPDRRepositoryChgEventEvent, testBadDecodeRequest)
{
    uint8_t eventDataFormat{};
    uint8_t numberOfChangeRecords{};
    size_t changeRecordDataOffset{};
    auto rc = decode_pldm_pdr_repository_chg_event_data(
        NULL, 0, &eventDataFormat, &numberOfChangeRecords,
        &changeRecordDataOffset);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, 2> eventData{};
    rc = decode_pldm_pdr_repository_chg_event_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t*>(eventData.data()), 0, &eventDataFormat,
        &numberOfChangeRecords, &changeRecordDataOffset);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    uint8_t eventDataOperation{};
    uint8_t numberOfChangeEntries{};
    size_t changeEntryDataOffset{};
    rc = decode_pldm_pdr_repository_change_record_data(
        NULL, 0, &eventDataOperation, &numberOfChangeEntries,
        &changeEntryDataOffset);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    std::array<uint8_t, 2> changeRecord{};
    rc = decode_pldm_pdr_repository_change_record_data(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t*>(changeRecord.data()), 0,
        &eventDataOperation, &numberOfChangeEntries, &changeEntryDataOffset);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetSensorReading, testGoodEncodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_SENSOR_READING_REQ_BYTES>
        requestMsg{};

    uint16_t sensorId = 0x1234;
    bool8_t rearmEventState = 0x01;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc =
        encode_get_sensor_reading_req(0, sensorId, rearmEventState, request);

    struct pldm_get_sensor_reading_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_sensor_reading_req*>(request->payload);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(sensorId, le16toh(req->sensor_id));
    EXPECT_EQ(rearmEventState, req->rearm_event_state);
}

TEST(GetSensorReading, testBadEncodeRequest)
{
    auto rc = encode_get_sensor_reading_req(0, 0, 0, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetSensorReading, testGoodDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_SENSOR_READING_REQ_BYTES>
        requestMsg{};

    uint16_t sensorId = 0xabcd;
    bool8_t rearmEventState = 0xa;

    uint16_t retsensorId;
    bool8_t retrearmEventState;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    struct pldm_get_sensor_reading_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_sensor_reading_req*>(request->payload);

    req->sensor_id = htole16(sensorId);
    req->rearm_event_state = rearmEventState;

    auto rc =
        decode_get_sensor_reading_req(request, requestMsg.size() - hdrSize,
                                      &retsensorId, &retrearmEventState);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(sensorId, retsensorId);
    EXPECT_EQ(rearmEventState, retrearmEventState);
}

TEST(GetSensorReading, testBadDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_SENSOR_READING_REQ_BYTES>
        requestMsg{};

    auto rc = decode_get_sensor_reading_req(
        nullptr, requestMsg.size() - hdrSize, nullptr, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    uint16_t sensorId = 0xabcd;
    bool8_t rearmEventState = 0xa;

    uint16_t retsensorId;
    bool8_t retrearmEventState;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    struct pldm_get_sensor_reading_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_sensor_reading_req*>(request->payload);

    req->sensor_id = htole16(sensorId);
    req->rearm_event_state = rearmEventState;

    rc = decode_get_sensor_reading_req(request, requestMsg.size() - hdrSize - 1,
                                       &retsensorId, &retrearmEventState);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetSensorReading, testGoodEncodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_SENSOR_READING_MIN_RESP_BYTES>
        responseMsg{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    uint8_t completionCode = 0;
    uint8_t sensor_dataSize = PLDM_EFFECTER_DATA_SIZE_UINT8;
    uint8_t sensor_operationalState = PLDM_SENSOR_ENABLED;
    uint8_t sensor_event_messageEnable = PLDM_NO_EVENT_GENERATION;
    uint8_t presentState = PLDM_SENSOR_NORMAL;
    uint8_t previousState = PLDM_SENSOR_WARNING;
    uint8_t eventState = PLDM_SENSOR_UPPERWARNING;
    uint8_t presentReading = 0x21;

    auto rc = encode_get_sensor_reading_resp(
        0, completionCode, sensor_dataSize, sensor_operationalState,
        sensor_event_messageEnable, presentState, previousState, eventState,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(&presentReading), response,
        responseMsg.size() - hdrSize);

    struct pldm_get_sensor_reading_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_sensor_reading_resp*>(
            response->payload);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, resp->completion_code);
    EXPECT_EQ(sensor_dataSize, resp->sensor_data_size);
    EXPECT_EQ(sensor_operationalState, resp->sensor_operational_state);
    EXPECT_EQ(sensor_event_messageEnable, resp->sensor_event_message_enable);
    EXPECT_EQ(presentState, resp->present_state);
    EXPECT_EQ(previousState, resp->previous_state);
    EXPECT_EQ(eventState, resp->event_state);
    EXPECT_EQ(presentReading,
              // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
              *(reinterpret_cast<uint8_t*>(&resp->present_reading[0])));
}

TEST(GetSensorReading, testBadEncodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_SENSOR_READING_MIN_RESP_BYTES + 3>
        responseMsg{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    uint8_t presentReading = 0x1;

    auto rc = encode_get_sensor_reading_resp(0, PLDM_SUCCESS, 0, 0, 0, 0, 0, 0,
                                             nullptr, nullptr,
                                             responseMsg.size() - hdrSize);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_get_sensor_reading_resp(
        0, PLDM_SUCCESS, 6, 1, 1, 1, 1, 1,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(&presentReading), response,
        responseMsg.size() - hdrSize);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    uint8_t sensor_dataSize = PLDM_EFFECTER_DATA_SIZE_UINT8;

    rc = encode_get_sensor_reading_resp(
        0, PLDM_SUCCESS, sensor_dataSize, 1, 1, 1, 1, 1,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(&presentReading), response,
        responseMsg.size() - hdrSize);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetSensorReading, testGoodDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_SENSOR_READING_MIN_RESP_BYTES + 3>
        responseMsg{};

    uint8_t completionCode = 0;
    uint8_t sensor_dataSize = PLDM_EFFECTER_DATA_SIZE_UINT32;
    uint8_t sensor_operationalState = PLDM_SENSOR_STATUSUNKOWN;
    uint8_t sensor_event_messageEnable = PLDM_EVENTS_ENABLED;
    uint8_t presentState = PLDM_SENSOR_CRITICAL;
    uint8_t previousState = PLDM_SENSOR_UPPERCRITICAL;
    uint8_t eventState = PLDM_SENSOR_WARNING;
    uint32_t presentReading = 0xabcdef11;

    uint8_t retcompletionCode;
    uint8_t retsensor_dataSize = PLDM_SENSOR_DATA_SIZE_UINT32;
    uint8_t retsensor_operationalState;
    uint8_t retsensor_event_messageEnable;
    uint8_t retpresentState;
    uint8_t retpreviousState;
    uint8_t reteventState;
    uint8_t retpresentReading[4];

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_get_sensor_reading_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_sensor_reading_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->sensor_data_size = sensor_dataSize;
    resp->sensor_operational_state = sensor_operationalState;
    resp->sensor_event_message_enable = sensor_event_messageEnable;
    resp->present_state = presentState;
    resp->previous_state = previousState;
    resp->event_state = eventState;

    uint32_t presentReading_le = htole32(presentReading);
    memcpy(resp->present_reading, &presentReading_le,
           sizeof(presentReading_le));

    auto rc = decode_get_sensor_reading_resp(
        response, responseMsg.size() - hdrSize, &retcompletionCode,
        &retsensor_dataSize, &retsensor_operationalState,
        &retsensor_event_messageEnable, &retpresentState, &retpreviousState,
        &reteventState, retpresentReading);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, retcompletionCode);
    EXPECT_EQ(sensor_dataSize, retsensor_dataSize);
    EXPECT_EQ(sensor_operationalState, retsensor_operationalState);
    EXPECT_EQ(sensor_event_messageEnable, retsensor_event_messageEnable);
    EXPECT_EQ(presentState, retpresentState);
    EXPECT_EQ(previousState, retpreviousState);
    EXPECT_EQ(eventState, reteventState);
    EXPECT_EQ(presentReading,
              // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
              *(reinterpret_cast<uint32_t*>(retpresentReading)));
}

TEST(GetSensorReading, testBadDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_SENSOR_READING_MIN_RESP_BYTES + 1>
        responseMsg{};

    auto rc = decode_get_sensor_reading_resp(
        nullptr, responseMsg.size() - hdrSize, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    uint8_t completionCode = 0;
    uint8_t sensor_dataSize = PLDM_EFFECTER_DATA_SIZE_UINT8;
    uint8_t sensor_operationalState = PLDM_SENSOR_INTEST;
    uint8_t sensor_event_messageEnable = PLDM_EVENTS_DISABLED;
    uint8_t presentState = PLDM_SENSOR_FATAL;
    uint8_t previousState = PLDM_SENSOR_UPPERFATAL;
    uint8_t eventState = PLDM_SENSOR_WARNING;
    uint8_t presentReading = 0xa;

    uint8_t retcompletionCode;
    uint8_t retsensor_dataSize = PLDM_SENSOR_DATA_SIZE_SINT16;
    uint8_t retsensor_operationalState;
    uint8_t retsensor_event_messageEnable;
    uint8_t retpresent_state;
    uint8_t retprevious_state;
    uint8_t retevent_state;
    uint8_t retpresentReading;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_get_sensor_reading_resp* resp =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_get_sensor_reading_resp*>(
            response->payload);

    resp->completion_code = completionCode;
    resp->sensor_data_size = sensor_dataSize;
    resp->sensor_operational_state = sensor_operationalState;
    resp->sensor_event_message_enable = sensor_event_messageEnable;
    resp->present_state = presentState;
    resp->previous_state = previousState;
    resp->event_state = eventState;
    resp->present_reading[0] = presentReading;

    rc = decode_get_sensor_reading_resp(
        response, responseMsg.size() - hdrSize, &retcompletionCode,
        &retsensor_dataSize, &retsensor_operationalState,
        &retsensor_event_messageEnable, &retpresent_state, &retprevious_state,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        &retevent_state, reinterpret_cast<uint8_t*>(&retpresentReading));

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

#ifdef LIBPLDM_API_TESTING
TEST(SetNumericSensorEnable, testDecodeRequest)
{
    int rc;
    struct pldm_set_numeric_sensor_enable_req decoded;

    const std::array<uint8_t, hdrSize + 5> req
        // PLDM header
        {
            PLDM_PLATFORM, 0x80, PLDM_SET_NUMERIC_SENSOR_ENABLE,
            0x67,          0x45, // sensor ID 0x4567
            0x00,                // sensorOperationalState
            0x01,                // sensorEventMessageEnable
            0x00,                // extra
        };

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto msg = reinterpret_cast<const pldm_msg*>(req.data());

    // Good decode
    rc = decode_set_numeric_sensor_enable_req(msg, 4, &decoded);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(decoded.sensor_id, 0x4567);
    EXPECT_EQ(decoded.op_state, PLDM_SENSOR_ENABLED);
    EXPECT_EQ(decoded.event_enable, PLDM_EVENTS_DISABLED);

    // Fail short
    rc = decode_set_numeric_sensor_enable_req(msg, 3, &decoded);
    EXPECT_EQ(rc, -EOVERFLOW);
    // Fail long
    rc = decode_set_numeric_sensor_enable_req(msg, 5, &decoded);
    EXPECT_EQ(rc, -EBADMSG);
}
#endif // LIBPLDM_API_TESTING

#ifdef LIBPLDM_API_TESTING
TEST(SetNumericSensorEnable, testDecodeInvalidOpRequest)
{
    int rc;
    struct pldm_set_numeric_sensor_enable_req decoded;

    const std::array<uint8_t, hdrSize + 4> req
        // PLDM header
        {
            PLDM_PLATFORM, 0x80, PLDM_SET_NUMERIC_SENSOR_ENABLE,
            0x67,          0x45, // sensor ID 0x4567
            0x30,                // Invalid sensorOperationalState
            0x01,                // sensorEventMessageEnable
        };

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto msg = reinterpret_cast<const pldm_msg*>(req.data());

    rc = decode_set_numeric_sensor_enable_req(msg, 4, &decoded);
    EXPECT_EQ(rc, -EPROTO);
}
#endif // LIBPLDM_API_TESTING

#ifdef LIBPLDM_API_TESTING
TEST(SetNumericSensorEnable, testDecodeInvalidEventRequest)
{
    int rc;
    struct pldm_set_numeric_sensor_enable_req decoded;

    const std::array<uint8_t, hdrSize + 4> req
        // PLDM header
        {
            PLDM_PLATFORM, 0x80, PLDM_SET_NUMERIC_SENSOR_ENABLE,
            0x67,          0x45, // sensor ID 0x4567
            0x00,                // sensorOperationalState
            0x77,                // Invalid sensorEventMessageEnable
        };

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto msg = reinterpret_cast<const pldm_msg*>(req.data());

    rc = decode_set_numeric_sensor_enable_req(msg, 4, &decoded);
    EXPECT_EQ(rc, -EPROTO);
}
#endif // LIBPLDM_API_TESTING

#ifdef LIBPLDM_API_TESTING
TEST(SetStateSensorEnables, testDecodeRequest)
{
    int rc;
    struct pldm_set_state_sensor_enables_req decoded;

    const std::array<uint8_t, hdrSize + 8> req
        // PLDM header
        {
            PLDM_PLATFORM, 0x80, PLDM_SET_STATE_SENSOR_ENABLES,
            0x67,          0x45, // sensor ID 0x4567
            0x02,                // count
            0x01,          0x00, // field 0
            0x02,          0x01, // field 1
            0x00,                // extra byte
        };

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto msg = reinterpret_cast<const pldm_msg*>(req.data());

    // Good decode
    rc = decode_set_state_sensor_enables_req(msg, 7, &decoded);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(decoded.sensor_id, 0x4567);
    EXPECT_EQ(decoded.field_count, 2);
    EXPECT_EQ(decoded.fields[0].op_state, PLDM_SET_SENSOR_DISABLED);
    EXPECT_EQ(decoded.fields[0].event_enable, PLDM_NO_EVENT_GENERATION);
    EXPECT_EQ(decoded.fields[1].op_state, PLDM_SET_SENSOR_UNAVAILABLE);
    EXPECT_EQ(decoded.fields[1].event_enable, PLDM_EVENTS_DISABLED);

    // Short message
    rc = decode_set_state_sensor_enables_req(msg, 6, &decoded);
    EXPECT_EQ(rc, -EOVERFLOW);

    // Overlength message
    rc = decode_set_state_sensor_enables_req(msg, 8, &decoded);
    EXPECT_EQ(rc, -EBADMSG);
}
#endif // LIBPLDM_API_TESTING

#ifdef LIBPLDM_API_TESTING
TEST(SetStateSensorEnables, testDecodeInvalidOpRequest)
{
    int rc;
    struct pldm_set_state_sensor_enables_req decoded;

    const std::array<uint8_t, hdrSize + 7> req
        // PLDM header
        {
            PLDM_PLATFORM, 0x80, PLDM_SET_STATE_SENSOR_ENABLES,
            0x67,          0x45, // sensor ID 0x4567
            0x02,                // count
            0x01,          0x00, // field 0
            0x99,          0x01, // field 1 invalid op
        };

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto msg = reinterpret_cast<const pldm_msg*>(req.data());

    rc = decode_set_state_sensor_enables_req(msg, 7, &decoded);
    EXPECT_EQ(rc, -EPROTO);
}
#endif // LIBPLDM_API_TESTING

#ifdef LIBPLDM_API_TESTING
TEST(SetStateSensorEnables, testDecodeInvalidEventRequest)
{
    int rc;
    struct pldm_set_state_sensor_enables_req decoded;

    const std::array<uint8_t, hdrSize + 7> req
        // PLDM header
        {
            PLDM_PLATFORM, 0x80, PLDM_SET_STATE_SENSOR_ENABLES,
            0x67,          0x45, // sensor ID 0x4567
            0x02,                // count
            0x01,          0x00, // field 0
            0x00,          0x77, // field 1 invalid event
        };

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto msg = reinterpret_cast<const pldm_msg*>(req.data());

    rc = decode_set_state_sensor_enables_req(msg, 7, &decoded);
    EXPECT_EQ(rc, -EPROTO);
}
#endif // LIBPLDM_API_TESTING

#ifdef LIBPLDM_API_TESTING
TEST(GetEventReceiver, testGoodEncodeRequest)
{
    std::array<uint8_t, hdrSize> requestMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = new (requestMsg.data()) pldm_msg;
    auto rc =
        encode_get_event_receiver_req(0, request, sizeof(struct pldm_msg));
    ASSERT_EQ(rc, 0);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(GetEventReceiver, testBadEncodeRequest)
{
    auto rc =
        encode_get_event_receiver_req(0, nullptr, sizeof(struct pldm_msg));
    EXPECT_EQ(rc, -EINVAL);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(GetEventReceiver, testGoodEncodeResponse)
{
    struct pldm_get_event_receiver_resp request_event_receiver_values;
    request_event_receiver_values.completion_code = 0;
    request_event_receiver_values.transport_protocol_type =
        PLDM_TRANSPORT_PROTOCOL_TYPE_MCTP;
    request_event_receiver_values.event_receiver_address.mctp_eid = 84;
    size_t payload_lenght = PLDM_GET_EVENT_RECEIVER_MIN_RESP_BYTES + 1;
    std::array<uint8_t, hdrSize + sizeof(pldm_get_event_receiver_resp)>
        responseMsg{};
    auto response = new (responseMsg.data()) pldm_msg;
    auto rc = encode_get_event_receiver_resp(0, &request_event_receiver_values,
                                             response, &payload_lenght);
    EXPECT_EQ(rc, 0);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(GetEventReceiver, testBadEncodeResponse)
{
    std::array<uint8_t, hdrSize + sizeof(pldm_get_event_receiver_resp)>
        responseMsg{};
    auto response = new (responseMsg.data()) pldm_msg;
    struct pldm_get_event_receiver_resp request_event_receiver_values;
    request_event_receiver_values.completion_code = 0;
    request_event_receiver_values.transport_protocol_type =
        PLDM_TRANSPORT_PROTOCOL_TYPE_MCTP;
    request_event_receiver_values.event_receiver_address.mctp_eid = 64;
    size_t payload_lenght = PLDM_GET_EVENT_RECEIVER_MIN_RESP_BYTES;
    // msg can not be null
    auto rc = encode_get_event_receiver_resp(0, &request_event_receiver_values,
                                             nullptr, &payload_lenght);
    EXPECT_EQ(rc, -EINVAL);
    // unsupported protocol
    request_event_receiver_values.transport_protocol_type = 1;
    rc = encode_get_event_receiver_resp(0, &request_event_receiver_values,
                                        response, &payload_lenght);
    EXPECT_EQ(rc, -ENOTSUP);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(GetEventReceiver, testGoodDecodeResponse)
{
    struct pldm_get_event_receiver_resp request_event_receiver_values;
    request_event_receiver_values.completion_code = 0;
    request_event_receiver_values.transport_protocol_type =
        PLDM_TRANSPORT_PROTOCOL_TYPE_MCTP;
    request_event_receiver_values.event_receiver_address.mctp_eid = 34;
    size_t payload_lenght = PLDM_GET_EVENT_RECEIVER_MIN_RESP_BYTES + 1;
    struct pldm_get_event_receiver_resp decoded_resp;
    std::array<uint8_t, hdrSize + sizeof(pldm_get_event_receiver_resp)>
        responseMsg{};
    auto response = new (responseMsg.data()) pldm_msg;
    auto rc = encode_get_event_receiver_resp(0, &request_event_receiver_values,
                                             response, &payload_lenght);
    EXPECT_EQ(rc, 0);
    rc = decode_get_event_receiver_resp(
        response, PLDM_GET_EVENT_RECEIVER_MIN_RESP_BYTES + 1, &decoded_resp);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(decoded_resp.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(decoded_resp.transport_protocol_type,
              request_event_receiver_values.transport_protocol_type);
    EXPECT_EQ(decoded_resp.event_receiver_address.mctp_eid,
              request_event_receiver_values.event_receiver_address.mctp_eid);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(GetEventReceiver, testBadDecodeResponse)
{
    struct pldm_get_event_receiver_resp decoded_resp;
    struct pldm_get_event_receiver_resp expected_resp;
    expected_resp.completion_code = 0;
    expected_resp.transport_protocol_type = PLDM_TRANSPORT_PROTOCOL_TYPE_MCTP;
    expected_resp.event_receiver_address.mctp_eid = 34;
    std::array<uint8_t, hdrSize + sizeof(pldm_get_event_receiver_resp)>
        responseMsg{};
    auto response = new (responseMsg.data()) pldm_msg;
    size_t payload_lenght = PLDM_GET_EVENT_RECEIVER_MIN_RESP_BYTES + 1;
    auto rc = encode_get_event_receiver_resp(0, &expected_resp, response,
                                             &payload_lenght);
    EXPECT_EQ(rc, 0);
    // message can not be null
    rc = decode_get_event_receiver_resp(
        nullptr, responseMsg.size() - sizeof(pldm_msg_hdr), &decoded_resp);
    EXPECT_EQ(rc, -EINVAL);
    // Allocated less than expected
    rc = decode_get_event_receiver_resp(
        response, PLDM_GET_EVENT_RECEIVER_MIN_RESP_BYTES - 1, &decoded_resp);
    EXPECT_EQ(rc, -EOVERFLOW);
    // Not supported protocol
    size_t transport_protocol_type_offset = hdrSize + 1;
    // Manually modify the transport_protocol_type to a not supported one
    responseMsg[transport_protocol_type_offset] = 1;
    rc = decode_get_event_receiver_resp(
        response, responseMsg.size() - sizeof(pldm_msg_hdr), &decoded_resp);
    EXPECT_EQ(rc, -ENOTSUP);
}
#endif

TEST(SetEventReceiver, testGoodEncodeRequest)
{
    uint8_t eventMessageGlobalEnable =
        PLDM_EVENT_MESSAGE_GLOBAL_ENABLE_ASYNC_KEEP_ALIVE;
    uint8_t transportProtocolType = PLDM_TRANSPORT_PROTOCOL_TYPE_MCTP;
    uint8_t eventReceiverAddressInfo = 0x08;
    uint16_t heartbeatTimer = 0x78;

    std::vector<uint8_t> requestMsg(hdrSize +
                                    PLDM_SET_EVENT_RECEIVER_REQ_BYTES);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_set_event_receiver_req(
        0, eventMessageGlobalEnable, transportProtocolType,
        eventReceiverAddressInfo, heartbeatTimer, request);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    struct pldm_set_event_receiver_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_set_event_receiver_req*>(request->payload);
    EXPECT_EQ(eventMessageGlobalEnable, req->event_message_global_enable);
    EXPECT_EQ(transportProtocolType, req->transport_protocol_type);
    EXPECT_EQ(eventReceiverAddressInfo, req->event_receiver_address_info);
    EXPECT_EQ(heartbeatTimer, le16toh(req->heartbeat_timer));
}

TEST(SetEventReceiver, testBadEncodeRequest)
{
    uint8_t eventMessageGlobalEnable =
        PLDM_EVENT_MESSAGE_GLOBAL_ENABLE_ASYNC_KEEP_ALIVE;
    uint8_t transportProtocolType = PLDM_TRANSPORT_PROTOCOL_TYPE_MCTP;
    uint8_t eventReceiverAddressInfo = 0x08;
    uint16_t heartbeatTimer = 0;

    std::vector<uint8_t> requestMsg(hdrSize +
                                    PLDM_SET_EVENT_RECEIVER_REQ_BYTES);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_set_event_receiver_req(
        0, eventMessageGlobalEnable, transportProtocolType,
        eventReceiverAddressInfo, heartbeatTimer, request);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(SetEventReceiver, testGoodDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_SET_EVENT_RECEIVER_RESP_BYTES>
        responseMsg{};

    uint8_t retcompletion_code = 0;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    auto rc = decode_set_event_receiver_resp(
        response, responseMsg.size() - sizeof(pldm_msg_hdr),
        &retcompletion_code);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(PLDM_SUCCESS, retcompletion_code);
}

TEST(SetEventReceiver, testBadDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_SET_EVENT_RECEIVER_RESP_BYTES>
        responseMsg{};
    uint8_t retcompletion_code = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_set_event_receiver_resp(
        response, responseMsg.size() - sizeof(pldm_msg_hdr), NULL);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = decode_set_event_receiver_resp(
        nullptr, responseMsg.size() - sizeof(pldm_msg_hdr),
        &retcompletion_code);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(SetEventReceiver, testGoodEncodeResponse)
{
    std::array<uint8_t,
               sizeof(pldm_msg_hdr) + PLDM_SET_EVENT_RECEIVER_RESP_BYTES>
        responseMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    uint8_t completionCode = 0;

    auto rc = encode_set_event_receiver_resp(0, PLDM_SUCCESS, response);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, response->payload[0]);
}

TEST(SetEventReceiver, testBadEncodeResponse)
{
    auto rc = encode_set_event_receiver_resp(0, PLDM_SUCCESS, NULL);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(SetEventReceiver, testGoodDecodeRequest)
{

    std::array<uint8_t, hdrSize + PLDM_SET_EVENT_RECEIVER_REQ_BYTES>
        requestMsg{};

    uint8_t eventMessageGlobalEnable =
        PLDM_EVENT_MESSAGE_GLOBAL_ENABLE_ASYNC_KEEP_ALIVE;
    uint8_t transportProtocolType = PLDM_TRANSPORT_PROTOCOL_TYPE_MCTP;
    uint8_t eventReceiverAddressInfo = 0x08;
    uint16_t heartbeatTimer = 0x78;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_set_event_receiver_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_set_event_receiver_req*>(request->payload);

    req->event_message_global_enable = eventMessageGlobalEnable;
    req->transport_protocol_type = transportProtocolType;
    req->event_receiver_address_info = eventReceiverAddressInfo;
    req->heartbeat_timer = htole16(heartbeatTimer);

    uint8_t reteventMessageGlobalEnable;
    uint8_t rettransportProtocolType;
    uint8_t reteventReceiverAddressInfo;
    uint16_t retheartbeatTimer;
    auto rc = decode_set_event_receiver_req(
        request, requestMsg.size() - hdrSize, &reteventMessageGlobalEnable,
        &rettransportProtocolType, &reteventReceiverAddressInfo,
        &retheartbeatTimer);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(eventMessageGlobalEnable, reteventMessageGlobalEnable);
    EXPECT_EQ(transportProtocolType, rettransportProtocolType);
    EXPECT_EQ(eventReceiverAddressInfo, reteventReceiverAddressInfo);
    EXPECT_EQ(heartbeatTimer, retheartbeatTimer);

    eventMessageGlobalEnable = PLDM_EVENT_MESSAGE_GLOBAL_ENABLE_ASYNC;
    req->event_message_global_enable = eventMessageGlobalEnable;
    rc = decode_set_event_receiver_req(
        request, PLDM_SET_EVENT_RECEIVER_MIN_REQ_BYTES,
        &reteventMessageGlobalEnable, &rettransportProtocolType,
        &reteventReceiverAddressInfo, &retheartbeatTimer);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(eventMessageGlobalEnable, reteventMessageGlobalEnable);
    EXPECT_EQ(transportProtocolType, rettransportProtocolType);
    EXPECT_EQ(eventReceiverAddressInfo, reteventReceiverAddressInfo);
}

TEST(SetEventReceiver, testBadDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_SET_EVENT_RECEIVER_REQ_BYTES>
        requestMsg{};

    auto rc = decode_set_event_receiver_req(NULL, requestMsg.size() - hdrSize,
                                            NULL, NULL, NULL, NULL);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    uint8_t eventMessageGlobalEnable =
        PLDM_EVENT_MESSAGE_GLOBAL_ENABLE_ASYNC_KEEP_ALIVE;
    uint8_t transportProtocolType = PLDM_TRANSPORT_PROTOCOL_TYPE_MCTP;
    uint8_t eventReceiverAddressInfo = 0x08;
    uint16_t heartbeatTimer = 0x78;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_set_event_receiver_req* req =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct pldm_set_event_receiver_req*>(request->payload);

    req->event_message_global_enable = eventMessageGlobalEnable;
    req->transport_protocol_type = transportProtocolType;
    req->event_receiver_address_info = eventReceiverAddressInfo;
    req->heartbeat_timer = htole16(heartbeatTimer);

    uint8_t reteventMessageGlobalEnable;
    uint8_t rettransportProtocolType;
    uint8_t reteventReceiverAddressInfo;
    uint16_t retheartbeatTimer;

    rc = decode_set_event_receiver_req(
        request, requestMsg.size() - hdrSize - 1, &reteventMessageGlobalEnable,
        &rettransportProtocolType, &reteventReceiverAddressInfo,
        &retheartbeatTimer);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    req->event_message_global_enable = PLDM_EVENT_MESSAGE_GLOBAL_ENABLE_ASYNC;
    rc = decode_set_event_receiver_req(
        request, PLDM_SET_EVENT_RECEIVER_MIN_REQ_BYTES - 1,
        &reteventMessageGlobalEnable, &rettransportProtocolType,
        &reteventReceiverAddressInfo, &retheartbeatTimer);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);

    req->event_message_global_enable =
        PLDM_EVENT_MESSAGE_GLOBAL_ENABLE_ASYNC_KEEP_ALIVE;
    req->heartbeat_timer = 0;
    rc = decode_set_event_receiver_req(
        request, PLDM_SET_EVENT_RECEIVER_REQ_BYTES,
        &reteventMessageGlobalEnable, &rettransportProtocolType,
        &reteventReceiverAddressInfo, &retheartbeatTimer);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(decodeNumericSensorPdrData, Uint8Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                     // record handle
        0x1,                     // PDRHeaderVersion
        PLDM_NUMERIC_SENSOR_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_SENSOR_PDR_MIN_LENGTH,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // sensorID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        1,
        0,                           // containerID=1
        PLDM_NO_INIT,                // sensorInit
        false,                       // sensorAuxiliaryNamesPDR
        PLDM_SENSOR_UNIT_DEGRESS_C,  // baseUint(2)=degrees C
        0,                           // unitModifier
        0,                           // rateUnit
        0,                           // baseOEMUnitHandle
        0,                           // auxUnit
        0,                           // auxUnitModifier
        0,                           // auxRateUnit
        0,                           // rel
        0,                           // auxOEMUnitHandle
        true,                        // isLinear
        PLDM_SENSOR_DATA_SIZE_UINT8, // sensorDataSize
        0,
        0,
        0xc0,
        0x3f, // resolution=1.5
        0,
        0,
        0x80,
        0x3f, // offset=1.0
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        3, // hysteresis = 3
        0, // supportedThresholds
        0, // thresholdAndHysteresisVolatility
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f,                          // updateInverval=1.0
        255,                           // maxReadable
        0,                             // minReadable
        PLDM_RANGE_FIELD_FORMAT_UINT8, // rangeFieldFormat
        0,                             // rangeFieldsupport
        50,                            // nominalValue = 50
        60,                            // normalMax = 60
        40,                            // normalMin = 40
        70,                            // warningHigh = 70
        30,                            // warningLow = 30
        80,                            // criticalHigh = 80
        20,                            // criticalLow = 20
        90,                            // fatalHigh = 90
        10                             // fatalLow = 10
    };

    struct pldm_numeric_sensor_value_pdr decodedPdr;
    auto rc =
        decode_numeric_sensor_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);
    EXPECT_EQ(1, decodedPdr.hdr.record_handle);
    EXPECT_EQ(1, decodedPdr.hdr.version);
    EXPECT_EQ(PLDM_NUMERIC_SENSOR_PDR, decodedPdr.hdr.type);
    EXPECT_EQ(0, decodedPdr.hdr.record_change_num);
    EXPECT_EQ(PLDM_PDR_NUMERIC_SENSOR_PDR_MIN_LENGTH, decodedPdr.hdr.length);
    EXPECT_EQ(1, decodedPdr.sensor_id);
    EXPECT_EQ(PLDM_ENTITY_POWER_SUPPLY, decodedPdr.entity_type);
    EXPECT_EQ(1, decodedPdr.entity_instance_num);
    EXPECT_EQ(1, decodedPdr.container_id);
    EXPECT_EQ(PLDM_NO_INIT, decodedPdr.sensor_init);
    EXPECT_EQ(false, decodedPdr.sensor_auxiliary_names_pdr);
    EXPECT_EQ(PLDM_SENSOR_UNIT_DEGRESS_C, decodedPdr.base_unit);
    EXPECT_EQ(0, decodedPdr.unit_modifier);
    EXPECT_EQ(0, decodedPdr.rate_unit);
    EXPECT_EQ(0, decodedPdr.base_oem_unit_handle);
    EXPECT_EQ(0, decodedPdr.aux_unit);
    EXPECT_EQ(0, decodedPdr.aux_unit_modifier);
    EXPECT_EQ(0, decodedPdr.aux_rate_unit);
    EXPECT_EQ(0, decodedPdr.rel);
    EXPECT_EQ(0, decodedPdr.aux_oem_unit_handle);
    EXPECT_EQ(true, decodedPdr.is_linear);
    EXPECT_EQ(PLDM_SENSOR_DATA_SIZE_UINT8, decodedPdr.sensor_data_size);
    EXPECT_FLOAT_EQ(1.5f, decodedPdr.resolution);
    EXPECT_FLOAT_EQ(1.0f, decodedPdr.offset);
    EXPECT_EQ(0, decodedPdr.accuracy);
    EXPECT_EQ(0, decodedPdr.plus_tolerance);
    EXPECT_EQ(0, decodedPdr.minus_tolerance);
    EXPECT_EQ(3, decodedPdr.hysteresis.value_u8);
    EXPECT_EQ(0, decodedPdr.supported_thresholds.byte);
    EXPECT_EQ(0, decodedPdr.threshold_and_hysteresis_volatility.byte);
    EXPECT_FLOAT_EQ(1.0f, decodedPdr.state_transition_interval);
    EXPECT_FLOAT_EQ(1.0f, decodedPdr.update_interval);
    EXPECT_EQ(255, decodedPdr.max_readable.value_u8);
    EXPECT_EQ(0, decodedPdr.min_readable.value_u8);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_UINT8, decodedPdr.range_field_format);
    EXPECT_EQ(0, decodedPdr.range_field_support.byte);
    EXPECT_EQ(50, decodedPdr.nominal_value.value_u8);
    EXPECT_EQ(60, decodedPdr.normal_max.value_u8);
    EXPECT_EQ(40, decodedPdr.normal_min.value_u8);
    EXPECT_EQ(70, decodedPdr.warning_high.value_u8);
    EXPECT_EQ(30, decodedPdr.warning_low.value_u8);
    EXPECT_EQ(80, decodedPdr.critical_high.value_u8);
    EXPECT_EQ(20, decodedPdr.critical_low.value_u8);
    EXPECT_EQ(90, decodedPdr.fatal_high.value_u8);
    EXPECT_EQ(10, decodedPdr.fatal_low.value_u8);
}

TEST(decodeNumericSensorPdrData, Sint8Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                     // record handle
        0x1,                     // PDRHeaderVersion
        PLDM_NUMERIC_SENSOR_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_SENSOR_PDR_FIXED_LENGTH +
            PLDM_PDR_NUMERIC_SENSOR_PDR_VARIED_SENSOR_DATA_SIZE_MIN_LENGTH +
            PLDM_PDR_NUMERIC_SENSOR_PDR_VARIED_RANGE_FIELD_MIN_LENGTH,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // sensorID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0,                           // containerID=1
        PLDM_NO_INIT,                  // sensorInit
        false,                         // sensorAuxiliaryNamesPDR
        PLDM_SENSOR_UNIT_DEGRESS_C,    // baseUint(2)=degrees C
        0,                             // unitModifier
        0,                             // rateUnit
        0,                             // baseOEMUnitHandle
        0,                             // auxUnit
        0,                             // auxUnitModifier
        0,                             // auxRateUnit
        0,                             // rel
        0,                             // auxOEMUnitHandle
        true,                          // isLinear
        PLDM_RANGE_FIELD_FORMAT_SINT8, // sensorDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        3, // hysteresis = 3
        0, // supportedThresholds
        0, // thresholdAndHysteresisVolatility
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f,                          // updateInverval=1.0
        0x64,                          // maxReadable = 100
        0x9c,                          // minReadable = -100
        PLDM_RANGE_FIELD_FORMAT_SINT8, // rangeFieldFormat
        0,                             // rangeFieldsupport
        0,                             // nominalValue = 0
        5,                             // normalMax = 5
        0xfb,                          // normalMin = -5
        10,                            // warningHigh = 10
        0xf6,                          // warningLow = -10
        20,                            // criticalHigh = 20
        0xec,                          // criticalLow = -20
        30,                            // fatalHigh = 30
        0xe2                           // fatalLow = -30
    };

    struct pldm_numeric_sensor_value_pdr decodedPdr;
    auto rc =
        decode_numeric_sensor_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);

    EXPECT_EQ(PLDM_SENSOR_DATA_SIZE_SINT8, decodedPdr.sensor_data_size);
    EXPECT_EQ(100, decodedPdr.max_readable.value_s8);
    EXPECT_EQ(-100, decodedPdr.min_readable.value_s8);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_SINT8, decodedPdr.range_field_format);
    EXPECT_EQ(0, decodedPdr.nominal_value.value_s8);
    EXPECT_EQ(5, decodedPdr.normal_max.value_s8);
    EXPECT_EQ(-5, decodedPdr.normal_min.value_s8);
    EXPECT_EQ(10, decodedPdr.warning_high.value_s8);
    EXPECT_EQ(-10, decodedPdr.warning_low.value_s8);
    EXPECT_EQ(20, decodedPdr.critical_high.value_s8);
    EXPECT_EQ(-20, decodedPdr.critical_low.value_s8);
    EXPECT_EQ(30, decodedPdr.fatal_high.value_s8);
    EXPECT_EQ(-30, decodedPdr.fatal_low.value_s8);
}

TEST(decodeNumericSensorPdrData, Uint16Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                     // record handle
        0x1,                     // PDRHeaderVersion
        PLDM_NUMERIC_SENSOR_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_SENSOR_PDR_FIXED_LENGTH +
            PLDM_PDR_NUMERIC_SENSOR_PDR_VARIED_SENSOR_DATA_SIZE_MIN_LENGTH * 2 +
            PLDM_PDR_NUMERIC_SENSOR_PDR_VARIED_RANGE_FIELD_MIN_LENGTH * 2,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // sensorID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0,                          // containerID=1
        PLDM_NO_INIT,                 // sensorInit
        false,                        // sensorAuxiliaryNamesPDR
        PLDM_SENSOR_UNIT_DEGRESS_C,   // baseUint(2)=degrees C
        0,                            // unitModifier
        0,                            // rateUnit
        0,                            // baseOEMUnitHandle
        0,                            // auxUnit
        0,                            // auxUnitModifier
        0,                            // auxRateUnit
        0,                            // rel
        0,                            // auxOEMUnitHandle
        true,                         // isLinear
        PLDM_SENSOR_DATA_SIZE_UINT16, // sensorDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        3,
        0, // hysteresis = 3
        0, // supportedThresholds
        0, // thresholdAndHysteresisVolatility
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f, // updateInverval=1.0
        0,
        0x10, // maxReadable = 4096
        0,
        0,                              // minReadable = 0
        PLDM_RANGE_FIELD_FORMAT_UINT16, // rangeFieldFormat
        0,                              // rangeFieldsupport
        0x88,
        0x13, // nominalValue = 5,000
        0x70,
        0x17, // normalMax = 6,000
        0xa0,
        0x0f, // normalMin = 4,000
        0x58,
        0x1b, // warningHigh = 7,000
        0xb8,
        0x0b, // warningLow = 3,000
        0x40,
        0x1f, // criticalHigh = 8,000
        0xd0,
        0x07, // criticalLow = 2,000
        0x28,
        0x23, // fatalHigh = 9,000
        0xe8,
        0x03 // fatalLow = 1,000
    };

    struct pldm_numeric_sensor_value_pdr decodedPdr;
    auto rc =
        decode_numeric_sensor_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);

    EXPECT_EQ(PLDM_SENSOR_DATA_SIZE_UINT16, decodedPdr.sensor_data_size);
    EXPECT_EQ(4096, decodedPdr.max_readable.value_u16);
    EXPECT_EQ(0, decodedPdr.min_readable.value_u16);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_UINT16, decodedPdr.range_field_format);
    EXPECT_EQ(5000, decodedPdr.nominal_value.value_u16);
    EXPECT_EQ(6000, decodedPdr.normal_max.value_u16);
    EXPECT_EQ(4000, decodedPdr.normal_min.value_u16);
    EXPECT_EQ(7000, decodedPdr.warning_high.value_u16);
    EXPECT_EQ(3000, decodedPdr.warning_low.value_u16);
    EXPECT_EQ(8000, decodedPdr.critical_high.value_u16);
    EXPECT_EQ(2000, decodedPdr.critical_low.value_u16);
    EXPECT_EQ(9000, decodedPdr.fatal_high.value_u16);
    EXPECT_EQ(1000, decodedPdr.fatal_low.value_u16);
}

TEST(decodeNumericSensorPdrData, Sint16Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                     // record handle
        0x1,                     // PDRHeaderVersion
        PLDM_NUMERIC_SENSOR_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_SENSOR_PDR_FIXED_LENGTH +
            PLDM_PDR_NUMERIC_SENSOR_PDR_VARIED_SENSOR_DATA_SIZE_MIN_LENGTH * 2 +
            PLDM_PDR_NUMERIC_SENSOR_PDR_VARIED_RANGE_FIELD_MIN_LENGTH * 2,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // sensorID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0,                          // containerID=1
        PLDM_NO_INIT,                 // sensorInit
        false,                        // sensorAuxiliaryNamesPDR
        PLDM_SENSOR_UNIT_DEGRESS_C,   // baseUint(2)=degrees C
        0,                            // unitModifier
        0,                            // rateUnit
        0,                            // baseOEMUnitHandle
        0,                            // auxUnit
        0,                            // auxUnitModifier
        0,                            // auxRateUnit
        0,                            // rel
        0,                            // auxOEMUnitHandle
        true,                         // isLinear
        PLDM_SENSOR_DATA_SIZE_SINT16, // sensorDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        3,
        0, // hysteresis
        0, // supportedThresholds
        0, // thresholdAndHysteresisVolatility
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f, // updateInverval=1.0
        0xe8,
        0x03, // maxReadable = 1000
        0x18,
        0xfc,                           // minReadable = -1000
        PLDM_RANGE_FIELD_FORMAT_SINT16, // rangeFieldFormat
        0,                              // rangeFieldsupport
        0,
        0, // nominalValue = 0
        0xf4,
        0x01, // normalMax = 500
        0x0c,
        0xfe, // normalMin = -500
        0xe8,
        0x03, // warningHigh = 1,000
        0x18,
        0xfc, // warningLow = -1,000
        0xd0,
        0x07, // criticalHigh = 2,000
        0x30,
        0xf8, // criticalLow = -2,000
        0xb8,
        0x0b, // fatalHigh = 3,000
        0x48,
        0xf4 // fatalLow = -3,000
    };

    struct pldm_numeric_sensor_value_pdr decodedPdr;
    auto rc =
        decode_numeric_sensor_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);

    EXPECT_EQ(PLDM_SENSOR_DATA_SIZE_SINT16, decodedPdr.sensor_data_size);
    EXPECT_EQ(1000, decodedPdr.max_readable.value_s16);
    EXPECT_EQ(-1000, decodedPdr.min_readable.value_s16);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_SINT16, decodedPdr.range_field_format);
    EXPECT_EQ(0, decodedPdr.nominal_value.value_s16);
    EXPECT_EQ(500, decodedPdr.normal_max.value_s16);
    EXPECT_EQ(-500, decodedPdr.normal_min.value_s16);
    EXPECT_EQ(1000, decodedPdr.warning_high.value_s16);
    EXPECT_EQ(-1000, decodedPdr.warning_low.value_s16);
    EXPECT_EQ(2000, decodedPdr.critical_high.value_s16);
    EXPECT_EQ(-2000, decodedPdr.critical_low.value_s16);
    EXPECT_EQ(3000, decodedPdr.fatal_high.value_s16);
    EXPECT_EQ(-3000, decodedPdr.fatal_low.value_s16);
}

TEST(decodeNumericSensorPdrData, Uint32Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                     // record handle
        0x1,                     // PDRHeaderVersion
        PLDM_NUMERIC_SENSOR_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_SENSOR_PDR_FIXED_LENGTH +
            PLDM_PDR_NUMERIC_SENSOR_PDR_VARIED_SENSOR_DATA_SIZE_MIN_LENGTH * 4 +
            PLDM_PDR_NUMERIC_SENSOR_PDR_VARIED_RANGE_FIELD_MIN_LENGTH * 4,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // sensorID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0,                          // containerID=1
        PLDM_NO_INIT,                 // sensorInit
        false,                        // sensorAuxiliaryNamesPDR
        PLDM_SENSOR_UNIT_DEGRESS_C,   // baseUint(2)=degrees C
        0,                            // unitModifier
        0,                            // rateUnit
        0,                            // baseOEMUnitHandle
        0,                            // auxUnit
        0,                            // auxUnitModifier
        0,                            // auxRateUnit
        0,                            // rel
        0,                            // auxOEMUnitHandle
        true,                         // isLinear
        PLDM_SENSOR_DATA_SIZE_UINT32, // sensorDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        3,
        0,
        0,
        0, // hysteresis
        0, // supportedThresholds
        0, // thresholdAndHysteresisVolatility
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f, // updateInverval=1.0
        0,
        0x10,
        0,
        0, // maxReadable = 4096
        0,
        0,
        0,
        0,                              // minReadable = 0
        PLDM_RANGE_FIELD_FORMAT_UINT32, // rangeFieldFormat
        0,                              // rangeFieldsupport
        0x40,
        0x4b,
        0x4c,
        0x00, // nominalValue = 5,000,000
        0x80,
        0x8d,
        0x5b,
        0x00, // normalMax = 6,000,000
        0x00,
        0x09,
        0x3d,
        0x00, // normalMin = 4,000,000
        0xc0,
        0xcf,
        0x6a,
        0x00, // warningHigh = 7,000,000
        0xc0,
        0xc6,
        0x2d,
        0x00, // warningLow = 3,000,000
        0x00,
        0x12,
        0x7a,
        0x00, // criticalHigh = 8,000,000
        0x80,
        0x84,
        0x1e,
        0x00, // criticalLow = 2,000,000
        0x40,
        0x54,
        0x89,
        0x00, // fatalHigh = 9,000,000
        0x40,
        0x42,
        0x0f,
        0x00 // fatalLow = 1,000,000
    };

    struct pldm_numeric_sensor_value_pdr decodedPdr;
    auto rc =
        decode_numeric_sensor_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);

    EXPECT_EQ(PLDM_SENSOR_DATA_SIZE_UINT32, decodedPdr.sensor_data_size);
    EXPECT_EQ(4096, decodedPdr.max_readable.value_u32);
    EXPECT_EQ(0, decodedPdr.min_readable.value_u32);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_UINT32, decodedPdr.range_field_format);
    EXPECT_EQ(5000000, decodedPdr.nominal_value.value_u32);
    EXPECT_EQ(6000000, decodedPdr.normal_max.value_u32);
    EXPECT_EQ(4000000, decodedPdr.normal_min.value_u32);
    EXPECT_EQ(7000000, decodedPdr.warning_high.value_u32);
    EXPECT_EQ(3000000, decodedPdr.warning_low.value_u32);
    EXPECT_EQ(8000000, decodedPdr.critical_high.value_u32);
    EXPECT_EQ(2000000, decodedPdr.critical_low.value_u32);
    EXPECT_EQ(9000000, decodedPdr.fatal_high.value_u32);
    EXPECT_EQ(1000000, decodedPdr.fatal_low.value_u32);
}

TEST(decodeNumericSensorPdrData, Sint32Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                     // record handle
        0x1,                     // PDRHeaderVersion
        PLDM_NUMERIC_SENSOR_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_SENSOR_PDR_FIXED_LENGTH +
            PLDM_PDR_NUMERIC_SENSOR_PDR_VARIED_SENSOR_DATA_SIZE_MIN_LENGTH * 4 +
            PLDM_PDR_NUMERIC_SENSOR_PDR_VARIED_RANGE_FIELD_MIN_LENGTH * 4,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // sensorID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0,                          // containerID=1
        PLDM_NO_INIT,                 // sensorInit
        false,                        // sensorAuxiliaryNamesPDR
        PLDM_SENSOR_UNIT_DEGRESS_C,   // baseUint(2)=degrees C
        0,                            // unitModifier
        0,                            // rateUnit
        0,                            // baseOEMUnitHandle
        0,                            // auxUnit
        0,                            // auxUnitModifier
        0,                            // auxRateUnit
        0,                            // rel
        0,                            // auxOEMUnitHandle
        true,                         // isLinear
        PLDM_SENSOR_DATA_SIZE_SINT32, // sensorDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        3,
        0,
        0,
        0, // hysteresis
        0, // supportedThresholds
        0, // thresholdAndHysteresisVolatility
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f, // updateInverval=1.0
        0xa0,
        0x86,
        0x01,
        0x00, // maxReadable = 100000
        0x60,
        0x79,
        0xfe,
        0xff,                           // minReadable = -10000
        PLDM_RANGE_FIELD_FORMAT_SINT32, // rangeFieldFormat
        0,                              // rangeFieldsupport
        0,
        0,
        0,
        0, // nominalValue = 0
        0x20,
        0xa1,
        0x07,
        0x00, // normalMax = 500,000
        0xe0,
        0x5e,
        0xf8,
        0xff, // normalMin = -500,000
        0x40,
        0x42,
        0x0f,
        0x00, // warningHigh = 1,000,000
        0xc0,
        0xbd,
        0xf0,
        0xff, // warningLow = -1,000,000
        0x80,
        0x84,
        0x1e,
        0x00, // criticalHigh = 2,000,000
        0x80,
        0x7b,
        0xe1,
        0xff, // criticalLow = -2,000,000
        0xc0,
        0xc6,
        0x2d,
        0x00, // fatalHigh = 3,000,000
        0x40,
        0x39,
        0xd2,
        0xff // fatalLow = -3,000,000
    };

    struct pldm_numeric_sensor_value_pdr decodedPdr;
    auto rc =
        decode_numeric_sensor_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);

    EXPECT_EQ(PLDM_SENSOR_DATA_SIZE_SINT32, decodedPdr.sensor_data_size);
    EXPECT_EQ(100000, decodedPdr.max_readable.value_s32);
    EXPECT_EQ(-100000, decodedPdr.min_readable.value_s32);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_SINT32, decodedPdr.range_field_format);
    EXPECT_EQ(0, decodedPdr.nominal_value.value_s32);
    EXPECT_EQ(500000, decodedPdr.normal_max.value_s32);
    EXPECT_EQ(-500000, decodedPdr.normal_min.value_s32);
    EXPECT_EQ(1000000, decodedPdr.warning_high.value_s32);
    EXPECT_EQ(-1000000, decodedPdr.warning_low.value_s32);
    EXPECT_EQ(2000000, decodedPdr.critical_high.value_s32);
    EXPECT_EQ(-2000000, decodedPdr.critical_low.value_s32);
    EXPECT_EQ(3000000, decodedPdr.fatal_high.value_s32);
    EXPECT_EQ(-3000000, decodedPdr.fatal_low.value_s32);
}

TEST(decodeNumericSensorPdrData, Real32Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                     // record handle
        0x1,                     // PDRHeaderVersion
        PLDM_NUMERIC_SENSOR_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_SENSOR_PDR_FIXED_LENGTH +
            PLDM_PDR_NUMERIC_SENSOR_PDR_VARIED_SENSOR_DATA_SIZE_MIN_LENGTH * 4 +
            PLDM_PDR_NUMERIC_SENSOR_PDR_VARIED_RANGE_FIELD_MIN_LENGTH * 4,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // sensorID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0,                          // containerID=1
        PLDM_NO_INIT,                 // sensorInit
        false,                        // sensorAuxiliaryNamesPDR
        PLDM_SENSOR_UNIT_DEGRESS_C,   // baseUint(2)=degrees C
        0,                            // unitModifier
        0,                            // rateUnit
        0,                            // baseOEMUnitHandle
        0,                            // auxUnit
        0,                            // auxUnitModifier
        0,                            // auxRateUnit
        0,                            // rel
        0,                            // auxOEMUnitHandle
        true,                         // isLinear
        PLDM_SENSOR_DATA_SIZE_SINT32, // sensorDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        3,
        0,
        0,
        0, // hysteresis
        0, // supportedThresholds
        0, // thresholdAndHysteresisVolatility
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f, // updateInverval=1.0
        0xa0,
        0x86,
        0x01,
        0x00, // maxReadable = 100000
        0x60,
        0x79,
        0xfe,
        0xff,                           // minReadable = -10000
        PLDM_RANGE_FIELD_FORMAT_REAL32, // rangeFieldFormat
        0,                              // rangeFieldsupport
        0,
        0,
        0,
        0, // nominalValue = 0.0
        0x33,
        0x33,
        0x48,
        0x42, // normalMax = 50.05
        0x33,
        0x33,
        0x48,
        0xc2, // normalMin = -50.05
        0x83,
        0x00,
        0xc8,
        0x42, // warningHigh = 100.001
        0x83,
        0x00,
        0xc8,
        0xc2, // warningLow = -100.001
        0x83,
        0x00,
        0x48,
        0x43, // criticalHigh = 200.002
        0x83,
        0x00,
        0x48,
        0xc3, // criticalLow = -200.002
        0x62,
        0x00,
        0x96,
        0x43, // fatalHigh = 300.003
        0x62,
        0x00,
        0x96,
        0xc3 // fatalLow = -300.003
    };

    struct pldm_numeric_sensor_value_pdr decodedPdr;
    auto rc =
        decode_numeric_sensor_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);

    EXPECT_EQ(PLDM_SENSOR_DATA_SIZE_SINT32, decodedPdr.sensor_data_size);
    EXPECT_EQ(100000, decodedPdr.max_readable.value_s32);
    EXPECT_EQ(-100000, decodedPdr.min_readable.value_s32);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_REAL32, decodedPdr.range_field_format);
    EXPECT_FLOAT_EQ(0, decodedPdr.nominal_value.value_f32);
    EXPECT_FLOAT_EQ(50.05f, decodedPdr.normal_max.value_f32);
    EXPECT_FLOAT_EQ(-50.05f, decodedPdr.normal_min.value_f32);
    EXPECT_FLOAT_EQ(100.001f, decodedPdr.warning_high.value_f32);
    EXPECT_FLOAT_EQ(-100.001f, decodedPdr.warning_low.value_f32);
    EXPECT_FLOAT_EQ(200.002f, decodedPdr.critical_high.value_f32);
    EXPECT_FLOAT_EQ(-200.002f, decodedPdr.critical_low.value_f32);
    EXPECT_FLOAT_EQ(300.003f, decodedPdr.fatal_high.value_f32);
    EXPECT_FLOAT_EQ(-300.003f, decodedPdr.fatal_low.value_f32);
}

TEST(decodeNumericSensorPdrDataDeathTest, InvalidSizeTest)
{
    // A corrupted PDR. The data after plusTolerance missed.
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                     // record handle
        0x1,                     // PDRHeaderVersion
        PLDM_NUMERIC_SENSOR_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_SENSOR_PDR_FIXED_LENGTH,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // sensorID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0,                         // containerID=1
        PLDM_NO_INIT,                // sensorInit
        false,                       // sensorAuxiliaryNamesPDR
        2,                           // baseUint(2)=degrees C
        0,                           // unitModifier
        0,                           // rateUnit
        0,                           // baseOEMUnitHandle
        0,                           // auxUnit
        0,                           // auxUnitModifier
        0,                           // auxRateUnit
        0,                           // rel
        0,                           // auxOEMUnitHandle
        true,                        // isLinear
        PLDM_SENSOR_DATA_SIZE_UINT8, // sensorDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0  // plusTolerance
    };

    struct pldm_numeric_sensor_value_pdr decodedPdr;
    int rc =
        decode_numeric_sensor_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

#ifdef LIBPLDM_API_TESTING
TEST(decodeNumericEffecterPdrData, Uint8Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                       // record handle
        0x1,                       // PDRHeaderVersion
        PLDM_NUMERIC_EFFECTER_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_EFFECTER_PDR_MIN_LENGTH,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // effecterID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        1,
        0, // containerID=1
        0x2,
        0x0,                           // effecter_semantic_id=2
        PLDM_NO_INIT,                  // effecterInit
        false,                         // effecterAuxiliaryNames
        PLDM_SENSOR_UNIT_DEGRESS_C,    // baseUint(2)=degrees C
        0,                             // unitModifier
        0,                             // rateUnit
        0,                             // baseOEMUnitHandle
        0,                             // auxUnit
        0,                             // auxUnitModifier
        4,                             // auxRateUnit
        0,                             // auxOEMUnitHandle
        true,                          // isLinear
        PLDM_EFFECTER_DATA_SIZE_UINT8, // effecterDataSize
        0,
        0,
        0xc0,
        0x3f, // resolution=1.5
        0,
        0,
        0x80,
        0x3f, // offset=1.0
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f,                          // transition_interval=1.0
        255,                           // maxSetdable
        0,                             // minSetable
        PLDM_RANGE_FIELD_FORMAT_UINT8, // rangeFieldFormat
        0x1f,                          // rangeFieldsupport
        50,                            // nominalValue = 50
        60,                            // normalMax = 60
        40,                            // normalMin = 40
        90,                            // rated_max = 90
        10                             // rated_min = 10
    };

    struct pldm_numeric_effecter_value_pdr decodedPdr;
    auto rc =
        decode_numeric_effecter_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);
    EXPECT_EQ(1, decodedPdr.hdr.record_handle);
    EXPECT_EQ(1, decodedPdr.hdr.version);
    EXPECT_EQ(PLDM_NUMERIC_EFFECTER_PDR, decodedPdr.hdr.type);
    EXPECT_EQ(0, decodedPdr.hdr.record_change_num);
    EXPECT_EQ(PLDM_PDR_NUMERIC_EFFECTER_PDR_MIN_LENGTH, decodedPdr.hdr.length);
    EXPECT_EQ(1, decodedPdr.effecter_id);
    EXPECT_EQ(PLDM_ENTITY_POWER_SUPPLY, decodedPdr.entity_type);
    EXPECT_EQ(1, decodedPdr.entity_instance);
    EXPECT_EQ(1, decodedPdr.container_id);
    EXPECT_EQ(2, decodedPdr.effecter_semantic_id);
    EXPECT_EQ(PLDM_NO_INIT, decodedPdr.effecter_init);
    EXPECT_EQ(false, decodedPdr.effecter_auxiliary_names);
    EXPECT_EQ(PLDM_SENSOR_UNIT_DEGRESS_C, decodedPdr.base_unit);
    EXPECT_EQ(0, decodedPdr.unit_modifier);
    EXPECT_EQ(0, decodedPdr.rate_unit);
    EXPECT_EQ(0, decodedPdr.base_oem_unit_handle);
    EXPECT_EQ(0, decodedPdr.aux_unit);
    EXPECT_EQ(0, decodedPdr.aux_unit_modifier);
    EXPECT_EQ(4, decodedPdr.aux_rate_unit);
    EXPECT_EQ(0, decodedPdr.aux_oem_unit_handle);
    EXPECT_EQ(true, decodedPdr.is_linear);
    EXPECT_EQ(PLDM_EFFECTER_DATA_SIZE_UINT8, decodedPdr.effecter_data_size);
    EXPECT_FLOAT_EQ(1.5f, decodedPdr.resolution);
    EXPECT_FLOAT_EQ(1.0f, decodedPdr.offset);
    EXPECT_EQ(0, decodedPdr.accuracy);
    EXPECT_EQ(0, decodedPdr.plus_tolerance);
    EXPECT_EQ(0, decodedPdr.minus_tolerance);
    EXPECT_FLOAT_EQ(1.0f, decodedPdr.state_transition_interval);
    EXPECT_FLOAT_EQ(1.0f, decodedPdr.transition_interval);
    EXPECT_EQ(255, decodedPdr.max_settable.value_u8);
    EXPECT_EQ(0, decodedPdr.min_settable.value_u8);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_UINT8, decodedPdr.range_field_format);
    EXPECT_EQ(0x1f, decodedPdr.range_field_support.byte);
    EXPECT_EQ(50, decodedPdr.nominal_value.value_u8);
    EXPECT_EQ(60, decodedPdr.normal_max.value_u8);
    EXPECT_EQ(40, decodedPdr.normal_min.value_u8);
    EXPECT_EQ(90, decodedPdr.rated_max.value_u8);
    EXPECT_EQ(10, decodedPdr.rated_min.value_u8);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(decodeNumericEffecterPdrData, Sint8Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                       // record handle
        0x1,                       // PDRHeaderVersion
        PLDM_NUMERIC_EFFECTER_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_EFFECTER_PDR_FIXED_LENGTH +
            PLDM_PDR_NUMERIC_EFFECTER_PDR_VARIED_EFFECTER_DATA_SIZE_MIN_LENGTH +
            PLDM_PDR_NUMERIC_EFFECTER_PDR_VARIED_RANGE_FIELD_MIN_LENGTH,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // effecterID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0, // containerID=1
        0x2,
        0x0,                           // effecter_semantic_id=2
        PLDM_NO_INIT,                  // effecterInit
        false,                         // effecterAuxiliaryNames
        PLDM_SENSOR_UNIT_DEGRESS_C,    // baseUint(2)=degrees C
        0,                             // unitModifier
        0,                             // rateUnit
        0,                             // baseOEMUnitHandle
        0,                             // auxUnit
        0,                             // auxUnitModifier
        0,                             // auxRateUnit
        0,                             // auxOEMUnitHandle
        true,                          // isLinear
        PLDM_RANGE_FIELD_FORMAT_SINT8, // effecterDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f,                          // transition_interval=1.0
        0x64,                          // maxSetdable = 100
        0x9c,                          // minSetable = -100
        PLDM_RANGE_FIELD_FORMAT_SINT8, // rangeFieldFormat
        0x1f,                          // rangeFieldsupport
        0,                             // nominalValue = 0
        5,                             // normalMax = 5
        0xfb,                          // normalMin = -5
        30,                            // rated_max = 30
        0xe2                           // rated_min = -30
    };

    struct pldm_numeric_effecter_value_pdr decodedPdr;
    auto rc =
        decode_numeric_effecter_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);

    EXPECT_EQ(PLDM_EFFECTER_DATA_SIZE_SINT8, decodedPdr.effecter_data_size);
    EXPECT_EQ(100, decodedPdr.max_settable.value_s8);
    EXPECT_EQ(-100, decodedPdr.min_settable.value_s8);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_SINT8, decodedPdr.range_field_format);
    EXPECT_EQ(0x1f, decodedPdr.range_field_support.byte);
    EXPECT_EQ(0, decodedPdr.nominal_value.value_s8);
    EXPECT_EQ(5, decodedPdr.normal_max.value_s8);
    EXPECT_EQ(-5, decodedPdr.normal_min.value_s8);
    EXPECT_EQ(30, decodedPdr.rated_max.value_s8);
    EXPECT_EQ(-30, decodedPdr.rated_min.value_s8);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(decodeNumericEffecterPdrData, Uint16Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                       // record handle
        0x1,                       // PDRHeaderVersion
        PLDM_NUMERIC_EFFECTER_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_EFFECTER_PDR_FIXED_LENGTH +
            PLDM_PDR_NUMERIC_EFFECTER_PDR_VARIED_EFFECTER_DATA_SIZE_MIN_LENGTH *
                2 +
            PLDM_PDR_NUMERIC_EFFECTER_PDR_VARIED_RANGE_FIELD_MIN_LENGTH * 2,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // effecterID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0, // containerID=1
        0x2,
        0x0,                            // effecter_semantic_id=2
        PLDM_NO_INIT,                   // effecterInit
        false,                          // effecterAuxiliaryNames
        PLDM_SENSOR_UNIT_DEGRESS_C,     // baseUint(2)=degrees C
        0,                              // unitModifier
        0,                              // rateUnit
        0,                              // baseOEMUnitHandle
        0,                              // auxUnit
        0,                              // auxUnitModifier
        0,                              // auxRateUnit
        0,                              // auxOEMUnitHandle
        true,                           // isLinear
        PLDM_EFFECTER_DATA_SIZE_UINT16, // effecterDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f, // transition_interval=1.0
        0,
        0x10, // maxSetdable = 4096
        0,
        0,                              // minSetable = 0
        PLDM_RANGE_FIELD_FORMAT_UINT16, // rangeFieldFormat
        0x1f,                           // rangeFieldsupport
        0x88,
        0x13, // nominalValue = 5,000
        0x70,
        0x17, // normalMax = 6,000
        0xa0,
        0x0f, // normalMin = 4,000
        0x28,
        0x23, // rated_max = 9,000
        0xe8,
        0x03 // rated_min = 1,000
    };

    struct pldm_numeric_effecter_value_pdr decodedPdr;
    auto rc =
        decode_numeric_effecter_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);

    EXPECT_EQ(PLDM_EFFECTER_DATA_SIZE_UINT16, decodedPdr.effecter_data_size);
    EXPECT_EQ(4096, decodedPdr.max_settable.value_u16);
    EXPECT_EQ(0, decodedPdr.min_settable.value_u16);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_UINT16, decodedPdr.range_field_format);
    EXPECT_EQ(0x1f, decodedPdr.range_field_support.byte);
    EXPECT_EQ(5000, decodedPdr.nominal_value.value_u16);
    EXPECT_EQ(6000, decodedPdr.normal_max.value_u16);
    EXPECT_EQ(4000, decodedPdr.normal_min.value_u16);
    EXPECT_EQ(9000, decodedPdr.rated_max.value_u16);
    EXPECT_EQ(1000, decodedPdr.rated_min.value_u16);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(decodeNumericEffecterPdrData, Sint16Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                       // record handle
        0x1,                       // PDRHeaderVersion
        PLDM_NUMERIC_EFFECTER_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_EFFECTER_PDR_FIXED_LENGTH +
            PLDM_PDR_NUMERIC_EFFECTER_PDR_VARIED_EFFECTER_DATA_SIZE_MIN_LENGTH *
                2 +
            PLDM_PDR_NUMERIC_EFFECTER_PDR_VARIED_RANGE_FIELD_MIN_LENGTH * 2,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // effecterID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0, // containerID=1
        0x2,
        0x0,                            // effecter_semantic_id=2
        PLDM_NO_INIT,                   // effecterInit
        false,                          // effecterAuxiliaryNames
        PLDM_SENSOR_UNIT_DEGRESS_C,     // baseUint(2)=degrees C
        0,                              // unitModifier
        0,                              // rateUnit
        0,                              // baseOEMUnitHandle
        0,                              // auxUnit
        0,                              // auxUnitModifier
        0,                              // auxRateUnit
        0,                              // auxOEMUnitHandle
        true,                           // isLinear
        PLDM_EFFECTER_DATA_SIZE_SINT16, // effecterDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f, // transition_interval=1.0
        0xe8,
        0x03, // maxSetdable = 1000
        0x18,
        0xfc,                           // minSetable = -1000
        PLDM_RANGE_FIELD_FORMAT_SINT16, // rangeFieldFormat
        0x1f,                           // rangeFieldsupport
        0,
        0, // nominalValue = 0
        0xf4,
        0x01, // normalMax = 500
        0x0c,
        0xfe, // normalMin = -500
        0xb8,
        0x0b, // rated_max = 3,000
        0x48,
        0xf4 // rated_min = -3,000
    };

    struct pldm_numeric_effecter_value_pdr decodedPdr;
    auto rc =
        decode_numeric_effecter_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);

    EXPECT_EQ(PLDM_EFFECTER_DATA_SIZE_SINT16, decodedPdr.effecter_data_size);
    EXPECT_EQ(1000, decodedPdr.max_settable.value_s16);
    EXPECT_EQ(-1000, decodedPdr.min_settable.value_s16);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_SINT16, decodedPdr.range_field_format);
    EXPECT_EQ(0x1f, decodedPdr.range_field_support.byte);
    EXPECT_EQ(0, decodedPdr.nominal_value.value_s16);
    EXPECT_EQ(500, decodedPdr.normal_max.value_s16);
    EXPECT_EQ(-500, decodedPdr.normal_min.value_s16);
    EXPECT_EQ(3000, decodedPdr.rated_max.value_s16);
    EXPECT_EQ(-3000, decodedPdr.rated_min.value_s16);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(decodeNumericEffecterPdrData, Uint32Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                       // record handle
        0x1,                       // PDRHeaderVersion
        PLDM_NUMERIC_EFFECTER_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_EFFECTER_PDR_FIXED_LENGTH +
            PLDM_PDR_NUMERIC_EFFECTER_PDR_VARIED_EFFECTER_DATA_SIZE_MIN_LENGTH *
                4 +
            PLDM_PDR_NUMERIC_EFFECTER_PDR_VARIED_RANGE_FIELD_MIN_LENGTH * 4,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // effecterID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0, // containerID=1
        0x2,
        0x0,                            // effecter_semantic_id=2
        PLDM_NO_INIT,                   // effecterInit
        false,                          // effecterAuxiliaryNames
        PLDM_SENSOR_UNIT_DEGRESS_C,     // baseUint(2)=degrees C
        0,                              // unitModifier
        0,                              // rateUnit
        0,                              // baseOEMUnitHandle
        0,                              // auxUnit
        0,                              // auxUnitModifier
        0,                              // auxRateUnit
        0,                              // auxOEMUnitHandle
        true,                           // isLinear
        PLDM_EFFECTER_DATA_SIZE_UINT32, // effecterDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f, // transition_interval=1.0
        0,
        0x10,
        0,
        0, // maxSetdable = 4096
        0,
        0,
        0,
        0,                              // minSetable = 0
        PLDM_RANGE_FIELD_FORMAT_UINT32, // rangeFieldFormat
        0x1f,                           // rangeFieldsupport
        0x40,
        0x4b,
        0x4c,
        0x00, // nominalValue = 5,000,000
        0x80,
        0x8d,
        0x5b,
        0x00, // normalMax = 6,000,000
        0x00,
        0x09,
        0x3d,
        0x00, // normalMin = 4,000,000
        0x40,
        0x54,
        0x89,
        0x00, // rated_max = 9,000,000
        0x40,
        0x42,
        0x0f,
        0x00 // rated_min = 1,000,000
    };

    struct pldm_numeric_effecter_value_pdr decodedPdr;
    auto rc =
        decode_numeric_effecter_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);

    EXPECT_EQ(PLDM_EFFECTER_DATA_SIZE_UINT32, decodedPdr.effecter_data_size);
    EXPECT_EQ(4096, decodedPdr.max_settable.value_u32);
    EXPECT_EQ(0, decodedPdr.min_settable.value_u32);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_UINT32, decodedPdr.range_field_format);
    EXPECT_EQ(0x1f, decodedPdr.range_field_support.byte);
    EXPECT_EQ(5000000, decodedPdr.nominal_value.value_u32);
    EXPECT_EQ(6000000, decodedPdr.normal_max.value_u32);
    EXPECT_EQ(4000000, decodedPdr.normal_min.value_u32);
    EXPECT_EQ(9000000, decodedPdr.rated_max.value_u32);
    EXPECT_EQ(1000000, decodedPdr.rated_min.value_u32);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(decodeNumericEffecterPdrData, Sint32Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                       // record handle
        0x1,                       // PDRHeaderVersion
        PLDM_NUMERIC_EFFECTER_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_EFFECTER_PDR_FIXED_LENGTH +
            PLDM_PDR_NUMERIC_EFFECTER_PDR_VARIED_EFFECTER_DATA_SIZE_MIN_LENGTH *
                4 +
            PLDM_PDR_NUMERIC_EFFECTER_PDR_VARIED_RANGE_FIELD_MIN_LENGTH * 4,
        0, // dataLength
        0,
        0, // PLDMTerminusHandle
        0x1,
        0x0, // effecterID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0, // containerID=1
        0x2,
        0x0,                            // effecter_semantic_id=2
        PLDM_NO_INIT,                   // effecterInit
        false,                          // effecterAuxiliaryNames
        PLDM_SENSOR_UNIT_DEGRESS_C,     // baseUint(2)=degrees C
        0,                              // unitModifier
        0,                              // rateUnit
        0,                              // baseOEMUnitHandle
        0,                              // auxUnit
        0,                              // auxUnitModifier
        0,                              // auxRateUnit
        0,                              // auxOEMUnitHandle
        true,                           // isLinear
        PLDM_EFFECTER_DATA_SIZE_SINT32, // effecterDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f, // transition_interval=1.0
        0xa0,
        0x86,
        0x01,
        0x00, // maxSetdable = 100000
        0x60,
        0x79,
        0xfe,
        0xff,                           // minSetable = -10000
        PLDM_RANGE_FIELD_FORMAT_SINT32, // rangeFieldFormat
        0x1f,                           // rangeFieldsupport
        0,
        0,
        0,
        0, // nominalValue = 0
        0x20,
        0xa1,
        0x07,
        0x00, // normalMax = 500,000
        0xe0,
        0x5e,
        0xf8,
        0xff, // normalMin = -500,000
        0xc0,
        0xc6,
        0x2d,
        0x00, // rated_max = 3,000,000
        0x40,
        0x39,
        0xd2,
        0xff // rated_min = -3,000,000
    };

    struct pldm_numeric_effecter_value_pdr decodedPdr;
    auto rc =
        decode_numeric_effecter_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);
    EXPECT_EQ(PLDM_EFFECTER_DATA_SIZE_SINT32, decodedPdr.effecter_data_size);
    EXPECT_EQ(100000, decodedPdr.max_settable.value_s32);
    EXPECT_EQ(-100000, decodedPdr.min_settable.value_s32);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_SINT32, decodedPdr.range_field_format);
    EXPECT_EQ(0x1f, decodedPdr.range_field_support.byte);
    EXPECT_EQ(0, decodedPdr.nominal_value.value_s32);
    EXPECT_EQ(500000, decodedPdr.normal_max.value_s32);
    EXPECT_EQ(-500000, decodedPdr.normal_min.value_s32);
    EXPECT_EQ(3000000, decodedPdr.rated_max.value_s32);
    EXPECT_EQ(-3000000, decodedPdr.rated_min.value_s32);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(decodeNumericEffecterPdrData, Real32Test)
{
    std::vector<uint8_t> pdr1{
        0x1,
        0x0,
        0x0,
        0x0,                       // record handle
        0x1,                       // PDRHeaderVersion
        PLDM_NUMERIC_EFFECTER_PDR, // PDRType
        0x0,
        0x0, // recordChangeNumber
        PLDM_PDR_NUMERIC_EFFECTER_PDR_FIXED_LENGTH +
            PLDM_PDR_NUMERIC_EFFECTER_PDR_VARIED_EFFECTER_DATA_SIZE_MIN_LENGTH *
                4 +
            PLDM_PDR_NUMERIC_EFFECTER_PDR_VARIED_RANGE_FIELD_MIN_LENGTH * 4,
        0, // dataLength
        0,

        0, // PLDMTerminusHandle
        0x1,
        0x0, // effecterID=1
        PLDM_ENTITY_POWER_SUPPLY,
        0, // entityType=Power Supply(120)
        1,
        0, // entityInstanceNumber
        0x1,
        0x0, // containerID=1
        0x2,
        0x0,                            // effecter_semantic_id=2
        PLDM_NO_INIT,                   // effecterInit
        false,                          // effecterAuxiliaryNames
        PLDM_SENSOR_UNIT_DEGRESS_C,     // baseUint(2)=degrees C
        0,                              // unitModifier
        0,                              // rateUnit
        0,                              // baseOEMUnitHandle
        0,                              // auxUnit
        0,                              // auxUnitModifier
        0,                              // auxRateUnit
        0,                              // auxOEMUnitHandle
        true,                           // isLinear
        PLDM_EFFECTER_DATA_SIZE_SINT32, // effecterDataSize
        0,
        0,
        0,
        0, // resolution
        0,
        0,
        0,
        0, // offset
        0,
        0, // accuracy
        0, // plusTolerance
        0, // minusTolerance
        0,
        0,
        0x80,
        0x3f, // stateTransistionInterval=1.0
        0,
        0,
        0x80,
        0x3f, // transition_interval=1.0
        0xa0,
        0x86,
        0x01,
        0x00, // maxSetdable = 100000
        0x60,
        0x79,
        0xfe,
        0xff,                           // minSetable = -10000
        PLDM_RANGE_FIELD_FORMAT_REAL32, // rangeFieldFormat
        0x1f,                           // rangeFieldsupport
        0,
        0,
        0,
        0, // nominalValue = 0.0
        0x33,
        0x33,
        0x48,
        0x42, // normalMax = 50.05
        0x33,
        0x33,
        0x48,
        0xc2, // normalMin = -50.05
        0x62,
        0x00,
        0x96,
        0x43, // rated_max = 300.003
        0x62,
        0x00,
        0x96,
        0xc3 // rated_min = -300.003
    };

    struct pldm_numeric_effecter_value_pdr decodedPdr;
    auto rc =
        decode_numeric_effecter_pdr_data(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(PLDM_SUCCESS, rc);

    EXPECT_EQ(PLDM_EFFECTER_DATA_SIZE_SINT32, decodedPdr.effecter_data_size);
    EXPECT_FLOAT_EQ(1.0f, decodedPdr.state_transition_interval);
    EXPECT_FLOAT_EQ(1.0f, decodedPdr.transition_interval);
    EXPECT_EQ(100000, decodedPdr.max_settable.value_s32);
    EXPECT_EQ(-100000, decodedPdr.min_settable.value_s32);
    EXPECT_EQ(PLDM_RANGE_FIELD_FORMAT_REAL32, decodedPdr.range_field_format);
    EXPECT_EQ(0x1f, decodedPdr.range_field_support.byte);
    EXPECT_FLOAT_EQ(0, decodedPdr.nominal_value.value_f32);
    EXPECT_FLOAT_EQ(50.05f, decodedPdr.normal_max.value_f32);
    EXPECT_FLOAT_EQ(-50.05f, decodedPdr.normal_min.value_f32);
    EXPECT_FLOAT_EQ(300.003f, decodedPdr.rated_max.value_f32);
    EXPECT_FLOAT_EQ(-300.003f, decodedPdr.rated_min.value_f32);
}
#endif

TEST(GetStateEffecterStates, testEncodeAndDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_STATE_EFFECTER_STATES_REQ_BYTES>
        requestMsg{};

    constexpr std::array<uint8_t,
                         hdrSize + PLDM_GET_STATE_EFFECTER_STATES_REQ_BYTES>
        expectedRequestMsg{
            {0x80, PLDM_PLATFORM, PLDM_GET_STATE_EFFECTER_STATES, 1, 0xab}};

    constexpr uint16_t effecter_id = 0xab01;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_get_state_effecter_states_req(
        0, effecter_id, request, PLDM_GET_STATE_EFFECTER_STATES_REQ_BYTES);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(requestMsg, expectedRequestMsg);

    uint16_t ret_effecter_id;

    rc = decode_get_state_effecter_states_req(
        request, requestMsg.size() - hdrSize, &ret_effecter_id);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(effecter_id, ret_effecter_id);

    // Test invalid length decode request

    rc = decode_get_state_effecter_states_req(
        request, requestMsg.size() - hdrSize - 1, &ret_effecter_id);

    EXPECT_EQ(rc, -EOVERFLOW);
}

TEST(GetStateEffecterStates, testBadEncodeRequest)
{
    std::vector<uint8_t> requestMsg(hdrSize +
                                    PLDM_GET_STATE_EFFECTER_STATES_REQ_BYTES);

    auto rc = encode_get_state_effecter_states_req(
        0, 0, nullptr, PLDM_GET_STATE_EFFECTER_STATES_REQ_BYTES);
    EXPECT_EQ(rc, -EINVAL);
}

TEST(GetStateEffecterStates, testBadDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_NUMERIC_EFFECTER_VALUE_REQ_BYTES>
        requestMsg{};

    auto rc = decode_get_state_effecter_states_req(
        nullptr, requestMsg.size() - hdrSize, nullptr);

    EXPECT_EQ(rc, -EINVAL);
}

TEST(GetStateEffecterStates, testEncodeAndDecodeResponse)
{
    constexpr uint8_t comp_effecterCnt = 0x2;
    constexpr uint8_t completionCode = 0;
    std::array<uint8_t,
               hdrSize + PLDM_GET_STATE_EFFECTER_STATES_MIN_RESP_BYTES +
                   PLDM_GET_EFFECTER_STATE_FIELD_SIZE * comp_effecterCnt>
        expectedResponseMsg{{0, PLDM_PLATFORM, PLDM_GET_STATE_EFFECTER_STATES,
                             completionCode, comp_effecterCnt,
                             EFFECTER_OPER_STATE_ENABLED_NOUPDATEPENDING, 2, 2,
                             EFFECTER_OPER_STATE_ENABLED_UPDATEPENDING, 2, 3}};

    decltype(expectedResponseMsg) responseMsg{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    std::array<get_effecter_state_field, comp_effecterCnt> stateField{
        {{EFFECTER_OPER_STATE_ENABLED_NOUPDATEPENDING, 2, 2},
         {EFFECTER_OPER_STATE_ENABLED_UPDATEPENDING, 2, 3}}};

    struct pldm_get_state_effecter_states_resp resp_fields{
        PLDM_SUCCESS, comp_effecterCnt, {stateField[0], stateField[1]}};

    auto rc = encode_get_state_effecter_states_resp(
        0, &resp_fields, response, responseMsg.size() - hdrSize);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(expectedResponseMsg, responseMsg);

    struct pldm_get_state_effecter_states_resp ret_resp_fields;

    rc = decode_get_state_effecter_states_resp(
        response, responseMsg.size() - hdrSize, &ret_resp_fields);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, ret_resp_fields.completion_code);
    EXPECT_EQ(comp_effecterCnt, ret_resp_fields.comp_effecter_count);
    EXPECT_EQ(stateField[0].effecter_op_state,
              ret_resp_fields.field[0].effecter_op_state);
    EXPECT_EQ(stateField[0].pending_state,
              ret_resp_fields.field[0].pending_state);
    EXPECT_EQ(stateField[0].present_state,
              ret_resp_fields.field[0].present_state);
    EXPECT_EQ(stateField[1].effecter_op_state,
              ret_resp_fields.field[1].effecter_op_state);
    EXPECT_EQ(stateField[1].pending_state,
              ret_resp_fields.field[1].pending_state);
    EXPECT_EQ(stateField[1].present_state,
              ret_resp_fields.field[1].present_state);

    // Test invalid length decode

    rc = decode_get_state_effecter_states_resp(
        response,
        responseMsg.size() - hdrSize + PLDM_GET_EFFECTER_STATE_FIELD_SIZE,
        &ret_resp_fields);

    EXPECT_EQ(rc, -EBADMSG);
}

TEST(GetStateEffecterStates, testBadEncodeResponse)
{
    struct pldm_get_state_effecter_states_resp resp{PLDM_SUCCESS, 0, {}};
    auto rc = decode_get_state_effecter_states_resp(nullptr, 0, &resp);

    EXPECT_EQ(rc, -EINVAL);
}

TEST(GetStateEffecterStates, testBadDecodeResponse)
{
    std::array<uint8_t, hdrSize +
                            PLDM_GET_STATE_EFFECTER_STATES_MIN_RESP_BYTES +
                            PLDM_GET_EFFECTER_STATE_FIELD_SIZE * 2>
        responseMsg{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_state_effecter_states_resp(
        response, responseMsg.size() - hdrSize, nullptr);

    EXPECT_EQ(rc, -EINVAL);
}

[[maybe_unused]] static size_t str16len(char16_t* startptr)
{
    char16_t* endptr = startptr;
    while (*endptr)
    {
        endptr++;
    }
    return endptr - startptr;
}

TEST(decodeEntityAuxNamePdrData, GoodTest)
{
    std::vector<uint8_t> pdr1{
        // Common PDR Header
        0x1, 0x0, 0x0, 0x0,              // record handle
        0x1,                             // PDRHeaderVersion
        PLDM_ENTITY_AUXILIARY_NAMES_PDR, // PDRType
        0x1,
        0x0, // recordChangeNumber
        0x27,
        0, // dataLength
        /* Entity Auxiliary Names PDR Data*/
        3,
        0, // entityType system software
        0x1,
        0x0, // Entity instance number =1
        PLDM_PLATFORM_ENTITY_SYSTEM_CONTAINER_ID,
        0,                // Overal system
        0,                // shared Name Count one name only
        03,               // nameStringCount
        0x65, 0x6e, 0x00, // Language Tag "en"
        0x00, 0x53, 0x00, 0x30, 0x00, 0x53, 0x00, 0x00, // Entity Name "S0S"
        0x66, 0x6e, 0x00,                               // Language Tag "en"
        0x00, 0x53, 0x00, 0x31, 0x00, 0x00,             // Entity Name "S1"
        0x67, 0x6e, 0x00,                               // Language Tag "en"
        0x00, 0x52, 0x00, 0x52, 0x00, 0x33, 0x00, 0x00  // Entity Name "RR3"
    };

    const char expectTag0[] = {0x65, 0x6e, 0x00};
    const char expectTag1[] = {0x66, 0x6e, 0x00};
    const char expectTag2[] = {0x67, 0x6e, 0x00};
    const char expectName0[] = {0x00, 0x53, 0x00, 0x30, 0x00, 0x53, 0x00, 0x00};
    const char expectName1[] = {0x00, 0x53, 0x00, 0x31, 0x00, 0x00};
    const char expectName2[] = {0x00, 0x52, 0x00, 0x52, 0x00, 0x33, 0x00, 0x00};
    auto names_offset = sizeof(struct pldm_pdr_hdr) +
                        PLDM_PDR_ENTITY_AUXILIARY_NAME_PDR_MIN_LENGTH;
    auto names_size = pdr1.size() - names_offset;
    size_t length = 0;

    size_t decodedPdrSize =
        sizeof(struct pldm_entity_auxiliary_names_pdr) + names_size;
    auto decodedPdr =
        (struct pldm_entity_auxiliary_names_pdr*)malloc(decodedPdrSize);
    EXPECT_NE(nullptr, decodedPdr);

    auto rc = decode_entity_auxiliary_names_pdr(pdr1.data(), pdr1.size(),
                                                decodedPdr, decodedPdrSize);

    EXPECT_EQ(0, rc);
    EXPECT_EQ(1, decodedPdr->hdr.record_handle);
    EXPECT_EQ(1, decodedPdr->hdr.version);
    EXPECT_EQ(PLDM_ENTITY_AUXILIARY_NAMES_PDR, decodedPdr->hdr.type);
    EXPECT_EQ(1, decodedPdr->hdr.record_change_num);
    EXPECT_EQ(pdr1.size() - sizeof(struct pldm_pdr_hdr),
              decodedPdr->hdr.length);
    EXPECT_EQ(3, decodedPdr->container.entity_type);
    EXPECT_EQ(1, decodedPdr->container.entity_instance_num);
    EXPECT_EQ(PLDM_PLATFORM_ENTITY_SYSTEM_CONTAINER_ID,
              decodedPdr->container.entity_container_id);
    EXPECT_EQ(0, decodedPdr->shared_name_count);
    EXPECT_EQ(3, decodedPdr->name_string_count);

    decodedPdr->names = (struct pldm_entity_auxiliary_name*)calloc(
        decodedPdr->name_string_count,
        sizeof(struct pldm_entity_auxiliary_name));
    EXPECT_NE(nullptr, decodedPdr->names);

    rc = decode_pldm_entity_auxiliary_names_pdr_index(decodedPdr);
    EXPECT_EQ(0, rc);

    length = strlen(decodedPdr->names[0].tag);
    EXPECT_EQ(strlen(expectTag0), length);
    EXPECT_EQ(strncmp(expectTag0, decodedPdr->names[0].tag, length + 1), 0);

    // NOLINTBEGIN(clang-analyzer-unix.Malloc)
    ASSERT_EQ(0,
              (uintptr_t)decodedPdr->names[0].name & (alignof(char16_t) - 1));
    // NOLINTEND(clang-analyzer-unix.Malloc)
    length = str16len((char16_t*)decodedPdr->names[0].name);
    EXPECT_EQ(str16len((char16_t*)expectName0), length);
    EXPECT_EQ(3, str16len((char16_t*)expectName0));
    EXPECT_EQ(memcmp(expectName0, decodedPdr->names[0].name,
                     sizeof(char16_t) * (length + 1)),
              0);

    length = strlen(decodedPdr->names[1].tag);
    EXPECT_EQ(strlen(expectTag1), length);
    EXPECT_EQ(strncmp(expectTag1, decodedPdr->names[1].tag, length + 1), 0);

    // NOLINTBEGIN(clang-analyzer-unix.Malloc)
    ASSERT_EQ(0,
              (uintptr_t)decodedPdr->names[1].name & (alignof(char16_t) - 1));
    // NOLINTEND(clang-analyzer-unix.Malloc)
    length = str16len((char16_t*)decodedPdr->names[1].name);
    EXPECT_EQ(str16len((char16_t*)expectName1), length);
    EXPECT_EQ(2, str16len((char16_t*)expectName1));
    EXPECT_EQ(memcmp(expectName1, decodedPdr->names[1].name,
                     sizeof(char16_t) * (length + 1)),
              0);

    length = strlen(decodedPdr->names[2].tag);
    EXPECT_EQ(strlen(expectTag2), length);
    EXPECT_EQ(strncmp(expectTag2, decodedPdr->names[2].tag, length + 1), 0);

    // NOLINTBEGIN(clang-analyzer-unix.Malloc)
    ASSERT_EQ(0,
              (uintptr_t)decodedPdr->names[2].name & (alignof(char16_t) - 1));
    // NOLINTEND(clang-analyzer-unix.Malloc)
    length = str16len((char16_t*)decodedPdr->names[2].name);
    EXPECT_EQ(str16len((char16_t*)expectName2), length);
    EXPECT_EQ(3, str16len((char16_t*)expectName2));
    EXPECT_EQ(memcmp(expectName2, decodedPdr->names[2].name,
                     sizeof(char16_t) * (length + 1)),
              0);

    free(decodedPdr->names);
    free(decodedPdr);
}

TEST(decodeEntityAuxNamePdrData, BadTest)
{
    std::vector<uint8_t> pdr1{
        // Common PDR Header
        0x1, 0x0, 0x0, 0x0,              // record handle
        0x1,                             // PDRHeaderVersion
        PLDM_ENTITY_AUXILIARY_NAMES_PDR, // PDRType
        0x1,
        0x0,  // recordChangeNumber
        0x25, // correct size is 0x27, input invalid size
        0,    // dataLength
        /* Entity Auxiliary Names PDR Data*/
        3,
        0, // entityType system software
        0x1,
        0x0, // Entity instance number =1
        PLDM_PLATFORM_ENTITY_SYSTEM_CONTAINER_ID,
        0,                // Overal system
        0,                // shared Name Count one name only
        0,                // Invalid nameStringCount
        0x65, 0x6e, 0x00, // Language Tag "en"
        0x00, 0x53, 0x00, 0x30, 0x00, 0x53, 0x00, 0x00, // Entity Name "S0S"
        0x66, 0x6e, 0x00,                               // Language Tag "en"
        0x00, 0x53, 0x00, 0x31, 0x00, 0x00,             // Entity Name "S1"
        0x67, 0x6e, 0x00,                               // Language Tag "en"
        0x00, 0x52, 0x00, 0x52, 0x00, 0x33, 0x00, 0x00  // Entity Name "RR3"
    };

    auto names_offset = sizeof(struct pldm_pdr_hdr) +
                        PLDM_PDR_ENTITY_AUXILIARY_NAME_PDR_MIN_LENGTH;
    auto names_size = pdr1.size() - names_offset;

    size_t decodedPdrSize =
        sizeof(struct pldm_entity_auxiliary_names_pdr) + names_size;
    auto decodedPdr =
        (struct pldm_entity_auxiliary_names_pdr*)malloc(decodedPdrSize);

    auto rc = decode_entity_auxiliary_names_pdr(pdr1.data(), pdr1.size(),
                                                decodedPdr, decodedPdrSize);

    EXPECT_EQ(-EBADMSG, rc);
    free(decodedPdr);
}

TEST(PlatformEventMessage, testGoodCperEventDataDecodeRequest)
{
    constexpr const size_t eventDataSize = 4;
    constexpr const size_t eventSize =
        PLDM_PLATFORM_CPER_EVENT_MIN_LENGTH + eventDataSize;
    std::array<uint8_t, eventSize> eventData{
        0x1,                   // format version
        0x0,                   // format type
        0x4,  0x0,             // event data length
        0x44, 0x33, 0x22, 0x11 // data
    };

    uint8_t expectedFormatVersion = 1;
    uint8_t expectedFormatType = 0;
    uint16_t expectedEventDataLength = 4;
    uint8_t expectCperEventData[] = {0x44, 0x33, 0x22, 0x11};

    size_t cperEventSize =
        sizeof(struct pldm_platform_cper_event) + eventDataSize;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto cper_event = reinterpret_cast<struct pldm_platform_cper_event*>(
        malloc(cperEventSize));

    auto rc = decode_pldm_platform_cper_event(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const void*>(eventData.data()), eventData.size(),
        cper_event, cperEventSize);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(cper_event->format_version, expectedFormatVersion);
    EXPECT_EQ(cper_event->format_type, expectedFormatType);
    EXPECT_EQ(cper_event->event_data_length, expectedEventDataLength);

    auto cperEventData = pldm_platform_cper_event_event_data(cper_event);
    EXPECT_NE(cperEventData, nullptr);
    if (cperEventData)
    {
        EXPECT_EQ(0, memcmp(expectCperEventData, cperEventData,
                            expectedEventDataLength));
    }

    free(cper_event);
}

TEST(PlatformEventMessage, testBadCperEventDataDecodeRequest)
{

    constexpr const size_t eventDataSize = 4;
    constexpr const size_t eventSize =
        PLDM_PLATFORM_CPER_EVENT_MIN_LENGTH + eventDataSize;
    std::array<uint8_t, eventSize> eventData{
        0x1,                   // format version
        0x0,                   // format type
        0x4,  0x0,             // event data length
        0x44, 0x33, 0x22, 0x11 // data
    };

    size_t cperEventSize =
        sizeof(struct pldm_platform_cper_event) + eventDataSize;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto cperEvent = reinterpret_cast<struct pldm_platform_cper_event*>(
        malloc(cperEventSize));

    auto rc = decode_pldm_platform_cper_event(NULL, eventData.size(), cperEvent,
                                              cperEventSize);
    EXPECT_EQ(rc, -EINVAL);

    rc = decode_pldm_platform_cper_event(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const void*>(eventData.data()), eventData.size(), NULL,
        cperEventSize);
    EXPECT_EQ(rc, -EINVAL);

    rc = decode_pldm_platform_cper_event(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventData.data()), eventData.size() - 1,
        cperEvent, cperEventSize);
    EXPECT_EQ(rc, -EOVERFLOW);

    rc = decode_pldm_platform_cper_event(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventData.data()), eventData.size(),
        cperEvent, cperEventSize - 1);
    EXPECT_EQ(rc, -EOVERFLOW);

    rc = decode_pldm_platform_cper_event(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<uint8_t*>(eventData.data()), eventData.size(),
        cperEvent, cperEventSize + 1);
    EXPECT_EQ(rc, 0);

    // Invalid CPER Event Format Type
    eventData[1] = 0x2;
    rc = decode_pldm_platform_cper_event(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const void*>(eventData.data()), eventData.size(),
        cperEvent, cperEventSize);

    EXPECT_EQ(rc, -EPROTO);

    // Invalid cper event data size
    eventData[1] = 0x1;
    eventData[2] = 3;
    rc = decode_pldm_platform_cper_event(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const void*>(eventData.data()), eventData.size(),
        cperEvent, cperEventSize);

    EXPECT_EQ(rc, -EBADMSG);

    eventData[2] = 5;
    rc = decode_pldm_platform_cper_event(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const void*>(eventData.data()), eventData.size(),
        cperEvent, cperEventSize);

    EXPECT_EQ(rc, -EOVERFLOW);

    free(cperEvent);
}

#ifdef LIBPLDM_API_TESTING
TEST(decodePldmFileDescriptorPdr, oemFileClassificationPresentTest)
{
    std::vector<uint8_t> pdr1{
        // Common PDR Header
        0x01, 0x0, 0x0, 0x0,      // Record Handle
        0x01,                     // PDR Header Version
        PLDM_FILE_DESCRIPTOR_PDR, // PDRType
        0x01, 0x00,               // Record Change Number
        0x2A, 0x00,               // Data Length = 42 bytes
        /* PLDM File Descriptor PDR Data*/
        0x01, 0x00, // Terminus Handle = 0x01
        0x01, 0x00, // File Identifier = 0x01
        0x09, 0x00, // Entity Type = Physical | Device File
        0x01, 0x00, // Entity instance number = 1
        PLDM_PLATFORM_ENTITY_SYSTEM_CONTAINER_ID,
        0,                      // Container ID = Overall system
        0x02, 0,                // Supper Dir File Identifier = 0x0002
        0x01,                   // File Classification = 0x01 (BootLog)
        0x01,                   // OEM File Classification = 0x01
        0x15, 0x00,             // File Capabilities = 0x0015
        0xff, 0xff, 0xff, 0xff, // File Version = 0xffffffff (Unversioned)
        0x00, 0x28, 0x00, 0x00, // File Maximum Size = 10KB
        0x02,                   // File Maximum File Descriptor count = 2
        0x06,                   // File Name Length = 6
        0x46, 0x69, 0x6C, 0x65, 0x31,
        0x00, // File Name = "File1\NULL"
        0x09, // OEM File Classification Name Length = 9
        0x4F, 0x45, 0x4D, 0x20, 0x46, 0x69, 0x6C, 0x65,
        0x00 // OEM File Classification Name = "OEM File\NULL"
    };

    const char expectFileName[] = "File1";
    const char expectOEMClassificationName[] = "OEM File";

    struct pldm_file_descriptor_pdr decodedPdr = {};

    auto rc =
        decode_pldm_file_descriptor_pdr(pdr1.data(), pdr1.size(), &decodedPdr);

    ASSERT_EQ(0, rc);
    EXPECT_EQ(1, decodedPdr.terminus_handle);
    EXPECT_EQ(1, decodedPdr.file_identifier);
    EXPECT_EQ(9, decodedPdr.container.entity_type);
    EXPECT_EQ(1, decodedPdr.container.entity_instance_num);
    EXPECT_EQ(PLDM_PLATFORM_ENTITY_SYSTEM_CONTAINER_ID,
              decodedPdr.container.entity_container_id);
    EXPECT_EQ(2, decodedPdr.superior_directory_file_identifier);
    EXPECT_EQ(1, decodedPdr.file_classification);
    EXPECT_EQ(1, decodedPdr.oem_file_classification);
    EXPECT_EQ(21, decodedPdr.file_capabilities.value);
    EXPECT_EQ(0xff, decodedPdr.file_version.alpha);
    EXPECT_EQ(0xff, decodedPdr.file_version.update);
    EXPECT_EQ(0xff, decodedPdr.file_version.minor);
    EXPECT_EQ(0xff, decodedPdr.file_version.major);
    EXPECT_EQ(10240, decodedPdr.file_maximum_size);
    EXPECT_EQ(2, decodedPdr.file_maximum_file_descriptor_count);
    EXPECT_EQ(6, decodedPdr.file_name.length);

    EXPECT_EQ(memcmp(expectFileName, decodedPdr.file_name.ptr,
                     sizeof(char) * decodedPdr.file_name.length),
              0);

    if (decodedPdr.oem_file_classification)
    {
        EXPECT_EQ(9, decodedPdr.oem_file_classification_name.length);
        EXPECT_EQ(memcmp(expectOEMClassificationName,
                         decodedPdr.oem_file_classification_name.ptr,
                         sizeof(char) *
                             decodedPdr.oem_file_classification_name.length),
                  0);
    }
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(decodePldmFileDescriptorPdr, BadTestUnAllocatedPtrParams)
{
    int rc;
    std::vector<uint8_t> pdr1{
        // Common PDR Header
        0x01, 0x0, 0x0, 0x0,      // Record Handle
        0x01,                     // PDR Header Version
        PLDM_FILE_DESCRIPTOR_PDR, // PDRType
        0x01, 0x00,               // Record Change Number
        0x20, 0x00,               // Data Length = 32 bytes
        /* PLDM File Descriptor PDR Data*/
        0x01, 0x00, // Terminus Handle = 0x01
        0x01, 0x00, // File Identifier = 0x01
        0x09, 0x00, // Entity Type = Physical | Device File
        0x01, 0x00, // Entity instance number = 1
        PLDM_PLATFORM_ENTITY_SYSTEM_CONTAINER_ID,
        0,                      // Container ID = Overall system
        0x02, 0,                // Supper Dir File Identifier = 0x0002
        0x01,                   // File Classification = 0x01 (BootLog)
        0x00,                   // OEM File Classification = 0x00
        0x15, 0x00,             // File Capabilities = 0x0015
        0xff, 0xff, 0xff, 0xff, // File Version = 0xffffffff (Unversioned)
        0x00, 0x28, 0x00, 0x00, // File Maximum Size = 10KB
        0x02,                   // File Maximum File Descriptor count = 2
        0x06,                   // File Name Length = 6
        0x46, 0x69, 0x6C, 0x65, 0x31,
        0x00, // File Name = "File1\NULL"
    };

    struct pldm_file_descriptor_pdr decodedPdr = {};

    rc = decode_pldm_file_descriptor_pdr(nullptr, pdr1.size(), &decodedPdr);
    EXPECT_EQ(-EINVAL, rc);

    rc = decode_pldm_file_descriptor_pdr(pdr1.data(), pdr1.size(), nullptr);
    EXPECT_EQ(-EINVAL, rc);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(decodePldmFileDescriptorPdr, BadTestInvalidExpectedParamLength)
{
    int rc;

    std::vector<uint8_t> pdr1{
        // Common PDR Header
        0x01, 0x0, 0x0, 0x0,      // Record Handle
        0x01,                     // PDR Header Version
        PLDM_FILE_DESCRIPTOR_PDR, // PDRType
        0x01, 0x00,               // Record Change Number
        0x20, 0x00,               // Data Length = 32 bytes
        /* PLDM File Descriptor PDR Data*/
        0x01, 0x00, // Terminus Handle = 0x01
        0x01, 0x00, // File Identifier = 0x01
        0x09, 0x00, // Entity Type = Physical | Device File
        0x01, 0x00, // Entity instance number = 1
        PLDM_PLATFORM_ENTITY_SYSTEM_CONTAINER_ID,
        0,                      // Container ID = Overall system
        0x02, 0,                // Supper Dir File Identifier = 0x0002
        0x01,                   // File Classification = 0x01 (BootLog)
        0x00,                   // OEM File Classification = 0x00
        0x15, 0x00,             // File Capabilities = 0x0015
        0xff, 0xff, 0xff, 0xff, // File Version = 0xffffffff (Unversioned)
        0x00, 0x28, 0x00, 0x00, // File Maximum Size = 10KB
        0x02,                   // File Maximum File Descriptor count = 2
        0x06,                   // File Name Length = 6
        0x46, 0x69, 0x6C, 0x65, 0x31,
        0x00, // File Name = "File1\NULL"
    };

    struct pldm_file_descriptor_pdr decodedPdr = {};

    /* Expect error: Invalid input data length*/
    rc = decode_pldm_file_descriptor_pdr(pdr1.data(), 1, &decodedPdr);
    EXPECT_EQ(-EOVERFLOW, rc);
}
#endif

#ifdef LIBPLDM_API_TESTING
TEST(decodePldmFileDescriptorPdr, BadTestDataBufferOverLength)
{
    int rc;

    /*Un-matched File Name Length*/
    std::vector<uint8_t> pdr1{
        // Common PDR Header
        0x01, 0x0, 0x0, 0x0,      // Record Handle
        0x01,                     // PDR Header Version
        PLDM_FILE_DESCRIPTOR_PDR, // PDRType
        0x01, 0x00,               // Record Change Number
        0x20, 0x00,               // Data Length = 32 bytes
        /* PLDM File Descriptor PDR Data*/
        0x01, 0x00, // Terminus Handle = 0x01
        0x01, 0x00, // File Identifier = 0x01
        0x09, 0x00, // Entity Type = Physical | Device File
        0x01, 0x00, // Entity instance number = 1
        PLDM_PLATFORM_ENTITY_SYSTEM_CONTAINER_ID,
        0,                      // Container ID = Overall system
        0x02, 0,                // Supper Dir File Identifier = 0x0002
        0x01,                   // File Classification = 0x01 (BootLog)
        0x00,                   // OEM File Classification = 0x00
        0x15, 0x00,             // File Capabilities = 0x0015
        0xff, 0xff, 0xff, 0xff, // File Version = 0xffffffff (Unversioned)
        0x00, 0x28, 0x00, 0x00, // File Maximum Size = 10KB
        0x02,                   // File Maximum File Descriptor count = 2
        0x05,                   // File Name Length = 5
        0x46, 0x69, 0x6C, 0x65, 0x31,
        0x00, // File Name = "File1\NULL"
    };

    struct pldm_file_descriptor_pdr decodedPdr = {};

    /*
     * Expect error: The original length of the data buffer is larger than
     * the target extract length.
     */
    rc = decode_pldm_file_descriptor_pdr(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(-EBADMSG, rc);
}

TEST(decodePldmFileDescriptorPdr, BadTestDataBufferUnderLength)
{
    int rc;

    /*Un-matched OEM File Classification Name Length*/
    std::vector<uint8_t> pdr1{
        // Common PDR Header
        0x01, 0x0, 0x0, 0x0,      // Record Handle
        0x01,                     // PDR Header Version
        PLDM_FILE_DESCRIPTOR_PDR, // PDRType
        0x01, 0x00,               // Record Change Number
        0x2A, 0x00,               // Data Length = 42 bytes
        /* PLDM File Descriptor PDR Data*/
        0x01, 0x00, // Terminus Handle = 0x01
        0x01, 0x00, // File Identifier = 0x01
        0x09, 0x00, // Entity Type = Physical | Device File
        0x01, 0x00, // Entity instance number = 1
        PLDM_PLATFORM_ENTITY_SYSTEM_CONTAINER_ID,
        0,                      // Container ID = Overall system
        0x02, 0,                // Supper Dir File Identifier = 0x0002
        0x01,                   // File Classification = 0x01 (BootLog)
        0x01,                   // OEM File Classification = 0x01
        0x15, 0x00,             // File Capabilities = 0x0015
        0xff, 0xff, 0xff, 0xff, // File Version = 0xffffffff (Unversioned)
        0x00, 0x28, 0x00, 0x00, // File Maximum Size = 10KB
        0x02,                   // File Maximum File Descriptor count = 2
        0x06,                   // File Name Length = 6
        0x46, 0x69, 0x6C, 0x65, 0x31,
        0x00, // File Name = "File1\NULL"
        0x0B, // OEM File Classification Name Length = 11
        0x4F, 0x45, 0x4D, 0x20, 0x46, 0x69, 0x6C, 0x65,
        0x00 // OEM File Classification Name = "OEM File\NULL"
    };

    struct pldm_file_descriptor_pdr decodedPdr = {};

    /*
     * Expect error: The original length of the data buffer is smaller than
     * the target extract length.
     */
    rc = decode_pldm_file_descriptor_pdr(pdr1.data(), pdr1.size(), &decodedPdr);
    EXPECT_EQ(-EOVERFLOW, rc);
}
#endif
