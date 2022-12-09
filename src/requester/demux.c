#include "pldm.h"
#include "base.h"
#include "container_of.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <stdio.h>

const uint8_t mctp_msg_type = MCTP_MSG_TYPE_PLDM;
struct transport_demux {
	struct pldm_transport transport;
	int socket;
};


#define transport_to_demux(b) \
        container_of(b, struct transport_demux, transport)

struct pldm_transport *pldm_transport_demux_core(struct transport_demux *b)
{
	return &b->transport;
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

static pldm_requester_rc_t pldm_transport_demux_recv(struct pldm_transport *b,
				     mctp_eid_t eid,
				     uint8_t **pldm_resp_msg,
				     size_t *resp_msg_len)
{
	struct transport_demux *demux = transport_to_demux(b);
	ssize_t min_len = sizeof(eid) + sizeof(mctp_msg_type) +
			  sizeof(struct pldm_msg_hdr);
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
		size_t mctp_prefix_len =
		    sizeof(eid) + sizeof(mctp_msg_type);
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
			return PLDM_REQUESTER_INVALID_RECV_LEN;
		}
		if ((mctp_prefix[0] != eid) ||
		    (mctp_prefix[1] != mctp_msg_type)) {
			free(*pldm_resp_msg);
			return PLDM_REQUESTER_NOT_PLDM_MSG;
		}
		*resp_msg_len = pldm_len;
		return PLDM_REQUESTER_SUCCESS;
	}
}

static pldm_requester_rc_t pldm_transport_demux_send(struct pldm_transport *b,
			      mctp_eid_t eid,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)
{
	struct transport_demux *demux = transport_to_demux(b);
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

struct transport_demux *pldm_transport_demux_init()
{
	struct transport_demux *demux = malloc(sizeof(struct transport_demux));
	if (!demux)
		return NULL;

	memset(demux, 0, sizeof(*demux));
	demux->transport.name = "libmctp-demux-daemon";
	demux->transport.version = 1;
	demux->transport.open = pldm_transport_demux_open;
	demux->transport.recv = pldm_transport_demux_recv;
	demux->transport.send = pldm_transport_demux_send;
	demux->socket = pldm_transport_demux_open();
	if (!demux->socket)
	{
		printf("could not open mctp-demux transport");
		return NULL;
	}
	return demux;
}

struct transport_demux *pldm_transport_demux_init_with_fd(int mctp_fd)
{
	struct transport_demux *demux = malloc(sizeof(struct transport_demux));
	if (!demux)
		return NULL;

	memset(demux, 0, sizeof(*demux));
	demux->transport.name = "libmctp-demux-daemon";
	demux->transport.version = 1;
	demux->transport.open = pldm_transport_demux_open;
	demux->transport.recv = pldm_transport_demux_recv;
	demux->transport.send = pldm_transport_demux_send;
	demux->socket = mctp_fd;
	return demux;
}

void pldm_transport_demux_destroy(struct transport_demux *b)
{
	//todo close socket
	free(b);
}

