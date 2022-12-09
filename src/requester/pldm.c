#include "libpldm/requester/pldm.h"
#include "../transport/transport.h"
#include "libpldm/instance_id/instance_id.h"
#include "base.h"

#include <bits/types/struct_iovec.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

struct pldm_requester {
	struct pldm_transport *transport;
	struct pldm_instance_id *idb;
};

pldm_requester_rc_t pldm_requester_init(struct pldm_requester **ctx,
					struct pldm_instance_id *idb)
{
	if (!idb) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}

	*ctx = calloc(sizeof(struct pldm_requester), 1);
	if (!*ctx) {
		return PLDM_REQUESTER_SETUP_FAIL;
	}
	return PLDM_REQUESTER_SUCCESS;
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
	if (!ctx || ctx->transport != NULL || !transport) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}
	ctx->transport = transport;

	return PLDM_REQUESTER_SUCCESS;
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

pldm_requester_rc_t pldm_requester_init_default(struct pldm_requester **ctx)
{
	struct pldm_instance_id *idb = NULL;
	int rc;

	rc = pldm_instance_id_init_default(&idb);
	if (rc < 0) {
		return rc;
	}

	return pldm_requester_init(ctx, idb);
}

/* Temporary for old api */
#include "libpldm/transport/mctp-demux.h"
#include "libpldm/transport/transport.h"
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

pldm_requester_rc_t pldm_open(void)
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
		struct pldm_transport *ctx;                                    \
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
				goto transport_out;                            \
			}                                                      \
		}                                                              \
		ctx = pldm_transport_mctp_demux_core(demux);                   \
		rc = pldm_transport_mctp_demux_map_tid(demux, tid, eid);       \
		if (rc) {                                                      \
			rc = PLDM_REQUESTER_OPEN_FAIL;                         \
			goto transport_out;                                    \
		}                                                              \
		rc = fn(ctx, tid, __VA_ARGS__);                                \
	transport_out:                                                         \
		if (!using_open_transport) {                                   \
			pldm_transport_mctp_demux_destroy(demux);              \
		}                                                              \
		return rc;                                                     \
	} while (0)

pldm_requester_rc_t pldm_recv_any(mctp_eid_t eid, int mctp_fd,
				  uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	PLDM_REQ_FN(eid, mctp_fd, pldm_transport_recv_msg,
		    (void **)pldm_resp_msg, resp_msg_len);
}

pldm_requester_rc_t pldm_recv(mctp_eid_t eid, int mctp_fd,
			      __attribute__((unused)) uint8_t instance_id,
			      uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	pldm_requester_rc_t rc =
	    pldm_recv_any(eid, mctp_fd, pldm_resp_msg, resp_msg_len);
	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)(*pldm_resp_msg);
	if (hdr->instance_id != instance_id) {
		free(*pldm_resp_msg);
		*pldm_resp_msg = NULL;
		return PLDM_REQUESTER_INSTANCE_ID_MISMATCH;
	}
	return rc;
}

pldm_requester_rc_t pldm_send_recv(mctp_eid_t eid, int mctp_fd,
				   const uint8_t *pldm_req_msg,
				   size_t req_msg_len, uint8_t **pldm_resp_msg,
				   size_t *resp_msg_len)
{
	PLDM_REQ_FN(eid, mctp_fd, pldm_transport_send_recv_msg, pldm_req_msg,
		    req_msg_len, (void **)pldm_resp_msg, resp_msg_len);
}

pldm_requester_rc_t pldm_send(mctp_eid_t eid, int mctp_fd,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)
{
	PLDM_REQ_FN(eid, mctp_fd, pldm_transport_send_msg, (void *)pldm_req_msg,
		    req_msg_len);
}

/* Adding this here for completeness in the case we can't smoothly
 * transition apps over to the new api */
void pldm_close()
{
	if (open_transport) {
		pldm_transport_mctp_demux_destroy(open_transport);
	}
	open_transport = NULL;
}
