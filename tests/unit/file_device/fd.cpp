// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

#include <libpldm/base.h>
#include <libpldm/control.h>
#include <libpldm/file.h>
#include <libpldm/file_fd.h>
#include <libpldm/sizes.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <expected>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

static constexpr auto hdrSize = sizeof(pldm_msg_hdr);

/* Arbitrary instance ID for tests */
static constexpr uint8_t TEST_IID = 3;

/* ------------------------------------------------------------------
 * Test callbacks
 * ------------------------------------------------------------------ */

struct FileState
{
    uint8_t open_cc = PLDM_SUCCESS;
    uint8_t read_cc = PLDM_SUCCESS;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    void* open_handle = reinterpret_cast<void*>(0x1234);

    bool open_called = false;
    bool close_called = false;
    bool heartbeat_called = false;

    uint16_t last_open_file_id = 0;
    bitfield16_t last_open_attr = {0};

    void* last_close_handle = nullptr;
    uint16_t last_close_file_id = 0;

    void* last_read_handle = nullptr;
    uint32_t last_read_offset = 0;
    uint32_t last_read_req_len = 0;
    std::vector<uint8_t> read_data;

    uint32_t heartbeat_adjusted_interval = 0;
    bool heartbeat_adjust = false;
};

static FileState g_file;

static uint8_t cb_open(void* ctx, uint16_t file_id, bitfield16_t attr,
                       void** app_handle_out)
{
    auto* s = static_cast<FileState*>(ctx);
    s->open_called = true;
    s->last_open_file_id = file_id;
    s->last_open_attr = attr;
    *app_handle_out = s->open_handle;
    return s->open_cc;
}

static void cb_close(void* ctx, uint16_t file_id, void* app_handle)
{
    auto* s = static_cast<FileState*>(ctx);
    s->close_called = true;
    s->last_close_handle = app_handle;
    s->last_close_file_id = file_id;
}

static uint8_t cb_read(void* ctx, uint16_t /*file_id*/, void* app_handle,
                       uint32_t offset, void* buf, uint32_t req_len,
                       uint32_t* actual_len)
{
    auto* s = static_cast<FileState*>(ctx);
    s->last_read_handle = app_handle;
    s->last_read_offset = offset;
    s->last_read_req_len = req_len;

    if (s->read_cc != PLDM_SUCCESS)
    {
        return s->read_cc;
    }

    uint32_t avail =
        offset < s->read_data.size() ? s->read_data.size() - offset : 0;
    uint32_t n = std::min(avail, req_len);
    if (n > 0)
    {
        memcpy(buf, s->read_data.data() + offset, n);
    }
    *actual_len = n;
    return PLDM_SUCCESS;
}

static void cb_heartbeat(void* ctx, uint16_t /*file_id*/, void* /*app_handle*/,
                         uint32_t* interval_ms)
{
    auto* s = static_cast<FileState*>(ctx);
    s->heartbeat_called = true;
    if (s->heartbeat_adjust)
    {
        *interval_ms = s->heartbeat_adjusted_interval;
    }
}

/* Helper: build a minimal request message for a given PLDM type/command */
static std::vector<uint8_t> make_request(uint8_t pldm_type, uint8_t command,
                                         const void* payload,
                                         size_t payload_len)
{
    struct pldm_header_info hdrinfo{};
    hdrinfo.msg_type = PLDM_REQUEST;
    hdrinfo.instance = TEST_IID;
    hdrinfo.pldm_type = pldm_type;
    hdrinfo.command = command;
    struct pldm_msg_hdr msghdr{};
    pack_pldm_header(&hdrinfo, &msghdr);
    std::vector<uint8_t> msg(hdrSize + payload_len);
    memcpy(msg.data(), &msghdr, hdrSize);
    if (payload_len > 0)
    {
        memcpy(msg.data() + hdrSize, payload, payload_len);
    }
    return msg;
}

/* Helper: call pldm_file_fd_handle_msg and return response bytes or errno */
static std::expected<std::vector<uint8_t>, int>
    dispatch(struct pldm_file_fd* fd, const std::vector<uint8_t>& req)
{
    std::vector<uint8_t> out(512);
    size_t out_len = out.size();
    int rc = pldm_file_fd_handle_msg(fd, req.data(), req.size(), out.data(),
                                     &out_len);
    if (rc != 0)
    {
        return std::unexpected(rc);
    }
    out.resize(out_len);
    return out;
}

/* Helper: extract completion code from response payload */
static uint8_t response_cc(const std::vector<uint8_t>& resp)
{
    if (resp.size() <= hdrSize)
    {
        return 0xff;
    }
    return resp[hdrSize];
}

/* ===================================================================
 * Fixture
 * =================================================================== */

class FdTest : public ::testing::Test
{
  protected:
    static constexpr size_t NUM_FDS = 4;
    PLDM_FILE_FD_BUFFER(fd_buf, NUM_FDS);
    struct pldm_file_fd* fd = nullptr;

