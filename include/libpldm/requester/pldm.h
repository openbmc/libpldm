#ifndef PLDM_H
#define PLDM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef uint8_t pldm_tid_t;
/* Delete when deleting old api */
typedef uint8_t mctp_eid_t;

typedef enum pldm_requester_error_codes {
	PLDM_REQUESTER_SUCCESS = 0,
	PLDM_REQUESTER_OPEN_FAIL = -1,
	PLDM_REQUESTER_NOT_PLDM_MSG = -2,
	PLDM_REQUESTER_NOT_RESP_MSG = -3,
	PLDM_REQUESTER_NOT_REQ_MSG = -4,
	PLDM_REQUESTER_RESP_MSG_TOO_SMALL = -5,
	PLDM_REQUESTER_INSTANCE_ID_MISMATCH = -6,
	PLDM_REQUESTER_SEND_FAIL = -7,
	PLDM_REQUESTER_RECV_FAIL = -8,
	PLDM_REQUESTER_INVALID_RECV_LEN = -9,
	PLDM_REQUESTER_INVALID_SETUP = -10,
	PLDM_REQUESTER_POLL_FAIL = -11,
} pldm_requester_rc_t;

struct pldm_transport;
/* ------ old api ---- being deprecated */
/**
 * @brief Connect to the MCTP socket and provide an fd to it. The fd can be
 *        used to pass as input to other APIs below, or can be polled.
 *
 * @return fd on success, pldm_requester_rc_t on error (errno may be set)
 */
pldm_requester_rc_t pldm_open(void);

/**
 * @brief Send a PLDM request message. Wait for corresponding response message,
 *        which once received, is returned to the caller.
 *
 * @param[in] eid - destination MCTP eid
 * @param[in] mctp_fd - MCTP socket fd
 * @param[in] pldm_req_msg - caller owned pointer to PLDM request msg
 * @param[in] req_msg_len - size of PLDM request msg
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM response msg,
 *             this function allocates memory, caller to free(*pldm_resp_msg) on
 *             success.
 * @param[out] resp_msg_len - caller owned pointer that will be made point to
 *             the size of the PLDM response msg.
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t pldm_send_recv(mctp_eid_t eid, int mctp_fd,
				   const uint8_t *pldm_req_msg,
				   size_t req_msg_len, uint8_t **pldm_resp_msg,
				   size_t *resp_msg_len);

/**
 * @brief Send a PLDM request message, don't wait for response. Essentially an
 *        async API. A user of this would typically have added the MCTP fd to an
 *        event loop for polling. Once there's data available, the user would
 *        invoke pldm_recv().
 *
 * @param[in] eid - destination MCTP eid
 * @param[in] mctp_fd - MCTP socket fd
 * @param[in] pldm_req_msg - caller owned pointer to PLDM request msg
 * @param[in] req_msg_len - size of PLDM request msg
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t pldm_send(mctp_eid_t eid, int mctp_fd,
			      const uint8_t *pldm_req_msg, size_t req_msg_len);

/**
 * @brief Read MCTP socket. If there's data available, return success only if
 *        data is a PLDM response message that matches eid and instance_id.
 *
 * @param[in] eid - destination MCTP eid
 * @param[in] mctp_fd - MCTP socket fd
 * @param[in] instance_id - PLDM instance id of previously sent PLDM request msg
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM response msg,
 *             this function allocates memory, caller to free(*pldm_resp_msg) on
 *             success.
 * @param[out] resp_msg_len - caller owned pointer that will be made point to
 *             the size of the PLDM response msg.
 *
 * @return pldm_requester_rc_t (errno may be set). failure is returned even
 *         when data was read, but didn't match eid or instance_id.
 */
pldm_requester_rc_t pldm_recv(mctp_eid_t eid, int mctp_fd, uint8_t instance_id,
			      uint8_t **pldm_resp_msg, size_t *resp_msg_len);

/**
 * @brief Read MCTP socket. If there's data available, return success only if
 *        data is a PLDM response message.
 *
 * @param[in] eid - destination MCTP eid
 * @param[in] mctp_fd - MCTP socket fd
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM response msg,
 *             this function allocates memory, caller to free(*pldm_resp_msg) on
 *             success.
 * @param[out] resp_msg_len - caller owned pointer that will be made point to
 *             the size of the PLDM response msg.
 *
 * @return pldm_requester_rc_t (errno may be set). failure is returned even
 *         when data was read, but wasn't a PLDM response message
 */
pldm_requester_rc_t pldm_recv_any(mctp_eid_t eid, int mctp_fd,
				  uint8_t **pldm_resp_msg,
				  size_t *resp_msg_len);

/* ------ new api ---- */
struct pldm_requester;

/**
 * @brief Initialises pldm requester core
 *
 * @return struct pldm_requester.
 */
struct pldm_requester *pldm_init(void);

/**
 * @brief Destroys pldm requester core.
 *
 * @pre The requester core should not have any registered transports.
 */
void pldm_destroy(struct pldm_requester *pldm);

/**
 * @brief Registers a transport with pldm requester core
 *
 * @param[in] pldm - pldm requester core
 * @param[in] transport - PLDM transport
 *
 * @pre Both input parameters must be initialised otherwise behaviour is
 * 	undefined.
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t pldm_register_transport(struct pldm_requester *pldm,
					    struct pldm_transport *transport);

/**
 * @brief Unregisters all transports associated with the pldm core
 *
 * @param[in] pldm - pldm requester core
 */
void pldm_unregister_transports(struct pldm_requester *pldm);

