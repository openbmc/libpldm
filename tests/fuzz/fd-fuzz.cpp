/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

/* Fuzzing should always have assertions */
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <libpldm/base.h>
#include <libpldm/firmware_fd.h>
#include <libpldm/firmware_update.h>
#include <libpldm/sizes.h>

#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "array.h"
#include "msgbuf.h"

/* Avoid out-of-memory, and
 * avoid wasting time on inputs larger than MCTP message limits */
static const uint32_t MAX_PART = 200;

/* Maximum "send" buffer. Should be larger than any expected sent message */
static const uint32_t MAX_SEND = 1024;

/* Arbitrary EID */
static const uint8_t FIXED_ADDR = 20;

static const uint8_t PROGRESS_PERCENT = 5;

static const ssize_t FUZZCTRL_SIZE = 0x400;

static bool printf_enabled;
// NOLINTNEXTLINE(cert-dcl50-cpp)
static void debug_printf(const char* fmt, ...)
{
    if (printf_enabled)
    {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }
}

struct fuzz_ops_ctx
{
    struct pldm_msgbuf* fuzz_ctrl;

    /* Details of in-progress update, for consistency checking */
    bool current_update;
    struct pldm_firmware_update_component update_comp;
    uint32_t offset;
    bool transferred;
    bool verified;
    bool applied;

    uint64_t now;
};

/* Returns true with roughly `percent` chance */
static bool fuzz_chance(struct fuzz_ops_ctx* ctx, uint8_t percent)
{
    uint8_t v;
    assert(percent <= 100);
    int rc = pldm_msgbuf_extract_uint8(ctx->fuzz_ctrl, v);
    if (rc != 0)
    {
        return false;
    }
    uint8_t cutoff = (uint32_t)percent * UINT8_MAX / 100;
    return v <= cutoff;
}

// openbmc 49871
static const uint8_t openbmc_iana[] = {
    0xcf,
    0xc2,
    0x00,
    0x00,
};

/* An arbitrary but valid set of descriptors.
 * Short to be readily discoverable by fuzzing */
static const pldm_descriptor FIXED_DESCRIPTORS[] = {
    {
        .descriptor_type = PLDM_FWUP_IANA_ENTERPRISE_ID,
        .descriptor_length = 4,
        .descriptor_data = openbmc_iana,
    },
};

static int cb_device_identifiers(void* ctx LIBPLDM_CC_UNUSED,
                                 uint8_t* descriptors_count,
                                 const struct pldm_descriptor** descriptors)
{
    debug_printf("cb_device_identifiers\n");
    *descriptors_count = 1;
    *descriptors = FIXED_DESCRIPTORS;
    return 0;
}

static const struct pldm_firmware_component_standalone comp = {
    .comp_classification = PLDM_COMP_UNKNOWN,
    .comp_identifier = 0,
    .comp_classification_index = 0,
    .active_ver =
        {
            .comparison_stamp = 1,
            .str =
                {
                    .str_type = PLDM_STR_TYPE_UTF_8,
                    .str_len = 3,
                    .str_data = "zzz",
                },
            .date = {0},
        },
    .pending_ver =
        {
            .comparison_stamp = 1,
            .str =
                {
                    .str_type = PLDM_STR_TYPE_UNKNOWN,
                    .str_len = 4,
                    .str_data = "fnnn",
                },
            .date = {0},
        },
    .comp_activation_methods = {0},
    .capabilities_during_update = {0},
};

static const struct pldm_firmware_component_standalone* comp_list[1] = {
    &comp,
};

static int cb_components(
    void* ctx, uint16_t* ret_entry_count,
    const struct pldm_firmware_component_standalone*** ret_entries)
{
    debug_printf("cb_components\n");
    struct fuzz_ops_ctx* fuzz_ctx = static_cast<struct fuzz_ops_ctx*>(ctx);

    *ret_entry_count = ARRAY_SIZE(comp_list);
    *ret_entries = comp_list;
    if (fuzz_chance(fuzz_ctx, 4))
    {
        return -EINVAL;
    }
    return 0;
}

