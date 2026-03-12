/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_DEMUX_H
#define LIBPLDM_DEMUX_H

#include <libpldm/_abi_annotation.h>
#include <libpldm/base.h>
#include <libpldm/pldm.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pldm_transport_mctp_demux;

/* Init the transport backend */
LIBPLDM_ABI_STABLE
int pldm_transport_mctp_demux_init(struct pldm_transport_mctp_demux **ctx);

/* Destroy the transport backend */
LIBPLDM_ABI_STABLE
void pldm_transport_mctp_demux_destroy(struct pldm_transport_mctp_demux *ctx);

/* Get the core pldm transport struct */
LIBPLDM_ABI_STABLE
struct pldm_transport *
pldm_transport_mctp_demux_core(struct pldm_transport_mctp_demux *ctx);

struct pollfd;
/* Init pollfd for async calls */
LIBPLDM_ABI_STABLE
int pldm_transport_mctp_demux_init_pollfd(struct pldm_transport *t,
					  struct pollfd *pollfd);

/* Inserts a TID-to-EID mapping into the transport's device map */
LIBPLDM_ABI_STABLE
int pldm_transport_mctp_demux_map_tid(struct pldm_transport_mctp_demux *ctx,
				      pldm_tid_t tid, mctp_eid_t eid);

/* Removes a TID-to-EID mapping from the transport's device map */
LIBPLDM_ABI_STABLE
int pldm_transport_mctp_demux_unmap_tid(struct pldm_transport_mctp_demux *ctx,
					pldm_tid_t tid, mctp_eid_t eid);

#ifdef __cplusplus
}
#endif

#endif /* LIBPLDM_DEMUX_H */
