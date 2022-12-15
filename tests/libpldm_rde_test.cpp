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