static int cb_imageset_versions(void* ctx, struct pldm_firmware_string* active,
                                struct pldm_firmware_string* pending)
{
    debug_printf("cb_imageset_versions\n");
    struct fuzz_ops_ctx* fuzz_ctx = static_cast<struct fuzz_ops_ctx*>(ctx);

    active->str_type = PLDM_STR_TYPE_ASCII;
    active->str_len = 4;
    memcpy(active->str_data, "1234", 4);
    pending->str_type = PLDM_STR_TYPE_ASCII;
    pending->str_len = 4;
    memcpy(pending->str_data, "1235", 4);
    if (fuzz_chance(fuzz_ctx, 4))
    {
        return -EINVAL;
    }
    return 0;
}

static enum pldm_component_response_codes
    cb_update_component(void* ctx, bool update,
                        const struct pldm_firmware_update_component* comp)
{
    debug_printf("cb_update_component update=%d\n", update);
    struct fuzz_ops_ctx* fuzz_ctx = static_cast<struct fuzz_ops_ctx*>(ctx);

    if (fuzz_chance(fuzz_ctx, 4))
    {
        return PLDM_CRC_COMP_PREREQUISITES_NOT_MET;
    }
    if (update)
    {
        /* Set up a new update */
        assert(!fuzz_ctx->current_update);
        debug_printf("cb_update_component set current_update=true\n");
        fuzz_ctx->current_update = true;
        fuzz_ctx->transferred = false;
        fuzz_ctx->verified = false;
        fuzz_ctx->applied = false;
        fuzz_ctx->offset = 0;
        memcpy(&fuzz_ctx->update_comp, comp, sizeof(*comp));
    }
    return PLDM_CRC_COMP_CAN_BE_UPDATED;
}

static uint32_t cb_transfer_size(void* ctx, uint32_t ua_max_transfer_size)
{
    debug_printf("cb_transfer_size ua_size=%zu\n",
                 (ssize_t)ua_max_transfer_size);
    struct fuzz_ops_ctx* fuzz_ctx = static_cast<struct fuzz_ops_ctx*>(ctx);

    if (fuzz_chance(fuzz_ctx, 50))
    {
        // Sometimes adjust it
        return MAX_PART - 20;
    }
    return ua_max_transfer_size;
}

static uint8_t
    cb_firmware_data(void* ctx, uint32_t offset,
                     const uint8_t* data LIBPLDM_CC_UNUSED, uint32_t len,
                     const struct pldm_firmware_update_component* comp)
{
    debug_printf("cb_firmware_data offset=%zu len %zu\n", (size_t)offset,
                 (size_t)len);
    struct fuzz_ops_ctx* fuzz_ctx = static_cast<struct fuzz_ops_ctx*>(ctx);

    assert(fuzz_ctx->current_update);
    assert(!fuzz_ctx->transferred);
    assert(!fuzz_ctx->verified);
    assert(!fuzz_ctx->applied);
    assert(offset == fuzz_ctx->offset);
    fuzz_ctx->offset += len;
    assert(fuzz_ctx->offset <= fuzz_ctx->update_comp.comp_image_size);
    assert(memcmp(comp, &fuzz_ctx->update_comp, sizeof(*comp)) == 0);

    if (fuzz_ctx->offset == fuzz_ctx->update_comp.comp_image_size)
    {
        fuzz_ctx->transferred = true;
    }

    if (fuzz_chance(fuzz_ctx, 2))
    {
        return PLDM_FWUP_TRANSFER_ERROR_IMAGE_CORRUPT;
    }
    return PLDM_FWUP_TRANSFER_SUCCESS;
}