    void SetUp() override
    {
        g_file = FileState{};

        struct pldm_file_fd_ops ops = {
            .ctx = &g_file,
            .open = cb_open,
            .close = cb_close,
            .read = cb_read,
            .heartbeat = cb_heartbeat,
        };

        int rc = pldm_file_fd_setup(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<struct pldm_file_fd*>(fd_buf),
            PLDM_FILE_FD_SIZE(NUM_FDS), NUM_FDS, &ops, sizeof(ops), nullptr);
        ASSERT_EQ(rc, 0);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        fd = reinterpret_cast<struct pldm_file_fd*>(fd_buf);
    }

    /* Issue a DfOpen request and return the response */
    std::expected<std::vector<uint8_t>, int> doOpen(uint16_t file_id)
    {
        struct pldm_file_df_open_req oreq{};
        oreq.file_identifier = file_id;
        oreq.file_attribute = {0};
        size_t plen = PLDM_DF_OPEN_REQ_BYTES;
        std::vector<uint8_t> buf(hdrSize + plen);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto* msg = reinterpret_cast<struct pldm_msg*>(buf.data());
        struct pldm_header_info hdrinfo{};
        hdrinfo.msg_type = PLDM_REQUEST;
        hdrinfo.instance = TEST_IID;
        hdrinfo.pldm_type = PLDM_FILE;
        hdrinfo.command = PLDM_FILE_CMD_DF_OPEN;
        pack_pldm_header(&hdrinfo, &msg->hdr);
        EXPECT_EQ(encode_pldm_file_df_open_req(TEST_IID, &oreq, msg, &plen), 0);
        return dispatch(fd, buf);
    }

    /* Open a file and return the assigned file_descriptor */
    uint16_t openFile(uint16_t file_id)
    {
        auto resp = doOpen(file_id);
        EXPECT_TRUE(resp.has_value());
        EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);
        struct pldm_file_df_open_resp oresp{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto* msg = reinterpret_cast<const struct pldm_msg*>(resp->data());
        decode_pldm_file_df_open_resp(msg, resp->size() - hdrSize, &oresp);
        return oresp.file_descriptor;
    }
};

/* ===================================================================
 * 1. Setup validation
 * =================================================================== */

TEST(FdSetup, NullArgs)
{
    PLDM_FILE_FD_BUFFER(buf, 1);
    struct pldm_file_fd_ops ops = {
        .ctx = nullptr,
        .open = cb_open,
        .close = cb_close,
        .read = cb_read,
        .heartbeat = nullptr,
    };

    EXPECT_EQ(pldm_file_fd_setup(nullptr, PLDM_FILE_FD_SIZE(1), 1, &ops,
                                 sizeof(ops), nullptr),
              -EINVAL);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    EXPECT_EQ(pldm_file_fd_setup(reinterpret_cast<struct pldm_file_fd*>(buf),
                                 PLDM_FILE_FD_SIZE(1), 1, nullptr, sizeof(ops),
                                 nullptr),
              -EINVAL);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    EXPECT_EQ(pldm_file_fd_setup(reinterpret_cast<struct pldm_file_fd*>(buf),
                                 PLDM_FILE_FD_SIZE(1), 1, &ops, 0, nullptr),
              -EINVAL);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    EXPECT_EQ(pldm_file_fd_setup(reinterpret_cast<struct pldm_file_fd*>(buf),
                                 PLDM_FILE_FD_SIZE(1), 0, &ops, sizeof(ops),
                                 nullptr),
              -EINVAL);
}

TEST(FdSetup, SizeTooSmall)
{
    PLDM_FILE_FD_BUFFER(buf, 1);
    struct pldm_file_fd_ops ops = {
        .ctx = nullptr,
        .open = cb_open,
        .close = cb_close,
        .read = cb_read,
        .heartbeat = nullptr,
    };

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    EXPECT_EQ(pldm_file_fd_setup(reinterpret_cast<struct pldm_file_fd*>(buf),
                                 PLDM_FILE_FD_SIZE(1) - 1, 1, &ops, sizeof(ops),
                                 nullptr),
              -EINVAL);
}

TEST(FdSetup, OpsWithUnknownNonZeroFieldReturnsE2BIG)
{
    PLDM_FILE_FD_BUFFER(buf, 1);
    struct
    {
        struct pldm_file_fd_ops known;
        uint8_t future_field;
    } big_ops = {};
    big_ops.known.open = cb_open;
    big_ops.known.close = cb_close;
    big_ops.known.read = cb_read;
    big_ops.future_field = 0x01;

    EXPECT_EQ(pldm_file_fd_setup(
                  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                  reinterpret_cast<struct pldm_file_fd*>(buf),
                  PLDM_FILE_FD_SIZE(1), 1,
                  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                  reinterpret_cast<const struct pldm_file_fd_ops*>(&big_ops),
                  sizeof(big_ops), nullptr),
              -E2BIG);
}

