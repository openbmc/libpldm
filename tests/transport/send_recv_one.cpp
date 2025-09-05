#include <libpldm/transport.h>

#include "array.h"
#include "environ/time.h"
#include "transport/test.h"

#include <gtest/gtest.h>

extern "C" {
long global_base_time = 300;
int libpldm_clock_gettime(struct timespec* ts)
{
    static struct timespec init_offset{};
    int rc;

    rc = clock_gettime(CLOCK_MONOTONIC, ts);
    if (rc < 0)
    {
        return rc;
    }
    if (init_offset.tv_sec == 0)
    {
        init_offset = *ts;
    }
    /* Adjust the time such that the start of the test
     * begins at an artificial global_base_time which can
     * be controlled from any of the TEST() methods */
    ts->tv_sec = ts->tv_sec - init_offset.tv_sec + global_base_time;
    return 0;
}
}

TEST(Transport, send_recv_one)
{
    /* To test the case when timestamp is closer to the 28 day
     * uptime we would potentially set this to something closer
     * to 2589793. But unfortunatley, the systems where the
     * unit tests would be run, could have long be a 64 bit
     * integer thus would pass with this anyway but fail on
     * a standard BMC 32bit SOC. Hence workaround this by
     * using `LONG_MAX - 10` which would fail on either
     * condition. */
    global_base_time = LONG_MAX - 10;
    uint8_t req[] = {0x81, 0x00, 0x01, 0x01};
    uint8_t resp[] = {0x01, 0x00, 0x01, 0x00};
    const struct pldm_transport_test_descriptor seq[] = {
        {
            .type = PLDM_TRANSPORT_TEST_ELEMENT_MSG_SEND,
            .send_msg =
                {
                    .dst = 1,
                    .msg = req,
                    .len = sizeof(req),
                },
        },
        {
            .type = PLDM_TRANSPORT_TEST_ELEMENT_LATENCY,
            .latency =
                {
                    .it_interval = {0, 0},
                    .it_value = {1, 0},
                },
        },
        {
            .type = PLDM_TRANSPORT_TEST_ELEMENT_MSG_RECV,
            .recv_msg =
                {
                    .src = 1,
                    .msg = resp,
                    .len = sizeof(resp),
                },
        },
    };
    struct pldm_transport_test* test = NULL;
    struct pldm_transport* ctx;
    size_t len;
    void* msg;
    int rc;

    EXPECT_EQ(pldm_transport_test_init(&test, seq, ARRAY_SIZE(seq)), 0);
    ctx = pldm_transport_test_core(test);
    rc = pldm_transport_send_recv_msg(ctx, 1, req, sizeof(req), &msg, &len);
    EXPECT_EQ(rc, PLDM_REQUESTER_SUCCESS);
    EXPECT_EQ(len, sizeof(resp));
    EXPECT_EQ(memcmp(msg, resp, len), 0);
    free(msg);
    pldm_transport_test_destroy(test);
}
