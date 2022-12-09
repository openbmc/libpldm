#include "base.h"
#include "container_of.h"
#include "libpldm/requester/pldm.h"
#include "transport_internal.h"

#include <errno.h>
#include <linux/mctp.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define MCTP_MSG_TYPE_PLDM 1
#define MCTP_MAX_NUM_EID 256
struct pldm_transport_afmctp {
	struct pldm_transport transport;
	int socket;
	pldm_tid_t tid_eid_map[MCTP_MAX_NUM_EID];
};

#define transport_to_afmctp(b)                                                 \
	container_of(b, struct pldm_transport_afmctp, transport)
struct pldm_transport *
pldm_transport_afmctp_core(struct pldm_transport_afmctp *b)
{
	return &b->transport;
}

static pldm_requester_rc_t pldm_transport_afmctp_open()
{
	int fd = -1;

	fd = socket(AF_MCTP, SOCK_DGRAM, 0);
	return fd;
}

int pldm_transport_afmctp_init_pollfd(struct pldm_transport_afmctp *ctx,
				      struct pollfd *pollfd)
{
	pollfd->fd = ctx->socket;
	pollfd->events = POLLIN;
	return 0;
}

static int pldm_transport_mctp_get_eid(struct pldm_transport_afmctp *ctx,
				       pldm_tid_t tid, mctp_eid_t *eid)
{
	int i;
	for (i = 0; i < MCTP_MAX_NUM_EID; i++) {
		if (ctx->tid_eid_map[i] == tid) {
			*eid = i;
			return 0;
		}
	}
	*eid = -1;
	return -1;
}

int pldm_transport_afmctp_map_tid(struct pldm_transport_afmctp *ctx,
				  pldm_tid_t tid, mctp_eid_t eid)
{
	ctx->tid_eid_map[eid] = tid;

	return 0;
}

int pldm_transport_afmctp_unmap_tid(struct pldm_transport_afmctp *ctx,
				    __attribute__((unused)) pldm_tid_t tid,
				    mctp_eid_t eid)
{
	ctx->tid_eid_map[eid] = 0;

	return 0;
}

static pldm_requester_rc_t pldm_transport_afmctp_recv(struct pldm_transport *t,
						      pldm_tid_t tid,
						      void **pldm_resp_msg,
						      size_t *resp_msg_len)
{
	struct pldm_transport_afmctp *afmctp = transport_to_afmctp(t);
	mctp_eid_t eid = 0;
	int rc = pldm_transport_mctp_get_eid(afmctp, tid, &eid);
	if (rc) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	ssize_t min_len = sizeof(struct pldm_msg_hdr);
	ssize_t length = recv(afmctp->socket, NULL, 0, MSG_PEEK | MSG_TRUNC);
	if (length <= 0) {
		return PLDM_REQUESTER_RECV_FAIL;
	} else {
		*pldm_resp_msg = malloc(length);
		length =
		    recv(afmctp->socket, *pldm_resp_msg, length, MSG_TRUNC);
		if (length < min_len) {
			free(*pldm_resp_msg);
			return PLDM_REQUESTER_INVALID_RECV_LEN;
		}
		*resp_msg_len = length;
		return PLDM_REQUESTER_SUCCESS;
	}
}

static pldm_requester_rc_t pldm_transport_afmctp_send(struct pldm_transport *t,
						      pldm_tid_t tid,
						      const void *pldm_req_msg,
						      size_t req_msg_len)
{
	struct pldm_transport_afmctp *afmctp = transport_to_afmctp(t);
	mctp_eid_t eid = 0;
	if (pldm_transport_mctp_get_eid(afmctp, tid, &eid)) {
		return PLDM_REQUESTER_SEND_FAIL;
	}

	struct sockaddr_mctp addr = {0};
	addr.smctp_family = AF_MCTP;
	addr.smctp_addr.s_addr = eid;
	addr.smctp_type = MCTP_MSG_TYPE_PLDM;
	addr.smctp_tag = MCTP_TAG_OWNER;

	int rc = sendto(afmctp->socket, (uint8_t *)pldm_req_msg, req_msg_len, 0,
			(struct sockaddr *)&addr, sizeof(addr));
	if (rc == -1) {
		return PLDM_REQUESTER_SEND_FAIL;
	}
	return PLDM_REQUESTER_SUCCESS;
}

struct pldm_transport_afmctp *pldm_transport_afmctp_init()
{
	struct pldm_transport_afmctp *afmctp =
	    malloc(sizeof(struct pldm_transport_afmctp));
	if (!afmctp)
		return NULL;

	memset(afmctp, 0, sizeof(*afmctp));
	afmctp->transport.name = "AF_MCTP";
	afmctp->transport.version = 1;
	afmctp->transport.recv = pldm_transport_afmctp_recv;
	afmctp->transport.send = pldm_transport_afmctp_send;
	afmctp->socket = pldm_transport_afmctp_open();
	if (afmctp->socket == -1) {
		free(afmctp);
		return NULL;
	}
	return afmctp;
}

void pldm_transport_afmctp_destroy(struct pldm_transport_afmctp *b) { free(b); }
