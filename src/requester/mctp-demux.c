#include "base.h"
#include "container_of.h"
#include "pldm.h"

#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

const uint8_t mctp_msg_type = MCTP_MSG_TYPE_PLDM;
struct pldm_transport_mctp_demux {
	struct pldm_transport transport;
	int socket;
	int num_mappings;
	/* In the future this probably needs to move to a tid-eid-uuid/network
	 * id mapping for multi mctp networks */
	uint8_t *tid_eid_mapping;
};

#define transport_to_demux(t)                                                  \
	container_of(t, struct pldm_transport_mctp_demux, transport)

struct pldm_transport *
pldm_transport_demux_core(struct pldm_transport_mctp_demux *t)
{
	return &t->transport;
}

static pldm_requester_rc_t pldm_transport_demux_open()
{
	int fd = -1;
	int rc = -1;

	fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (-1 == fd) {
		return fd;
	}

	const char path[] = "\0mctp-mux";
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, path, sizeof(path) - 1);
	rc = connect(fd, (struct sockaddr *)&addr,
		     sizeof(path) + sizeof(addr.sun_family) - 1);
	if (-1 == rc) {
		return PLDM_REQUESTER_OPEN_FAIL;
	}
	rc = write(fd, &mctp_msg_type, sizeof(mctp_msg_type));
	if (-1 == rc) {
		return PLDM_REQUESTER_OPEN_FAIL;
	}

	return fd;
}

int pldm_transport_mctp_demux_init_pollfd(struct pldm_transport_mctp_demux *ctx,
					  struct pollfd *pollfd)
{
	pollfd->fd = ctx->socket;
	pollfd->events = POLLIN;
	return 0;
}

static int
pldm_transport_mctp_find_mapping_index(struct pldm_transport_mctp_demux *ctx,
				       pldm_tid_t tid, mctp_eid_t eid,
				       int *index)
{
	int i;
	for (i = 0; i < ctx->num_mappings; i += 2) {
		if (ctx->tid_eid_mapping[i] == tid &&
		    ctx->tid_eid_mapping[i + 1] == eid) {
			*index = i;
			return 0;
		}
	}
	return -1;
}

static int pldm_transport_mctp_get_eid(struct pldm_transport_mctp_demux *ctx,
				       pldm_tid_t tid, mctp_eid_t *eid)
{
	int i;
	for (i = 0; i < ctx->num_mappings; i += 2) {
		if (ctx->tid_eid_mapping[i] == tid) {
			*eid = ctx->tid_eid_mapping[i + 1];
			return 0;
		}
	}
	return -1;
}

int pldm_transport_mctp_map_tid(struct pldm_transport_mctp_demux *ctx,
				pldm_tid_t tid, mctp_eid_t eid)
{
	/* Check if tid-eid pair already exists */
	int index;
	int rc = pldm_transport_mctp_find_mapping_index(ctx, tid, eid, &index);
	if (!rc) {
		return 0;
	}

	// should we start with a certain amount of memory preallocated so we
	// don't need to realloc each time?
	ctx->num_mappings++;
	ctx->tid_eid_mapping = realloc(ctx->tid_eid_mapping,
				       ctx->num_mappings * 2 * sizeof(uint8_t));
	ctx->tid_eid_mapping[ctx->num_mappings - 1] = tid;
	ctx->tid_eid_mapping[ctx->num_mappings] = eid;

	return 0;
}

int pldm_transport_mctp_unmap_tid(struct pldm_transport_mctp_demux *ctx,
				  pldm_tid_t tid, mctp_eid_t eid)
{
	int index;
	int rc = pldm_transport_mctp_find_mapping_index(ctx, tid, eid, &index);
	if (rc) {
		return 0;
	}

	// Shift everything over
	int i;
	for (i = index; i < ctx->num_mappings - 1; i += 2) {
		ctx->tid_eid_mapping[i] = ctx->tid_eid_mapping[i + 2];
		ctx->tid_eid_mapping[i + 1] = ctx->tid_eid_mapping[i + 3];
	}
	ctx->num_mappings--;

	return 0;
}

