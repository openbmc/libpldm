#include "base.h"
#include "container_of.h"
#include "pldm.h"

#include <errno.h>
#include <linux/mctp.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

struct tid_eid_mapping {
	pldm_tid_t tid;
	mctp_eid_t eid;
};

struct pldm_transport_afmctp {
	struct pldm_transport transport;
	int socket;
	int num_mappings;
	struct tid_eid_mapping tid_eid_map[256];
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

static int
pldm_transport_mctp_find_mapping_index(struct pldm_transport_afmctp *ctx,
				       pldm_tid_t tid, mctp_eid_t eid,
				       int *index)
{
	int i;
	for (i = 0; i < ctx->num_mappings; i++) {
		if (ctx->tid_eid_map[i].tid == tid &&
		    ctx->tid_eid_map[i].eid == eid) {
			*index = i;
			return 0;
		}
	}
	return -1;
}

static int pldm_transport_mctp_get_eid(struct pldm_transport_afmctp *ctx,
				       pldm_tid_t tid, mctp_eid_t *eid)
{
	int i;
	for (i = 0; i < ctx->num_mappings; i++) {
		if (ctx->tid_eid_map[i].tid == tid) {
			*eid = ctx->tid_eid_map[i].eid;
			return 0;
		}
	}
	return -1;
}

int pldm_transport_afmctp_map_tid(struct pldm_transport_afmctp *ctx,
				  pldm_tid_t tid, mctp_eid_t eid)
{
	/* Check if tid-eid pair already exists */
	int index;
	int rc = pldm_transport_mctp_find_mapping_index(ctx, tid, eid, &index);
	if (!rc) {
		return 0;
	}

	ctx->tid_eid_map[ctx->num_mappings].tid = tid;
	ctx->tid_eid_map[ctx->num_mappings].eid = eid;
	ctx->num_mappings++;

	return 0;
}

int pldm_transport_afmctp_unmap_tid(struct pldm_transport_afmctp *ctx,
				    pldm_tid_t tid, mctp_eid_t eid)
{
	int index;
	int rc = pldm_transport_mctp_find_mapping_index(ctx, tid, eid, &index);
	if (rc) {
		return 0;
	}

	/* Shift everything over */
	int i;
	for (i = index; i < ctx->num_mappings - 1; i++) {
		ctx->tid_eid_map[i].tid = ctx->tid_eid_map[i + 1].tid;
		ctx->tid_eid_map[i].eid = ctx->tid_eid_map[i + 1].eid;
	}
	ctx->num_mappings--;
	/* Zero the last mapping */
	ctx->tid_eid_map[ctx->num_mappings].tid = 0;
	ctx->tid_eid_map[ctx->num_mappings].eid = 0;

	return 0;
}

static pldm_requester_rc_t pldm_transport_afmctp_recv(struct pldm_transport *t,
						      pldm_tid_t tid,
						      uint8_t **pldm_resp_msg,
						      size_t *resp_msg_len)
{
	struct pldm_transport_afmctp *afmctp = transport_to_afmctp(t);
	mctp_eid_t eid = 0;
	int rc = pldm_transport_mctp_get_eid(afmctp, tid, &eid);
	if (rc) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	ssize_t min_len = sizeof(struct pldm_msg_hdr);
	struct sockaddr_mctp addr = {0};
	addr.smctp_family = AF_MCTP;
	addr.smctp_addr.s_addr = eid;
	addr.smctp_type = MCTP_MSG_TYPE_PLDM;
	addr.smctp_tag = MCTP_TAG_OWNER;
	socklen_t addrlen = sizeof(addr);

	ssize_t length = recvfrom(afmctp->socket, NULL, 0, MSG_PEEK | MSG_TRUNC,
				  (struct sockaddr *)&addr, &addrlen);
	if (length <= 0) {
		return PLDM_REQUESTER_RECV_FAIL;
	} else {
		*pldm_resp_msg = malloc(length);
		length =
		    recvfrom(afmctp->socket, *pldm_resp_msg, length, MSG_TRUNC,
			     (struct sockaddr *)&addr, &addrlen);
		if (length < min_len) {
			free(*pldm_resp_msg);
			return PLDM_REQUESTER_INVALID_RECV_LEN;
		}
		*resp_msg_len = length;
		return PLDM_REQUESTER_SUCCESS;
	}
}

static pldm_requester_rc_t
pldm_transport_afmctp_send(struct pldm_transport *t, pldm_tid_t tid,
			   const uint8_t *pldm_req_msg, size_t req_msg_len)
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
	if (-1 == rc) {
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
	if (-1 == afmctp->socket) {
		free(afmctp);
		return NULL;
	}
	return afmctp;
}

void pldm_transport_afmctp_destroy(struct pldm_transport_afmctp *b) { free(b); }