TEST(FdSetup, OpsWithUnknownZeroFieldsSucceeds)
{
    PLDM_FILE_FD_BUFFER(buf, 1);
    struct
    {
        struct pldm_file_fd_ops known;
        uint8_t future_field;
    } big_ops = {};
    big_ops.known.open = cb_open;
    big_ops.known.close = cb_close;
    big_ops.known.read = cb_read;

    EXPECT_EQ(pldm_file_fd_setup(
                  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                  reinterpret_cast<struct pldm_file_fd*>(buf),
                  PLDM_FILE_FD_SIZE(1), 1,
                  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                  reinterpret_cast<const struct pldm_file_fd_ops*>(&big_ops),
                  sizeof(big_ops), nullptr),
              0);
}

TEST(FdNew, MallocAndFree)
{
    struct pldm_file_fd_ops ops = {
        .ctx = &g_file,
        .open = cb_open,
        .close = cb_close,
        .read = cb_read,
        .heartbeat = nullptr,
    };
    struct pldm_file_fd* fd = pldm_file_fd_new(2, &ops, sizeof(ops), nullptr);
    ASSERT_NE(fd, nullptr);
    free(fd);
}

/* ===================================================================
 * 2. DfOpen
 * =================================================================== */

TEST_F(FdTest, DfOpenSuccess)
{
    auto resp = doOpen(7);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);
    EXPECT_TRUE(g_file.open_called);
    EXPECT_EQ(g_file.last_open_file_id, 7);
}

TEST_F(FdTest, DfOpenAssignsNonZeroFileDescriptor)
{
    uint16_t fdesc = openFile(1);
    EXPECT_NE(fdesc, 0);
}

TEST_F(FdTest, DfOpenDistinctFileDescriptors)
{
    uint16_t fd1 = openFile(1);
    uint16_t fd2 = openFile(2);
    EXPECT_NE(fd1, fd2);
}

TEST_F(FdTest, DfOpenMaxNumFdsExceeded)
{
    for (size_t i = 0; i < NUM_FDS; i++)
    {
        openFile(static_cast<uint16_t>(i + 1));
    }
    auto resp = doOpen(999);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_FILE_CC_MAX_NUM_FDS_EXCEEDED);
}

TEST_F(FdTest, DfOpenCallbackFailurePropagatesCc)
{
    g_file.open_cc = PLDM_FILE_CC_UNABLE_TO_OPEN_FILE;
    auto resp = doOpen(1);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_FILE_CC_UNABLE_TO_OPEN_FILE);
}

TEST_F(FdTest, DfOpenShortPayload)
{
    auto req = make_request(PLDM_FILE, PLDM_FILE_CMD_DF_OPEN, nullptr, 0);
    auto resp = dispatch(fd, req);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_ERROR_INVALID_LENGTH);
}

/* ===================================================================
 * 3. DfClose
 * =================================================================== */

TEST_F(FdTest, DfCloseSuccess)
{
    uint16_t fdesc = openFile(1);

    struct pldm_file_df_close_req creq{};
    creq.file_descriptor = fdesc;
    creq.df_close_options = {0};
    size_t plen = PLDM_DF_CLOSE_REQ_BYTES;
    std::vector<uint8_t> buf(hdrSize + plen);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<struct pldm_msg*>(buf.data());
    struct pldm_header_info hdrinfo{};
    hdrinfo.msg_type = PLDM_REQUEST;
    hdrinfo.instance = TEST_IID;
    hdrinfo.pldm_type = PLDM_FILE;
    hdrinfo.command = PLDM_FILE_CMD_DF_CLOSE;
    pack_pldm_header(&hdrinfo, &msg->hdr);
    ASSERT_EQ(encode_pldm_file_df_close_req(TEST_IID, &creq, msg, &plen), 0);

    auto resp = dispatch(fd, buf);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);
    EXPECT_TRUE(g_file.close_called);
    EXPECT_EQ(g_file.last_close_handle, g_file.open_handle);
}

TEST_F(FdTest, DfCloseInvalidFileDescriptor)
{
    struct pldm_file_df_close_req creq{};
    creq.file_descriptor = 0xbeef;
    creq.df_close_options = {0};
    size_t plen = PLDM_DF_CLOSE_REQ_BYTES;
    std::vector<uint8_t> buf(hdrSize + plen);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<struct pldm_msg*>(buf.data());
    struct pldm_header_info hdrinfo{};
    hdrinfo.msg_type = PLDM_REQUEST;
    hdrinfo.instance = TEST_IID;
    hdrinfo.pldm_type = PLDM_FILE;
    hdrinfo.command = PLDM_FILE_CMD_DF_CLOSE;
    pack_pldm_header(&hdrinfo, &msg->hdr);
    ASSERT_EQ(encode_pldm_file_df_close_req(TEST_IID, &creq, msg, &plen), 0);

    auto resp = dispatch(fd, buf);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_FILE_CC_INVALID_FILE_DESCRIPTOR);
}

