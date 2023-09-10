/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_AF_MCTP_H
#define LIBPLDM_AF_MCTP_H

#include "libpldm/base.h"
#include "libpldm/pldm.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pldm_transport_af_mctp;
struct pldm_responder_af_mctp;
struct pldm_responder;
struct sockaddr_mctp;

/* Init the transport backend */
int pldm_transport_af_mctp_init(struct pldm_transport_af_mctp **ctx);

/* Destroy the transport backend */
void pldm_transport_af_mctp_destroy(struct pldm_transport_af_mctp *ctx);

/* Get the core pldm transport struct */
struct pldm_transport *
pldm_transport_af_mctp_core(struct pldm_transport_af_mctp *ctx);

#ifdef PLDM_HAS_POLL
struct pollfd;
/* Init pollfd for async calls */
int pldm_transport_af_mctp_init_pollfd(struct pldm_transport *t,
				       struct pollfd *pollfd);
#endif

/* Inserts a TID-to-EID mapping into the transport's device map */
int pldm_transport_af_mctp_map_tid(struct pldm_transport_af_mctp *ctx,
				   pldm_tid_t tid, mctp_eid_t eid);

/* Removes a TID-to-EID mapping from the transport's device map */
int pldm_transport_af_mctp_unmap_tid(struct pldm_transport_af_mctp *ctx,
				     pldm_tid_t tid, mctp_eid_t eid);

/* Promote the transport to respond to requests */
int pldm_transport_af_mctp_bind(struct pldm_transport_af_mctp *transport,
				struct sockaddr_mctp *smctp, size_t len,
				struct pldm_responder_af_mctp **responder);

void pldm_responder_af_mctp_destroy(struct pldm_responder_af_mctp *responder);

struct pldm_responder *
pldm_responder_af_mctp_core(struct pldm_responder_af_mctp *responder);

#ifdef __cplusplus
}
#endif

#endif /* LIBPLDM_AF_MCTP*/
