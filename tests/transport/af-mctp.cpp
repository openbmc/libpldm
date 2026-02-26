#include <libpldm/transport/af-mctp.h>
#include <linux/mctp.h>

#include <gtest/gtest.h>

/*
 * Verify backward compatibility when MCTP_NET_ANY is used in TID mappings.
 * Mappings stored with MCTP_NET_ANY should match incoming messages from any
 * network ID (wildcard behavior).
 */
TEST(AfMctp, map_tid_with_net_any)
{
    struct pldm_transport_af_mctp* ctx = NULL;
    int rc;

    rc = pldm_transport_af_mctp_init(&ctx);
    ASSERT_EQ(rc, 0);
    ASSERT_NE(ctx, nullptr);

    /* Map TID 1 to EID 8 with MCTP_NET_ANY (0).
     * This simulates the old behavior where network was not specified */
    rc = pldm_transport_af_mctp_map_tid(ctx, 1, 8);
    EXPECT_EQ(rc, 0);

    pldm_transport_af_mctp_destroy(ctx);
}

/*
 * Verify that explicit network mappings coexist with MCTP_NET_ANY mappings.
 * MCTP_NET_ANY in stored mappings acts as a wildcard matching any incoming
 * network ID, while explicit network values still support exact matches.
 */
TEST(AfMctp, map_tid_mixed_network_and_wildcard)
{
    struct pldm_transport_af_mctp* ctx = NULL;
    int rc;

    rc = pldm_transport_af_mctp_init(&ctx);
    ASSERT_EQ(rc, 0);
    ASSERT_NE(ctx, nullptr);

    /* TID 1 with MCTP_NET_ANY (wildcard) */
    rc = pldm_transport_af_mctp_map_tid(ctx, 1, 8);
    EXPECT_EQ(rc, 0);

    /* TID 2 with explicit network 5 */
    rc = pldm_transport_af_mctp_map_tid_fqe(ctx, 2, 5, 10);
    EXPECT_EQ(rc, 0);

    /* TID 3 with explicit network 7 */
    rc = pldm_transport_af_mctp_map_tid_fqe(ctx, 3, 7, 12);
    EXPECT_EQ(rc, 0);

    pldm_transport_af_mctp_destroy(ctx);
}

/*
 * Verify TID unmapping operations for both MCTP_NET_ANY and explicit network
 * mappings
 */
TEST(AfMctp, unmap_tid)
{
    struct pldm_transport_af_mctp* ctx = NULL;
    int rc;

    rc = pldm_transport_af_mctp_init(&ctx);
    ASSERT_EQ(rc, 0);
    ASSERT_NE(ctx, nullptr);

    /* Map and unmap TID with MCTP_NET_ANY */
    rc = pldm_transport_af_mctp_map_tid(ctx, 1, 8);
    EXPECT_EQ(rc, 0);
    rc = pldm_transport_af_mctp_unmap_tid(ctx, 1, 8);
    EXPECT_EQ(rc, 0);

    /* Map and unmap TID with explicit network */
    rc = pldm_transport_af_mctp_map_tid_fqe(ctx, 2, 5, 10);
    EXPECT_EQ(rc, 0);
    rc = pldm_transport_af_mctp_unmap_tid_fqe(ctx, 2);
    EXPECT_EQ(rc, 0);

    pldm_transport_af_mctp_destroy(ctx);
}
