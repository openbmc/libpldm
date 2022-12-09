#include "libpldm/requester/pldm.h"
#include "../transport/transport_internal.h"
#include "base.h"

#include <bits/types/struct_iovec.h>
#ifdef PLDM_HAS_POLL
#include <poll.h>
#endif
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/* -- new API -- */
struct pldm_requester {
	struct pldm_transport *transport;
};

struct pldm_requester *pldm_requester_init(void)
{
	return calloc(sizeof(struct pldm_requester), 1);
}

pldm_requester_rc_t pldm_requester_destroy(struct pldm_requester *ctx)
{

	if (ctx->transport) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}
	free(ctx);
	return PLDM_REQUESTER_SUCCESS;
}

pldm_requester_rc_t
pldm_requester_register_transport(struct pldm_requester *ctx,
				  struct pldm_transport *transport)
{
	if (!ctx || ctx->transport != NULL) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}
	ctx->transport = transport;

	return 0;
}

pldm_requester_rc_t
pldm_requester_unregister_transports(struct pldm_requester *ctx)
{
	if (!ctx) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}
	ctx->transport = NULL;
	return PLDM_REQUESTER_SUCCESS;
}

#ifndef PLDM_HAS_POLL
struct pollfd {
	int fd;	       /* file descriptor */
	short events;  /* requested events */
	short revents; /* returned events */
};

static inline int poll(struct pollfd *fds, int nfds,
		       int timeout __attribute__((unused)))
{
	int i;

	for (i = 0; i < nfds; i++) {
		fds[i].revents = 0;
	}

	return 0;
}
#endif
pldm_requester_rc_t pldm_requester_poll(struct pldm_requester *ctx, int timeout)
{
	struct pollfd pollfd;
	int rc = 0;
	if (!ctx || !ctx->transport) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}
	if (!ctx->transport->init_pollfd) {
		return PLDM_REQUESTER_SUCCESS;
	}

	ctx->transport->init_pollfd(ctx->transport, &pollfd);
	rc = poll(&pollfd, 1, timeout);
	if (rc < 0) {
		return PLDM_REQUESTER_POLL_FAIL;
	}

	return PLDM_REQUESTER_SUCCESS;
}

pldm_requester_rc_t pldm_requester_send_msg(struct pldm_requester *ctx,
					    pldm_tid_t tid,
					    const void *pldm_req_msg,
					    size_t req_msg_len)
{
	if (!ctx || !ctx->transport) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}

	if (req_msg_len < sizeof(struct pldm_msg_hdr)) {
		return PLDM_REQUESTER_NOT_REQ_MSG;
	}

	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)pldm_req_msg;
	if ((hdr->request != PLDM_REQUEST) &&
	    (hdr->request != PLDM_ASYNC_REQUEST_NOTIFY)) {
		return PLDM_REQUESTER_NOT_REQ_MSG;
	}

	return ctx->transport->send(ctx->transport, tid, pldm_req_msg,
				    req_msg_len);
}

pldm_requester_rc_t pldm_requester_recv_msg(struct pldm_requester *ctx,
					    pldm_tid_t tid, uint8_t instance_id,
					    void **pldm_resp_msg,
					    size_t *resp_msg_len)
{
	pldm_requester_rc_t rc =
	    pldm_requester_recv_msg_any(ctx, tid, pldm_resp_msg, resp_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}

	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)(*pldm_resp_msg);
	if (hdr->instance_id != instance_id) {
		free(*pldm_resp_msg);
		*pldm_resp_msg = NULL;
		return PLDM_REQUESTER_INSTANCE_ID_MISMATCH;
	}

	return PLDM_REQUESTER_SUCCESS;
}

pldm_requester_rc_t pldm_requester_recv_msg_any(struct pldm_requester *ctx,
						pldm_tid_t tid,
						void **pldm_resp_msg,
						size_t *resp_msg_len)
{
	if (!ctx || !ctx->transport) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}

	pldm_requester_rc_t rc = ctx->transport->recv(
	    ctx->transport, tid, pldm_resp_msg, resp_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}

	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)(*pldm_resp_msg);
	if (hdr->request != PLDM_RESPONSE) {
		free(*pldm_resp_msg);
		*pldm_resp_msg = NULL;
		return PLDM_REQUESTER_NOT_RESP_MSG;
	}

	uint8_t pldm_rc = 0;
	if (*resp_msg_len < (sizeof(struct pldm_msg_hdr) + sizeof(pldm_rc))) {
		free(*pldm_resp_msg);
		*pldm_resp_msg = NULL;
		return PLDM_REQUESTER_RESP_MSG_TOO_SMALL;
	}

	return PLDM_REQUESTER_SUCCESS;
}

pldm_requester_rc_t
pldm_requester_send_recv_msg(struct pldm_requester *ctx, pldm_tid_t tid,
			     const void *pldm_req_msg, size_t req_msg_len,
			     void **pldm_resp_msg, size_t *resp_msg_len)