TEST_F(FdTest, DfCloseAllowsFileDescriptorReuse)
{
    uint16_t fdesc1 = openFile(1);

    struct pldm_file_df_close_req creq{};
    creq.file_descriptor = fdesc1;
    creq.df_close_options = {0};
    size_t plen = PLDM_DF_CLOSE_REQ_BYTES;
    std::vector<uint8_t> buf(hdrSize + plen);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<struct pldm_msg*>(buf.data());
    struct pldm_header_info hdrinfo{};
    hdrinfo.msg_type = PLDM_REQUEST;
    hdrinfo.instance = TEST_IID;
    hdrinfo.pldm_type = PLDM_FILE;
    hdrinfo.command = PLDM_FILE_CMD_DF_CLOSE;
    pack_pldm_header(&hdrinfo, &msg->hdr);
    ASSERT_EQ(encode_pldm_file_df_close_req(TEST_IID, &creq, msg, &plen), 0);
    auto resp = dispatch(fd, buf);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);

    for (size_t i = 0; i < NUM_FDS; i++)
    {
        openFile(static_cast<uint16_t>(i + 100));
    }
}

TEST_F(FdTest, DfCloseShortPayload)
{
    auto req = make_request(PLDM_FILE, PLDM_FILE_CMD_DF_CLOSE, nullptr, 0);
    auto resp = dispatch(fd, req);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_ERROR_INVALID_LENGTH);
}

/* ===================================================================
 * 4. DfHeartbeat
 * =================================================================== */

TEST_F(FdTest, DfHeartbeatEchoesRequesterIntervalWithoutCallbackAdjust)
{
    uint16_t fdesc = openFile(1);

    struct pldm_file_df_heartbeat_req hreq{};
    hreq.file_descriptor = fdesc;
    hreq.requester_max_interval = 5000;
    size_t plen = PLDM_DF_HEARTBEAT_REQ_BYTES;
    std::vector<uint8_t> buf(hdrSize + plen);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<struct pldm_msg*>(buf.data());
    struct pldm_header_info hdrinfo{};
    hdrinfo.msg_type = PLDM_REQUEST;
    hdrinfo.instance = TEST_IID;
    hdrinfo.pldm_type = PLDM_FILE;
    hdrinfo.command = PLDM_FILE_CMD_DF_HEARTBEAT;
    pack_pldm_header(&hdrinfo, &msg->hdr);
    ASSERT_EQ(encode_pldm_file_df_heartbeat_req(TEST_IID, &hreq, msg, &plen),
              0);

    auto resp = dispatch(fd, buf);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);
    EXPECT_TRUE(g_file.heartbeat_called);

    struct pldm_file_df_heartbeat_resp hresp{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* rmsg = reinterpret_cast<const struct pldm_msg*>(resp->data());
    ASSERT_EQ(decode_pldm_file_df_heartbeat_resp(rmsg, resp->size() - hdrSize,
                                                 &hresp),
              0);
    EXPECT_EQ(hresp.responder_max_interval, 5000u);
}

TEST_F(FdTest, DfHeartbeatCallbackCanAdjustInterval)
{
    uint16_t fdesc = openFile(1);
    g_file.heartbeat_adjust = true;
    g_file.heartbeat_adjusted_interval = 1000;

    struct pldm_file_df_heartbeat_req hreq{};
    hreq.file_descriptor = fdesc;
    hreq.requester_max_interval = 5000;
    size_t plen = PLDM_DF_HEARTBEAT_REQ_BYTES;
    std::vector<uint8_t> buf(hdrSize + plen);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<struct pldm_msg*>(buf.data());
    struct pldm_header_info hdrinfo{};
    hdrinfo.msg_type = PLDM_REQUEST;
    hdrinfo.instance = TEST_IID;
    hdrinfo.pldm_type = PLDM_FILE;
    hdrinfo.command = PLDM_FILE_CMD_DF_HEARTBEAT;
    pack_pldm_header(&hdrinfo, &msg->hdr);
    ASSERT_EQ(encode_pldm_file_df_heartbeat_req(TEST_IID, &hreq, msg, &plen),
              0);

    auto resp = dispatch(fd, buf);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);

    struct pldm_file_df_heartbeat_resp hresp{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* rmsg = reinterpret_cast<const struct pldm_msg*>(resp->data());
    ASSERT_EQ(decode_pldm_file_df_heartbeat_resp(rmsg, resp->size() - hdrSize,
                                                 &hresp),
              0);
    EXPECT_EQ(hresp.responder_max_interval, 1000u);
}

