/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

/* Fuzzing should always have assertions */
#ifdef NDEBUG
#undef NDEBUG
#endif

#include "msgbuf.hpp"

#include <libpldm/base.h>
#include <libpldm/pdr.h>
#include <libpldm/platform.h>
#include <libpldm/platform_device.h>
#include <libpldm/sizes.h>
#include <unistd.h>

#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

static const uint32_t MAX_SEND = 1024;
static const uint32_t MAX_PART = 200;
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
    struct pldm_msgbuf_ro* fuzz_ctrl;
};

static bool fuzz_chance(struct fuzz_ops_ctx* ctx, uint8_t percent)
{
    uint8_t v = 0;
    assert(percent <= 100);
    int rc = pldm_msgbuf_extract_uint8(ctx->fuzz_ctrl, v);
    if (rc != 0)
    {
        return false;
    }
    uint8_t cutoff = (uint32_t)percent * UINT8_MAX / 100;
    return v <= cutoff;
}

static uint8_t cb_get_sensor_reading(
    void* ctx, const pldm_numeric_sensor_value_pdr* /*pdr*/,
    bool8_t /*rearm_event_state*/, struct pldm_platform_pd_sensor_state* state)
{
    debug_printf("cb_get_sensor_reading\n");
    auto* fuzz_ctx = static_cast<fuzz_ops_ctx*>(ctx);

    if (fuzz_chance(fuzz_ctx, 4))
    {
        return PLDM_PLATFORM_INVALID_SENSOR_ID;
    }

    state->operational_state = PLDM_SENSOR_ENABLED;
    state->event_enable = PLDM_NO_EVENT_GENERATION;
    state->present_state = 0;
    state->previous_state = 0;
    state->event_state = 0;
    memset(&state->present_reading, 0, sizeof(state->present_reading));
    state->present_reading.value_u8 = 42;
    return PLDM_SUCCESS;
}

/* Small dummy PDR records to give GET_PDR something to return */
static const uint8_t dummy_pdr_data[] = {
    0x00, 0x00, 0x00, 0x00, /* record_handle (filled by pldm_pdr_add) */
    0x01,                   /* version */
    0x01,                   /* type (TERMINUS_LOCATOR) */
    0x00, 0x00,             /* record_change_num */
    0x00, 0x00,             /* length */
};

static pldm_pdr* build_pdr(void)
{
    pldm_pdr* repo = pldm_pdr_init();
    if (!repo)
    {
        return nullptr;
    }

    /* Add two dummy records */
    uint32_t h = 0;
    pldm_pdr_add(repo, dummy_pdr_data, sizeof(dummy_pdr_data), false, 0, &h);
    h = 0;
    pldm_pdr_add(repo, dummy_pdr_data, sizeof(dummy_pdr_data), false, 0, &h);

    /* Add a minimal numeric sensor PDR for sensor_id=1 so the fuzzer
     * exercises both the PDR-found and PDR-not-found code paths */
    static const uint8_t numeric_sensor_pdr[] = {
        0x00,
        0x00,
        0x00,
        0x00,                    /* record_handle */
        0x01,                    /* version */
        PLDM_NUMERIC_SENSOR_PDR, /* type */
        0x00,
        0x00, /* record_change_num */
        PLDM_PDR_NUMERIC_SENSOR_PDR_MIN_LENGTH,
        0x00, /* length */
        0x00,
        0x00, /* terminus_handle */
        0x01,
        0x00, /* sensor_id = 1 */
        0x00,
        0x00, /* entity_type */
        0x01,
        0x00, /* entity_instance_num */
        0x00,
        0x00,                        /* container_id */
        PLDM_NO_INIT,                /* sensor_init */
        0x00,                        /* sensor_auxiliary_names_pdr */
        0x00,                        /* base_unit */
        0x00,                        /* unit_modifier */
        0x00,                        /* rate_unit */
        0x00,                        /* base_oem_unit_handle */
        0x00,                        /* aux_unit */
        0x00,                        /* aux_unit_modifier */
        0x00,                        /* aux_rate_unit */
        0x00,                        /* rel */
        0x00,                        /* aux_oem_unit_handle */
        0x01,                        /* is_linear */
        PLDM_SENSOR_DATA_SIZE_UINT8, /* sensor_data_size */
        0x00,
        0x00,
        0x00,
        0x00, /* resolution */
        0x00,
        0x00,
        0x00,
        0x00, /* offset */
        0x00,
        0x00, /* accuracy */
        0x00, /* plus_tolerance */
        0x00, /* minus_tolerance */
        0x00, /* hysteresis */
        0x00, /* supported_thresholds */
        0x00, /* threshold_and_hysteresis_volatility */
        0x00,
        0x00,
        0x00,
        0x00, /* state_transition_interval */
        0x00,
        0x00,
        0x00,
        0x00,                          /* update_interval */
        0xff,                          /* max_readable */
        0x00,                          /* min_readable */
        PLDM_RANGE_FIELD_FORMAT_UINT8, /* range_field_format */
        0x00,                          /* range_field_support */
        0x00,
        0x00,
        0x00,
        0x00,
        0x00, /* nominal/normal range */
        0x00,
        0x00,
        0x00,
        0x00, /* warning range */
        0x00,
        0x00, /* critical range */
        0x00,
        0x00, /* fatal range */
    };
    h = 0;
    pldm_pdr_add(repo, numeric_sensor_pdr, sizeof(numeric_sensor_pdr), false, 0,
                 &h);

    return repo;
}

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
    printf_enabled = getenv("TRACEPD");
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t* input, size_t len)
{
    PLDM_MSGBUF_RO_DEFINE_P(fuzzproto);
    PLDM_MSGBUF_RO_DEFINE_P(fuzzctrl);
    int rc;

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
    ops_ctx->fuzz_ctrl = fuzzctrl;

    pldm_pdr* pdr = build_pdr();
    assert(pdr);

    struct pldm_platform_pd* pd = pldm_platform_pd_new(pdr, nullptr);
    assert(pd);
    pldm_platform_pd_set_sensor_ops(pd, cb_get_sensor_reading, ops_ctx.get());

    while (true)
    {
        uint32_t send_len = 0;
        rc = pldm_msgbuf_extract_uint32(fuzzctrl, send_len);
        if (rc)
        {
            break;
        }
        send_len %= (MAX_SEND + 1);
        std::vector<uint8_t> send_buf(send_len);

        uint32_t part_len;
        rc = pldm_msgbuf_extract_uint32(fuzzproto, part_len);
        if (rc)
        {
            break;
        }
        part_len %= (MAX_PART + 1);
        std::vector<uint8_t> part_buf(part_len);
        rc = pldm_msgbuf_extract_array_uint8(fuzzproto, part_len,
                                             part_buf.data(), part_buf.size());
        if (rc != 0)
        {
            break;
        }

        size_t out_len = send_buf.size();
        pldm_platform_pd_handle_msg(pd, part_buf.data(), part_buf.size(),
                                    send_buf.data(), &out_len);
        assert(out_len <= send_buf.size());
    }

    rc = pldm_msgbuf_discard(fuzzproto, rc);
    rc = pldm_msgbuf_discard(fuzzctrl, rc);
    (void)rc;

    free(pd);
    pldm_pdr_destroy(pdr);
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
