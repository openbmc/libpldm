#include "pldm.h"
#include "base.h"

#include <bits/types/struct_iovec.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "libpldm/requester/mctp-demux.h"
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
		pldm = NULL;
	}
}

pldm_requester_rc_t pldm_recv_msg_any_inst(struct pldm *pldm, pldm_tid_t tid,
					   uint8_t **pldm_resp_msg,
					   size_t *resp_msg_len)
{
	if (!pldm->transport) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	pldm_requester_rc_t rc = pldm->transport->recv(
	    pldm->transport, tid, pldm_resp_msg, resp_msg_len);
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

pldm_requester_rc_t pldm_recv_msg(struct pldm *pldm, pldm_tid_t tid,
				  uint8_t instance_id, uint8_t **pldm_resp_msg,
				  size_t *resp_msg_len)
{
	if (!pldm->transport) {
		return PLDM_REQUESTER_RECV_FAIL;
	}

	pldm_requester_rc_t rc =
	    pldm_recv_msg_any_inst(pldm, tid, pldm_resp_msg, resp_msg_len);
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

pldm_requester_rc_t pldm_send_recv_msg(struct pldm *pldm, pldm_tid_t tid,
				       const uint8_t *pldm_req_msg,
				       size_t req_msg_len,
				       uint8_t **pldm_resp_msg,
				       size_t *resp_msg_len)
{
	if (!pldm->transport) {
		return PLDM_REQUESTER_SEND_FAIL;
	}

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
	if (!pldm->transport) {
		return PLDM_REQUESTER_SEND_FAIL;
	}

	return pldm->transport->send(pldm->transport, tid, pldm_req_msg,
				     req_msg_len);
}

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
	pldm_requester_rc_t rc;

	if (open_transport) {
		return -1;
	}

	struct pldm *pldm = pldm_init();
	if (!pldm) {
		return -1;
	}
	struct pldm_transport_mctp_demux *demux = pldm_transport_demux_init();
	if (!demux) {
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return PLDM_REQUESTER_OPEN_FAIL;
	}

	// Doing this rather than using pollfd because that would create more
	// memory used that we can't neccesarily clean up.
	fd = pldm_transport_mctp_get_socket_fd(pldm->transport);

	open_transport = demux;

	pldm_destroy(pldm);
	return fd;
}

/*
 * The fd can be for a socket we opened or one the consumer opened. So while we
 * could check if the fd matches the one in the open_transport struct we (might)
 * have, if it doesn't we still have to use it anyway. So let's just use the fd
 * blindly.
 */
pldm_requester_rc_t pldm_recv_any(mctp_eid_t eid, int mctp_fd,
				  uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	struct pldm *pldm = pldm_init();
	pldm_requester_rc_t rc;
	if (!pldm) {
		return -1;
	}

	struct pldm_transport_mctp_demux *demux =
	    pldm_transport_demux_init_with_fd(mctp_fd);
	if (!demux) {
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return PLDM_REQUESTER_OPEN_FAIL;
	}

	pldm_tid_t tid;
	rc = pldm_transport_mctp_get_tid_from_eid(demux, &tid, eid);
	if (rc) {
		return PLDM_REQUESTER_RECV_FAIL;
	}
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
		return -1;
	}
	struct pldm_transport_mctp_demux *demux =
	    pldm_transport_demux_init_with_fd(mctp_fd);
	if (!demux) {
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return PLDM_REQUESTER_OPEN_FAIL;
	}

	pldm_tid_t tid;
	rc = pldm_transport_mctp_get_tid_from_eid(demux, &tid, eid);
	if (rc) {
		return PLDM_REQUESTER_RECV_FAIL;
	}
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
		return -1;
	}
	struct pldm_transport_mctp_demux *demux =
	    pldm_transport_demux_init_with_fd(mctp_fd);
	if (!demux) {
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return PLDM_REQUESTER_OPEN_FAIL;
	}

	pldm_tid_t tid;
	rc = pldm_transport_mctp_get_tid_from_eid(demux, &tid, eid);
	if (rc) {
		return PLDM_REQUESTER_SEND_FAIL;
	}
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
		return -1;
	}
	struct pldm_transport_mctp_demux *demux =
	    pldm_transport_demux_init_with_fd(mctp_fd);
	if (!demux) {
		return -1;
	}
	rc = pldm_register_transport(pldm, pldm_transport_demux_core(demux));
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return PLDM_REQUESTER_OPEN_FAIL;
	}

	pldm_tid_t tid;
	rc = pldm_transport_mctp_get_tid_from_eid(demux, &tid, eid);
	if (rc) {
		return PLDM_REQUESTER_SEND_FAIL;
	}
	rc = pldm_send_msg(pldm, tid, pldm_req_msg, req_msg_len);

	pldm_transport_demux_destroy(demux);
	pldm_destroy(pldm);

	return rc;
}

/* Adding this here for completeness in the case we can't smoothly transition
 * apps over to the new api */
void pldm_close()
{
	if (open_transport) {
		pldm_transport_demux_destroy(open_transport);
	}
	return;
}
