#include <libpldm/base.h>
#include <libpldm/pldm_types.h>
#include <libpldm/rde.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

static constexpr auto hdrSize = sizeof(pldm_msg_hdr);

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateRedfishParameters, encodeDecodeRequestRoundTrip)
{
    struct pldm_rde_negotiate_redfish_parameters_req req = {};
    req.mc_concurrency_support = 3;
    req.mc_feature_support.value = 0x00ab;

    std::array<uint8_t,
               hdrSize + PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_BYTES>
        reqMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(reqMsg.data());

    size_t reqLen = PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_BYTES;
    ASSERT_EQ(
        encode_pldm_rde_negotiate_redfish_parameters_req(0, &req, msg, &reqLen),
        0);
    EXPECT_EQ(reqLen, PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_BYTES);

    struct pldm_rde_negotiate_redfish_parameters_req decoded = {};
    ASSERT_EQ(
        decode_pldm_rde_negotiate_redfish_parameters_req(
            msg, PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_BYTES, &decoded),
        0);
    EXPECT_EQ(decoded.mc_concurrency_support, req.mc_concurrency_support);
    EXPECT_EQ(decoded.mc_feature_support.value, req.mc_feature_support.value);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateRedfishParameters, requestRejectsBadArgs)
{
    struct pldm_rde_negotiate_redfish_parameters_req req = {};
    req.mc_concurrency_support = 0; // invalid: must be non-zero
    pldm_msg msg{};

    size_t reqLen = PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_BYTES;
    EXPECT_EQ(encode_pldm_rde_negotiate_redfish_parameters_req(0, &req, &msg,
                                                               &reqLen),
              -EINVAL);
    EXPECT_EQ(encode_pldm_rde_negotiate_redfish_parameters_req(0, nullptr, &msg,
                                                               &reqLen),
              -EINVAL);

    // A zero concurrency value on the wire is rejected on decode.
    std::array<uint8_t,
               hdrSize + PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_BYTES>
        reqMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* wire = reinterpret_cast<pldm_msg*>(reqMsg.data());
    struct pldm_rde_negotiate_redfish_parameters_req decoded = {};
    EXPECT_EQ(
        decode_pldm_rde_negotiate_redfish_parameters_req(
            wire, PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_BYTES, &decoded),
        -EBADMSG);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateRedfishParameters, encodeDecodeResponseRoundTrip)
{
    const std::array<uint8_t, 4> providerName{'A', 'C', 'M', 'E'};

    struct pldm_rde_negotiate_redfish_parameters_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    resp.device_concurrency_support = 2;
    resp.device_capabilities_flags.byte = 0x05;
    resp.device_feature_support.value = 0x00c3;
    resp.device_configuration_signature = 0xdeadbeef;
    resp.provider_name_format = 1;
    resp.provider_name.ptr = providerName.data();
    resp.provider_name.length = providerName.size();

    size_t payloadLen = PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_BYTES +
                        providerName.size();
    std::vector<uint8_t> respMsg(hdrSize + payloadLen, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    ASSERT_EQ(encode_pldm_rde_negotiate_redfish_parameters_resp(0, &resp, msg,
                                                                &payloadLen),
              0);
    EXPECT_EQ(payloadLen, PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_BYTES +
                              providerName.size());

    struct pldm_rde_negotiate_redfish_parameters_resp decoded = {};
    ASSERT_EQ(decode_pldm_rde_negotiate_redfish_parameters_resp(msg, payloadLen,
                                                                &decoded),
              0);
    EXPECT_EQ(decoded.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(decoded.device_concurrency_support,
              resp.device_concurrency_support);
    EXPECT_EQ(decoded.device_capabilities_flags.byte,
              resp.device_capabilities_flags.byte);
    EXPECT_EQ(decoded.device_feature_support.value,
              resp.device_feature_support.value);
    EXPECT_EQ(decoded.device_configuration_signature,
              resp.device_configuration_signature);
    EXPECT_EQ(decoded.provider_name_format, resp.provider_name_format);
    ASSERT_EQ(decoded.provider_name.length, providerName.size());
    EXPECT_EQ(0, memcmp(decoded.provider_name.ptr, providerName.data(),
                        providerName.size()));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateRedfishParameters, responseErrorIsCompletionCodeOnly)
{
    struct pldm_rde_negotiate_redfish_parameters_resp resp = {};
    resp.completion_code = PLDM_ERROR;
    // device_concurrency_support left zero: an error response must not
    // require it, since only the completion code is emitted.

    std::array<uint8_t, hdrSize + 1> respMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    size_t respLen = 1;
    ASSERT_EQ(encode_pldm_rde_negotiate_redfish_parameters_resp(0, &resp, msg,
                                                                &respLen),
              0);
    EXPECT_EQ(respLen, sizeof(resp.completion_code));
    EXPECT_EQ(msg->payload[0], PLDM_ERROR);

    struct pldm_rde_negotiate_redfish_parameters_resp decoded = {};
    ASSERT_EQ(
        decode_pldm_rde_negotiate_redfish_parameters_resp(msg, 1, &decoded), 0);
    EXPECT_EQ(decoded.completion_code, PLDM_ERROR);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateRedfishParameters, decodeResponseRejectsZeroConcurrency)
{
    // cc=SUCCESS, device_concurrency_support=0 (invalid), empty name.
    std::array<uint8_t,
               hdrSize + PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_BYTES>
        respMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());
    msg->payload[0] = PLDM_SUCCESS;
    msg->payload[1] = 0; // device_concurrency_support

    struct pldm_rde_negotiate_redfish_parameters_resp decoded = {};
    EXPECT_EQ(decode_pldm_rde_negotiate_redfish_parameters_resp(
                  msg, PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_BYTES,
                  &decoded),
              -EBADMSG);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateRedfishParameters, responseRejectsNullAndOversizedName)
{
    struct pldm_rde_negotiate_redfish_parameters_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    resp.device_concurrency_support = 1;
    pldm_msg msg{};

    size_t respLen = 64;
    EXPECT_EQ(encode_pldm_rde_negotiate_redfish_parameters_resp(0, nullptr,
                                                                &msg, &respLen),
              -EINVAL);

    // provider_name longer than a uint8 length field can express.
    resp.provider_name.length = 256;
    respLen = 512;
    EXPECT_EQ(encode_pldm_rde_negotiate_redfish_parameters_resp(0, &resp, &msg,
                                                                &respLen),
              -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateMediumParameters, encodeDecodeRequestRoundTrip)
{
    struct pldm_rde_negotiate_medium_parameters_req req = {};
    req.mc_maximum_transfer_chunk_size_bytes = 256;

    std::array<uint8_t,
               hdrSize + PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_REQ_BYTES>
        reqMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(reqMsg.data());

    size_t reqLen = PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_REQ_BYTES;
    ASSERT_EQ(
        encode_pldm_rde_negotiate_medium_parameters_req(0, &req, msg, &reqLen),
        0);
    EXPECT_EQ(reqLen, PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_REQ_BYTES);

    struct pldm_rde_negotiate_medium_parameters_req decoded = {};
    ASSERT_EQ(
        decode_pldm_rde_negotiate_medium_parameters_req(
            msg, PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_REQ_BYTES, &decoded),
        0);
    EXPECT_EQ(decoded.mc_maximum_transfer_chunk_size_bytes,
              req.mc_maximum_transfer_chunk_size_bytes);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateMediumParameters, encodeDecodeResponseRoundTrip)
{
    struct pldm_rde_negotiate_medium_parameters_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    resp.device_maximum_transfer_chunk_size_bytes = 512;

    std::array<uint8_t,
               hdrSize + PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_RESP_BYTES>
        respMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    size_t respLen = PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_RESP_BYTES;
    ASSERT_EQ(encode_pldm_rde_negotiate_medium_parameters_resp(0, &resp, msg,
                                                               &respLen),
              0);
    EXPECT_EQ(respLen, PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_RESP_BYTES);

    struct pldm_rde_negotiate_medium_parameters_resp decoded = {};
    ASSERT_EQ(
        decode_pldm_rde_negotiate_medium_parameters_resp(
            msg, PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_RESP_BYTES, &decoded),
        0);
    EXPECT_EQ(decoded.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(decoded.device_maximum_transfer_chunk_size_bytes,
              resp.device_maximum_transfer_chunk_size_bytes);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateMediumParameters, responseErrorIsCompletionCodeOnly)
{
    struct pldm_rde_negotiate_medium_parameters_resp resp = {};
    resp.completion_code = PLDM_ERROR;
    // device_maximum_transfer_chunk_size_bytes left zero: an error response
    // must not require it, since only the completion code is emitted.

    std::array<uint8_t, hdrSize + 1> respMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    size_t respLen = 1;
    ASSERT_EQ(encode_pldm_rde_negotiate_medium_parameters_resp(0, &resp, msg,
                                                               &respLen),
              0);
    EXPECT_EQ(respLen, sizeof(resp.completion_code));
    EXPECT_EQ(msg->payload[0], PLDM_ERROR);

    struct pldm_rde_negotiate_medium_parameters_resp decoded = {};
    ASSERT_EQ(
        decode_pldm_rde_negotiate_medium_parameters_resp(msg, 1, &decoded), 0);
    EXPECT_EQ(decoded.completion_code, PLDM_ERROR);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(NegotiateMediumParameters, requestRejectsBadArgs)
{
    struct pldm_rde_negotiate_medium_parameters_req req = {};
    // invalid: below the 64-byte minimum transfer size.
    req.mc_maximum_transfer_chunk_size_bytes =
        PLDM_RDE_MIN_TRANSFER_SIZE_BYTES - 1;
    pldm_msg msg{};

    size_t reqLen = PLDM_RDE_NEGOTIATE_MEDIUM_PARAMETERS_REQ_BYTES;
    EXPECT_EQ(
        encode_pldm_rde_negotiate_medium_parameters_req(0, &req, &msg, &reqLen),
        -EINVAL);
    EXPECT_EQ(encode_pldm_rde_negotiate_medium_parameters_req(0, nullptr, &msg,
                                                              &reqLen),
              -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(GetSchemaDictionary, encodeDecodeRequestRoundTrip)
{
    struct pldm_rde_get_schema_dictionary_req req = {};
    req.resource_id = 0xdeadbeef;
    req.requested_schema_class = PLDM_RDE_SCHEMA_MAJOR;

    std::array<uint8_t, hdrSize + PLDM_RDE_GET_SCHEMA_DICTIONARY_REQ_BYTES>
        reqMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(reqMsg.data());

    size_t reqLen = PLDM_RDE_GET_SCHEMA_DICTIONARY_REQ_BYTES;
    ASSERT_EQ(encode_pldm_rde_get_schema_dictionary_req(0, &req, msg, &reqLen),
              0);
    EXPECT_EQ(reqLen, PLDM_RDE_GET_SCHEMA_DICTIONARY_REQ_BYTES);

    struct pldm_rde_get_schema_dictionary_req decoded = {};
    ASSERT_EQ(decode_pldm_rde_get_schema_dictionary_req(
                  msg, PLDM_RDE_GET_SCHEMA_DICTIONARY_REQ_BYTES, &decoded),
              0);
    EXPECT_EQ(decoded.resource_id, req.resource_id);
    EXPECT_EQ(decoded.requested_schema_class, req.requested_schema_class);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(GetSchemaDictionary, encodeDecodeResponseRoundTrip)
{
    struct pldm_rde_get_schema_dictionary_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    resp.dictionary_format = 1;
    resp.transfer_handle = 0x12345678;

    std::array<uint8_t, hdrSize + PLDM_RDE_GET_SCHEMA_DICTIONARY_RESP_BYTES>
        respMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    size_t respLen = PLDM_RDE_GET_SCHEMA_DICTIONARY_RESP_BYTES;
    ASSERT_EQ(
        encode_pldm_rde_get_schema_dictionary_resp(0, &resp, msg, &respLen), 0);
    EXPECT_EQ(respLen, PLDM_RDE_GET_SCHEMA_DICTIONARY_RESP_BYTES);

    struct pldm_rde_get_schema_dictionary_resp decoded = {};
    ASSERT_EQ(decode_pldm_rde_get_schema_dictionary_resp(
                  msg, PLDM_RDE_GET_SCHEMA_DICTIONARY_RESP_BYTES, &decoded),
              0);
    EXPECT_EQ(decoded.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(decoded.dictionary_format, resp.dictionary_format);
    EXPECT_EQ(decoded.transfer_handle, resp.transfer_handle);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(GetSchemaDictionary, responseErrorIsCompletionCodeOnly)
{
    struct pldm_rde_get_schema_dictionary_resp resp = {};
    resp.completion_code = PLDM_ERROR;
    // dictionary_format and transfer_handle left zero: an error response must
    // not require them, since only the completion code is emitted.

    std::array<uint8_t, hdrSize + 1> respMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    size_t respLen = 1;
    ASSERT_EQ(
        encode_pldm_rde_get_schema_dictionary_resp(0, &resp, msg, &respLen), 0);
    EXPECT_EQ(respLen, sizeof(resp.completion_code));
    EXPECT_EQ(msg->payload[0], PLDM_ERROR);

    struct pldm_rde_get_schema_dictionary_resp decoded = {};
    ASSERT_EQ(decode_pldm_rde_get_schema_dictionary_resp(msg, 1, &decoded), 0);
    EXPECT_EQ(decoded.completion_code, PLDM_ERROR);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(GetSchemaDictionary, requestRejectsBadArgs)
{
    struct pldm_rde_get_schema_dictionary_req req = {};
    // invalid: not a defined schemaClass.
    req.requested_schema_class = PLDM_RDE_SCHEMA_MAX;
    pldm_msg msg{};

    size_t reqLen = PLDM_RDE_GET_SCHEMA_DICTIONARY_REQ_BYTES;
    EXPECT_EQ(encode_pldm_rde_get_schema_dictionary_req(0, &req, &msg, &reqLen),
              -EINVAL);
    EXPECT_EQ(
        encode_pldm_rde_get_schema_dictionary_req(0, nullptr, &msg, &reqLen),
        -EINVAL);

    // An out-of-range schemaClass on the wire is rejected on decode.
    std::array<uint8_t, hdrSize + PLDM_RDE_GET_SCHEMA_DICTIONARY_REQ_BYTES>
        wireMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* wire = reinterpret_cast<pldm_msg*>(wireMsg.data());
    wire->payload[4] = PLDM_RDE_SCHEMA_MAX;
    struct pldm_rde_get_schema_dictionary_req decoded = {};
    EXPECT_EQ(decode_pldm_rde_get_schema_dictionary_req(
                  wire, PLDM_RDE_GET_SCHEMA_DICTIONARY_REQ_BYTES, &decoded),
              -EBADMSG);
}
#endif