TEST_F(FdTest, DfHeartbeatInvalidFileDescriptor)
{
    struct pldm_file_df_heartbeat_req hreq{};
    hreq.file_descriptor = 0xbeef;
    hreq.requester_max_interval = 5000;
    size_t plen = PLDM_DF_HEARTBEAT_REQ_BYTES;
    std::vector<uint8_t> buf(hdrSize + plen);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<struct pldm_msg*>(buf.data());
    struct pldm_header_info hdrinfo{};
    hdrinfo.msg_type = PLDM_REQUEST;
    hdrinfo.instance = TEST_IID;
    hdrinfo.pldm_type = PLDM_FILE;
    hdrinfo.command = PLDM_FILE_CMD_DF_HEARTBEAT;
    pack_pldm_header(&hdrinfo, &msg->hdr);
    ASSERT_EQ(encode_pldm_file_df_heartbeat_req(TEST_IID, &hreq, msg, &plen),
              0);

    auto resp = dispatch(fd, buf);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_FILE_CC_INVALID_FILE_DESCRIPTOR);
}

TEST_F(FdTest, DfHeartbeatNoCallbackEchoesUnchanged)
{
    struct pldm_file_fd_ops ops = {
        .ctx = &g_file,
        .open = cb_open,
        .close = cb_close,
        .read = cb_read,
        .heartbeat = nullptr,
    };
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    ASSERT_EQ(pldm_file_fd_setup(reinterpret_cast<struct pldm_file_fd*>(fd_buf),
                                 PLDM_FILE_FD_SIZE(NUM_FDS), NUM_FDS, &ops,
                                 sizeof(ops), nullptr),
              0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    fd = reinterpret_cast<struct pldm_file_fd*>(fd_buf);
    uint16_t fdesc = openFile(1);

    struct pldm_file_df_heartbeat_req hreq{};
    hreq.file_descriptor = fdesc;
    hreq.requester_max_interval = 2500;
    size_t plen = PLDM_DF_HEARTBEAT_REQ_BYTES;
    std::vector<uint8_t> buf(hdrSize + plen);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<struct pldm_msg*>(buf.data());
    struct pldm_header_info hdrinfo{};
    hdrinfo.msg_type = PLDM_REQUEST;
    hdrinfo.instance = TEST_IID;
    hdrinfo.pldm_type = PLDM_FILE;
    hdrinfo.command = PLDM_FILE_CMD_DF_HEARTBEAT;
    pack_pldm_header(&hdrinfo, &msg->hdr);
    ASSERT_EQ(encode_pldm_file_df_heartbeat_req(TEST_IID, &hreq, msg, &plen),
              0);

    auto resp = dispatch(fd, buf);
    ASSERT_TRUE(resp.has_value());
    struct pldm_file_df_heartbeat_resp hresp{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* rmsg = reinterpret_cast<const struct pldm_msg*>(resp->data());
    ASSERT_EQ(decode_pldm_file_df_heartbeat_resp(rmsg, resp->size() - hdrSize,
                                                 &hresp),
              0);
    EXPECT_EQ(hresp.responder_max_interval, 2500u);
}

/* ===================================================================
 * 5. DfRead (via MultipartReceive)
 * =================================================================== */

static std::vector<uint8_t> make_multipart_receive_req(uint8_t transfer_opflag,
                                                       uint32_t transfer_ctx,
                                                       uint32_t transfer_handle,
                                                       uint32_t section_offset,
                                                       uint32_t section_length)
{
    struct pldm_base_multipart_receive_req mreq{};
    mreq.pldm_type = PLDM_FILE;
    mreq.transfer_opflag = transfer_opflag;
    mreq.transfer_ctx = transfer_ctx;
    mreq.transfer_handle = transfer_handle;
    mreq.section_offset = section_offset;
    mreq.section_length = section_length;

    size_t plen = 512;
    std::vector<uint8_t> buf(hdrSize + plen);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<struct pldm_msg*>(buf.data());
    struct pldm_header_info hdrinfo{};
    hdrinfo.msg_type = PLDM_REQUEST;
    hdrinfo.instance = TEST_IID;
    hdrinfo.pldm_type = PLDM_BASE;
    hdrinfo.command = PLDM_MULTIPART_RECEIVE;
    pack_pldm_header(&hdrinfo, &msg->hdr);
    if (encode_pldm_base_multipart_receive_req(TEST_IID, &mreq, msg, &plen) !=
        0)
    {
        return {};
    }
    buf.resize(hdrSize + plen);
    return buf;
}

TEST_F(FdTest, DfReadFirstPartFitsEntirelyIsStartAndEnd)
{
    uint16_t fdesc = openFile(1);
    g_file.read_data = {1, 2, 3, 4, 5};

    auto req =
        make_multipart_receive_req(PLDM_XFER_FIRST_PART, fdesc, 0, 0, 1024);
    auto resp = dispatch(fd, req);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_SUCCESS);

    struct pldm_base_multipart_receive_resp mresp{};
    uint32_t checksum = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* rmsg = reinterpret_cast<const struct pldm_msg*>(resp->data());
    ASSERT_EQ(decode_pldm_base_multipart_receive_resp(
                  rmsg, resp->size() - hdrSize, &mresp, &checksum),
              0);
    EXPECT_EQ(mresp.transfer_flag,
              PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_START_AND_END);
    EXPECT_EQ(mresp.next_transfer_handle, 0u);
    ASSERT_EQ(mresp.data.length, 5u);
    EXPECT_EQ(memcmp(mresp.data.ptr, g_file.read_data.data(), 5), 0);
}

TEST_F(FdTest, DfReadMultiPartSequence)
{
    uint16_t fdesc = openFile(1);
    g_file.read_data = std::vector<uint8_t>(10, 0);
    for (size_t i = 0; i < g_file.read_data.size(); i++)
    {
        g_file.read_data[i] = static_cast<uint8_t>(i);
    }

    auto req1 =
        make_multipart_receive_req(PLDM_XFER_FIRST_PART, fdesc, 0, 0, 4);
    auto resp1 = dispatch(fd, req1);
    ASSERT_TRUE(resp1.has_value());
    EXPECT_EQ(response_cc(*resp1), PLDM_SUCCESS);

    struct pldm_base_multipart_receive_resp mresp1{};
    uint32_t checksum1 = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* rmsg1 = reinterpret_cast<const struct pldm_msg*>(resp1->data());
    ASSERT_EQ(decode_pldm_base_multipart_receive_resp(
                  rmsg1, resp1->size() - hdrSize, &mresp1, &checksum1),
              0);
    EXPECT_EQ(mresp1.transfer_flag,
              PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_START);
    ASSERT_EQ(mresp1.data.length, 4u);
    uint32_t next_handle = mresp1.next_transfer_handle;
    EXPECT_NE(next_handle, 0u);

    auto req2 = make_multipart_receive_req(PLDM_XFER_NEXT_PART, fdesc,
                                           next_handle, 4, 4);
    auto resp2 = dispatch(fd, req2);
    ASSERT_TRUE(resp2.has_value());
    EXPECT_EQ(response_cc(*resp2), PLDM_SUCCESS);

    struct pldm_base_multipart_receive_resp mresp2{};
    uint32_t checksum2 = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* rmsg2 = reinterpret_cast<const struct pldm_msg*>(resp2->data());
    ASSERT_EQ(decode_pldm_base_multipart_receive_resp(
                  rmsg2, resp2->size() - hdrSize, &mresp2, &checksum2),
              0);
    EXPECT_EQ(mresp2.transfer_flag,
              PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_MIDDLE);
    ASSERT_EQ(mresp2.data.length, 4u);
}

TEST_F(FdTest, DfReadEofOnFinalPart)
{
    uint16_t fdesc = openFile(1);
    g_file.read_data = {1, 2, 3};

    auto req1 =
        make_multipart_receive_req(PLDM_XFER_FIRST_PART, fdesc, 0, 0, 2);
    auto resp1 = dispatch(fd, req1);
    ASSERT_TRUE(resp1.has_value());

    struct pldm_base_multipart_receive_resp mresp1{};
    uint32_t checksum1 = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* rmsg1 = reinterpret_cast<const struct pldm_msg*>(resp1->data());
    ASSERT_EQ(decode_pldm_base_multipart_receive_resp(
                  rmsg1, resp1->size() - hdrSize, &mresp1, &checksum1),
              0);
    uint32_t next_handle = mresp1.next_transfer_handle;

    auto req2 = make_multipart_receive_req(PLDM_XFER_NEXT_PART, fdesc,
                                           next_handle, 2, 2);
    auto resp2 = dispatch(fd, req2);
    ASSERT_TRUE(resp2.has_value());

    struct pldm_base_multipart_receive_resp mresp2{};
    uint32_t checksum2 = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* rmsg2 = reinterpret_cast<const struct pldm_msg*>(resp2->data());
    ASSERT_EQ(decode_pldm_base_multipart_receive_resp(
                  rmsg2, resp2->size() - hdrSize, &mresp2, &checksum2),
              0);
    EXPECT_EQ(mresp2.transfer_flag,
              PLDM_BASE_MULTIPART_RECEIVE_TRANSFER_FLAG_END);
    EXPECT_EQ(mresp2.next_transfer_handle, 0u);
    ASSERT_EQ(mresp2.data.length, 1u);
}

TEST_F(FdTest, DfReadNextPartWithStaleHandleRejected)
{
    uint16_t fdesc = openFile(1);
    g_file.read_data = {1, 2, 3, 4};

    auto req1 =
        make_multipart_receive_req(PLDM_XFER_FIRST_PART, fdesc, 0, 0, 2);
    auto resp1 = dispatch(fd, req1);
    ASSERT_TRUE(resp1.has_value());

    auto req2 = make_multipart_receive_req(PLDM_XFER_NEXT_PART, fdesc,
                                           0xdeadbeef, 2, 2);
    auto resp2 = dispatch(fd, req2);
    ASSERT_TRUE(resp2.has_value());
    EXPECT_EQ(response_cc(*resp2), PLDM_ERROR_INVALID_DATA);
}

TEST_F(FdTest, DfReadNextPartWithoutFirstPartRejected)
{
    uint16_t fdesc = openFile(1);

    auto req = make_multipart_receive_req(PLDM_XFER_NEXT_PART, fdesc, 1, 0, 2);
    auto resp = dispatch(fd, req);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_ERROR_INVALID_DATA);
}