{
	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)pldm_req_msg;

	pldm_requester_rc_t rc =
	    pldm_requester_send_msg(ctx, tid, pldm_req_msg, req_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}

	while (1) {
		rc = pldm_requester_poll(ctx, -1);
		if (rc != PLDM_REQUESTER_SUCCESS) {
			break;
		}
		rc = pldm_requester_recv_msg(ctx, tid, hdr->instance_id,
					     pldm_resp_msg, resp_msg_len);
		if (rc == PLDM_REQUESTER_SUCCESS) {
			break;
		}
	}

	return rc;
}

/* Temporary for old api */
#include "libpldm/transport/mctp-demux.h"
extern int
pldm_transport_mctp_demux_get_socket_fd(struct pldm_transport_mctp_demux *ctx);
extern struct pldm_transport_mctp_demux *
pldm_transport_mctp_demux_init_with_fd(int mctp_fd);

/* ---  old APIS written in terms of the new API -- */
/*
 * pldm_open returns the file descriptor to the MCTP socket, which needs to
 * persist over api calls (so a consumer can poll it for incoming messages).
 * So we need a global variable to store the transport struct
 */
static struct pldm_transport_mctp_demux *open_transport;

pldm_requester_rc_t pldm_open()
{
	int fd;

	if (open_transport) {
		return -1;
	}

	struct pldm_transport_mctp_demux *demux =
	    pldm_transport_mctp_demux_init();
	if (!demux) {
		return -1;
	}

	fd = pldm_transport_mctp_demux_get_socket_fd(demux);

	open_transport = demux;

	return fd;
}

/* This macro does the setup and teardown required for the old API to use the
 * new API. Since the setup/teardown logic is the same for all four send/recv
 * functions, it makes sense to only define it once. */
#define PLDM_REQ_FN(eid, fd, fn, ...)                                          \
	do {                                                                   \
		struct pldm_transport_mctp_demux *demux;                       \
		bool using_open_transport = false;                             \
		pldm_requester_rc_t rc;                                        \
		pldm_tid_t tid = 1;                                            \
		struct pldm_requester *ctx = pldm_requester_init();            \
		if (!ctx) {                                                    \
			return PLDM_REQUESTER_OPEN_FAIL;                       \
		}                                                              \
		/* The fd can be for a socket we opened or one the consumer    \
		 * opened. */                                                  \
		if (open_transport &&                                          \
		    mctp_fd == pldm_transport_mctp_demux_get_socket_fd(        \
				   open_transport)) {                          \
			using_open_transport = true;                           \
			demux = open_transport;                                \
		} else {                                                       \
			demux = pldm_transport_mctp_demux_init_with_fd(fd);    \
			if (!demux) {                                          \
				rc = PLDM_REQUESTER_OPEN_FAIL;                 \
				goto pldm_out;                                 \
			}                                                      \
		}                                                              \
		rc = pldm_requester_register_transport(                        \
		    ctx, pldm_transport_mctp_demux_core(demux));               \
		if (rc != PLDM_REQUESTER_SUCCESS) {                            \
			rc = PLDM_REQUESTER_OPEN_FAIL;                         \
			goto transport_out;                                    \
		}                                                              \
		rc = pldm_transport_mctp_demux_map_tid(demux, tid, eid);       \
		if (rc) {                                                      \
			rc = PLDM_REQUESTER_OPEN_FAIL;                         \
			goto transport_out;                                    \
		}                                                              \
		rc = fn(ctx, tid, __VA_ARGS__);                                \
	transport_out:                                                         \
		pldm_requester_unregister_transports(ctx);                     \
		if (!using_open_transport) {                                   \
			pldm_transport_mctp_demux_destroy(demux);              \
		}                                                              \
	pldm_out:                                                              \
		pldm_requester_destroy(ctx);                                   \
		return rc;                                                     \
	} while (0)

pldm_requester_rc_t pldm_recv_any(mctp_eid_t eid, int mctp_fd,
				  uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	PLDM_REQ_FN(eid, mctp_fd, pldm_requester_recv_msg_any,
		    (void **)pldm_resp_msg, resp_msg_len);
}

pldm_requester_rc_t pldm_recv(mctp_eid_t eid, int mctp_fd, uint8_t instance_id,
			      uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	PLDM_REQ_FN(eid, mctp_fd, pldm_requester_recv_msg, instance_id,
		    (void **)pldm_resp_msg, resp_msg_len);
}

pldm_requester_rc_t pldm_send_recv(mctp_eid_t eid, int mctp_fd,
				   const uint8_t *pldm_req_msg,
				   size_t req_msg_len, uint8_t **pldm_resp_msg,
				   size_t *resp_msg_len)
{
	PLDM_REQ_FN(eid, mctp_fd, pldm_requester_send_recv_msg, pldm_req_msg,
		    req_msg_len, (void **)pldm_resp_msg, resp_msg_len);
}

pldm_requester_rc_t pldm_send(mctp_eid_t eid, int mctp_fd,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)
{
	PLDM_REQ_FN(eid, mctp_fd, pldm_requester_send_msg, (void *)pldm_req_msg,
		    req_msg_len);
}

/* Adding this here for completeness in the case we can't smoothly
 * transition apps over to the new api */
void pldm_close()
{
	if (open_transport) {
		pldm_transport_mctp_demux_destroy(open_transport);
	}
	return;
}
