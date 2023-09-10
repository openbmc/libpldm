#ifndef LIBPLDM_RESPONDER_H
#define LIBPLDM_RESPONDER_H

struct pldm_responder;

int pldm_responder_recv_msg(struct pldm_responder *responder, pldm_tid_t *tid,
			    void **pldm_msg, size_t *len);

int pldm_responder_send_msg(struct pldm_responder *responder, pldm_tid_t tid,
			    const void *pldm_msg, size_t len);

#endif /* LIBPLDM_RESPONDER_H */
