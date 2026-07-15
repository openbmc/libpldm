#include <libpldm/base.h>
#include <libpldm/pldm_types.h>
#include <libpldm/rde.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <string>
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

#if HAVE_LIBPLDM_API_TESTING
TEST(GetSchemaURI, encodeDecodeRequestRoundTrip)
{
    struct pldm_rde_get_schema_uri_req req = {};
    req.resource_id = 0x0a0b0c0d;
    req.requested_schema_class = PLDM_RDE_SCHEMA_MAJOR;
    req.oem_extension_number = 0;

    std::array<uint8_t, hdrSize + PLDM_RDE_GET_SCHEMA_URI_REQ_BYTES> reqMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(reqMsg.data());

    size_t reqLen = PLDM_RDE_GET_SCHEMA_URI_REQ_BYTES;
    ASSERT_EQ(encode_pldm_rde_get_schema_uri_req(0, &req, msg, &reqLen), 0);
    EXPECT_EQ(reqLen, PLDM_RDE_GET_SCHEMA_URI_REQ_BYTES);

    struct pldm_rde_get_schema_uri_req decoded = {};
    ASSERT_EQ(decode_pldm_rde_get_schema_uri_req(
                  msg, PLDM_RDE_GET_SCHEMA_URI_REQ_BYTES, &decoded),
              0);
    EXPECT_EQ(decoded.resource_id, req.resource_id);
    EXPECT_EQ(decoded.requested_schema_class, req.requested_schema_class);
    EXPECT_EQ(decoded.oem_extension_number, req.oem_extension_number);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(GetSchemaURI, encodeDecodeResponseRoundTripWalksFragments)
{
    // Fragment lengths include the null terminator per DSP0218 Table 2.
    const char frag0[] = "/redfish/v1/";
    const char frag1[] = "Systems";
    std::array<struct pldm_rde_varstring, 2> uris{};
    uris[0].string_format = PLDM_RDE_VARSTRING_ASCII;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    uris[0].string_data.ptr = reinterpret_cast<const uint8_t*>(frag0);
    uris[0].string_data.length = sizeof(frag0);
    uris[1].string_format = PLDM_RDE_VARSTRING_ASCII;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    uris[1].string_data.ptr = reinterpret_cast<const uint8_t*>(frag1);
    uris[1].string_data.length = sizeof(frag1);

    const size_t payloadLen =
        PLDM_RDE_GET_SCHEMA_URI_RESP_FIXED_BYTES +
        (PLDM_RDE_VARSTRING_HEADER_BYTES + sizeof(frag0)) +
        (PLDM_RDE_VARSTRING_HEADER_BYTES + sizeof(frag1));
    std::vector<uint8_t> respMsg(hdrSize + payloadLen, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    struct pldm_rde_get_schema_uri_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    resp.string_fragment_count = 2;

    size_t encLen = payloadLen;
    ASSERT_EQ(encode_pldm_rde_get_schema_uri_resp(0, &resp, uris.data(), msg,
                                                  &encLen),
              0);
    EXPECT_EQ(encLen, payloadLen);

    struct pldm_rde_get_schema_uri_resp decoded = {};
    struct pldm_rde_varstring_iter iter = {};
    ASSERT_EQ(
        decode_pldm_rde_get_schema_uri_resp(msg, payloadLen, &decoded, &iter),
        0);
    EXPECT_EQ(decoded.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(decoded.string_fragment_count, 2);

    std::vector<std::string> got;
    struct pldm_rde_varstring frag = {};
    int rc = 0;
    foreach_pldm_rde_varstring(iter, frag, rc)
    {
        EXPECT_EQ(frag.string_format, PLDM_RDE_VARSTRING_ASCII);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        got.emplace_back(reinterpret_cast<const char*>(frag.string_data.ptr),
                         frag.string_data.length);
    }
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(got.size(), 2U);
    EXPECT_EQ(got[0], std::string(frag0, sizeof(frag0)));
    EXPECT_EQ(got[1], std::string(frag1, sizeof(frag1)));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(GetSchemaURI, responseErrorIsCompletionCodeOnly)
{
    std::array<uint8_t, hdrSize + 1> respMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    // An error response requires no fragments, so uris may be null.
    struct pldm_rde_get_schema_uri_resp resp = {};
    resp.completion_code = PLDM_ERROR;
    size_t respLen = 1;
    ASSERT_EQ(
        encode_pldm_rde_get_schema_uri_resp(0, &resp, nullptr, msg, &respLen),
        0);
    EXPECT_EQ(respLen, 1U);
    EXPECT_EQ(msg->payload[0], PLDM_ERROR);

    struct pldm_rde_get_schema_uri_resp decoded = {};
    struct pldm_rde_varstring_iter iter = {};
    ASSERT_EQ(decode_pldm_rde_get_schema_uri_resp(msg, 1, &decoded, &iter), 0);
    EXPECT_EQ(decoded.completion_code, PLDM_ERROR);
    EXPECT_TRUE(pldm_rde_varstring_iter_end(&iter));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(GetSchemaURI, requestRejectsBadArgs)
{
    struct pldm_rde_get_schema_uri_req req = {};
    // invalid: not a defined schemaClass.
    req.requested_schema_class = PLDM_RDE_SCHEMA_MAX;
    pldm_msg msg{};

    size_t reqLen = PLDM_RDE_GET_SCHEMA_URI_REQ_BYTES;
    EXPECT_EQ(encode_pldm_rde_get_schema_uri_req(0, &req, &msg, &reqLen),
              -EINVAL);
    EXPECT_EQ(encode_pldm_rde_get_schema_uri_req(0, nullptr, &msg, &reqLen),
              -EINVAL);

    // A success response with a zero fragment count is rejected on encode.
    pldm_msg respMsg{};
    size_t respLen = PLDM_RDE_GET_SCHEMA_URI_RESP_FIXED_BYTES;
    struct pldm_rde_get_schema_uri_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    resp.string_fragment_count = 0;
    struct pldm_rde_varstring uri = {};
    EXPECT_EQ(
        encode_pldm_rde_get_schema_uri_resp(0, &resp, &uri, &respMsg, &respLen),
        -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(GetResourceETag, encodeDecodeRequestRoundTrip)
{
    struct pldm_rde_get_resource_etag_req req = {};
    req.resource_id = 0xffffffff;

    std::array<uint8_t, hdrSize + PLDM_RDE_GET_RESOURCE_ETAG_REQ_BYTES>
        reqMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(reqMsg.data());

    size_t reqLen = PLDM_RDE_GET_RESOURCE_ETAG_REQ_BYTES;
    ASSERT_EQ(encode_pldm_rde_get_resource_etag_req(0, &req, msg, &reqLen), 0);
    EXPECT_EQ(reqLen, PLDM_RDE_GET_RESOURCE_ETAG_REQ_BYTES);

    struct pldm_rde_get_resource_etag_req decoded = {};
    ASSERT_EQ(decode_pldm_rde_get_resource_etag_req(
                  msg, PLDM_RDE_GET_RESOURCE_ETAG_REQ_BYTES, &decoded),
              0);
    EXPECT_EQ(decoded.resource_id, req.resource_id);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(GetResourceETag, encodeDecodeResponseRoundTrip)
{
    // ETag length includes the NUL terminator per DSP0218 Table 2.
    const char etag[] = "\"abc123\"";

    struct pldm_rde_get_resource_etag_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    resp.etag.string_format = PLDM_RDE_VARSTRING_UTF_8;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    resp.etag.string_data.ptr = reinterpret_cast<const uint8_t*>(etag);
    resp.etag.string_data.length = sizeof(etag);

    const size_t payloadLen =
        PLDM_RDE_GET_RESOURCE_ETAG_RESP_MIN_BYTES + sizeof(etag);
    std::vector<uint8_t> respMsg(hdrSize + payloadLen, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    size_t encLen = payloadLen;
    ASSERT_EQ(encode_pldm_rde_get_resource_etag_resp(0, &resp, msg, &encLen),
              0);
    EXPECT_EQ(encLen, payloadLen);

    struct pldm_rde_get_resource_etag_resp decoded = {};
    ASSERT_EQ(decode_pldm_rde_get_resource_etag_resp(msg, payloadLen, &decoded),
              0);
    EXPECT_EQ(decoded.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(decoded.etag.string_format, PLDM_RDE_VARSTRING_UTF_8);
    ASSERT_EQ(decoded.etag.string_data.length, sizeof(etag));
    EXPECT_EQ(0, memcmp(decoded.etag.string_data.ptr, etag, sizeof(etag)));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(GetResourceETag, responseErrorIsCompletionCodeOnly)
{
    struct pldm_rde_get_resource_etag_resp resp = {};
    resp.completion_code = PLDM_RDE_CC_ETAG_CALCULATION_ONGOING;
    // etag left empty: an error response must not require it.

    std::array<uint8_t, hdrSize + 1> respMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    size_t respLen = 1;
    ASSERT_EQ(encode_pldm_rde_get_resource_etag_resp(0, &resp, msg, &respLen),
              0);
    EXPECT_EQ(respLen, sizeof(resp.completion_code));
    EXPECT_EQ(msg->payload[0], PLDM_RDE_CC_ETAG_CALCULATION_ONGOING);

    struct pldm_rde_get_resource_etag_resp decoded = {};
    ASSERT_EQ(decode_pldm_rde_get_resource_etag_resp(msg, 1, &decoded), 0);
    EXPECT_EQ(decoded.completion_code, PLDM_RDE_CC_ETAG_CALCULATION_ONGOING);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(GetResourceETag, responseRejectsBadEtag)
{
    pldm_msg msg{};
    size_t respLen = 64;

    // A success response with an empty etag is rejected: the etag must
    // include the NUL terminator.
    struct pldm_rde_get_resource_etag_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    const char etag[] = "x";
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    resp.etag.string_data.ptr = reinterpret_cast<const uint8_t*>(etag);
    resp.etag.string_data.length = 0;
    EXPECT_EQ(encode_pldm_rde_get_resource_etag_resp(0, &resp, &msg, &respLen),
              -EINVAL);

    // A non-empty etag with a null pointer is rejected.
    resp.etag.string_data.ptr = nullptr;
    resp.etag.string_data.length = sizeof(etag);
    EXPECT_EQ(encode_pldm_rde_get_resource_etag_resp(0, &resp, &msg, &respLen),
              -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationInit, encodeDecodeRequestRoundTrip)
{
    const uint8_t locator[] = {0x01, 0x02, 0x03};
    const uint8_t payload[] = {0xaa, 0xbb, 0xcc, 0xdd};

    struct pldm_rde_rde_operation_init_req req = {};
    req.resource_id = 0x11223344;
    req.operation_id = 0x8001;
    req.operation_type = PLDM_RDE_OPERATION_TYPE_UPDATE;
    // locator_valid | contains_request_payload
    req.operation_flags.byte = 0x03;
    req.send_data_transfer_handle = 0x55667788;
    req.operation_locator.ptr = locator;
    req.operation_locator.length = sizeof(locator);
    req.request_payload.ptr = payload;
    req.request_payload.length = sizeof(payload);

    const size_t payloadLen = PLDM_RDE_OPERATION_INIT_REQ_FIXED_BYTES +
                              sizeof(locator) + sizeof(payload);
    std::vector<uint8_t> reqMsg(hdrSize + payloadLen, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(reqMsg.data());

    size_t reqLen = payloadLen;
    ASSERT_EQ(encode_pldm_rde_rde_operation_init_req(0, &req, msg, &reqLen), 0);
    EXPECT_EQ(reqLen, payloadLen);

    struct pldm_rde_rde_operation_init_req decoded = {};
    ASSERT_EQ(decode_pldm_rde_rde_operation_init_req(msg, payloadLen, &decoded),
              0);
    EXPECT_EQ(decoded.resource_id, req.resource_id);
    EXPECT_EQ(decoded.operation_id, req.operation_id);
    EXPECT_EQ(decoded.operation_type, req.operation_type);
    EXPECT_EQ(decoded.operation_flags.byte, req.operation_flags.byte);
    EXPECT_EQ(decoded.send_data_transfer_handle, req.send_data_transfer_handle);
    ASSERT_EQ(decoded.operation_locator.length, sizeof(locator));
    EXPECT_EQ(0,
              memcmp(decoded.operation_locator.ptr, locator, sizeof(locator)));
    ASSERT_EQ(decoded.request_payload.length, sizeof(payload));
    EXPECT_EQ(0, memcmp(decoded.request_payload.ptr, payload, sizeof(payload)));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationInit, requestRejectsBadArgs)
{
    pldm_msg msg{};
    size_t reqLen = PLDM_RDE_OPERATION_INIT_REQ_FIXED_BYTES;

    // An out-of-range operation type is rejected.
    struct pldm_rde_rde_operation_init_req req = {};
    req.operation_type = PLDM_RDE_OPERATION_TYPE_MAX;
    EXPECT_EQ(encode_pldm_rde_rde_operation_init_req(0, &req, &msg, &reqLen),
              -EINVAL);
    EXPECT_EQ(encode_pldm_rde_rde_operation_init_req(0, nullptr, &msg, &reqLen),
              -EINVAL);

    // A non-empty locator with a null pointer is rejected.
    struct pldm_rde_rde_operation_init_req bad = {};
    bad.operation_type = PLDM_RDE_OPERATION_TYPE_READ;
    bad.operation_locator.ptr = nullptr;
    bad.operation_locator.length = 4;
    EXPECT_EQ(encode_pldm_rde_rde_operation_init_req(0, &bad, &msg, &reqLen),
              -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationInit, encodeDecodeResponseRoundTrip)
{
    // ETag length includes the NUL terminator per DSP0218 Table 2.
    const char etag[] = "\"v1\"";
    const uint8_t payload[] = {0x10, 0x20, 0x30};

    struct pldm_rde_rde_operation_init_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    resp.operation_status = PLDM_RDE_OPERATION_STATUS_COMPLETED;
    resp.completion_percentage = 100;
    resp.completion_time_seconds = 0;
    resp.operation_execution_flags.byte = 0x04; // HaveResultPayload
    resp.result_transfer_handle = 0;
    resp.permission_flags.byte = 0x01; // read access
    resp.etag.string_format = PLDM_RDE_VARSTRING_UTF_8;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    resp.etag.string_data.ptr = reinterpret_cast<const uint8_t*>(etag);
    resp.etag.string_data.length = sizeof(etag);
    resp.response_payload.ptr = payload;
    resp.response_payload.length = sizeof(payload);

    const size_t payloadLen = PLDM_RDE_OPERATION_INIT_RESP_FIXED_BYTES +
                              PLDM_RDE_VARSTRING_HEADER_BYTES + sizeof(etag) +
                              sizeof(payload);
    std::vector<uint8_t> respMsg(hdrSize + payloadLen, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    size_t encLen = payloadLen;
    ASSERT_EQ(encode_pldm_rde_rde_operation_init_resp(0, &resp, msg, &encLen),
              0);
    EXPECT_EQ(encLen, payloadLen);

    struct pldm_rde_rde_operation_init_resp decoded = {};
    ASSERT_EQ(
        decode_pldm_rde_rde_operation_init_resp(msg, payloadLen, &decoded), 0);
    EXPECT_EQ(decoded.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(decoded.operation_status, PLDM_RDE_OPERATION_STATUS_COMPLETED);
    EXPECT_EQ(decoded.completion_percentage, 100);
    EXPECT_EQ(decoded.completion_time_seconds, 0U);
    EXPECT_EQ(decoded.operation_execution_flags.byte, 0x04);
    EXPECT_EQ(decoded.result_transfer_handle, 0U);
    EXPECT_EQ(decoded.permission_flags.byte, 0x01);
    EXPECT_EQ(decoded.etag.string_format, PLDM_RDE_VARSTRING_UTF_8);
    ASSERT_EQ(decoded.etag.string_data.length, sizeof(etag));
    EXPECT_EQ(0, memcmp(decoded.etag.string_data.ptr, etag, sizeof(etag)));
    ASSERT_EQ(decoded.response_payload.length, sizeof(payload));
    EXPECT_EQ(0,
              memcmp(decoded.response_payload.ptr, payload, sizeof(payload)));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationInit, responseErrorIsCompletionCodeOnly)
{
    struct pldm_rde_rde_operation_init_resp resp = {};
    resp.completion_code = PLDM_RDE_CC_ERROR_NO_SUCH_RESOURCE;

    std::array<uint8_t, hdrSize + 1> respMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    size_t respLen = 1;
    ASSERT_EQ(encode_pldm_rde_rde_operation_init_resp(0, &resp, msg, &respLen),
              0);
    EXPECT_EQ(respLen, sizeof(resp.completion_code));
    EXPECT_EQ(msg->payload[0], PLDM_RDE_CC_ERROR_NO_SUCH_RESOURCE);

    struct pldm_rde_rde_operation_init_resp decoded = {};
    ASSERT_EQ(decode_pldm_rde_rde_operation_init_resp(msg, 1, &decoded), 0);
    EXPECT_EQ(decoded.completion_code, PLDM_RDE_CC_ERROR_NO_SUCH_RESOURCE);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationInit, responseRejectsBadArgs)
{
    pldm_msg msg{};
    size_t respLen = 64;

    // A success response with an empty etag is rejected: the etag must
    // include the NUL terminator.
    struct pldm_rde_rde_operation_init_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    const char etag[] = "x";
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    resp.etag.string_data.ptr = reinterpret_cast<const uint8_t*>(etag);
    resp.etag.string_data.length = 0;
    EXPECT_EQ(encode_pldm_rde_rde_operation_init_resp(0, &resp, &msg, &respLen),
              -EINVAL);

    // A non-empty response payload with a null pointer is rejected.
    resp.etag.string_data.length = sizeof(etag);
    resp.response_payload.ptr = nullptr;
    resp.response_payload.length = 4;
    EXPECT_EQ(encode_pldm_rde_rde_operation_init_resp(0, &resp, &msg, &respLen),
              -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationComplete, encodeDecodeRequestRoundTrip)
{
    struct pldm_rde_rde_operation_complete_req req = {};
    req.resource_id = 0x0a0b0c0d;
    req.operation_id = 0x8042;

    std::array<uint8_t, hdrSize + PLDM_RDE_OPERATION_COMPLETE_REQ_BYTES>
        reqMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(reqMsg.data());

    size_t reqLen = PLDM_RDE_OPERATION_COMPLETE_REQ_BYTES;
    ASSERT_EQ(encode_pldm_rde_rde_operation_complete_req(0, &req, msg, &reqLen),
              0);
    EXPECT_EQ(reqLen, PLDM_RDE_OPERATION_COMPLETE_REQ_BYTES);

    struct pldm_rde_rde_operation_complete_req decoded = {};
    ASSERT_EQ(decode_pldm_rde_rde_operation_complete_req(
                  msg, PLDM_RDE_OPERATION_COMPLETE_REQ_BYTES, &decoded),
              0);
    EXPECT_EQ(decoded.resource_id, req.resource_id);
    EXPECT_EQ(decoded.operation_id, req.operation_id);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationComplete, encodeDecodeResponseRoundTrip)
{
    for (uint8_t cc :
         {uint8_t{PLDM_SUCCESS}, uint8_t{PLDM_RDE_CC_ERROR_NO_SUCH_RESOURCE}})
    {
        struct pldm_rde_rde_operation_complete_resp resp = {};
        resp.completion_code = cc;

        std::array<uint8_t, hdrSize + PLDM_RDE_OPERATION_COMPLETE_RESP_BYTES>
            respMsg{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

        size_t respLen = PLDM_RDE_OPERATION_COMPLETE_RESP_BYTES;
        ASSERT_EQ(encode_pldm_rde_rde_operation_complete_resp(0, &resp, msg,
                                                              &respLen),
                  0);
        EXPECT_EQ(respLen, PLDM_RDE_OPERATION_COMPLETE_RESP_BYTES);
        EXPECT_EQ(msg->payload[0], cc);

        struct pldm_rde_rde_operation_complete_resp decoded = {};
        ASSERT_EQ(decode_pldm_rde_rde_operation_complete_resp(
                      msg, PLDM_RDE_OPERATION_COMPLETE_RESP_BYTES, &decoded),
                  0);
        EXPECT_EQ(decoded.completion_code, cc);
    }
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationComplete, requestRejectsBadArgs)
{
    pldm_msg msg{};
    size_t reqLen = PLDM_RDE_OPERATION_COMPLETE_REQ_BYTES;
    EXPECT_EQ(
        encode_pldm_rde_rde_operation_complete_req(0, nullptr, &msg, &reqLen),
        -EINVAL);

    struct pldm_rde_rde_operation_complete_req req = {};
    EXPECT_EQ(
        encode_pldm_rde_rde_operation_complete_req(0, &req, nullptr, &reqLen),
        -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationStatus, encodeDecodeRequestRoundTrip)
{
    struct pldm_rde_rde_operation_status_req req = {};
    req.resource_id = 0x0a0b0c0d;
    req.operation_id = 0x8042;

    std::array<uint8_t, hdrSize + PLDM_RDE_OPERATION_STATUS_REQ_BYTES> reqMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(reqMsg.data());

    size_t reqLen = PLDM_RDE_OPERATION_STATUS_REQ_BYTES;
    ASSERT_EQ(encode_pldm_rde_rde_operation_status_req(0, &req, msg, &reqLen),
              0);
    EXPECT_EQ(reqLen, PLDM_RDE_OPERATION_STATUS_REQ_BYTES);

    struct pldm_rde_rde_operation_status_req decoded = {};
    ASSERT_EQ(decode_pldm_rde_rde_operation_status_req(
                  msg, PLDM_RDE_OPERATION_STATUS_REQ_BYTES, &decoded),
              0);
    EXPECT_EQ(decoded.resource_id, req.resource_id);
    EXPECT_EQ(decoded.operation_id, req.operation_id);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationStatus, encodeDecodeResponseRoundTrip)
{
    const char etag[] = "\"v9\"";
    const uint8_t payload[] = {0x01, 0x02};

    struct pldm_rde_rde_operation_status_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    resp.operation_status = PLDM_RDE_OPERATION_STATUS_RUNNING;
    resp.completion_percentage = 42;
    resp.completion_time_seconds = 0xffffffff;
    resp.operation_execution_flags.byte = 0x00;
    resp.result_transfer_handle = 0x11223344;
    resp.permission_flags.byte = 0x03;
    resp.etag.string_format = PLDM_RDE_VARSTRING_UTF_8;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    resp.etag.string_data.ptr = reinterpret_cast<const uint8_t*>(etag);
    resp.etag.string_data.length = sizeof(etag);
    resp.response_payload.ptr = payload;
    resp.response_payload.length = sizeof(payload);

    const size_t payloadLen = PLDM_RDE_OPERATION_STATUS_RESP_FIXED_BYTES +
                              PLDM_RDE_VARSTRING_HEADER_BYTES + sizeof(etag) +
                              sizeof(payload);
    std::vector<uint8_t> respMsg(hdrSize + payloadLen, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    size_t encLen = payloadLen;
    ASSERT_EQ(encode_pldm_rde_rde_operation_status_resp(0, &resp, msg, &encLen),
              0);
    EXPECT_EQ(encLen, payloadLen);

    struct pldm_rde_rde_operation_status_resp decoded = {};
    ASSERT_EQ(
        decode_pldm_rde_rde_operation_status_resp(msg, payloadLen, &decoded),
        0);
    EXPECT_EQ(decoded.operation_status, PLDM_RDE_OPERATION_STATUS_RUNNING);
    EXPECT_EQ(decoded.completion_percentage, 42);
    EXPECT_EQ(decoded.completion_time_seconds, 0xffffffffU);
    EXPECT_EQ(decoded.result_transfer_handle, 0x11223344U);
    EXPECT_EQ(decoded.permission_flags.byte, 0x03);
    ASSERT_EQ(decoded.etag.string_data.length, sizeof(etag));
    EXPECT_EQ(0, memcmp(decoded.etag.string_data.ptr, etag, sizeof(etag)));
    ASSERT_EQ(decoded.response_payload.length, sizeof(payload));
    EXPECT_EQ(0,
              memcmp(decoded.response_payload.ptr, payload, sizeof(payload)));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationStatus, responseErrorIsCompletionCodeOnly)
{
    struct pldm_rde_rde_operation_status_resp resp = {};
    resp.completion_code = PLDM_RDE_CC_ERROR_UNSUPPORTED;

    std::array<uint8_t, hdrSize + 1> respMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    size_t respLen = 1;
    ASSERT_EQ(
        encode_pldm_rde_rde_operation_status_resp(0, &resp, msg, &respLen), 0);
    EXPECT_EQ(respLen, sizeof(resp.completion_code));

    struct pldm_rde_rde_operation_status_resp decoded = {};
    ASSERT_EQ(decode_pldm_rde_rde_operation_status_resp(msg, 1, &decoded), 0);
    EXPECT_EQ(decoded.completion_code, PLDM_RDE_CC_ERROR_UNSUPPORTED);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
// DSP0218 Table 68 defines the RDEOperationStatus response to be byte-identical
// to the RDEOperationInit response; the two encoders share a helper, so their
// payloads must match for equal field values.
TEST(RDEOperationStatus, responseBodyMatchesOperationInit)
{
    const char etag[] = "\"same\"";
    const uint8_t payload[] = {0xde, 0xad, 0xbe, 0xef};

    const size_t payloadLen = PLDM_RDE_OPERATION_INIT_RESP_FIXED_BYTES +
                              PLDM_RDE_VARSTRING_HEADER_BYTES + sizeof(etag) +
                              sizeof(payload);

    struct pldm_rde_rde_operation_status_resp sResp = {};
    sResp.completion_code = PLDM_SUCCESS;
    sResp.operation_status = PLDM_RDE_OPERATION_STATUS_COMPLETED;
    sResp.completion_percentage = 100;
    sResp.completion_time_seconds = 7;
    sResp.operation_execution_flags.byte = 0x04;
    sResp.result_transfer_handle = 0x55667788;
    sResp.permission_flags.byte = 0x21;
    sResp.etag.string_format = PLDM_RDE_VARSTRING_UTF_8;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    sResp.etag.string_data.ptr = reinterpret_cast<const uint8_t*>(etag);
    sResp.etag.string_data.length = sizeof(etag);
    sResp.response_payload.ptr = payload;
    sResp.response_payload.length = sizeof(payload);

    struct pldm_rde_rde_operation_init_resp iResp = {};
    iResp.completion_code = sResp.completion_code;
    iResp.operation_status = sResp.operation_status;
    iResp.completion_percentage = sResp.completion_percentage;
    iResp.completion_time_seconds = sResp.completion_time_seconds;
    iResp.operation_execution_flags = sResp.operation_execution_flags;
    iResp.result_transfer_handle = sResp.result_transfer_handle;
    iResp.permission_flags = sResp.permission_flags;
    iResp.etag = sResp.etag;
    iResp.response_payload = sResp.response_payload;

    std::vector<uint8_t> sMsg(hdrSize + payloadLen, 0);
    std::vector<uint8_t> iMsg(hdrSize + payloadLen, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* sm = reinterpret_cast<pldm_msg*>(sMsg.data());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* im = reinterpret_cast<pldm_msg*>(iMsg.data());

    size_t sLen = payloadLen;
    size_t iLen = payloadLen;
    ASSERT_EQ(encode_pldm_rde_rde_operation_status_resp(0, &sResp, sm, &sLen),
              0);
    ASSERT_EQ(encode_pldm_rde_rde_operation_init_resp(0, &iResp, im, &iLen), 0);
    ASSERT_EQ(sLen, iLen);
    // The payloads (all bytes after the PLDM header) must be identical.
    EXPECT_EQ(0, memcmp(sm->payload, im->payload, sLen));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationEnumerate, encodeDecodeRequestRoundTrip)
{
    std::array<uint8_t, hdrSize + 4> reqMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(reqMsg.data());

    size_t reqLen = 0;
    ASSERT_EQ(encode_pldm_rde_rde_operation_enumerate_req(0, msg, &reqLen), 0);
    EXPECT_EQ(reqLen, 0U);

    EXPECT_EQ(decode_pldm_rde_rde_operation_enumerate_req(msg, 0), 0);
    // The request carries no parameters, so a non-empty payload is rejected.
    EXPECT_NE(decode_pldm_rde_rde_operation_enumerate_req(msg, 4), 0);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationEnumerate, encodeDecodeResponseRoundTripWalksEntries)
{
    std::array<struct pldm_rde_op_entry, 2> entries{};
    entries[0].resource_id = 0x00001111;
    entries[0].operation_id = 0x8001;
    entries[0].operation_type = PLDM_RDE_OPERATION_TYPE_READ;
    entries[1].resource_id = 0x00002222;
    entries[1].operation_id = 0x8002;
    entries[1].operation_type = PLDM_RDE_OPERATION_TYPE_UPDATE;

    const size_t payloadLen = PLDM_RDE_OPERATION_ENUMERATE_RESP_FIXED_BYTES +
                              2 * PLDM_RDE_OPERATION_ENUMERATE_ENTRY_BYTES;
    std::vector<uint8_t> respMsg(hdrSize + payloadLen, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    struct pldm_rde_rde_operation_enumerate_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    resp.operation_count = 2;

    size_t encLen = payloadLen;
    ASSERT_EQ(encode_pldm_rde_rde_operation_enumerate_resp(
                  0, &resp, entries.data(), msg, &encLen),
              0);
    EXPECT_EQ(encLen, payloadLen);

    struct pldm_rde_rde_operation_enumerate_resp decoded = {};
    struct pldm_rde_operation_enumerate_iter iter = {};
    ASSERT_EQ(decode_pldm_rde_rde_operation_enumerate_resp(msg, payloadLen,
                                                           &decoded, &iter),
              0);
    EXPECT_EQ(decoded.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(decoded.operation_count, 2);

    std::vector<struct pldm_rde_op_entry> got;
    struct pldm_rde_op_entry entry = {};
    int rc = 0;
    foreach_pldm_rde_op_entry(iter, entry, rc)
    {
        got.push_back(entry);
    }
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(got.size(), 2U);
    EXPECT_EQ(got[0].resource_id, entries[0].resource_id);
    EXPECT_EQ(got[0].operation_id, entries[0].operation_id);
    EXPECT_EQ(got[0].operation_type, entries[0].operation_type);
    EXPECT_EQ(got[1].resource_id, entries[1].resource_id);
    EXPECT_EQ(got[1].operation_id, entries[1].operation_id);
    EXPECT_EQ(got[1].operation_type, entries[1].operation_type);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationEnumerate, responseEmptyWhenCountZero)
{
    std::array<uint8_t, hdrSize + PLDM_RDE_OPERATION_ENUMERATE_RESP_FIXED_BYTES>
        respMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    struct pldm_rde_rde_operation_enumerate_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    resp.operation_count = 0;

    size_t encLen = PLDM_RDE_OPERATION_ENUMERATE_RESP_FIXED_BYTES;
    ASSERT_EQ(encode_pldm_rde_rde_operation_enumerate_resp(0, &resp, nullptr,
                                                           msg, &encLen),
              0);
    EXPECT_EQ(encLen, PLDM_RDE_OPERATION_ENUMERATE_RESP_FIXED_BYTES);

    struct pldm_rde_rde_operation_enumerate_resp decoded = {};
    struct pldm_rde_operation_enumerate_iter iter = {};
    ASSERT_EQ(decode_pldm_rde_rde_operation_enumerate_resp(msg, encLen,
                                                           &decoded, &iter),
              0);
    EXPECT_EQ(decoded.operation_count, 0);
    EXPECT_TRUE(pldm_rde_operation_enumerate_iter_end(&iter));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationEnumerate, responseErrorIsCompletionCodeOnly)
{
    std::array<uint8_t, hdrSize + 1> respMsg{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    struct pldm_rde_rde_operation_enumerate_resp resp = {};
    resp.completion_code = PLDM_ERROR;
    size_t respLen = 1;
    ASSERT_EQ(encode_pldm_rde_rde_operation_enumerate_resp(0, &resp, nullptr,
                                                           msg, &respLen),
              0);
    EXPECT_EQ(respLen, 1U);

    struct pldm_rde_rde_operation_enumerate_resp decoded = {};
    struct pldm_rde_operation_enumerate_iter iter = {};
    ASSERT_EQ(
        decode_pldm_rde_rde_operation_enumerate_resp(msg, 1, &decoded, &iter),
        0);
    EXPECT_EQ(decoded.completion_code, PLDM_ERROR);
    EXPECT_TRUE(pldm_rde_operation_enumerate_iter_end(&iter));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(RDEOperationEnumerate, responseRejectsTruncatedEntries)
{
    // Encode two entries, then decode claiming a payload that holds only one:
    // the fixed-size entries must exactly fill the trailing bytes.
    std::array<struct pldm_rde_op_entry, 2> entries{};
    entries[0].operation_type = PLDM_RDE_OPERATION_TYPE_READ;
    entries[1].operation_type = PLDM_RDE_OPERATION_TYPE_READ;

    const size_t fullLen = PLDM_RDE_OPERATION_ENUMERATE_RESP_FIXED_BYTES +
                           2 * PLDM_RDE_OPERATION_ENUMERATE_ENTRY_BYTES;
    std::vector<uint8_t> respMsg(hdrSize + fullLen, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<pldm_msg*>(respMsg.data());

    struct pldm_rde_rde_operation_enumerate_resp resp = {};
    resp.completion_code = PLDM_SUCCESS;
    resp.operation_count = 2;
    size_t encLen = fullLen;
    ASSERT_EQ(encode_pldm_rde_rde_operation_enumerate_resp(
                  0, &resp, entries.data(), msg, &encLen),
              0);

    // operation_count is 2 in the buffer, but only one entry's worth of
    // trailing bytes is presented.
    const size_t shortLen = PLDM_RDE_OPERATION_ENUMERATE_RESP_FIXED_BYTES +
                            PLDM_RDE_OPERATION_ENUMERATE_ENTRY_BYTES;
    struct pldm_rde_rde_operation_enumerate_resp decoded = {};
    struct pldm_rde_operation_enumerate_iter iter = {};
    EXPECT_EQ(decode_pldm_rde_rde_operation_enumerate_resp(msg, shortLen,
                                                           &decoded, &iter),
              -EBADMSG);
}
#endif