/**
 * @brief Waits for a PLDM response to arrive.
 *
 * @pre The pldm requester core must be initialised and have a registered
 * 	transport. Otherwise PLDM_REQUESTER_INVALID_SETUP is returned. This
 * 	should be called after pldm_send_msg has been called.
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t pldm_wait_for_message(struct pldm_requester *pldm,
					  int timeout);

/**
 * @brief Synchronously send a PLDM request and receive the response. Control is
 * 	  returned to the caller once the response is received.
 *
 * @pre The pldm requester core must be initialised and have a registered
 * 	transport. Otherwise PLDM_REQUESTER_INVALID_SETUP is returned. If the
 * 	transport requires a TID to transport specific identifier mapping, this
 * 	must already be set up.
 *
 *
 * @param[in] pldm - pldm requester core with a registered transport
 * @param[in] tid - destination PLDM TID
 * @param[in] pldm_req_msg - caller owned pointer to PLDM request msg or async
 * 	      notification. Behaviour undefined if this is not a valid PLDM msg.
 * @param[in] req_msg_len - size of PLDM request msg. If this is less than the
 * 	      minimum size of a PLDM msg PLDM_REQUESTER_NOT_REQ_MSG is returned.
 * 	      Otherwise, if this is not the correct length of the PLDM msg,
 * 	      behaviour is undefined.
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM response msg if
 * 	       return code is PLDM_REQUESTER_SUCCESS, otherwise NULL. On success
 * 	       this function allocates memory, caller to free(*pldm_resp_msg).
 * @param[out] resp_msg_len - caller owned pointer that will be made point to
 *             the size of the PLDM response msg.
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t pldm_send_recv_msg(struct pldm_requester *pldm,
				       pldm_tid_t tid, const void *pldm_req_msg,
				       size_t req_msg_len, void **pldm_resp_msg,
				       size_t *resp_msg_len);

/**
 * @brief Asynchronously send a PLDM message. Control is immediately returned to
 * 	  the caller.
 *
 * @pre The pldm requester core must be initialised and have a registered
 * 	transport. Otherwise PLDM_REQUESTER_INVALID_SETUP is returned. If the
 * 	transport requires a TID to transport specific identifier mapping, this
 * 	must already be set up.
 *
 * @param[in] pldm - pldm requester core
 * @param[in] tid - destination PLDM TID
 * @param[in] pldm_req_msg - caller owned pointer to PLDM request msg or async
 *            notification. Behaviour undefined if this is not a valid PLDM msg.
 * @param[in] req_msg_len - size of PLDM request msg. If this is less than the
 * 	      minimum size of a PLDM msg PLDM_REQUESTER_NOT_REQ_MSG is returned.
 * 	      Otherwise, if this is not the correct length of the PLDM msg,
 * 	      behaviour is undefined.
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t pldm_send_msg(struct pldm_requester *pldm, pldm_tid_t tid,
				  const void *pldm_req_msg, size_t req_msg_len);

/**
 * @brief Asynchronously get a PLDM response message for the supplied TID that
 *	  matches the given instance ID. Control is immediately returned to the
 *	  caller.
 *
 * @pre The pldm requester core must be initialised and have a registered
 * 	transport. Otherwise PLDM_REQUESTER_INVALID_SETUP is returned. If the
 * 	transport requires a TID to transport specific identifier mapping, this
 * 	must already be set up.
 *
 * @param[in] pldm - pldm requester core
 * @param[in] tid - destination PLDM TID
 * @param[in] instance_id - PLDM instance id of previously sent PLDM request msg
 * 	      or async notification that we are expecting a response to. If
 * 	      there is no message available with this instance ID an error is
 *	      returned.
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM response msg if
 * 	       return code is PLDM_REQUESTER_SUCCESS, otherwise NULL. On success
 * 	       this function allocates memory, caller to free(*pldm_resp_msg).
 * @param[out] resp_msg_len - caller owned pointer that will be made point to
 *             the size of the PLDM response msg.
 *
 * @return pldm_requester_rc_t (errno may be set). Return success only if there
 *	   is a PLDM response message available for the TID that matches the
 *	   instance ID. Return an error if no message available. Return
 *	   PLDM_REQUESTER_INSTANCE_ID_MISMATCH if data was read, but instance
 *	   IDs didn't match - it is undefined if these erroneous messages can be
 *	   retrieved later.
 */
pldm_requester_rc_t pldm_recv_msg(struct pldm_requester *pldm, pldm_tid_t tid,
				  uint8_t instance_id, void **pldm_resp_msg,
				  size_t *resp_msg_len);

/**
 * @brief Asynchronously get a PLDM response message for the given TID
 * 	  regardless of instance ID. Control is immediately returned to the
 * 	  caller.
 *
 * @pre The pldm requester core must be initialised and have a registered
 * 	transport. Otherwise PLDM_REQUESTER_INVALID_SETUP is returned. If the
 * 	transport requires a TID to transport specific identifier mapping, this
 * 	must already be set up.
 *
 * @param[in] pldm - pldm requester core
 * @param[in] tid - destination PLDM TID
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM response msg if
 * 	       return code is PLDM_REQUESTER_SUCCESS, otherwise NULL. On success
 * 	       this function allocates memory, caller to free(*pldm_resp_msg).
 * @param[out] resp_msg_len - caller owned pointer that will be made point to
 *             the size of the PLDM response msg.
 *
 * @return pldm_requester_rc_t (errno may be set). Failure is returned if no
 * 	   PLDM response messages are available.
 *
 */
pldm_requester_rc_t pldm_recv_msg_any(struct pldm_requester *pldm,
				      pldm_tid_t tid, void **pldm_resp_msg,
				      size_t *resp_msg_len);

#ifdef __cplusplus
}
#endif

#endif /* PLDM_H */
