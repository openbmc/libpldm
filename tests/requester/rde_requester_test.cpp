#include <string.h>

#include <array>
#include <cstring>
#include <iostream>
#include <vector>

#include "libpldm/requester/pldm_rde_requester.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
struct pldm_rde_requester_context rde_contexts[256];
std::vector<uint32_t> resource_ids;
int rde_context_counter = 0;

uint8_t TEST_MC_CONCURRENCY;
uint32_t TEST_MC_TRANSFER_SIZE;
bitfield8_t* TEST_DEV_CAPABILITES;
bitfield16_t* TEST_MC_FEATURES;
uint8_t TEST_NUMBER_OF_RESOURCES;
std::string TEST_DEV_ID;
int TEST_NET_ID;
int TEST_INSTANCE_ID;

std::map<uint8_t, int> rde_command_request_size = {
    {PLDM_NEGOTIATE_REDFISH_PARAMETERS, 3},
    {PLDM_NEGOTIATE_MEDIUM_PARAMETERS, 4},
    {PLDM_GET_SCHEMA_DICTIONARY, 5},
    {PLDM_RDE_MULTIPART_RECEIVE, 7}};

void initial_setup()
{
    TEST_INSTANCE_ID = 1;
    TEST_MC_CONCURRENCY = uint8_t(3);
    TEST_MC_TRANSFER_SIZE = uint32_t(2056);
    TEST_MC_FEATURES = (bitfield16_t*)malloc(sizeof(bitfield16_t));
    TEST_DEV_CAPABILITES = (bitfield8_t*)malloc(sizeof(bitfield8_t));
    TEST_MC_FEATURES->value = (uint16_t)102;

    TEST_NUMBER_OF_RESOURCES = 2;
    resource_ids.emplace_back(0x00000000);
    resource_ids.emplace_back(0x00010000);

    TEST_NET_ID = 9;
    TEST_DEV_ID = "rde_dev";
}

void free_memory(void* context)
{
    free(context);
}
struct pldm_rde_requester_context*
    allocate_memory_to_contexts(uint8_t number_of_contexts)
{
    int rc;
    int end = rde_context_counter + number_of_contexts;
    for (rde_context_counter = 0; rde_context_counter < end;
         rde_context_counter++)
    {
        struct pldm_rde_requester_context* current_ctx =
            (struct pldm_rde_requester_context*)malloc(
                sizeof(struct pldm_rde_requester_context));
        IGNORE(rc);
        rc = pldm_rde_create_context(current_ctx);
        if (rc)
        {
            return NULL;
        }
        rde_contexts[rde_context_counter] = *current_ctx;
    }

    return &rde_contexts[0];
}

TEST(ContextManagerInitializationSuccess, RDERequesterTest)
{
    initial_setup();
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(
        TEST_DEV_ID.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);

    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    EXPECT_EQ(manager->mc_concurrency, TEST_MC_CONCURRENCY);
    EXPECT_EQ(manager->mc_transfer_size, TEST_MC_TRANSFER_SIZE);
    EXPECT_EQ(manager->mc_feature_support, TEST_MC_FEATURES);
    EXPECT_EQ(manager->device_name, TEST_DEV_ID);
    EXPECT_EQ(manager->net_id, TEST_NET_ID);
    EXPECT_EQ(manager->number_of_resources, TEST_NUMBER_OF_RESOURCES);
    EXPECT_EQ(manager->resource_ids[0], 0x00000000);
    EXPECT_EQ(manager->resource_ids[1], 0x00010000);
}

TEST(ContextManagerInitializationFailureDueToNullManager, RDERequesterTest)
{
    initial_setup();

    struct pldm_rde_requester_manager* manager = NULL;
    int rc = pldm_rde_init_context(
        TEST_DEV_ID.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);
}

TEST(ContextManagerInitializationFailureDueToWrongDevId, RDERequesterTest)
{
    initial_setup();
    std::string incorrect_dev_id;
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();
    int rc = pldm_rde_init_context(
        incorrect_dev_id.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);

    incorrect_dev_id = "";
    rc = pldm_rde_init_context(
        incorrect_dev_id.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);

    incorrect_dev_id = "VERY_LONG_DEV_ID";
    rc = pldm_rde_init_context(
        incorrect_dev_id.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);
}

