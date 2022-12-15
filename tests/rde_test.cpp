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
