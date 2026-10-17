// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "responder.c"

#include <array>

#include "firmware_device/fd-internal.h"

#include <gtest/gtest.h>

TEST(Responder, track_untrack_one)
{
    struct pldm_responder_cookie jar{};
    struct pldm_responder_cookie cookie = {
        .tid = 1,
        .instance_id = 1,
        .type = 0,
        .command = 0x01, /* SetTID */
        .next = nullptr,
    };

    ASSERT_EQ(pldm_responder_cookie_track(&jar, &cookie), 0);
    ASSERT_NE(jar.next, nullptr);
    ASSERT_EQ(pldm_responder_cookie_untrack(&jar, 1, 1, 0, 0x01), &cookie);
    ASSERT_EQ(jar.next, nullptr);
}

TEST(Responder, untrack_none)
{
    struct pldm_responder_cookie jar{};

    ASSERT_EQ(jar.next, nullptr);
    ASSERT_EQ(pldm_responder_cookie_untrack(&jar, 1, 1, 0, 0x01), nullptr);
    ASSERT_EQ(jar.next, nullptr);
}

TEST(Responder, track_one_untrack_bad)
{
    struct pldm_responder_cookie jar{};
    struct pldm_responder_cookie cookie = {
        .tid = 1,
        .instance_id = 1,
        .type = 0,
        .command = 0x01, /* SetTID */
        .next = nullptr,
    };

    ASSERT_EQ(pldm_responder_cookie_track(&jar, &cookie), 0);
    ASSERT_NE(jar.next, nullptr);
    ASSERT_EQ(pldm_responder_cookie_untrack(&jar, 2, 1, 0, 0x01), nullptr);
    ASSERT_EQ(pldm_responder_cookie_untrack(&jar, 1, 2, 0, 0x01), nullptr);
    ASSERT_EQ(pldm_responder_cookie_untrack(&jar, 1, 1, 1, 0x01), nullptr);
    ASSERT_EQ(pldm_responder_cookie_untrack(&jar, 1, 1, 0, 0x02), nullptr);
    ASSERT_NE(jar.next, nullptr);
    ASSERT_EQ(pldm_responder_cookie_untrack(&jar, 1, 1, 0, 0x01), &cookie);
    ASSERT_EQ(jar.next, nullptr);
}

TEST(Responder, track_untrack_two)
{
    struct pldm_responder_cookie jar{};
    struct pldm_responder_cookie cookies[] = {
        {
            .tid = 1,
            .instance_id = 1,
            .type = 0,
            .command = 0x01, /* SetTID */
            .next = nullptr,
        },
        {
            .tid = 2,
            .instance_id = 1,
            .type = 0,
            .command = 0x01, /* SetTID */
            .next = nullptr,
        },
    };

    ASSERT_EQ(pldm_responder_cookie_track(&jar, &cookies[0]), 0);
    ASSERT_EQ(pldm_responder_cookie_track(&jar, &cookies[1]), 0);
    ASSERT_NE(jar.next, nullptr);
    ASSERT_EQ(pldm_responder_cookie_untrack(&jar, 2, 1, 0, 0x01), &cookies[1]);
    ASSERT_EQ(pldm_responder_cookie_untrack(&jar, 1, 1, 0, 0x01), &cookies[0]);
    ASSERT_EQ(jar.next, nullptr);
}

namespace
{

constexpr pldm_tid_t uaAddress = 8;

struct FirmwareDeviceTestContext
{
    uint64_t now = 0;
};

uint64_t firmwareDeviceNow(void* ctx)
{
    return static_cast<FirmwareDeviceTestContext*>(ctx)->now;
}

const pldm_fd_ops firmwareDeviceOps = [] {
    pldm_fd_ops ops{};
    ops.now = firmwareDeviceNow;
    return ops;
}();

TEST(FirmwareDevice, ResetsRequestStateAndRefreshesTimeoutOnCancel)
{
    FirmwareDeviceTestContext context{.now = 1000};
    struct pldm_fd fd{};
    fd.ops = &firmwareDeviceOps;
    fd.ops_ctx = &context;

    const auto setApplyState = [&fd] {
        fd.state = PLDM_FD_STATE_APPLY;
        fd.req.state = pldm_fd_req::PLDM_FD_REQ_SENT;
        fd.req.complete = true;
        fd.req.result = PLDM_FWUP_APPLY_SUCCESS;
        fd.ua_address = uaAddress;
        fd.ua_address_set = true;
    };
    const auto cancel = [&fd](bool component) {
        std::array<uint8_t, 128> request{};
        std::array<uint8_t, 128> response{};
        auto* msg = reinterpret_cast<pldm_msg*>(request.data());
        const int encodeRc = component
                                 ? encode_cancel_update_component_req(4, msg, 0)
                                 : encode_cancel_update_req(4, msg, 0);
        if (encodeRc != PLDM_SUCCESS)
        {
            return encodeRc;
        }
        size_t responseLength = response.size();
        return pldm_fd_handle_msg(&fd, uaAddress, request.data(),
                                  sizeof(pldm_msg_hdr), response.data(),
                                  &responseLength);
    };

    setApplyState();
    ASSERT_EQ(cancel(false), 0);
    EXPECT_EQ(fd.state, PLDM_FD_STATE_IDLE);
    EXPECT_EQ(fd.req.state, pldm_fd_req::PLDM_FD_REQ_UNUSED);

    setApplyState();
    fd.update_timestamp_fd_t1 = 0;
    ASSERT_EQ(pldm_fd_set_update_idle_timeout(&fd, 100), 0);
    ASSERT_EQ(cancel(true), 0);
    EXPECT_EQ(fd.state, PLDM_FD_STATE_READY_XFER);
    EXPECT_EQ(fd.req.state, pldm_fd_req::PLDM_FD_REQ_UNUSED);
    EXPECT_EQ(fd.update_timestamp_fd_t1, context.now);

    std::array<uint8_t, 128> progressMessage{};
    size_t progressLength = progressMessage.size();
    pldm_tid_t address = 0;
    EXPECT_EQ(pldm_fd_progress(&fd, progressMessage.data(), &progressLength,
                               &address),
              -EINVAL);
    EXPECT_EQ(progressLength, 0);
    EXPECT_EQ(fd.state, PLDM_FD_STATE_READY_XFER);
}

} // namespace