TEST_F(FdTest, DfReadUnknownTransferCtxRejected)
{
    auto req =
        make_multipart_receive_req(PLDM_XFER_FIRST_PART, 0xbeef, 0, 0, 2);
    auto resp = dispatch(fd, req);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_ERROR_INVALID_DATA);
}

TEST_F(FdTest, DfReadWrongPldmTypeReturnsNomsg)
{
    uint16_t fdesc = openFile(1);

    struct pldm_base_multipart_receive_req mreq{};
    mreq.pldm_type = PLDM_BASE; /* not PLDM_FILE */
    mreq.transfer_opflag = PLDM_XFER_FIRST_PART;
    mreq.transfer_ctx = fdesc;
    mreq.transfer_handle = 0;
    mreq.section_offset = 0;
    mreq.section_length = 2;

    size_t plen = 512;
    std::vector<uint8_t> buf(hdrSize + plen);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* msg = reinterpret_cast<struct pldm_msg*>(buf.data());
    struct pldm_header_info hdrinfo{};
    hdrinfo.msg_type = PLDM_REQUEST;
    hdrinfo.instance = TEST_IID;
    hdrinfo.pldm_type = PLDM_BASE;
    hdrinfo.command = PLDM_MULTIPART_RECEIVE;
    pack_pldm_header(&hdrinfo, &msg->hdr);
    ASSERT_EQ(
        encode_pldm_base_multipart_receive_req(TEST_IID, &mreq, msg, &plen), 0);
    buf.resize(hdrSize + plen);

    std::vector<uint8_t> out(512);
    size_t out_len = out.size();
    int rc = pldm_file_fd_handle_msg(fd, buf.data(), buf.size(), out.data(),
                                     &out_len);
    EXPECT_EQ(rc, -ENOMSG);
}