static uint8_t cb_verify(void* ctx,
                         const struct pldm_firmware_update_component* comp,
                         bool* ret_pending,
                         uint8_t* ret_percent_complete LIBPLDM_CC_UNUSED)
{
    debug_printf("cb_verify\n");
    struct fuzz_ops_ctx* fuzz_ctx = static_cast<struct fuzz_ops_ctx*>(ctx);

    assert(fuzz_ctx->current_update);
    assert(fuzz_ctx->transferred);
    assert(!fuzz_ctx->verified);
    assert(!fuzz_ctx->applied);
    assert(memcmp(comp, &fuzz_ctx->update_comp, sizeof(*comp)) == 0);

    if (fuzz_chance(fuzz_ctx, 5))
    {
        debug_printf("cb_verify set failure\n");
        return PLDM_FWUP_VERIFY_ERROR_VERSION_MISMATCH;
    }

    if (fuzz_chance(fuzz_ctx, 50))
    {
        debug_printf("cb_verify set ret_pending=true\n");
        *ret_pending = true;
    }
    else
    {
        fuzz_ctx->verified = true;
    }

    return PLDM_SUCCESS;
}

static uint8_t cb_apply(void* ctx,
                        const struct pldm_firmware_update_component* comp,
                        bool* ret_pending,
                        uint8_t* ret_percent_complete LIBPLDM_CC_UNUSED)
{
    debug_printf("cb_apply\n");
    struct fuzz_ops_ctx* fuzz_ctx = static_cast<struct fuzz_ops_ctx*>(ctx);

    assert(fuzz_ctx->current_update);
    assert(fuzz_ctx->transferred);
    assert(fuzz_ctx->verified);
    assert(!fuzz_ctx->applied);
    assert(memcmp(comp, &fuzz_ctx->update_comp, sizeof(*comp)) == 0);

    if (fuzz_chance(fuzz_ctx, 5))
    {
        debug_printf("cb_apply set failure\n");
        return PLDM_FWUP_APPLY_FAILURE_MEMORY_ISSUE;
    }

    if (fuzz_chance(fuzz_ctx, 50))
    {
        debug_printf("cb_apply set ret_pending=true\n");
        *ret_pending = true;
    }
    else
    {
        debug_printf("cb_apply set current_update=false\n");
        fuzz_ctx->current_update = false;
        fuzz_ctx->applied = true;
    }

    return PLDM_SUCCESS;
}

static uint8_t cb_activate(void* ctx, bool self_contained LIBPLDM_CC_UNUSED,
                           uint16_t* ret_estimated_time LIBPLDM_CC_UNUSED)
{
    debug_printf("cb_activate\n");
    struct fuzz_ops_ctx* fuzz_ctx = static_cast<struct fuzz_ops_ctx*>(ctx);

    assert(!fuzz_ctx->current_update);
    if (fuzz_chance(fuzz_ctx, 5))
    {
        return PLDM_ERROR;
    }
    return PLDM_SUCCESS;
}

static void cb_cancel_update_component(
    void* ctx, const struct pldm_firmware_update_component* comp)
{
    debug_printf("cb_cancel_update_component\n");
    struct fuzz_ops_ctx* fuzz_ctx = static_cast<struct fuzz_ops_ctx*>(ctx);

    assert(fuzz_ctx->current_update);
    assert(fuzz_ctx->offset <= fuzz_ctx->update_comp.comp_image_size);
    assert(memcmp(comp, &fuzz_ctx->update_comp, sizeof(*comp)) == 0);
    fuzz_ctx->current_update = false;
}

static uint64_t cb_now(void* ctx)
{
    struct fuzz_ops_ctx* fuzz_ctx = static_cast<struct fuzz_ops_ctx*>(ctx);

    // Arbitrary 3s increment. FD code has a 1s retry timeout.
    fuzz_ctx->now += 3000;
    return fuzz_ctx->now;
}

static const struct pldm_fd_ops fuzz_ops = {
    .device_identifiers = cb_device_identifiers,
    .components = cb_components,
    .imageset_versions = cb_imageset_versions,
    .update_component = cb_update_component,
    .transfer_size = cb_transfer_size,
    .firmware_data = cb_firmware_data,
    .verify = cb_verify,
    .apply = cb_apply,
    .activate = cb_activate,
    .cancel_update_component = cb_cancel_update_component,
    .now = cb_now,
};

