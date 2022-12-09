#include "pldm.h"
#include "base.h"

#include <bits/types/struct_iovec.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "libpldm/requester/mctp-demux.h"
#include <stdio.h>
struct pldm {
	struct pldm_transport *transport;
};

/* -- new API -- */
struct pldm *pldm_init()
{
	struct pldm *pldm = malloc(sizeof(struct pldm));
	if (!pldm)
		return NULL;
	memset(pldm, 0, sizeof(*pldm));
	return pldm;
}

int pldm_register_transport(struct pldm *pldm, struct pldm_transport *transport)
{
	if (pldm->transport != NULL) {
		return -1;
	}
	pldm->transport = transport;

	return 0;
}

void pldm_unregister_transport(struct pldm *pldm) { pldm->transport = NULL; }

void pldm_destroy(struct pldm *pldm)
{
	if (pldm) {
		free(pldm);
	}
}

pldm_requester_rc_t pldm_recv_msg_any_inst(struct pldm *pldm, pldm_tid_t tid,
					   uint8_t **pldm_resp_msg,
					   size_t *resp_msg_len)
{
	pldm_requester_rc_t rc = pldm->transport->recv(
	    pldm->transport, tid, pldm_resp_msg, resp_msg_len);
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

pldm_requester_rc_t pldm_recv_msg(struct pldm *pldm, pldm_tid_t tid,
				  uint8_t instance_id, uint8_t **pldm_resp_msg,
				  size_t *resp_msg_len)
{
	pldm_requester_rc_t rc =
	    pldm_recv_msg_any_inst(pldm, tid, pldm_resp_msg, resp_msg_len);
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

pldm_requester_rc_t pldm_send_recv_msg(struct pldm *pldm, pldm_tid_t tid,
				       const uint8_t *pldm_req_msg,
				       size_t req_msg_len,
				       uint8_t **pldm_resp_msg,
				       size_t *resp_msg_len)
{
	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)pldm_req_msg;
	if ((hdr->request != PLDM_REQUEST) &&
	    (hdr->request != PLDM_ASYNC_REQUEST_NOTIFY)) {
		return PLDM_REQUESTER_NOT_REQ_MSG;
	}

	pldm_requester_rc_t rc =
	    pldm_send_msg(pldm, tid, pldm_req_msg, req_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}

	while (1) {
		rc = pldm_recv_msg(pldm, tid, hdr->instance_id, pldm_resp_msg,
				   resp_msg_len);
		if (rc == PLDM_REQUESTER_SUCCESS) {
			break;
		}
	}

	return rc;
}

pldm_requester_rc_t pldm_send_msg(struct pldm *pldm, pldm_tid_t tid,
				  const uint8_t *pldm_req_msg,
				  size_t req_msg_len)
{
	return pldm->transport->send(pldm->transport, tid, pldm_req_msg,
				     req_msg_len);
}

/* ---  old APIS written in terms of the new API -- */

/* don't destroy backend as we want the socket to stay open, as some apps might
 * use the fd to it. -- need to check */
pldm_requester_rc_t pldm_open()
{
	int fd;
	pldm_requester_rc_t rc;
	struct pldm *pldm = pldm_init();
	if (!pldm) {
		printf("could not initialise pldm core");
		return -1;
	}
	// init opens up the backend
	struct pldm_transport_mctp_demux *demux = pldm_transport_demux_init();
	if (!demux) {
		printf("could not initialise mctp-demux transport");
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS)
		return PLDM_REQUESTER_OPEN_FAIL;
	// need temp funcs for this
	fd = pldm_transport_mctp_get_socket_fd(pldm->transport);
	pldm_destroy(pldm);
	return fd;
}

pldm_requester_rc_t pldm_recv_any(mctp_eid_t eid, int mctp_fd,
				  uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	struct pldm *pldm = pldm_init();
	pldm_requester_rc_t rc;
	if (!pldm) {
		printf("could not initialise pldm core");
		return -1;
	}
	struct pldm_transport_mctp_demux *demux =
	    pldm_transport_demux_init_with_fd(mctp_fd);
	if (!demux) {
		printf("could not initialise mctp-demux transport");
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS)
		return PLDM_REQUESTER_OPEN_FAIL;

	int tid = pldm_transport_mctp_query_tid(demux, eid);
	rc = pldm_recv_msg_any_inst(pldm, tid, pldm_resp_msg, resp_msg_len);

	pldm_transport_demux_destroy(demux);
	pldm_destroy(pldm);

	return rc;
}

pldm_requester_rc_t pldm_recv(mctp_eid_t eid, int mctp_fd, uint8_t instance_id,
			      uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	struct pldm *pldm = pldm_init();
	pldm_requester_rc_t rc;
	if (!pldm) {
		printf("could not initialise pldm core");
		return -1;
	}
	struct pldm_transport_mctp_demux *demux =
	    pldm_transport_demux_init_with_fd(mctp_fd);
	if (!demux) {
		printf("could not initialise mctp-demux transport");
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS)
		return PLDM_REQUESTER_OPEN_FAIL;

	int tid = pldm_transport_mctp_query_tid(demux, eid);
	rc = pldm_recv_msg(pldm, tid, instance_id, pldm_resp_msg, resp_msg_len);

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
	if (!pldm) {
		printf("could not initialise pldm core");
		return -1;
	}
	struct pldm_transport_mctp_demux *demux =
	    pldm_transport_demux_init_with_fd(mctp_fd);
	if (!demux) {
		printf("could not initialise mctp-demux transport");
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS)
		return PLDM_REQUESTER_OPEN_FAIL;

	int tid = pldm_transport_mctp_query_tid(demux, eid);
	rc = pldm_send_recv_msg(pldm, tid, pldm_req_msg, req_msg_len,
				pldm_resp_msg, resp_msg_len);

	pldm_transport_demux_destroy(demux);
	pldm_destroy(pldm);

	return rc;
}

pldm_requester_rc_t pldm_send(mctp_eid_t eid, int mctp_fd,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)
{
	pldm_requester_rc_t rc;
	struct pldm *pldm = pldm_init();
	if (!pldm) {
		printf("could not initialise pldm core");
		return -1;
	}
	struct pldm_transport_mctp_demux *demux =
	    pldm_transport_demux_init_with_fd(mctp_fd);
	if (!demux) {
		printf("could not initialise mctp-demux transport");
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS)
		return PLDM_REQUESTER_OPEN_FAIL;

	int tid = pldm_transport_mctp_query_tid(demux, eid);
	rc = pldm_send_msg(pldm, tid, pldm_req_msg, req_msg_len);

	pldm_transport_demux_destroy(demux);
	pldm_destroy(pldm);

	return rc;
}