TEST_F(FdTest, DfReadCallbackFailurePropagatesCc)
{
    uint16_t fdesc = openFile(1);
    g_file.read_cc = PLDM_FILE_CC_INVALID_FILE_DESCRIPTOR;

    auto req = make_multipart_receive_req(PLDM_XFER_FIRST_PART, fdesc, 0, 0, 2);
    auto resp = dispatch(fd, req);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_FILE_CC_INVALID_FILE_DESCRIPTOR);
}

TEST_F(FdTest, DfReadRespectsSectionLength)
{
    uint16_t fdesc = openFile(1);
    g_file.read_data = std::vector<uint8_t>(100, 0x7);

    auto req =
        make_multipart_receive_req(PLDM_XFER_FIRST_PART, fdesc, 0, 0, 10);
    auto resp = dispatch(fd, req);
    ASSERT_TRUE(resp.has_value());

    struct pldm_base_multipart_receive_resp mresp{};
    uint32_t checksum = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* rmsg = reinterpret_cast<const struct pldm_msg*>(resp->data());
    ASSERT_EQ(decode_pldm_base_multipart_receive_resp(
                  rmsg, resp->size() - hdrSize, &mresp, &checksum),
              0);
    EXPECT_EQ(mresp.data.length, 10u);
}

/* ===================================================================
 * 6. Dispatch edge cases
 * =================================================================== */

TEST_F(FdTest, UnknownCommand)
{
    auto req = make_request(PLDM_FILE, 0xfe, nullptr, 0);
    auto resp = dispatch(fd, req);
    ASSERT_TRUE(resp.has_value());
    EXPECT_EQ(response_cc(*resp), PLDM_ERROR_UNSUPPORTED_PLDM_CMD);
}

TEST_F(FdTest, WrongPldmType)
{
    auto req = make_request(PLDM_PLATFORM, PLDM_FILE_CMD_DF_OPEN, nullptr, 0);
    std::vector<uint8_t> out(512);
    size_t out_len = out.size();
    int rc = pldm_file_fd_handle_msg(fd, req.data(), req.size(), out.data(),
                                     &out_len);
    EXPECT_EQ(rc, -ENOMSG);
}

