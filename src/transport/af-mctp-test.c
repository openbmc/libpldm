/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include "af-mctp-internal.h"
#include "af-mctp-test.h"

#include <libpldm/api.h>
#include <libpldm/transport/af-mctp.h>

#include <errno.h>
#include <stdlib.h>

LIBPLDM_ABI_TESTING
int pldm_transport_af_mctp_test_init(struct pldm_transport_af_mctp **ctx)
{
	if (!ctx || *ctx) {
		return -EINVAL;
	}

	struct pldm_transport_af_mctp *tmp = calloc(1, sizeof(*tmp));
	if (!tmp) {
		return -ENOMEM;
	}

	*ctx = tmp;
	return 0;
}

LIBPLDM_ABI_TESTING
void pldm_transport_af_mctp_test_destroy(struct pldm_transport_af_mctp *ctx)
{
	free(ctx);
}

LIBPLDM_ABI_TESTING
int pldm_transport_af_mctp_test_lookup_tid(struct pldm_transport_af_mctp *ctx,
					   uint32_t network, mctp_eid_t eid,
					   pldm_tid_t *tid)
{
	return pldm_transport_af_mctp_get_tid(ctx, network, eid, tid);
}
