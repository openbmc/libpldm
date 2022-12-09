#include "pldm.h"
#include "base.h"

#include <bits/types/struct_iovec.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <stdio.h>
#include "libpldm/requester/demux.h"
struct pldm {
	struct pldm_transport *transport;
};

// new API
struct pldm *pldm_init()
{
	struct pldm *pldm = malloc(sizeof(struct pldm));
	if (!pldm) return NULL;
	memset(pldm, 0, sizeof(*pldm));
	return pldm;
}

int pldm_register_transport(struct pldm *pldm, struct pldm_transport *transport)
{
	pldm->transport = transport;
	return 0;
}

void pldm_destroy(struct pldm * pldm)
{
	if (pldm)
		free(pldm);
}

pldm_requester_rc_t pldm_recv_msg_any_inst(struct pldm_transport *b, mctp_eid_t eid,
				  uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	pldm_requester_rc_t rc =
	    b->recv(b, eid, pldm_resp_msg, resp_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}

	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)(*pldm_resp_msg);
	if (hdr->request != PLDM_RESPONSE) {
		free(*pldm_resp_msg);
		return PLDM_REQUESTER_NOT_RESP_MSG;
	}

	uint8_t pldm_rc = 0;
	if (*resp_msg_len < (sizeof(struct pldm_msg_hdr) + sizeof(pldm_rc))) {
		free(*pldm_resp_msg);
		return PLDM_REQUESTER_RESP_MSG_TOO_SMALL;
	}

	return PLDM_REQUESTER_SUCCESS;
}

pldm_requester_rc_t pldm_recv_msg(struct pldm_transport *b, mctp_eid_t eid,  uint8_t instance_id,
			      uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	pldm_requester_rc_t rc =
	    pldm_recv_msg_any_inst(b, eid, pldm_resp_msg, resp_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}

	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)(*pldm_resp_msg);
	if (hdr->instance_id != instance_id) {
		free(*pldm_resp_msg);
		return PLDM_REQUESTER_INSTANCE_ID_MISMATCH;
	}

	return PLDM_REQUESTER_SUCCESS;
}

pldm_requester_rc_t pldm_send_recv_msg(struct pldm_transport *b, mctp_eid_t eid,
				   const uint8_t *pldm_req_msg,
				   size_t req_msg_len, uint8_t **pldm_resp_msg,
				   size_t *resp_msg_len)
{
	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)pldm_req_msg;
	if ((hdr->request != PLDM_REQUEST) &&
	    (hdr->request != PLDM_ASYNC_REQUEST_NOTIFY)) {
		return PLDM_REQUESTER_NOT_REQ_MSG;
	}

	pldm_requester_rc_t rc =
	    pldm_send_msg(b, eid, pldm_req_msg, req_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}

	while (1) {
		rc = pldm_recv_msg(b, eid, hdr->instance_id, pldm_resp_msg,
			       resp_msg_len);
		if (rc == PLDM_REQUESTER_SUCCESS) {
			break;
		}
	}

	return rc;
}

pldm_requester_rc_t pldm_send_msg(struct pldm_transport *b, mctp_eid_t eid,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)
{
	return b->send(b, eid, pldm_req_msg, req_msg_len);
}




//old APIS written in terms of the new API


/* don't destroy backend as we want the socket to stay open, as some apps use the fd to it */ 
pldm_requester_rc_t pldm_open()
{
	int fd;
	pldm_requester_rc_t rc;
	struct pldm *pldm = pldm_init();
	if (!pldm)
	{
		printf("could not initialise pldm core");
		return -1;
	}
	// init opens up the backend
	struct transport_demux *demux = pldm_transport_demux_init();
	if (!demux)
	{
		printf("could not initialise mctp-demux transport");
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS)
		return PLDM_REQUESTER_OPEN_FAIL;
	fd = pldm->transport->socket;
	pldm_destroy(pldm);
	return fd;
}

pldm_requester_rc_t pldm_recv_any(mctp_eid_t eid, int mctp_fd,
				  uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	struct pldm *pldm = pldm_init();
	pldm_requester_rc_t rc;
	if (!pldm)
	{
		printf("could not initialise pldm core");
		return -1;
	}
	struct transport_demux *demux = pldm_transport_demux_init_with_fd(mctp_fd);
	if (!demux)
	{
		printf("could not initialise mctp-demux transport");
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS)
		return PLDM_REQUESTER_OPEN_FAIL;

	rc = pldm_recv_msg_any_inst(pldm_transport_demux_core(demux),
				    eid, pldm_resp_msg, resp_msg_len);

	pldm_transport_demux_destroy(demux);
	pldm_destroy(pldm);
	
	return rc;
}

pldm_requester_rc_t pldm_recv(mctp_eid_t eid, int mctp_fd, uint8_t instance_id,
			      uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	struct pldm *pldm = pldm_init();
	pldm_requester_rc_t rc;
	if (!pldm)
	{
		printf("could not initialise pldm core");
		return -1;
	}
	struct transport_demux *demux = pldm_transport_demux_init_with_fd(mctp_fd);
	if (!demux)
	{
		printf("could not initialise mctp-demux transport");
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS)
		return PLDM_REQUESTER_OPEN_FAIL;

	rc = pldm_recv_msg(pldm_transport_demux_core(demux),
	 		eid, instance_id, pldm_resp_msg, resp_msg_len);

	pldm_transport_demux_destroy(demux);
	pldm_destroy(pldm);

	return rc;
}

pldm_requester_rc_t pldm_send_recv(mctp_eid_t eid, int mctp_fd,
				   const uint8_t *pldm_req_msg,
				   size_t req_msg_len, uint8_t **pldm_resp_msg,
				   size_t *resp_msg_len)
{
	struct pldm *pldm = pldm_init();
	pldm_requester_rc_t rc;
	if (!pldm)
	{
		printf("could not initialise pldm core");
		return -1;
	}
	struct transport_demux *demux = pldm_transport_demux_init_with_fd(mctp_fd);
	if (!demux)
	{
		printf("could not initialise mctp-demux transport");
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS)
		return PLDM_REQUESTER_OPEN_FAIL;

	rc = pldm_send_recv_msg(pldm_transport_demux_core(demux),
			 eid, pldm_req_msg, req_msg_len, pldm_resp_msg, resp_msg_len);

	pldm_transport_demux_destroy(demux);
	pldm_destroy(pldm);

	return rc;
}

pldm_requester_rc_t pldm_send(mctp_eid_t eid, int mctp_fd,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)
{
	pldm_requester_rc_t rc;
	struct pldm *pldm = pldm_init();
	if (!pldm)
	{
		printf("could not initialise pldm core");
		return -1;
	}
	struct transport_demux *demux = pldm_transport_demux_init_with_fd(mctp_fd);
	if (!demux)
	{
		printf("could not initialise mctp-demux transport");
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS)
		return PLDM_REQUESTER_OPEN_FAIL;

	rc =  pldm_send_msg(pldm_transport_demux_core(demux),
		 eid, pldm_req_msg, req_msg_len);

	pldm_transport_demux_destroy(demux);
	pldm_destroy(pldm);

	return rc;
}

