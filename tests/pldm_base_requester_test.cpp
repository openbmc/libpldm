#include <stdint.h>
#include <stdlib.h>

#include <cstring>
#include <iostream>
#include <vector>

#include "libpldm/utils.h"
#include "requester/pldm_base_requester.h"

#include <gtest/gtest.h>
#define IGNORE(x) (void)(x)

struct requester_base_context* testContext;
uint8_t testInstanceId = 0x01;

void setupContext()
{
    testContext = (struct requester_base_context*)malloc(
        sizeof(struct requester_base_context));
    pldm_base_init_context(testContext);
    pldm_base_start_discovery(testContext);
}

TEST(ContextInitializationSuccess, ContextInitialization)
{
    struct requester_base_context* testContext =
        (struct requester_base_context*)malloc(
            sizeof(struct requester_base_context));
    pldm_base_init_context(testContext);
    EXPECT_EQ(true, testContext->initialized);
    EXPECT_EQ(PLDM_BASE_REQUESTER_NO_PENDING_ACTION, testContext->requester_status);
}

TEST(ContextInitializationFailure, ContextInitialization)
{
    struct requester_base_context* testContext =
        (struct requester_base_context*)malloc(
            sizeof(struct requester_base_context));
    auto rc = pldm_base_init_context(testContext);
    EXPECT_EQ(false, testContext->initialized);
    EXPECT_EQ(PLDM_BASE_CONTEXT_INITIALIZATION_ERROR, rc);
}

TEST(StartBaseDiscoveryFailure, StartBaseDiscovery)
{
    struct requester_base_context* testContext =
        (struct requester_base_context*)malloc(
            sizeof(struct requester_base_context));
    pldm_base_init_context(testContext);
    testContext->requester_status = PLDM_BASE_REQUESTER_WAITING_FOR_RESPONSE;
    auto rc = pldm_base_start_discovery(testContext);
    EXPECT_EQ(PLDM_BASE_CONTEXT_NOT_READY, rc);
}

TEST(StartBaseDiscoverySuccess, StartBaseDiscovery)
{
    setupContext();
    EXPECT_EQ(true, testContext->initialized);
    EXPECT_EQ(PLDM_GET_TID, testContext->next_command);
    EXPECT_EQ(PLDM_BASE_REQUESTER_READY_TO_PICK_NEXT_REQUEST,
              testContext->requester_status);
}

TEST(GetNextCommandTIDTest, GetNextCommand)
{
    setupContext();

    // Create TID Request
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr));
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = pldm_base_get_next_request(testContext, testInstanceId, request);
    EXPECT_EQ(PLDM_BASE_REQUESTER_SUCCESS, rc);

    std::cerr << "TEST: " << (unsigned)requestMsg[0];
    // PLDM Header first byte is 100000001 (0x81 in hex)
    // (Most sig bit = 1 (Request Type), Least sig bit =1 (Instance Id))
    EXPECT_EQ(0x81, requestMsg[0]);
    EXPECT_EQ(PLDM_BASE, requestMsg[1]);
    EXPECT_EQ(PLDM_GET_TID, requestMsg[2]);
}

TEST(GetNextCommandPLDMTypesTest, GetNextCommand)
{
    setupContext();
    testContext->next_command = PLDM_GET_PLDM_TYPES;

    // Create PLDM Type Request
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr));
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = pldm_base_get_next_request(testContext, testInstanceId, request);
    EXPECT_EQ(PLDM_BASE_REQUESTER_SUCCESS, rc);

    // PLDM Header first byte is 100000001 (0x81 in hex)
    // (Most sig bit = 1 (Request Type), Least sig bit =1 (Instance Id))
    EXPECT_EQ(0x81, requestMsg[0]);
    EXPECT_EQ(PLDM_BASE, requestMsg[1]);
    EXPECT_EQ(PLDM_GET_PLDM_TYPES, requestMsg[2]);
}

TEST(GetNextCommandPLDMVersionTest, GetNextCommand)
{
    setupContext();
    testContext->next_command = PLDM_GET_PLDM_VERSION;

    // Create PLDM Version Request
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) +
                                    PLDM_GET_VERSION_REQ_BYTES);
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = pldm_base_get_next_request(testContext, testInstanceId, request);
    EXPECT_EQ(PLDM_BASE_REQUESTER_SUCCESS, rc);

    // PLDM Header first byte is 100000001 (0x81 in hex)
    // (Most sig bit = 1 (Request Type), Least sig bit =1 (Instance Id))
    EXPECT_EQ(0x81, requestMsg[0]);
    EXPECT_EQ(PLDM_BASE, requestMsg[1]);
    EXPECT_EQ(PLDM_GET_PLDM_VERSION, requestMsg[2]);
}

