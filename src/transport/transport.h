/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_SRC_TRANSPORT_TRANSPORT_H
#define LIBPLDM_SRC_TRANSPORT_TRANSPORT_H

#include <libpldm/base.h>
#include <libpldm/pldm.h>
#include <sys/time.h>

struct pollfd;

/**
 * @brief Return the default clock's current value.
 *
 * @out tv - The current tv_sec, tv_usec pair.
 * @return - 0 on success, negative error code on failure.
 */
int pldm_transport_get_timeval(struct timeval *tv);

/**
 * @brief Generic PLDM transport struct
 *
 * @var name - name of the transport
 * @var version - version of transport to use
 * @var recv - pointer to the transport specific function to receive a message
 * @var send - pointer to the transport specific function to send a message
 * @var init_pollfd - pointer to the transport specific init_pollfd function
 */
struct pldm_transport {
	const char *name;
	uint8_t version;
	pldm_requester_rc_t (*recv)(struct pldm_transport *transport,
				    pldm_tid_t *tid, void **pldm_resp_msg,
				    size_t *msg_len);
	pldm_requester_rc_t (*send)(struct pldm_transport *transport,
				    pldm_tid_t tid, const void *pldm_msg,
				    size_t msg_len);
	int (*init_pollfd)(struct pldm_transport *transport,
			   struct pollfd *pollfd);
	int (*get_timeval)(struct timeval *tv);
};

#endif // LIBPLDM_SRC_TRANSPORT_TRANSPORT_H
