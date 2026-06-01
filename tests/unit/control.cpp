#include <libpldm/base.h>
#include <libpldm/control.h>
#include <libpldm/pldm.h>

#include <cerrno>
#include <cstdint>

#include "control-internal.h"

#include <gtest/gtest.h>

#if HAVE_LIBPLDM_API_TESTING

static constexpr uint8_t PLDM_TYPE_TEST = 0x02;

static const uint32_t TEST_VERSIONS[2] = {0xf1f1f000, 0x539dbeba};
static const bitfield8_t TEST_COMMANDS[32] = {};

class ControlTest : public ::testing::Test
{
  protected:
    struct pldm_control control = {};

    void SetUp() override
    {
        ASSERT_EQ(pldm_control_setup(&control, sizeof(control)), 0);
    }

    /* Send a NegotiateTransferParameters request. Returns the rc from
     * pldm_control_handle_msg and fills resp_msg/resp_len. */
    int negotiate(uint16_t part_size, uint8_t support_byte0,
                  pldm_msg* resp_msg, size_t resp_msg_size, size_t& resp_len)
    {
        PLDM_MSG_DEFINE_P(req,
                          PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES);

        struct pldm_base_negotiate_transfer_params_req req_data = {};
        req_data.requester_part_size = part_size;
        req_data.requester_protocol_support[0].byte = support_byte0;

        size_t payload_len = PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES;
        if (encode_pldm_base_negotiate_transfer_params_req(0, &req_data, req,
                                                           &payload_len))
        {
            return -EINVAL;
        }

        resp_len = resp_msg_size;
        return pldm_control_handle_msg(&control, req_buf, sizeof(req_buf),
                                       resp_msg, &resp_len);
    }

    /* Convenience: negotiate and decode a success response. */
    int negotiate_decode(uint16_t part_size, uint8_t support_byte0,
                         struct pldm_base_negotiate_transfer_params_resp& resp)
    {
        PLDM_MSG_DEFINE_P(resp_msg,
                          PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES);
        size_t resp_len = 0;
        int rc = negotiate(part_size, support_byte0, resp_msg,
                           sizeof(resp_msg_buf), resp_len);
        if (rc)
        {
            return rc;
        }
        return decode_pldm_base_negotiate_transfer_params_resp(
            resp_msg, resp_len - sizeof(pldm_msg_hdr), &resp);
    }
};

#endif

/* --- pldm_control_set_multipart_size ------------------------------------ */

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, SetMultipartSizeValid)
{
    EXPECT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 256), 0);
    EXPECT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 4096), 0);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, SetMultipartSizeBelowMinimum)
{
    EXPECT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 255),
              -EINVAL);
    EXPECT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 1), -EINVAL);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, SetMultipartSizeZeroDisables)
{
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 256), 0);
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 0), 0);

    /* With max=0, PLDM_BASE must not appear in the negotiation bitmask. */
    struct pldm_base_negotiate_transfer_params_resp resp = {};
    ASSERT_EQ(negotiate_decode(256, 0x01 /* PLDM_BASE */, resp), 0);
    EXPECT_EQ(resp.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(resp.responder_protocol_support[0].byte & 0x01, 0);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, SetMultipartSizeUnregisteredType)
{
    EXPECT_EQ(pldm_control_set_multipart_size(&control, PLDM_TYPE_TEST, 256),
              -ENOENT);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, SetMultipartSizeResetsNegotiatedState)
{
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 256), 0);

    struct pldm_base_negotiate_transfer_params_resp resp = {};
    ASSERT_EQ(negotiate_decode(256, 0x01, resp), 0);
    ASSERT_EQ(resp.completion_code, PLDM_SUCCESS);

    /* Re-configuring must clear the stored negotiated size. */
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 512), 0);

    uint16_t negotiated = 0;
    EXPECT_EQ(pldm_control_get_multipart_size(&control, PLDM_BASE, &negotiated),
              -ENODATA);
}
#endif

/* --- pldm_control_get_multipart_size ------------------------------------ */

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, GetMultipartSizeNotNegotiated)
{
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 256), 0);
    uint16_t negotiated = 0;
    EXPECT_EQ(pldm_control_get_multipart_size(&control, PLDM_BASE, &negotiated),
              -ENODATA);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, GetMultipartSizeUnregisteredType)
{
    uint16_t negotiated = 0;
    EXPECT_EQ(
        pldm_control_get_multipart_size(&control, PLDM_TYPE_TEST, &negotiated),
        -ENOENT);
}
#endif