TEST_F(FdTest, ResponseMessageRejected)
{
    std::vector<uint8_t> msg(hdrSize);
    struct pldm_header_info hdr{};
    hdr.msg_type = PLDM_RESPONSE;
    hdr.instance = TEST_IID;
    hdr.pldm_type = PLDM_FILE;
    hdr.command = PLDM_FILE_CMD_DF_OPEN;
    struct pldm_msg_hdr msghdr{};
    pack_pldm_header(&hdr, &msghdr);
    memcpy(msg.data(), &msghdr, hdrSize);

    std::vector<uint8_t> out(512);
    size_t out_len = out.size();
    int rc = pldm_file_fd_handle_msg(fd, msg.data(), msg.size(), out.data(),
                                     &out_len);
    EXPECT_EQ(rc, -EPROTO);
}

TEST_F(FdTest, NullPointers)
{
    std::vector<uint8_t> buf(512);
    size_t out_len = buf.size();

    EXPECT_EQ(
        pldm_file_fd_handle_msg(nullptr, buf.data(), 1, buf.data(), &out_len),
        -EINVAL);
    EXPECT_EQ(pldm_file_fd_handle_msg(fd, nullptr, 1, buf.data(), &out_len),
              -EINVAL);
    EXPECT_EQ(pldm_file_fd_handle_msg(fd, buf.data(), 1, nullptr, &out_len),
              -EINVAL);
    EXPECT_EQ(pldm_file_fd_handle_msg(fd, buf.data(), 1, buf.data(), nullptr),
              -EINVAL);
}

TEST_F(FdTest, ShortInput)
{
    std::vector<uint8_t> buf(512);
    size_t out_len = buf.size();
    EXPECT_EQ(pldm_file_fd_handle_msg(fd, buf.data(), hdrSize - 1, buf.data(),
                                      &out_len),
              -EOVERFLOW);
}

TEST_F(FdTest, TinyOutputBuffer)
{
    auto req = make_request(PLDM_FILE, PLDM_FILE_CMD_DF_OPEN, nullptr, 0);
    std::vector<uint8_t> out(hdrSize); /* room for header but no cc byte */
    size_t out_len = out.size();
    EXPECT_EQ(pldm_file_fd_handle_msg(fd, req.data(), req.size(), out.data(),
                                      &out_len),
              -EOVERFLOW);
}

TEST_F(FdTest, AsyncNotifyRejected)
{
    std::vector<uint8_t> msg(hdrSize);
    struct pldm_header_info hdr{};
    hdr.msg_type = PLDM_ASYNC_REQUEST_NOTIFY;
    hdr.instance = TEST_IID;
    hdr.pldm_type = PLDM_FILE;
    hdr.command = PLDM_FILE_CMD_DF_OPEN;
    struct pldm_msg_hdr msghdr{};
    pack_pldm_header(&hdr, &msghdr);
    memcpy(msg.data(), &msghdr, hdrSize);

    std::vector<uint8_t> out(512);
    size_t out_len = out.size();
    int rc = pldm_file_fd_handle_msg(fd, msg.data(), msg.size(), out.data(),
                                     &out_len);
    EXPECT_EQ(rc, -EPROTO);
}

/* ===================================================================
 * 7. pldm_control integration
 * =================================================================== */

TEST(FdControl, RegistersFileType)
{
    alignas(8) uint8_t ctrl_buf[PLDM_SIZEOF_PLDM_CONTROL];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* control = reinterpret_cast<pldm_control*>(ctrl_buf);
    ASSERT_EQ(pldm_control_setup(control, PLDM_SIZEOF_PLDM_CONTROL), 0);

    struct pldm_file_fd_ops ops = {
        .ctx = &g_file,
        .open = cb_open,
        .close = cb_close,
        .read = cb_read,
        .heartbeat = nullptr,
    };
    struct pldm_file_fd* fd = pldm_file_fd_new(2, &ops, sizeof(ops), control);
    ASSERT_NE(fd, nullptr);

    std::vector<uint8_t> req(hdrSize);
    struct pldm_header_info hdr{};
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = 0;
    hdr.pldm_type = PLDM_BASE;
    hdr.command = PLDM_GET_PLDM_TYPES;
    struct pldm_msg_hdr msghdr{};
    pack_pldm_header(&hdr, &msghdr);
    memcpy(req.data(), &msghdr, hdrSize);

    std::vector<uint8_t> resp(512);
    size_t resp_len = resp.size();
    int rc = pldm_control_handle_msg(control, req.data(), req.size(),
                                     resp.data(), &resp_len);
    EXPECT_EQ(rc, 0);
    EXPECT_GT(resp_len, hdrSize + 1u);

    uint8_t cc = resp[hdrSize];
    EXPECT_EQ(cc, PLDM_SUCCESS);

    /* PLDM_FILE == 0x07 -> byte 0 bit 7 */
    uint8_t types_byte0 = resp[hdrSize + 1];
    EXPECT_NE(types_byte0 & (1u << PLDM_FILE), 0u);

    free(fd);
}
