#include <endian.h>

#include "libpldm/rde.h"

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(NegotiateRedfishParametersTest, EncodeDecodeRequestSuccess)
{
    uint8_t instanceId = 11;
    uint8_t mcConcurrencySupport = 13;
    bitfield16_t mcFeatureSupport = {.value = 0x7389};

    std::array<uint8_t, sizeof(struct pldm_msg_hdr) +
                            PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_SIZE>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    EXPECT_EQ(encode_rde_negotiate_redfish_parameters_req(
                  instanceId, mcConcurrencySupport, &mcFeatureSupport,
                  PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_SIZE, request),
              PLDM_SUCCESS);

    EXPECT_EQ(request->hdr.instance_id, instanceId);
    EXPECT_EQ(request->hdr.type, PLDM_RDE);
    EXPECT_EQ(request->hdr.request, 1);
    EXPECT_EQ(request->hdr.command, PLDM_NEGOTIATE_REDFISH_PARAMETERS);

    uint8_t decodedMcConcurrencySupport;
    bitfield16_t decodedMcFeatureSupport;

    EXPECT_EQ(decode_rde_negotiate_redfish_parameters_req(
                  request, PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_REQ_SIZE,
                  &decodedMcConcurrencySupport, &decodedMcFeatureSupport),
              PLDM_SUCCESS);

    EXPECT_EQ(decodedMcConcurrencySupport, mcConcurrencySupport);
    EXPECT_EQ(decodedMcFeatureSupport.value, mcFeatureSupport.value);
}

TEST(NegotiateRedfishParametersTest, EncodeDecodeResponseSuccess)
{
    uint8_t completionCode = 0;
    uint8_t instanceId = 11;

    uint8_t deviceConcurrencySupport = 1;
    bitfield8_t deviceCapabilitiesFlags = {.byte = 0x3F};
    bitfield16_t deviceFeatureSupport = {.value = 0x7389};
    uint32_t deviceConfigurationSignature = 0xABCDEF12;
    constexpr const char* device = "This is a test";

    constexpr size_t payloadLength =
        PLDM_RDE_NEGOTIATE_REDFISH_PARAMETERS_RESP_MIN_SIZE + 14;

    // Already has the space for the null character in sizeof(struct
    // pldm_rde_negotiate_redfish_parameters_resp).
    std::array<uint8_t, sizeof(struct pldm_msg_hdr) + payloadLength>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    EXPECT_EQ(encode_negotiate_redfish_parameters_resp(
                  instanceId, completionCode, deviceConcurrencySupport,
                  &deviceCapabilitiesFlags, &deviceFeatureSupport,
                  deviceConfigurationSignature, device,
                  PLDM_RDE_VARSTRING_ASCII, payloadLength, response),
              PLDM_SUCCESS);

    // verify header.
    EXPECT_EQ(response->hdr.instance_id, instanceId);
    EXPECT_EQ(response->hdr.request, 0);
    EXPECT_EQ(response->hdr.type, PLDM_RDE);
    EXPECT_EQ(response->hdr.command, PLDM_NEGOTIATE_REDFISH_PARAMETERS);

    // verify payload.
    uint8_t decodedCompletionCode;
    uint8_t decodedDeviceConcurrencySupport;
    bitfield8_t decodedDeviceCapabilitiesFlags;
    bitfield16_t decodedDeviceFeatureSupport;
    uint32_t decodedDeviceConfigurationSignature;
    struct pldm_rde_varstring decodedProviderName;

    EXPECT_EQ(decode_negotiate_redfish_parameters_resp(
                  response, payloadLength, &decodedCompletionCode,
                  &decodedDeviceConcurrencySupport,
                  &decodedDeviceCapabilitiesFlags, &decodedDeviceFeatureSupport,
                  &decodedDeviceConfigurationSignature, &decodedProviderName),
              PLDM_SUCCESS);

    EXPECT_EQ(decodedCompletionCode, completionCode);
    EXPECT_EQ(decodedDeviceConcurrencySupport, deviceConcurrencySupport);
    EXPECT_EQ(decodedDeviceCapabilitiesFlags.byte,
              deviceCapabilitiesFlags.byte);
    EXPECT_EQ(decodedDeviceFeatureSupport.value, deviceFeatureSupport.value);
    EXPECT_EQ(decodedDeviceConfigurationSignature,
              deviceConfigurationSignature);
    EXPECT_EQ(decodedProviderName.string_format, PLDM_RDE_VARSTRING_ASCII);
    EXPECT_EQ(decodedProviderName.string_length_bytes, strlen(device) + 1);
    EXPECT_EQ(strncmp(device, decodedProviderName.string_data, strlen(device)),
              0);
}
