#include <libpldm/transport/af-mctp.h>

#include "transport/af-mctp-internal.h"

#include <gtest/gtest.h>

/* map_tid() with MCTP_NET_ANY matches any network during lookup */
#if LIBPLDM_API_TESTING
TEST(AfMctpTidLookup, any_network_match)
{
    struct pldm_transport_af_mctp ctx{};
    constexpr uint32_t network = 7;
    constexpr pldm_tid_t tid = 1;
    constexpr mctp_eid_t eid = 8;
    constexpr mctp_eid_t unmappedEid = 42;
    pldm_tid_t found1 = 0;
    pldm_tid_t found2 = 0;

    ASSERT_EQ(pldm_transport_af_mctp_map_tid(&ctx, tid, eid), 0);

    /* Lookup with arbitrary network succeeds against MCTP_NET_ANY entry */
    EXPECT_EQ(pldm_transport_af_mctp_get_tid(&ctx, network, eid, &found1), 0);
    EXPECT_EQ(found1, tid);

    /* Same network but different EID must not match */
    EXPECT_NE(
        pldm_transport_af_mctp_get_tid(&ctx, network, unmappedEid, &found2), 0);
}
#endif

/* map_tid_fqe() entries only match their exact network ID */
#if LIBPLDM_API_TESTING
TEST(AfMctpTidLookup, network_specific_match)
{
    struct pldm_transport_af_mctp ctx{};
    constexpr uint32_t network3 = 13;
    constexpr uint32_t network1 = 5;
    constexpr uint32_t network2 = 9;
    constexpr mctp_eid_t eid = 0x12;
    constexpr pldm_tid_t tid1 = 1;
    constexpr pldm_tid_t tid2 = 2;
    pldm_tid_t found1 = 0;
    pldm_tid_t found2 = 0;
    pldm_tid_t found3 = 0;

    ASSERT_EQ(pldm_transport_af_mctp_map_tid_fqe(&ctx, tid1, network1, eid), 0);
    ASSERT_EQ(pldm_transport_af_mctp_map_tid_fqe(&ctx, tid2, network2, eid), 0);

    /* Each network resolves to its own TID */
    EXPECT_EQ(pldm_transport_af_mctp_get_tid(&ctx, network1, eid, &found1), 0);
    EXPECT_EQ(found1, tid1);

    EXPECT_EQ(pldm_transport_af_mctp_get_tid(&ctx, network2, eid, &found2), 0);
    EXPECT_EQ(found2, tid2);

    /* Unmapped network must fail */
    EXPECT_NE(pldm_transport_af_mctp_get_tid(&ctx, network3, eid, &found3), 0);
}
#endif

/* When both APIs coexist, exact network takes precedence over MCTP_NET_ANY */
#if LIBPLDM_API_TESTING
TEST(AfMctpTidLookup, exact_network_preferred_over_any_net)
{
    struct pldm_transport_af_mctp ctx{};
    constexpr uint32_t network = 7;
    constexpr uint32_t otherNetwork = 99;
    constexpr mctp_eid_t eid = 8;
    constexpr mctp_eid_t unmappedEid = 42;
    constexpr pldm_tid_t anyNetTid = 1;
    constexpr pldm_tid_t exactTid = 5;
    pldm_tid_t found1 = 0;
    pldm_tid_t found2 = 0;
    pldm_tid_t found3 = 0;

    /* MCTP_NET_ANY mapping has lower TID index, so it is scanned first */
    ASSERT_EQ(pldm_transport_af_mctp_map_tid(&ctx, anyNetTid, eid), 0);
    ASSERT_EQ(pldm_transport_af_mctp_map_tid_fqe(&ctx, exactTid, network, eid),
              0);

    /* Exact network match must take precedence over MCTP_NET_ANY */
    EXPECT_EQ(pldm_transport_af_mctp_get_tid(&ctx, network, eid, &found1), 0);
    EXPECT_EQ(found1, exactTid);

    /* Non-matching network still resolves via the MCTP_NET_ANY entry */
    EXPECT_EQ(pldm_transport_af_mctp_get_tid(&ctx, otherNetwork, eid, &found2),
              0);
    EXPECT_EQ(found2, anyNetTid);

    /* Correct network but wrong EID must not match any entry */
    EXPECT_NE(
        pldm_transport_af_mctp_get_tid(&ctx, network, unmappedEid, &found3), 0);
}
#endif
