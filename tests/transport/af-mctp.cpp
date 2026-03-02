#include <libpldm/transport/af-mctp.h>

#include <gtest/gtest.h>

TEST(AfMctpTidLookup, MatchesAnyNetworkWhenMappedWithWildcard)
{
    constexpr pldm_tid_t tid = 1;
    constexpr mctp_eid_t eid = 8;
    constexpr uint32_t network = 7;
    struct pldm_transport_af_mctp* ctx = nullptr;
    pldm_tid_t found = 0;

    ASSERT_EQ(pldm_transport_af_mctp_test_init(&ctx), 0);
    ASSERT_EQ(pldm_transport_af_mctp_map_tid(ctx, tid, eid), 0);

    EXPECT_EQ(pldm_transport_af_mctp_test_lookup_tid(ctx, network, eid, &found),
              0);
    EXPECT_EQ(found, tid);

    pldm_transport_af_mctp_test_destroy(ctx);
}

TEST(AfMctpTidLookup, DistinguishesSameEidOnDifferentNetworks)
{
    constexpr pldm_tid_t tid1 = 1;
    constexpr pldm_tid_t tid2 = 2;
    constexpr mctp_eid_t eid = 0x12;
    constexpr uint32_t network1 = 5;
    constexpr uint32_t network2 = 9;
    struct pldm_transport_af_mctp* ctx = nullptr;
    pldm_tid_t found = 0;

    ASSERT_EQ(pldm_transport_af_mctp_test_init(&ctx), 0);
    ASSERT_EQ(pldm_transport_af_mctp_map_tid_fqe(ctx, tid1, network1, eid), 0);
    ASSERT_EQ(pldm_transport_af_mctp_map_tid_fqe(ctx, tid2, network2, eid), 0);

    EXPECT_EQ(
        pldm_transport_af_mctp_test_lookup_tid(ctx, network1, eid, &found), 0);
    EXPECT_EQ(found, tid1);

    found = 0;
    EXPECT_EQ(
        pldm_transport_af_mctp_test_lookup_tid(ctx, network2, eid, &found), 0);
    EXPECT_EQ(found, tid2);

    pldm_transport_af_mctp_test_destroy(ctx);
}
