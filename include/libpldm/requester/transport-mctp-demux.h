/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_DEMUX_H
#define LIBPLDM_DEMUX_H

struct pldm_transport_mctp_demux;

/* Init the transport backend */
struct pldm_transport_mctp_demux *pldm_transport_mctp_demux_init(void);

/* Destroy the transport backend */
void pldm_transport_mctp_demux_destroy(struct pldm_transport_mctp_demux *ctx);

/* Get the core pldm transport struct */
struct pldm_transport *
pldm_transport_mctp_demux_core(struct pldm_transport_mctp_demux *ctx);

/* Inserts a TID-to-EID mapping into the transport's device map */
int pldm_transport_mctp_demux_map_tid(struct pldm_transport_mctp_demux *ctx,
				      pldm_tid_t tid, mctp_eid_t eid);

/* Removes a TID-to-EID mapping from the transport's device map */
int pldm_transport_mctp_demux_unmap_tid(struct pldm_transport_mctp_demux *ctx,
					pldm_tid_t tid, mctp_eid_t eid);

#endif // LIBPLDM_DEMUX_H
