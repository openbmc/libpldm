/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef _LIBPLDM_DEMUX_H
#define _LIBPLDM_DEMUX_H

struct transport_demux;

void pldm_transport_demux_destroy(struct transport_demux *b);
struct transport_demux *pldm_transport_demux_init();
struct pldm_transport *pldm_transport_demux_core(struct transport_demux *b);

//temporary for old api
struct transport_demux *pldm_transport_demux_init_with_fd(int mctp_fd);


#endif //_LIBPLDM_DEMUX_H