extern "C" int LLVMFuzzerInitialize(int* argc LIBPLDM_CC_UNUSED,
                                    char*** argv LIBPLDM_CC_UNUSED)
{
    printf_enabled = getenv("TRACEFWFD");
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t* input, size_t len)
{
    PLDM_MSGBUF_DEFINE_P(fuzzproto);
    PLDM_MSGBUF_DEFINE_P(fuzzctrl);
    int rc;

    /* Split input into two parts. First FUZZCTRL_SIZE (0x400 bytes currently)
     * is used for fuzzing control (random choices etc).
     * The remainder is a PLDM packet stream, of length:data */
    if (len < FUZZCTRL_SIZE)
    {
        return 0;
    }
    size_t proto_size = len - FUZZCTRL_SIZE;
    rc = pldm_msgbuf_init_errno(fuzzctrl, 0, input, FUZZCTRL_SIZE);
    assert(rc == 0);
    rc =
        pldm_msgbuf_init_errno(fuzzproto, 0, &input[FUZZCTRL_SIZE], proto_size);
    assert(rc == 0);

    auto ops_ctx = std::make_unique<fuzz_ops_ctx>();
    memset(ops_ctx.get(), 0x0, sizeof(fuzz_ops_ctx));
    /* callbacks may consume bytes from the fuzz control */
    ops_ctx->fuzz_ctrl = fuzzctrl;

    struct pldm_fd* fd = pldm_fd_new(&fuzz_ops, ops_ctx.get(), NULL);
    assert(fd);

    while (true)
    {
        /* Arbitrary length send buffer */
        uint32_t send_len;
        rc = pldm_msgbuf_extract_uint32(fuzzctrl, send_len);
        if (rc)
        {
            break;
        }
        send_len %= (MAX_SEND + 1);
        std::vector<uint8_t> send_buf(send_len);

        size_t len = send_buf.size();
        /* Either perform pldm_fd_handle_msg() or pldm_fd_progress() */
        if (fuzz_chance(ops_ctx.get(), PROGRESS_PERCENT))
        {
            uint8_t address = FIXED_ADDR;
            pldm_fd_progress(fd, send_buf.data(), &len, &address);
        }
        else
        {
            uint32_t part_len;
            rc = pldm_msgbuf_extract_uint32(fuzzproto, part_len);
            if (rc)
            {
                break;
            }
            part_len = std::min(part_len, MAX_PART);
            /* Fresh allocation so that ASAN will notice overflow reads */
            std::vector<uint8_t> part_buf(part_len);
            rc = pldm_msgbuf_extract_array_uint8(
                fuzzproto, part_len, part_buf.data(), part_buf.size());
            if (rc != 0)
            {
                break;
            }
            pldm_fd_handle_msg(fd, FIXED_ADDR, part_buf.data(), part_buf.size(),
                               send_buf.data(), &len);
        }
        assert(len <= send_buf.size());
    }
    rc = pldm_msgbuf_discard(fuzzproto, rc);
    rc = pldm_msgbuf_discard(fuzzctrl, rc);
    (void)rc;

    free(fd);
    return 0;
}

#ifdef HFND_FUZZING_ENTRY_FUNCTION
#define USING_HONGGFUZZ 1
#else
#define USING_HONGGFUZZ 0
#endif

#ifdef __AFL_FUZZ_TESTCASE_LEN
#define USING_AFL 1
#else
#define USING_AFL 0
#endif

#if USING_AFL
__AFL_FUZZ_INIT();
#endif

#if !USING_AFL && !USING_HONGGFUZZ
/* Let it build without AFL taking stdin instead */
static void run_standalone()
{
    while (true)
    {
        unsigned char buf[1024000];
        ssize_t len = read(STDIN_FILENO, buf, sizeof(buf));
        if (len <= 0)
        {
            break;
        }
        LLVMFuzzerTestOneInput(buf, len);
    }
}
#endif

#if !USING_HONGGFUZZ
int main(int argc, char** argv)
{
    LLVMFuzzerInitialize(&argc, &argv);

#if USING_AFL
    __AFL_INIT();
    uint8_t* buf = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(100000))
    {
        size_t len = __AFL_FUZZ_TESTCASE_LEN;
        LLVMFuzzerTestOneInput(buf, len);
    }
#else
    run_standalone();
#endif

    return 0;
}
#endif // !USING_HONGGFUZZ
