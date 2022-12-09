#include "pldm.h"
#include "base.h"
#include "mctp.h"
#include "container_of.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

struct transport_afmctp {
	struct pldm_transport transport;
	int socket;
};

#define transport_to_afmctp(b) \
        container_of(b, struct transport_afmctp, transport)
struct pldm_transport *pldm_transport_afmctp_core(struct transport_afmctp *b)
{
	return &b->transport;
}

static pldm_requester_rc_t pldm_transport_afmctp_open(struct pldm_transport *b)
{
	int fd = -1;
	int rc = -1;
	struct transport_afmctp *afmctp = transport_to_afmctp(b);

	fd = socket(AF_MCTP, SOCK_DGRAM, 0);
	if (-1 == fd) {
		return fd;
	}

	struct sockaddr_mctp addr = {0};
	addr.smctp_family = AF_MCTP;
	addr.smctp_addr.s_addr = MCTP_ADDR_ANY;
	addr.smctp_type = MCTP_MSG_TYPE_PLDM;
	addr.smctp_tag = MCTP_TAG_OWNER;
	addr.smctp_network = MCTP_NET_ANY;
	rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (-1 == rc) {
		return PLDM_REQUESTER_OPEN_FAIL;
	}
	afmctp->socket = fd;		
	b->socket = fd;
// should we just return success?
	return fd;
}

static pldm_requester_rc_t pldm_transport_afmctp_recv(struct pldm_transport *b, 
				     mctp_eid_t eid,
				     uint8_t **pldm_resp_msg,
				     size_t *resp_msg_len)
{
	struct transport_afmctp *afmctp = transport_to_afmctp(b);
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
        } else if (length < min_len) {
                /* read and discard */
                uint8_t buf[length];
                recv(afmctp->socket, buf, length, 0);
                return PLDM_REQUESTER_INVALID_RECV_LEN;
        } else {
                ssize_t bytes = recvfrom(afmctp->socket, *pldm_resp_msg, length, MSG_TRUNC, (struct sockaddr *)&addr, &addrlen);
                if (length != bytes) {
                        free(*pldm_resp_msg);
                        return PLDM_REQUESTER_INVALID_RECV_LEN;
                }
                *resp_msg_len = length;
                return PLDM_REQUESTER_SUCCESS;
        }
}


static pldm_requester_rc_t pldm_transport_afmctp_send(struct pldm_transport *b,
			      mctp_eid_t eid,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)
{
	struct transport_afmctp *afmctp = transport_to_afmctp(b);
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

struct transport_afmctp *pldm_transport_afmctp_init()
{
	struct transport_afmctp *afmctp = malloc(sizeof(struct transport_afmctp));
	if (!afmctp)
		return NULL;

	memset(afmctp, 0, sizeof(*afmctp));
	afmctp->transport.name = "AF_MCTP";
	afmctp->transport.version = 1;
	afmctp->transport.open = pldm_transport_afmctp_open;
	afmctp->transport.recv = pldm_transport_afmctp_recv;
	afmctp->transport.send = pldm_transport_afmctp_send;
	return afmctp;
}

void pldm_transport_afmctp_destroy(struct transport_afmctp *b)
{
	free(b);
}