/* --- NegotiateTransferParameters via pldm_control_handle_msg ------------ */

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, NegotiateRejectsPartSizeBelowMinimum)
{
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 256), 0);

    PLDM_MSG_DEFINE_P(resp_msg,
                      PLDM_BASE_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES);
    size_t resp_len = 0;
    ASSERT_EQ(negotiate(255, 0x01, resp_msg, sizeof(resp_msg_buf), resp_len),
              0);
    EXPECT_EQ(resp_msg->payload[0], PLDM_ERROR_INVALID_DATA);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, NegotiateClampsToResponderMax)
{
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 512), 0);

    struct pldm_base_negotiate_transfer_params_resp resp = {};
    ASSERT_EQ(negotiate_decode(4096, 0x01, resp), 0);
    EXPECT_EQ(resp.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(resp.responder_part_size, 512);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, NegotiateClampsToRequesterSize)
{
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 4096), 0);

    struct pldm_base_negotiate_transfer_params_resp resp = {};
    ASSERT_EQ(negotiate_decode(256, 0x01, resp), 0);
    EXPECT_EQ(resp.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(resp.responder_part_size, 256);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, NegotiateNoMatchingTypes)
{
    /* No types have multipart enabled; bitmask intersection must be empty. */
    struct pldm_base_negotiate_transfer_params_resp resp = {};
    ASSERT_EQ(negotiate_decode(256, 0x01, resp), 0);
    EXPECT_EQ(resp.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(resp.responder_part_size, 256);
    for (const auto& b : resp.responder_protocol_support)
    {
        EXPECT_EQ(b.byte, 0);
    }

    uint16_t negotiated = 0;
    EXPECT_EQ(pldm_control_get_multipart_size(&control, PLDM_BASE, &negotiated),
              -ENODATA);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, NegotiateZeroMaxExcludesType)
{
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 256), 0);
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 0), 0);

    struct pldm_base_negotiate_transfer_params_resp resp = {};
    ASSERT_EQ(negotiate_decode(256, 0x01, resp), 0);
    EXPECT_EQ(resp.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(resp.responder_protocol_support[0].byte & 0x01, 0);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, NegotiateMultipleTypesMinChosen)
{
    ASSERT_EQ(pldm_control_add_type(&control, PLDM_TYPE_TEST, TEST_VERSIONS, 2,
                                    TEST_COMMANDS),
              0);
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 512), 0);
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_TYPE_TEST, 1024),
              0);

    /* Request both types; expect minimum of the two maxima. */
    struct pldm_base_negotiate_transfer_params_resp resp = {};
    /* byte0: bit0=PLDM_BASE, bit2=PLDM_TYPE_TEST */
    ASSERT_EQ(negotiate_decode(2048, 0x05, resp), 0);
    EXPECT_EQ(resp.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(resp.responder_part_size, 512);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, NegotiateStateUpdatedOnSuccess)
{
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 1024), 0);

    struct pldm_base_negotiate_transfer_params_resp resp = {};
    ASSERT_EQ(negotiate_decode(512, 0x01, resp), 0);
    ASSERT_EQ(resp.completion_code, PLDM_SUCCESS);
    ASSERT_EQ(resp.responder_part_size, 512);

    uint16_t negotiated = 0;
    ASSERT_EQ(pldm_control_get_multipart_size(&control, PLDM_BASE, &negotiated),
              0);
    EXPECT_EQ(negotiated, 512);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, NegotiateSecondCallCeiling)
{
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 4096), 0);

    struct pldm_base_negotiate_transfer_params_resp resp = {};
    ASSERT_EQ(negotiate_decode(512, 0x01, resp), 0);
    ASSERT_EQ(resp.responder_part_size, 512);

    /* Second call with a larger request must still return 512. */
    ASSERT_EQ(negotiate_decode(4096, 0x01, resp), 0);
    EXPECT_EQ(resp.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(resp.responder_part_size, 512);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST_F(ControlTest, NegotiateSecondCallCanGoLower)
{
    ASSERT_EQ(pldm_control_set_multipart_size(&control, PLDM_BASE, 4096), 0);

    struct pldm_base_negotiate_transfer_params_resp resp = {};
    ASSERT_EQ(negotiate_decode(1024, 0x01, resp), 0);
    ASSERT_EQ(resp.responder_part_size, 1024);

    /* Second call with a smaller request must return the smaller value. */
    ASSERT_EQ(negotiate_decode(256, 0x01, resp), 0);
    EXPECT_EQ(resp.completion_code, PLDM_SUCCESS);
    EXPECT_EQ(resp.responder_part_size, 256);
}
#endif
