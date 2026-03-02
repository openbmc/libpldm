/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_TRANSPORT_AF_MCTP_INTERNAL_H
#define LIBPLDM_TRANSPORT_AF_MCTP_INTERNAL_H

#include "responder.h"
#include "socket.h"
#include "transport.h"

#include <libpldm/base.h>

#include <linux/mctp.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !HAVE_STRUCT_MCTP_FQ_ADDR
struct mctp_fq_addr {
	unsigned int net;
	mctp_eid_t eid;
};
#endif

struct pldm_transport_af_mctp {
	struct pldm_transport transport;
	int socket;
	struct mctp_fq_addr tid_map[PLDM_MAX_TIDS];
	struct pldm_socket_sndbuf socket_send_buf;
	bool bound;
	struct pldm_responder_cookie cookie_jar;
};

struct pldm_responder_cookie_af_mctp {
	struct pldm_responder_cookie req;
	struct sockaddr_mctp smctp;
};

int pldm_transport_af_mctp_get_tid(struct pldm_transport_af_mctp *ctx,
				   uint32_t network, mctp_eid_t eid,
				   pldm_tid_t *tid);

#ifdef __cplusplus
}
#endif

#endif // LIBPLDM_TRANSPORT_AF_MCTP_INTERNAL_H
