#ifndef TRANSPORT_PLDM_H
#define TRANSPORT_PLDM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libpldm/base.h"
#include "libpldm/pldm.h"
#include <stddef.h>

struct pldm_transport;

/**
 * @brief Waits for a PLDM event.
 *
 * @pre The pldm transport instance must be initialised; otherwise,
 * 	PLDM_REQUESTER_INVALID_SETUP is returned. This should be called after
 * 	pldm_transport_send_msg has been called.
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t pldm_transport_poll(struct pldm_transport *transport,
					int timeout);

/**
 * @brief Asynchronously send a PLDM message. Control is immediately returned to
 * 	  the caller.
 *
 * @pre The pldm transport instance must be initialised; otherwise,
 * 	PLDM_REQUESTER_INVALID_SETUP is returned. If the transport requires a
 * 	TID to transport specific identifier mapping, this must already be set
 * 	up.
 *
 * @param[in] ctx - pldm transport instance
 * @param[in] tid - destination PLDM TID
 * @param[in] pldm_req_msg - caller owned pointer to PLDM request msg or async
 *            notification. If this is NULL, PLDM_REQUESTER_INVALID_SETUP is
 * 	      returned.
 * @param[in] req_msg_len - size of PLDM request msg. If this is less than the
 * 	      minimum size of a PLDM msg PLDM_REQUESTER_NOT_REQ_MSG is returned.
 * 	      Otherwise, if this is not the correct length of the PLDM msg,
 * 	      behaviour is undefined.
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t pldm_transport_send_msg(struct pldm_transport *transport,
					    pldm_tid_t tid,
					    const void *pldm_req_msg,
					    size_t req_msg_len);

/**
 * @brief Asynchronously get a PLDM response message for the given TID
 * 	  regardless of instance ID. Control is immediately returned to the
 * 	  caller.
 *
 * @pre The pldm transport instance must be initialised; otherwise,
 * 	PLDM_REQUESTER_INVALID_SETUP is returned. If the transport requires a
 * 	TID to transport specific identifier mapping, this must already be set
 * 	up.
 *
 * @param[in] ctx - pldm transport instance
 * @param[in] tid - destination PLDM TID
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM response msg if
 * 	       return code is PLDM_REQUESTER_SUCCESS; otherwise, NULL. On
 * 	       success this function allocates memory, caller to
 * 	       free(*pldm_resp_msg).
 * @param[out] resp_msg_len - caller owned pointer that will be made to point to
 *             the size of the PLDM response msg. If NULL,
 * 	       PLDM_REQUESTER_INVALID_SETUP is returned.
 *
 * @return pldm_requester_rc_t (errno may be set). Failure is returned if no
 * 	   PLDM response messages are available.
 *
 */
pldm_requester_rc_t pldm_transport_recv_msg(struct pldm_transport *transport,
					    pldm_tid_t tid,
					    void **pldm_resp_msg,
					    size_t *resp_msg_len);

/**
 * @brief Synchronously send a PLDM request and receive the response. Control is
 * 	  returned to the caller once the response is received.
 *
 * @pre The pldm transport instance must be initialised; otherwise,
 * 	PLDM_REQUESTER_INVALID_SETUP is returned. If the transport requires a
 * 	TID to transport specific identifier mapping, this must already be set
 * 	up.
 *
 * @param[in] ctx - pldm transport instance with a registered transport
 * @param[in] tid - destination PLDM TID
 * @param[in] pldm_req_msg - caller owned pointer to PLDM request msg or async
 * 	      notification. If NULL, PLDM_REQUESTER_INVALID_SETUP is returned.
 * @param[in] req_msg_len - size of PLDM request msg. If this is less than the
 * 	      minimum size of a PLDM msg PLDM_REQUESTER_NOT_REQ_MSG is returned.
 * 	      Otherwise, if this is not the correct length of the PLDM msg,
 * 	      behaviour is undefined.
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM response msg if
 * 	       return code is PLDM_REQUESTER_SUCCESS; otherwise, NULL. On
 * 	       success this function allocates memory, caller to
 * 	       free(*pldm_resp_msg).
 * @param[out] resp_msg_len - caller owned pointer that will be made to point to
 *             the size of the PLDM response msg. If NULL,
 * 	       PLDM_REQUESTER_INVALID_SETUP is returned.
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t
pldm_transport_send_recv_msg(struct pldm_transport *transport, pldm_tid_t tid,
			     const void *pldm_req_msg, size_t req_msg_len,
			     void **pldm_resp_msg, size_t *resp_msg_len);

#ifdef __cplusplus
}
#endif

#endif /* TRANSPORT_PLDM_H */
