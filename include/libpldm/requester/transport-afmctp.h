/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef _LIBPLDM_AF_MCTP_H
#define _LIBPLDM_AF_MCTP_H

#ifdef __cplusplus
extern "C" {
#endif

struct pldm_transport_afmctp;

/* Init the transport backend */
struct pldm_transport_afmctp *pldm_transport_afmctp_init(void);

/* Destroy the transport backend */
void pldm_transport_afmctp_destroy(struct pldm_transport_afmctp *ctx);

/* Get the core pldm transport struct */
struct pldm_transport *
pldm_transport_afmctp_core(struct pldm_transport_afmctp *ctx);

#ifdef PLDM_HAS_POLL
struct pollfd;
/* Init pollfd for async calls */
int pldm_transport_afmctp_init_pollfd(struct pldm_transport_afmctp *ctx,
				      struct pollfd *pollfd);
#endif

/* Inserts a TID-to-EID mapping into the transport's device map */
int pldm_transport_afmctp_map_tid(struct pldm_transport_afmctp *ctx,
				  pldm_tid_t tid, mctp_eid_t eid);

/* Removes a TID-to-EID mapping from the transport's device map */
int pldm_transport_afmctp_unmap_tid(struct pldm_transport_afmctp *ctx,
				    pldm_tid_t tid, mctp_eid_t eid);

#ifdef __cplusplus
}
#endif

#endif /* _LIBPLDM_AF MCTP*/
