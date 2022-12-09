/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef _LIBPLDM_AF_MCTP_H
#define _LIBPLDM_AF_MCTP_H

struct transport_afmctp;
struct transport_afmctp *pldm_transport_afmctp_init();
struct pldm_transport *pldm_transport_afmctp_core(struct transport_afmctp *b);
#endif /* _LIBPLDM_AF MCTP*/
