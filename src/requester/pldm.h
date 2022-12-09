#ifndef PLDM_INTERNAL_H
#define PLDM_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libpldm/requester/pldm.h"
struct pldm_transport {
	const char *name;
	uint8_t version;
	pldm_requester_rc_t (*recv)(struct pldm_transport *transport,
				    pldm_tid_t tid, void **pldm_resp_msg,
				    size_t *resp_msg_len);
	pldm_requester_rc_t (*send)(struct pldm_transport *transport,
				    pldm_tid_t tid, const void *pldm_req_msg,
				    size_t req_msg_len);
};

#ifdef __cplusplus
}
#endif

#endif // PLDM_INTERNAL_H
