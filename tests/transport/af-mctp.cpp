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

TEST(AfMctpTidLookup, PrefersExactNetworkOverWildcard)
{
    constexpr pldm_tid_t wildcardTid = 2;
    constexpr pldm_tid_t networkSpecificTid = 1;
    constexpr mctp_eid_t eid = 0x12;
    constexpr uint32_t network = 5;
    constexpr uint32_t otherNetwork = 9;
    struct pldm_transport_af_mctp* ctx = nullptr;
    pldm_tid_t found = 0;

    ASSERT_EQ(pldm_transport_af_mctp_test_init(&ctx), 0);
    ASSERT_EQ(pldm_transport_af_mctp_map_tid_fqe(ctx, networkSpecificTid,
                                                 network, eid),
              0);
    ASSERT_EQ(pldm_transport_af_mctp_map_tid(ctx, wildcardTid, eid), 0);

    EXPECT_EQ(pldm_transport_af_mctp_test_lookup_tid(ctx, network, eid, &found),
              0);
    EXPECT_EQ(found, networkSpecificTid);

    found = 0;
    EXPECT_EQ(
        pldm_transport_af_mctp_test_lookup_tid(ctx, otherNetwork, eid, &found),
        0);
    EXPECT_EQ(found, wildcardTid);

    pldm_transport_af_mctp_test_destroy(ctx);
}
