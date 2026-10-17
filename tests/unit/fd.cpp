/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#include <array>
#include <cerrno>

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

int cancel(struct pldm_fd& fd, bool component)
{
    std::array<uint8_t, 128> request{};
    std::array<uint8_t, 128> response{};
    auto* msg = reinterpret_cast<pldm_msg*>(request.data());
    int rc = component ? encode_cancel_update_component_req(instanceId, msg, 0)
                       : encode_cancel_update_req(instanceId, msg, 0);
    if (rc != PLDM_SUCCESS)
    {
        return -EINVAL;
    }

    size_t responseLength = response.size();
    return pldm_fd_handle_msg(&fd, uaAddress, request.data(),
                              sizeof(pldm_msg_hdr), response.data(),
                              &responseLength);
}

int applyCompleteResp(struct pldm_fd& fd)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + 1> applyResp{};
    std::array<uint8_t, 128> out{};
    auto* msg = reinterpret_cast<pldm_msg*>(applyResp.data());
    if (encode_apply_complete_resp(instanceId, PLDM_SUCCESS, msg, 1) !=
        PLDM_SUCCESS)
    {
        return -EINVAL;
    }

    size_t outLength = out.size();
    return pldm_fd_handle_msg(&fd, uaAddress, applyResp.data(),
                              applyResp.size(), out.data(), &outLength);
}

bool getStatus(struct pldm_fd& fd, uint8_t* state, uint8_t* auxState)
{
    std::array<uint8_t, 128> request{};
    std::array<uint8_t, 128> response{};
    auto* requestMsg = reinterpret_cast<pldm_msg*>(request.data());
    if (encode_get_status_req(instanceId, requestMsg, 0) != PLDM_SUCCESS)
    {
        return false;
    }

    size_t responseLength = response.size();
    if (pldm_fd_handle_msg(&fd, uaAddress, request.data(), sizeof(pldm_msg_hdr),
                           response.data(), &responseLength) != 0)
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
               reinterpret_cast<const pldm_msg*>(response.data()),
               responseLength - sizeof(pldm_msg_hdr), &completionCode, state,
               &previousState, auxState, &auxStateStatus, &progressPercent,
               &reasonCode, &updateFlags) == PLDM_SUCCESS &&
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