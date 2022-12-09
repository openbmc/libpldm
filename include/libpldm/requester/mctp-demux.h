/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef _LIBPLDM_DEMUX_H
#define _LIBPLDM_DEMUX_H

struct pldm_transport_mctp_demux;

/* Init the transport backend */
struct pldm_transport_mctp_demux *pldm_transport_demux_init();

/* Destroy the transport backend */
void pldm_transport_demux_destroy(struct pldm_transport_mctp_demux *ctx);

/* Get the core pldm transport struct */
struct pldm_transport *
pldm_transport_demux_core(struct pldm_transport_mctp_demux *ctx);

/* Asks an PLDM-capable MCTP device for its TID(s) */
int pldm_transport_mctp_query_tid(struct pldm_transport_mctp_demux *ctx,
				  mctp_eid_t eid);

/* Inserts a TID-to-EID mapping into the transport's device map */
int pldm_transport_mctp_map_tid(struct pldm_transport_mctp_demux *ctx,
				pldm_tid_t tid, mctp_eid_t eid);

/* Removes a TID-to-EID mapping from the transport's device map */
int pldm_transport_mctp_unmap_tid(struct pldm_transport_mctp_demux *ctx,
				  pldm_tid_t tid, mctp_eid_t eid);

/* Assigns a PLDM TID to the device addressed by the provided MCTP EID and
 * updates the TID-to-EID mapping in the transport's device map */
int pldm_transport_mctp_assign_tid(struct pldm_transport_mctp_demux *ctx,
				   pldm_tid_t tid, mctp_eid_t eid);

/* temporary functions to support the transition from the old api to the new */

/* The old api allows for the user to setup a MCTP socket and then use the
 * library */
struct pldm_transport_mctp_demux *
pldm_transport_demux_init_with_fd(int mctp_fd);

/* The old api exposes the fd to the MCTP socket */
int pldm_transport_mctp_get_socket_fd(struct pldm_transport *transport);

#endif //_LIBPLDM_DEMUX_H
