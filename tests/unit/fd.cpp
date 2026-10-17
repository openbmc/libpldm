/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#include <array>
#include <cerrno>
#include <new>

#include "firmware_device/fd-internal.h"

#include <gtest/gtest.h>

namespace
{

constexpr pldm_tid_t uaAddress = 8;
constexpr uint8_t instanceId = 4;

uint64_t firmwareDeviceNow(void* ctx)
{
    return *static_cast<uint64_t*>(ctx);
}

void firmwareDeviceCancelUpdateComponent(
    void* /*ctx*/, const struct pldm_firmware_update_component* /*comp*/)
{
}

const pldm_fd_ops firmwareDeviceOps = {
    .device_identifiers = nullptr,
    .components = nullptr,
    .imageset_versions = nullptr,
    .update_component = nullptr,
    .transfer_size = nullptr,
    .firmware_data = nullptr,
    .verify = nullptr,
    .apply = nullptr,
    .activate = nullptr,
    .cancel_update_component = firmwareDeviceCancelUpdateComponent,
    .now = firmwareDeviceNow,
};

void setApplyState(struct pldm_fd& fd)
{
    fd.state = PLDM_FD_STATE_APPLY;
    fd.req.state = pldm_fd_req::PLDM_FD_REQ_SENT;
    fd.req.instance_id = instanceId;
    fd.req.command = PLDM_APPLY_COMPLETE;
    fd.req.complete = true;
    fd.req.result = PLDM_FWUP_APPLY_SUCCESS;
    fd.ua_address = uaAddress;
    fd.ua_address_set = true;
}

constexpr size_t maxPayloadLength = 128;

int cancel(struct pldm_fd& fd, bool component)
{
    PLDM_MSG_DEFINE_P(request, maxPayloadLength);
    PLDM_MSG_DEFINE_P(response, maxPayloadLength);
    size_t requestLength = 0;
    int rc = 0;
    if (component)
    {
        rc = encode_cancel_update_component_req(
            instanceId, request, PLDM_CANCEL_UPDATE_COMPONENT_REQ_BYTES);
        requestLength = PLDM_MSG_SIZE(PLDM_CANCEL_UPDATE_COMPONENT_REQ_BYTES);
    }
    else
    {
        rc = encode_cancel_update_req(instanceId, request,
                                      PLDM_CANCEL_UPDATE_REQ_BYTES);
        requestLength = PLDM_MSG_SIZE(PLDM_CANCEL_UPDATE_REQ_BYTES);
    }
    if (rc != PLDM_SUCCESS)
    {
        return -EINVAL;
    }

    size_t responseLength = PLDM_MSG_SIZE(maxPayloadLength);
    return pldm_fd_handle_msg(&fd, uaAddress, request, requestLength, response,
                              &responseLength);
}

int applyCompleteResp(struct pldm_fd& fd)
{
    /* ApplyComplete response payload is only the completion code */
    constexpr size_t applyCompleteRespBytes = 1;
    PLDM_MSG_DEFINE_P(applyResp, applyCompleteRespBytes);
    PLDM_MSG_DEFINE_P(out, maxPayloadLength);
    if (encode_apply_complete_resp(instanceId, PLDM_SUCCESS, applyResp,
                                   applyCompleteRespBytes) != PLDM_SUCCESS)
    {
        return -EINVAL;
    }

    size_t outLength = PLDM_MSG_SIZE(maxPayloadLength);
    return pldm_fd_handle_msg(&fd, uaAddress, applyResp,
                              PLDM_MSG_SIZE(applyCompleteRespBytes), out,
                              &outLength);
}

bool getStatus(struct pldm_fd& fd, uint8_t* state, uint8_t* auxState)
{
    PLDM_MSG_DEFINE_P(request, maxPayloadLength);
    PLDM_MSG_DEFINE_P(response, maxPayloadLength);
    if (encode_get_status_req(instanceId, request, PLDM_GET_STATUS_REQ_BYTES) !=
        PLDM_SUCCESS)
    {
        return false;
    }

    size_t responseLength = PLDM_MSG_SIZE(maxPayloadLength);
    if (pldm_fd_handle_msg(&fd, uaAddress, request,
                           PLDM_MSG_SIZE(PLDM_GET_STATUS_REQ_BYTES), response,
                           &responseLength) != 0)
    {
        return false;
    }

    uint8_t completionCode = 0xff;
    uint8_t previousState = 0xff;
    uint8_t auxStateStatus = 0xff;
    uint8_t progressPercent = 0xff;
    uint8_t reasonCode = 0xff;
    bitfield32_t updateFlags = {};
    return decode_get_status_resp(
               response, responseLength - sizeof(pldm_msg_hdr), &completionCode,
               state, &previousState, auxState, &auxStateStatus,
               &progressPercent, &reasonCode, &updateFlags) == PLDM_SUCCESS &&
           completionCode == PLDM_SUCCESS;
}

TEST(FirmwareDevice, CancelUpdateResetsRequestStateAndGetStatusAuxiliaryState)
{
    uint64_t now = 1000;
    struct pldm_fd fd{};
    fd.ops = &firmwareDeviceOps;
    fd.ops_ctx = &now;
    setApplyState(fd);

    ASSERT_EQ(cancel(fd, false), 0);
    EXPECT_EQ(fd.state, PLDM_FD_STATE_IDLE);
    EXPECT_EQ(fd.req.state, pldm_fd_req::PLDM_FD_REQ_UNUSED);

    uint8_t state = 0xff;
    uint8_t auxState = 0xff;
    ASSERT_TRUE(getStatus(fd, &state, &auxState));
    EXPECT_EQ(state, PLDM_FD_STATE_IDLE);
    EXPECT_EQ(auxState, PLDM_FD_IDLE_LEARN_COMPONENTS_READ_XFER);
}

TEST(FirmwareDevice, CancelUpdateComponentResetsRequestStateAndRefreshesTimeout)
{
    uint64_t now = 1000;
    struct pldm_fd fd{};
    fd.ops = &firmwareDeviceOps;
    fd.ops_ctx = &now;
    setApplyState(fd);
    fd.update_timestamp_fd_t1 = 0;
    ASSERT_EQ(pldm_fd_set_update_idle_timeout(&fd, 100), 0);

    ASSERT_EQ(cancel(fd, true), 0);
    EXPECT_EQ(fd.state, PLDM_FD_STATE_READY_XFER);
    EXPECT_EQ(fd.req.state, pldm_fd_req::PLDM_FD_REQ_UNUSED);
    EXPECT_EQ(fd.update_timestamp_fd_t1, now);

    uint8_t state = 0xff;
    uint8_t auxState = 0xff;
    ASSERT_TRUE(getStatus(fd, &state, &auxState));
    EXPECT_EQ(state, PLDM_FD_STATE_READY_XFER);
    EXPECT_EQ(auxState, PLDM_FD_IDLE_LEARN_COMPONENTS_READ_XFER);
}

TEST(FirmwareDevice, CancelUpdateComponentDoesNotTimeOutOnStaleTimestamp)
{
    uint64_t now = 1000;
    struct pldm_fd fd{};
    fd.ops = &firmwareDeviceOps;
    fd.ops_ctx = &now;
    setApplyState(fd);
    fd.update_timestamp_fd_t1 = 0;
    ASSERT_EQ(pldm_fd_set_update_idle_timeout(&fd, 100), 0);

    ASSERT_EQ(cancel(fd, true), 0);
    ASSERT_EQ(fd.state, PLDM_FD_STATE_READY_XFER);

    std::array<uint8_t, 128> progressMessage{};
    size_t progressLength = progressMessage.size();
    pldm_tid_t address = 0;
    EXPECT_EQ(pldm_fd_progress(&fd, progressMessage.data(), &progressLength,
                               &address),
              -EINVAL);
    EXPECT_EQ(progressLength, 0);
    EXPECT_EQ(fd.state, PLDM_FD_STATE_READY_XFER);
}

TEST(FirmwareDevice, ApplyCompleteResetsRequestStateAndGetStatusAuxiliaryState)
{
    uint64_t now = 1000;
    struct pldm_fd fd{};
    fd.ops = &firmwareDeviceOps;
    fd.ops_ctx = &now;
    setApplyState(fd);

    ASSERT_EQ(applyCompleteResp(fd), 0);
    EXPECT_EQ(fd.state, PLDM_FD_STATE_READY_XFER);
    EXPECT_EQ(fd.req.state, pldm_fd_req::PLDM_FD_REQ_UNUSED);

    uint8_t state = 0xff;
    uint8_t auxState = 0xff;
    ASSERT_TRUE(getStatus(fd, &state, &auxState));
    EXPECT_EQ(state, PLDM_FD_STATE_READY_XFER);
    EXPECT_EQ(auxState, PLDM_FD_IDLE_LEARN_COMPONENTS_READ_XFER);
}

TEST(FirmwareDevice, ApplyCompleteFailureThenCancelUpdateResetsAuxiliaryState)
{
    uint64_t now = 1000;
    struct pldm_fd fd{};
    fd.ops = &firmwareDeviceOps;
    fd.ops_ctx = &now;
    setApplyState(fd);
    /* The apply result is reported as GetStatus AuxStateStatus, so use a
     * failure code that is valid in both domains */
    fd.req.result = PLDM_FWUP_VENDOR_APPLY_RESULT_RANGE_MIN;

    ASSERT_EQ(applyCompleteResp(fd), 0);
    EXPECT_EQ(fd.state, PLDM_FD_STATE_APPLY);
    EXPECT_EQ(fd.req.state, pldm_fd_req::PLDM_FD_REQ_FAILED);

    uint8_t state = 0xff;
    uint8_t auxState = 0xff;
    ASSERT_TRUE(getStatus(fd, &state, &auxState));
    EXPECT_EQ(state, PLDM_FD_STATE_APPLY);
    EXPECT_EQ(auxState, PLDM_FD_OPERATION_FAILED);

    ASSERT_EQ(cancel(fd, false), 0);
    EXPECT_EQ(fd.state, PLDM_FD_STATE_IDLE);
    EXPECT_EQ(fd.req.state, pldm_fd_req::PLDM_FD_REQ_UNUSED);

    ASSERT_TRUE(getStatus(fd, &state, &auxState));
    EXPECT_EQ(state, PLDM_FD_STATE_IDLE);
    EXPECT_EQ(auxState, PLDM_FD_IDLE_LEARN_COMPONENTS_READ_XFER);
}

} // namespace