static pldm_requester_rc_t pldm_transport_demux_recv(struct pldm_transport *t,
						     pldm_tid_t tid,
						     uint8_t **pldm_resp_msg,
						     size_t *resp_msg_len)
{
	struct pldm_transport_mctp_demux *demux = transport_to_demux(t);
	mctp_eid_t eid = 0;
	int rc = pldm_transport_mctp_get_eid(demux, tid, &eid);
	if (rc) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	ssize_t min_len =
	    sizeof(eid) + sizeof(mctp_msg_type) + sizeof(struct pldm_msg_hdr);
	ssize_t length = recv(demux->socket, NULL, 0, MSG_PEEK | MSG_TRUNC);
	if (length <= 0) {
		return PLDM_REQUESTER_RECV_FAIL;
	} else if (length < min_len) {
		/* read and discard */
		uint8_t buf[length];
		recv(demux->socket, buf, length, 0);
		return PLDM_REQUESTER_INVALID_RECV_LEN;
	} else {
		struct iovec iov[2];
		size_t mctp_prefix_len = sizeof(eid) + sizeof(mctp_msg_type);
		uint8_t mctp_prefix[mctp_prefix_len];
		size_t pldm_len = length - mctp_prefix_len;
		iov[0].iov_len = mctp_prefix_len;
		iov[0].iov_base = mctp_prefix;
		*pldm_resp_msg = malloc(pldm_len);
		iov[1].iov_len = pldm_len;
		iov[1].iov_base = *pldm_resp_msg;
		struct msghdr msg = {0};
		msg.msg_iov = iov;
		msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
		ssize_t bytes = recvmsg(demux->socket, &msg, 0);
		if (length != bytes) {
			free(*pldm_resp_msg);
			*pldm_resp_msg = NULL;
			return PLDM_REQUESTER_INVALID_RECV_LEN;
		}
		if ((mctp_prefix[0] != eid) ||
		    (mctp_prefix[1] != mctp_msg_type)) {
			free(*pldm_resp_msg);
			*pldm_resp_msg = NULL;
			return PLDM_REQUESTER_NOT_PLDM_MSG;
		}
		*resp_msg_len = pldm_len;
		return PLDM_REQUESTER_SUCCESS;
	}
}

static pldm_requester_rc_t
pldm_transport_demux_send(struct pldm_transport *t, pldm_tid_t tid,
			  const uint8_t *pldm_req_msg, size_t req_msg_len)
{
	struct pldm_transport_mctp_demux *demux = transport_to_demux(t);
	mctp_eid_t eid = 0;
	if (pldm_transport_mctp_get_eid(demux, tid, &eid)) {
		return PLDM_REQUESTER_SEND_FAIL;
	}

	uint8_t hdr[2] = {eid, mctp_msg_type};

	struct iovec iov[2];
	iov[0].iov_base = hdr;
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = (uint8_t *)pldm_req_msg;
	iov[1].iov_len = req_msg_len;

	struct msghdr msg = {0};
	msg.msg_iov = iov;
	msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);

	ssize_t rc = sendmsg(demux->socket, &msg, 0);
	if (rc == -1) {
		return PLDM_REQUESTER_SEND_FAIL;
	}
	return PLDM_REQUESTER_SUCCESS;
}

struct pldm_transport_mctp_demux *pldm_transport_demux_init()
{
	struct pldm_transport_mctp_demux *demux =
	    malloc(sizeof(struct pldm_transport_mctp_demux));
	if (!demux)
		return NULL;

	memset(demux, 0, sizeof(*demux));
	demux->transport.name = "libmctp-demux-daemon";
	demux->transport.version = 1;
	demux->transport.recv = pldm_transport_demux_recv;
	demux->transport.send = pldm_transport_demux_send;
	demux->socket = pldm_transport_demux_open();
	if (!demux->socket) {
		return NULL;
	}
	return demux;
}

void pldm_transport_demux_destroy(struct pldm_transport_mctp_demux *t)
{
	free(t->tid_eid_mapping);
	free(t);
	t = NULL;
}

/* Temporary for old api */
struct pldm_transport_mctp_demux *pldm_transport_demux_init_with_fd(int mctp_fd)
{
	struct pldm_transport_mctp_demux *demux =
	    malloc(sizeof(struct pldm_transport_mctp_demux));
	if (!demux)
		return NULL;

	memset(demux, 0, sizeof(*demux));
	demux->transport.name = "libmctp-demux-daemon";
	demux->transport.version = 1;
	demux->transport.recv = pldm_transport_demux_recv;
	demux->transport.send = pldm_transport_demux_send;
	demux->socket = mctp_fd;
	return demux;
}

int pldm_transport_mctp_get_socket_fd(struct pldm_transport *transport)
{
	struct pldm_transport_mctp_demux *demux = transport_to_demux(transport);
	return demux->socket;
}

int pldm_transport_mctp_get_tid_from_eid(struct pldm_transport_mctp_demux *ctx,
					 pldm_tid_t *tid, mctp_eid_t eid)
{
	int i;
	for (i = 0; i < ctx->num_mappings; i += 2) {
		if (ctx->tid_eid_mapping[i + 1] == eid) {
			*tid = ctx->tid_eid_mapping[i];
			return 0;
		}
	}
	return -1;
}
