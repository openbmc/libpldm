#include <string.h>

#include <array>
#include <cstring>
#include <iostream>
#include <vector>

#include "libpldm/requester/pldm_base_requester.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

std::string TEST_DEVICE_ID = "DEVICE_ID";
int TEST_NET_ID = 1;
uint8_t TEST_INSTANCE_ID = 1;

std::map<uint8_t, int> command_request_size = {
    {PLDM_GET_TID, 0},
    {PLDM_GET_PLDM_TYPES, 0},
    {PLDM_GET_PLDM_VERSION, PLDM_GET_VERSION_REQ_BYTES},
    {PLDM_GET_PLDM_COMMANDS, PLDM_GET_COMMANDS_REQ_BYTES}};

int get_request_bytes(uint8_t request_type)
{
    auto it = command_request_size.find(request_type);
    if (it != command_request_size.end())
    {
        return it->second;
    }
    return -1;
}
TEST(BaseContextInitializationSuccess, PLDMBaseDiscovery)
{
    struct requester_base_context* ctx = new requester_base_context();
    int rc = pldm_base_init_context(ctx, TEST_DEVICE_ID.c_str(), TEST_NET_ID);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    EXPECT_EQ(ctx->initialized, true);
    EXPECT_EQ(ctx->requester_status, PLDM_BASE_REQUESTER_NO_PENDING_ACTION);
}

TEST(BaseContextStartDiscovery, PLDMBaseDiscovery)
{
    // Initializing context
    struct requester_base_context* ctx = new requester_base_context();
    int rc = pldm_base_init_context(ctx, TEST_DEVICE_ID.c_str(), TEST_NET_ID);

    rc = pldm_base_start_discovery(ctx);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    EXPECT_EQ(ctx->next_command, PLDM_GET_TID);
    EXPECT_EQ(ctx->requester_status,
              PLDM_BASE_REQUESTER_READY_TO_PICK_NEXT_REQUEST);
}

TEST(BaseContextStartDiscoveryFailure, PLDMBaseDiscovery)
{
    struct requester_base_context* ctx = new requester_base_context();

    int rc = pldm_base_start_discovery(ctx);
    EXPECT_EQ(rc, PLDM_BASE_CONTEXT_NOT_READY);
}

int test_get_next_request_seq(struct requester_base_context** ctx,
                              uint8_t next_command)
{
    (*ctx)->next_command = next_command;
    int requestBytes = get_request_bytes((*ctx)->next_command);
    std::vector<uint8_t> msg_tid(sizeof(pldm_msg_hdr) + requestBytes);
    auto request = reinterpret_cast<pldm_msg*>(msg_tid.data());
    return pldm_base_get_next_request(*ctx, TEST_INSTANCE_ID, request);
}

TEST(GetNextRequestInSequenceSuccess, PLDMBaseDiscovery)
{
    int rc;
    // Initializing context
    struct requester_base_context* ctx = new requester_base_context();
    rc = pldm_base_init_context(ctx, TEST_DEVICE_ID.c_str(), TEST_NET_ID);
    rc = pldm_base_start_discovery(ctx);

    rc = test_get_next_request_seq(&ctx, PLDM_GET_TID);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);

    rc = test_get_next_request_seq(&ctx, PLDM_GET_PLDM_TYPES);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);

    rc = test_get_next_request_seq(&ctx, PLDM_GET_PLDM_VERSION);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);

    rc = test_get_next_request_seq(&ctx, PLDM_GET_PLDM_COMMANDS);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);

    rc = test_get_next_request_seq(&ctx, PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND);

}

TEST(GetNextRequestInSequenceFailure, PLDMBaseDiscovery)
{
    int rc;
    // Initializing context
    struct requester_base_context* ctx = new requester_base_context();
    rc = test_get_next_request_seq(&ctx, 0x0023);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_NO_NEXT_COMMAND_FOUND);
}