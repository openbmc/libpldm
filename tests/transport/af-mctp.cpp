#include <libpldm/transport/af-mctp.h>

#include "transport/af-mctp-test.h"

#include <gtest/gtest.h>

TEST(AfMctpTidLookup, any_network_match)
{
    struct pldm_transport_af_mctp* ctx = nullptr;
    constexpr uint32_t network = 7;
    constexpr pldm_tid_t tid = 1;
    constexpr mctp_eid_t eid = 8;
    pldm_tid_t found = 0;

    ASSERT_EQ(pldm_transport_af_mctp_test_init(&ctx), 0);
    ASSERT_EQ(pldm_transport_af_mctp_map_tid(ctx, tid, eid), 0);

    EXPECT_EQ(pldm_transport_af_mctp_test_lookup_tid(ctx, network, eid, &found),
              0);
    EXPECT_EQ(found, tid);

    pldm_transport_af_mctp_test_destroy(ctx);
}

TEST(AfMctpTidLookup, network_specific_match)
{
    struct pldm_transport_af_mctp* ctx = nullptr;
    constexpr uint32_t network3 = 13;
    constexpr uint32_t network1 = 5;
    constexpr uint32_t network2 = 9;
    constexpr mctp_eid_t eid = 0x12;
    constexpr pldm_tid_t tid1 = 1;
    constexpr pldm_tid_t tid2 = 2;
    pldm_tid_t found1 = 0;
    pldm_tid_t found2 = 0;
    pldm_tid_t found3 = 0;

    ASSERT_EQ(pldm_transport_af_mctp_test_init(&ctx), 0);
    ASSERT_EQ(pldm_transport_af_mctp_map_tid_fqe(ctx, tid1, network1, eid), 0);
    ASSERT_EQ(pldm_transport_af_mctp_map_tid_fqe(ctx, tid2, network2, eid), 0);

    EXPECT_EQ(
        pldm_transport_af_mctp_test_lookup_tid(ctx, network1, eid, &found1), 0);
    EXPECT_EQ(found1, tid1);

    EXPECT_EQ(
        pldm_transport_af_mctp_test_lookup_tid(ctx, network2, eid, &found2), 0);
    EXPECT_EQ(found2, tid2);

    EXPECT_NE(
        pldm_transport_af_mctp_test_lookup_tid(ctx, network3, eid, &found3), 0);

    pldm_transport_af_mctp_test_destroy(ctx);
}
