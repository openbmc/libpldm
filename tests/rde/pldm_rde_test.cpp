#include <libpldm/rde/pldm_rde.h>

#include <memory>
#include <vector>

#include <gtest/gtest.h>

static constexpr uint8_t TEST_INSTANCE_ID = 1;

TEST(RdeNegotiateParams, EncodeSuccess)
{
    std::vector<uint8_t> requestMsg(
        sizeof(pldm_msg_hdr) +
        sizeof(struct pldm_rde_negotiate_redfish_parameters_req));
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    uint8_t concurrencySupport = 1;
    bitfield16_t featureSupport;
    featureSupport.value = 0x01;

    int rc = encode_negotiate_redfish_parameters_req(
        TEST_INSTANCE_ID, concurrencySupport, &featureSupport, request);
    EXPECT_EQ(PLDM_SUCCESS, rc);
}

TEST(RDENegotiateParams, DecodeSuccess)
{
    size_t payloadLengthSize =
        sizeof(struct pldm_rde_negotiate_redfish_parameters_req);
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) + payloadLengthSize);
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    uint8_t concurrencySupport = 1;
    bitfield16_t featureSupport;
    featureSupport.value = 0x01;

    int rc = encode_negotiate_redfish_parameters_req(
        TEST_INSTANCE_ID, concurrencySupport, &featureSupport, request);

    uint8_t concurrencySupportResponse;
    bitfield16_t featureSupportResponse;

    rc = decode_negotiate_redfish_parameters_req(request, payloadLengthSize,
                                                 &concurrencySupportResponse,
                                                 &featureSupportResponse);

    EXPECT_EQ(PLDM_SUCCESS, rc);
    EXPECT_EQ(concurrencySupport, concurrencySupportResponse);
    EXPECT_EQ(featureSupport.value, featureSupportResponse.value);
}
