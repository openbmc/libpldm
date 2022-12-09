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
#define MCTP_MSG_TYPE_PLDM 1

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
} pldm_requester_rc_t;

struct pldm_transport {
	const char *name;
	uint8_t version;
	pldm_requester_rc_t (*recv)(struct pldm_transport *transport,
				    pldm_tid_t tid, uint8_t **pldm_resp_msg,
				    size_t *resp_msg_len);
	pldm_requester_rc_t (*send)(struct pldm_transport *transport,
				    pldm_tid_t tid, const uint8_t *pldm_req_msg,
				    size_t req_msg_len);
};

/* ------ old api ---- being deprecated */
/**
 * @brief Connect to the MCTP socket and provide an fd to it. The fd can be
 *        used to pass as input to other APIs below, or can be polled.
 *
 * @return fd on success, pldm_requester_rc_t on error (errno may be set)
 */
pldm_requester_rc_t pldm_open();

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
struct pldm;
struct pldm *pldm_init(void);
void pldm_destroy(struct pldm *pldm);

/**
 * @brief Registers a transport with pldm core
 *
 * @param[in] pldm - PLDM core
 * @param[in] transport - PLDM transport
 *
 * @return int.
 */
int pldm_register_transport(struct pldm *pldm,
			    struct pldm_transport *transport);

/**
 * @brief Unregisters the transport associated with the pldm core
 *
 * @param[in] pldm - PLDM core
 */
void pldm_unregister_transport(struct pldm *pldm);

/**
 * @brief Send a PLDM request message. Wait for corresponding response message,
 *        which once received, is returned to the caller. Blocking call.
 *
 * @param[in] pldm - PLDM core
 * @param[in] tid - destination PLDM TID
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
pldm_requester_rc_t pldm_send_recv_msg(struct pldm *pldm, pldm_tid_t tid,
				       const uint8_t *pldm_req_msg,
				       size_t req_msg_len,
				       uint8_t **pldm_resp_msg,
				       size_t *resp_msg_len);

/**
 * @brief Send a PLDM request message, don't wait for response.  A user of this
 * 	  would typically have added the transports fd to an event loop for
 * polling. Once there's data available, the user would invoke pldm_recv_msg().
 *
 * @param[in] pldm - PLDM core
 * @param[in] tid - destination PLDM TID
 * @param[in] pldm_req_msg - caller owned pointer to PLDM request msg
 * @param[in] req_msg_len - size of PLDM request msg
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t pldm_send_msg(struct pldm *pldm, pldm_tid_t tid,
				  const uint8_t *pldm_req_msg,
				  size_t req_msg_len);

/**
 * @brief Get a PLDM response message. Return success only if
 *        data is a PLDM response message that matches the tid and instance_id.
 *
 *
 * @param[in] pldm - PLDM core
 * @param[in] tid - destination PLDM TID
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
pldm_requester_rc_t pldm_recv_msg(struct pldm *pldm, pldm_tid_t tid,
				  uint8_t instance_id, uint8_t **pldm_resp_msg,
				  size_t *resp_msg_len);

/**
 * @brief Get a PLDM response message for the given TID regardless of
 * instance_id. Return failure if no PLDM messages there.
 *
 * @param[in] pldm - PLDM core
 * @param[in] tid - destination PLDM TID
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM response msg,
 *             this function allocates memory, caller to free(*pldm_resp_msg) on
 *             success.
 * @param[out] resp_msg_len - caller owned pointer that will be made point to
 *             the size of the PLDM response msg.
 *
 * @return pldm_requester_rc_t (errno may be set). failure is returned even
 *         when data was read, but wasn't a PLDM response message
 */
pldm_requester_rc_t pldm_recv_msg_any_inst(struct pldm *pldm, pldm_tid_t tid,
					   uint8_t **pldm_resp_msg,
					   size_t *resp_msg_len);

#ifdef __cplusplus
}
#endif

#endif /* PLDM_H */
