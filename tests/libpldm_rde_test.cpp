#include <endian.h>

#include "libpldm/pldm_rde.h"

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(NegotiateRedfishParametersTest, EncodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint8_t mcConcurrencySupport = 13;
    bitfield16_t mcFeatureSupport = {.value = 0x7389};

    std::array<uint8_t,
               sizeof(struct pldm_msg_hdr) +
                   sizeof(struct pldm_rde_negotiate_redfish_parameters_req)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto req_payload =
        reinterpret_cast<pldm_rde_negotiate_redfish_parameters_req*>(
            request->payload);

    EXPECT_EQ(encode_negotiate_redfish_parameters_req(
                  instanceId, mcConcurrencySupport, &mcFeatureSupport, request),
              PLDM_SUCCESS);

    EXPECT_EQ(request->hdr.instance_id, instanceId);
    EXPECT_EQ(request->hdr.type, PLDM_RDE);
    EXPECT_EQ(request->hdr.request, 1);
    EXPECT_EQ(request->hdr.command, PLDM_NEGOTIATE_REDFISH_PARAMETERS);
    EXPECT_EQ(req_payload->mc_concurrency_support, mcConcurrencySupport);
    EXPECT_EQ(le16toh(req_payload->mc_feature_support.value),
              mcFeatureSupport.value);
}

TEST(NegotiateRedfishParametersTest, DecodeRequestSuccess)
{
    uint8_t mcConcurrencySupport = 1;
    bitfield16_t mcFeatureSupport = {.value = 0x7389};

    std::array<uint8_t,
               sizeof(struct pldm_msg_hdr) +
                   sizeof(struct pldm_rde_negotiate_redfish_parameters_req)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto req_payload =
        reinterpret_cast<pldm_rde_negotiate_redfish_parameters_req*>(
            request->payload);

    req_payload->mc_concurrency_support = mcConcurrencySupport;
    req_payload->mc_feature_support.value = htole16(mcFeatureSupport.value);

    uint8_t decodedMcConcurrencySupport;
    bitfield16_t decodedMcFeatureSupport;
    EXPECT_EQ(decode_negotiate_redfish_parameters_req(
                  request,
                  sizeof(struct pldm_rde_negotiate_redfish_parameters_req),
                  &decodedMcConcurrencySupport, &decodedMcFeatureSupport),
              PLDM_SUCCESS);

    EXPECT_EQ(decodedMcConcurrencySupport, mcConcurrencySupport);
    EXPECT_EQ(decodedMcFeatureSupport.value, mcFeatureSupport.value);
}

TEST(NegotiateRedfishParametersTest, EncodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;

    uint8_t deviceConcurrencySupport = 1;
    bitfield8_t deviceCapabilitiesFlags = {.byte = 0x3F};
    bitfield16_t deviceFeatureSupport = {.value = 0x7389};
    uint32_t deviceConfigurationSignature = 0xABCDEF12;
    constexpr const char* device = "This is a test";

    // Already has the space for the null character in sizeof(struct
    // pldm_rde_negotiate_redfish_parameters_resp).
    std::array<uint8_t,
               sizeof(struct pldm_msg_hdr) +
                   sizeof(struct pldm_rde_negotiate_redfish_parameters_resp) +
                   14>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    EXPECT_EQ(encode_negotiate_redfish_parameters_resp(
                  instanceId, completionCode, deviceConcurrencySupport,
                  deviceCapabilitiesFlags, deviceFeatureSupport,
                  deviceConfigurationSignature, device,
                  PLDM_RDE_VARSTRING_ASCII, response),
              PLDM_SUCCESS);

    // verify header.
    EXPECT_EQ(response->hdr.instance_id, instanceId);
    EXPECT_EQ(response->hdr.request, 0);
    EXPECT_EQ(response->hdr.type, PLDM_RDE);
    EXPECT_EQ(response->hdr.command, PLDM_NEGOTIATE_REDFISH_PARAMETERS);

    // verify payload.
    auto resp_payload =
        reinterpret_cast<pldm_rde_negotiate_redfish_parameters_resp*>(
            response->payload);
    EXPECT_EQ(resp_payload->completion_code, completionCode);
    EXPECT_EQ(resp_payload->device_concurrency_support,
              deviceConcurrencySupport);
    EXPECT_EQ(resp_payload->device_capabilities_flags.byte,
              deviceCapabilitiesFlags.byte);
    EXPECT_EQ(le16toh(resp_payload->device_feature_support.value),
              deviceFeatureSupport.value);
    EXPECT_EQ(le32toh(resp_payload->device_configuration_signature),
              deviceConfigurationSignature);
    EXPECT_EQ(resp_payload->device_provider_name.string_format,
              PLDM_RDE_VARSTRING_ASCII);
    EXPECT_EQ(resp_payload->device_provider_name.string_length_bytes,
              strlen(device) + 1);
    EXPECT_EQ(memcmp(resp_payload->device_provider_name.string_data, device,
                     strlen(device) + 1),
              0);
}
