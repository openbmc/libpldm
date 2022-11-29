#include <stdint.h>
#include <stdlib.h>

#include <cstring>
#include <iostream>
#include <vector>

#include "libpldm/utils.h"
#include "requester/rde_requester.h"

#include <gtest/gtest.h>

struct requester_base_context* testContext;
uint8_t* testEid;

void setupContext()
{
    testContext = (struct requester_base_context*)malloc(
        sizeof(struct requester_base_context));
    testEid = (uint8_t*)malloc(sizeof(uint8_t));
    *testEid = (uint8_t)1;
    pldm_base_start_discovery(testContext, testEid);
}

TEST(ContextOnInitialization, StartBaseDiscovery)
{
    setupContext();

    EXPECT_EQ(*testEid, testContext->eid);
    EXPECT_EQ(true, testContext->initialized);
}

TEST(RequestBufferOnInitializationTest, StartBaseDiscovery)
{
    setupContext();
    int bufferLength = pldm_get_request_queue_size();
    int totalBaseCommands = 4;
    EXPECT_EQ(totalBaseCommands, bufferLength);
}

TEST(GetNextCommandTest, GetNextCommand)
{
    setupContext();
    struct rde_pldm_request* request =
        (struct rde_pldm_request*)malloc(sizeof(struct rde_pldm_request));

    pldm_base_get_next_request(testContext, &request);
    EXPECT_EQ(PLDM_GET_TID, request->pldmCommand);

    pldm_base_get_next_request(testContext, &request);
    EXPECT_EQ(PLDM_GET_PLDM_VERSION, request->pldmCommand);

    pldm_base_get_next_request(testContext, &request);
    EXPECT_EQ(PLDM_GET_PLDM_COMMANDS, request->pldmCommand);

    pldm_base_get_next_request(testContext, &request);
    EXPECT_EQ(PLDM_GET_PLDM_TYPES, request->pldmCommand);

    // Buffer gets empty after all 4 commands
    int bufferLength = pldm_get_request_queue_size();
    EXPECT_EQ(0, bufferLength);
}

// TODO: Write test cases to add new commands in buffer and edge cases of buffer