TEST(ContextManagerInitializationFailureDueToNullAllocatorFunctions,
     RDERequesterTest)
{
    initial_setup();

    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();
    int rc = pldm_rde_init_context(TEST_DEV_ID.c_str(), TEST_NET_ID, manager,
                                   TEST_MC_CONCURRENCY, TEST_MC_TRANSFER_SIZE,
                                   TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
                                   &resource_ids.front(), NULL, free_memory);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);

    rc = pldm_rde_init_context(
        TEST_DEV_ID.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, NULL);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);
}

TEST(StartRDEDiscoverySuccess, RDEDiscoveryTest)
{
    initial_setup();
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(
        TEST_DEV_ID.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context base_context = rde_contexts[0];

    rc = pldm_rde_start_discovery(&base_context);

    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    EXPECT_EQ(base_context.next_command, PLDM_NEGOTIATE_REDFISH_PARAMETERS);
}

TEST(StartRDEDiscoveryFailure, RDEDiscoveryTest)
{
    initial_setup();
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(
        TEST_DEV_ID.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context base_context = rde_contexts[0];

    base_context.context_status = CONTEXT_BUSY;
    rc = pldm_rde_start_discovery(&base_context);
    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_NOT_READY);
}

TEST(CreateRequesterContextSuccess, RDEDiscoveryTest)
{
    initial_setup();
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(
        TEST_DEV_ID.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* current_ctx =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(current_ctx);

    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    EXPECT_EQ(current_ctx->context_status, CONTEXT_FREE);
    EXPECT_EQ(current_ctx->requester_status,
              PLDM_RDE_REQUESTER_READY_TO_PICK_NEXT_REQUEST);
    EXPECT_EQ(current_ctx->next_command,
              PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND);
}

TEST(CreateRequesterContextFailure, RDEDiscoveryTest)
{
    initial_setup();
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(
        TEST_DEV_ID.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* current_ctx = NULL;
    rc = pldm_rde_create_context(current_ctx);

    EXPECT_EQ(rc, PLDM_RDE_CONTEXT_INITIALIZATION_ERROR);
}

int test_get_next_request_seq(pldm_rde_requester_manager** manager,
                              struct pldm_rde_requester_context** ctx,
                              uint8_t next_command)
{
    (*ctx)->next_command = next_command;
    int requestBytes = 0;
    if (rde_command_request_size.find((*ctx)->next_command) !=
        rde_command_request_size.end())
    {
        requestBytes = rde_command_request_size[(*ctx)->next_command];
    }
    std::vector<uint8_t> msg(sizeof(pldm_msg_hdr) + requestBytes);
    auto request = reinterpret_cast<pldm_msg*>(msg.data());
    return pldm_rde_get_next_discovery_command(TEST_INSTANCE_ID, *manager, *ctx,
                                               request);
}
TEST(GetNextRequestInSequenceSuccess, PLDMRDEDiscovery)
{

    initial_setup();
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(
        TEST_DEV_ID.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    rc = test_get_next_request_seq(&manager, &base_context,
                                   PLDM_NEGOTIATE_REDFISH_PARAMETERS);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);

    rc = test_get_next_request_seq(&manager, &base_context,
                                   PLDM_NEGOTIATE_MEDIUM_PARAMETERS);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
}

TEST(GetNextRequestInSequenceFailure, PLDMRDEDiscovery)
{
    initial_setup();
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(
        TEST_DEV_ID.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);
    rc = test_get_next_request_seq(&manager, &base_context,
                                   0x0023); // Unknown request code to encode
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_ENCODING_REQUEST_FAILURE);
}

TEST(PushDiscoveryResponseRedfishParamSuccess, PLDMRDEDiscovery)
{
    initial_setup();
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(
        TEST_DEV_ID.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    std::vector<uint8_t> response(sizeof(pldm_msg_hdr) + 12, 0);
    uint8_t* responseMsg = response.data();
    size_t responseMsgSize = sizeof(pldm_msg_hdr) + 12;
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(responseMsg);

    rc = test_get_next_request_seq(&manager, &base_context,
                                   PLDM_NEGOTIATE_REDFISH_PARAMETERS);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);
    rc = encode_negotiate_redfish_parameters_resp(
        TEST_INSTANCE_ID, /*completion_code=*/0,
        /*device_concurrency=*/TEST_MC_CONCURRENCY, *TEST_DEV_CAPABILITES,
        /*device_capabilities_flags=*/*TEST_MC_FEATURES,
        /*dev provider name*/ 0x00f,
        /*Example*/ TEST_DEV_ID.c_str(),
        /*device_configuration_signature*/ PLDM_RDE_VARSTRING_UTF_16,
        responsePtr);
    EXPECT_EQ(rc, 0);

    rc = pldm_rde_discovery_push_response(manager, base_context, responsePtr,
                                          responseMsgSize);

    struct pldm_rde_device_info* deviceInfo = manager->device;
    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
    EXPECT_EQ(deviceInfo->device_concurrency, TEST_MC_CONCURRENCY);
    EXPECT_EQ(deviceInfo->device_capabilities_flag.byte,
              TEST_DEV_CAPABILITES->byte);
    EXPECT_EQ(deviceInfo->device_feature_support.value,
              TEST_MC_FEATURES->value);
    EXPECT_EQ(base_context->next_command, PLDM_NEGOTIATE_MEDIUM_PARAMETERS);

    rc = test_get_next_request_seq(&manager, &base_context,
                                   PLDM_NEGOTIATE_MEDIUM_PARAMETERS);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);

    rc = encode_negotiate_medium_parameters_resp(
        TEST_INSTANCE_ID, /*completion_code=*/0,
        /*device_maximum_transfer_bytes*/ 256, responsePtr);
    EXPECT_EQ(rc, 0);
}

TEST(PushDiscoveryResponseRedfishMediumParamSuccess, PLDMRDEDiscovery)
{
    initial_setup();
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(
        TEST_DEV_ID.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);

    std::vector<uint8_t> response(sizeof(pldm_msg_hdr) + 6, 0);
    uint8_t* responseMsg = response.data();
    size_t responseMsgSize = sizeof(pldm_msg_hdr) + 6;
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(responseMsg);

    rc = test_get_next_request_seq(&manager, &base_context,
                                   PLDM_NEGOTIATE_MEDIUM_PARAMETERS);
    EXPECT_EQ(rc, PLDM_BASE_REQUESTER_SUCCESS);

    rc = encode_negotiate_medium_parameters_resp(
        TEST_INSTANCE_ID, /*completion_code=*/0,
        /*device_maximum_transfer_bytes*/ 256, responsePtr);
    EXPECT_EQ(rc, 0);

    manager->device = (struct pldm_rde_device_info*)malloc(
        sizeof(struct pldm_rde_device_info));
    manager->device->device_maximum_transfer_chunk_size = 256;
    manager->mc_transfer_size = 256;
    rc = pldm_rde_discovery_push_response(manager, base_context, responsePtr,
                                          responseMsgSize);

    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_SUCCESS);
    EXPECT_EQ(manager->negotiated_transfer_size, 256);
}

TEST(PushDiscoveryResponseFailure, PLDMRDEDiscovery)
{
    initial_setup();
    struct pldm_rde_requester_manager* manager =
        new pldm_rde_requester_manager();

    int rc = pldm_rde_init_context(
        TEST_DEV_ID.c_str(), TEST_NET_ID, manager, TEST_MC_CONCURRENCY,
        TEST_MC_TRANSFER_SIZE, TEST_MC_FEATURES, TEST_NUMBER_OF_RESOURCES,
        &resource_ids.front(), allocate_memory_to_contexts, free_memory);

    struct pldm_rde_requester_context* base_context =
        new pldm_rde_requester_context();
    rc = pldm_rde_create_context(base_context);
    std::vector<uint8_t> response(sizeof(pldm_msg_hdr) + 12, 0);
    uint8_t* responseMsg = response.data();
    size_t responseMsgSize = sizeof(pldm_msg_hdr) + 12;
    auto responsePtr = reinterpret_cast<struct pldm_msg*>(responseMsg);
    rc = pldm_rde_discovery_push_response(manager, base_context, responsePtr,
                                          responseMsgSize);

    EXPECT_EQ(rc, PLDM_RDE_REQUESTER_NO_NEXT_COMMAND_FOUND);
}