TEST(GetNextCommandPLDMCommandsTest, GetNextCommand)
{
    setupContext();
    testContext->next_command = PLDM_GET_PLDM_COMMANDS;

    // Create PLDM Commands Request
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) +
                                    PLDM_GET_COMMANDS_REQ_BYTES);
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = pldm_base_get_next_request(testContext, testInstanceId, request);
    EXPECT_EQ(PLDM_BASE_REQUESTER_SUCCESS, rc);

    // PLDM Header first byte is 100000001 (0x81 in hex)
    // (Most sig bit = 1 (Request Type), Least sig bit =1 (Instance Id))
    EXPECT_EQ(0x81, requestMsg[0]);
    EXPECT_EQ(PLDM_BASE, requestMsg[1]);
    EXPECT_EQ(PLDM_GET_PLDM_COMMANDS, requestMsg[2]);
}

TEST(PushResponseTID, PushResponseInContext)
{
    setupContext();
    // Create Response - 0x0b as the TID returned
    std::vector<uint8_t> response = {0x01, 0x00, 0x02, 0x00, 0x0b};
    size_t responseMsgSize = sizeof(pldm_msg_hdr) + PLDM_GET_TID_RESP_BYTES;
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(response.data());

    auto rc =
        pldm_base_push_response(testContext, responsePtr, responseMsgSize);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(0x0b, testContext->tid);
}

TEST(PushResponseTypes, PushResponseInContext)
{
    setupContext();
    testContext->next_command = PLDM_GET_PLDM_TYPES;
    // Create Response
    std::vector<uint8_t> response = {0x01, 0x00, 0x04, 0x00, 0x01};
    size_t responseMsgSize = sizeof(pldm_msg_hdr) + PLDM_GET_TYPES_RESP_BYTES;
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(response.data());

    uint8_t expected_pldm_types[] = {0x01, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00};
    auto rc =
        pldm_base_push_response(testContext, responsePtr, responseMsgSize);
    EXPECT_EQ(0, rc);
    for (int i = 0; i < 8; i++)
    {
        EXPECT_EQ(expected_pldm_types[i], testContext->pldm_types[i].byte);
    }
}

TEST(PushResponseVersion, PushResponseInContext)
{
    setupContext();
    testContext->next_command = PLDM_GET_PLDM_VERSION;
    testContext->command_pldm_type = PLDM_BASE;
    // Create Response
    std::vector<uint8_t> response = {0x01, 0x00, 0x03, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x05, 0x00, 0xf0, 0xf1,
                                     0xf1, 0xba, 0xbe, 0x9d, 0x53};
    size_t responseMsgSize = sizeof(pldm_msg_hdr) + PLDM_GET_VERSION_RESP_BYTES;
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(response.data());

    auto rc =
        pldm_base_push_response(testContext, responsePtr, responseMsgSize);
    EXPECT_EQ(0, rc);
    ver32_t expectedVersion = {0x00, 0xf0, 0xf1, 0xf1};
    ver32_t versionReceived = testContext->pldm_versions[PLDM_BASE];

    EXPECT_EQ(expectedVersion.alpha, versionReceived.alpha);
    EXPECT_EQ(expectedVersion.update, versionReceived.update);
    EXPECT_EQ(expectedVersion.minor, versionReceived.minor);
    EXPECT_EQ(expectedVersion.major, versionReceived.major);
}

TEST(PushResponseCommands, PushResponseInContext)
{
    setupContext();
    testContext->next_command = PLDM_GET_PLDM_COMMANDS;
    testContext->command_pldm_type = PLDM_BASE;
    testContext->pldm_versions[PLDM_BASE] = {0x00, 0xf0, 0xf1, 0xf1};
    // Create Response
    std::vector<uint8_t> response = {0x01, 0x00, 0x05, 0x00, 0xbc};
    size_t responseMsgSize =
        sizeof(pldm_msg_hdr) + PLDM_GET_COMMANDS_RESP_BYTES;
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(response.data());

    auto rc =
        pldm_base_push_response(testContext, responsePtr, responseMsgSize);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(0xbc, testContext->pldm_commands[PLDM_BASE][0]);
}

TEST(GetNextPLDMTypeSuccess, GetNextPLDMType)
{
    setupContext();
    bitfield8_t types[8] = {{0x45}, {0x04}, {0x00}, {0x00},
                            {0x00}, {0x00}, {0x00}, {0x00}};

    for (int i = 0; i < 8; i++)
        testContext->pldm_types[i] = types[i];

    pldm_base_get_next_pldm_type(testContext, /***/ PLDM_BASE,
                                 &(testContext->command_pldm_type));
    EXPECT_EQ(PLDM_PLATFORM, testContext->command_pldm_type);

    pldm_base_get_next_pldm_type(testContext, /***/ PLDM_PLATFORM,
                                 &(testContext->command_pldm_type));
    EXPECT_EQ(/**PLDM_RDE_TYPE=*/0x06, testContext->command_pldm_type);

    pldm_base_get_next_pldm_type(testContext, /**PLDM_RDE_TYPE=*/0x06,
                                 &(testContext->command_pldm_type));
    EXPECT_EQ(/**TEST_TYPE = 0xa (or)*/ 10, testContext->command_pldm_type);
}