/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_SRC_TRANSPORT_AF_MCTP_TEST_H
#define LIBPLDM_SRC_TRANSPORT_AF_MCTP_TEST_H

#include <libpldm/base.h>

#include <linux/mctp.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pldm_transport_af_mctp;

int pldm_transport_af_mctp_test_init(struct pldm_transport_af_mctp **ctx);
void pldm_transport_af_mctp_test_destroy(struct pldm_transport_af_mctp *ctx);
int pldm_transport_af_mctp_test_lookup_tid(struct pldm_transport_af_mctp *ctx,
					   uint32_t network, mctp_eid_t eid,
					   pldm_tid_t *tid);

#ifdef __cplusplus
}
#endif

#endif // LIBPLDM_SRC_TRANSPORT_AF_MCTP_TEST_H
