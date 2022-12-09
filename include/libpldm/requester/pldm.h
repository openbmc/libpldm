#ifndef REQUESTER_PLDM_H
#define REQUESTER_PLDM_H

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
	PLDM_REQUESTER_INSTANCE_ID_FAIL = -12,
	PLDM_REQUESTER_INSTANCE_IDS_EXHAUSTED = -13,
} pldm_requester_rc_t;

/* ------ Old API ---- being deprecated */
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

/* ------ New API ---- */
struct pldm_requester;
struct pldm_transport;

/**
 * @brief Initialises pldm requester instance
 *
 * @return struct pldm_requester.
 */
struct pldm_requester *pldm_requester_init(void);

/**
 * @brief Destroys pldm requester instance.
 *
 * @pre The requester instance should not have any registered transports.
 * 	Returns PLDM_REQUESTER_INVALID_SETUP if there are any.
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t pldm_requester_destroy(struct pldm_requester *ctx);

/**
 * @brief Registers a transport with pldm requester instance
 *
 * @param[in] ctx - pldm requester instance
 * @param[in] transport - PLDM transport
 *
 * @pre Both input parameters must be initialised; otherwise, behaviour is
 * 	undefined.
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t
pldm_requester_register_transport(struct pldm_requester *ctx,
				  struct pldm_transport *transport);

/**
 * @brief Unregisters all transports associated with the pldm requester instance
 *
 * @param[in] ctx - pldm requester instance
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
pldm_requester_rc_t
pldm_requester_unregister_transports(struct pldm_requester *ctx);

/**
 * @brief Allocates an instance id for a destination TID
 *
 * @param[in] ctx - pldm requester instance
 * @param[in] tid - PLDM TID
 * @param[in] instance_id - caller owned pointer to a PLDM instance ID. Return
 * 	      PLDM_REQUESTER_INVALID_SETUP if this is NULL. On success, this
 * 	      points to an instance ID to use for a PLDM request message. If
 * 	      there are no instance IDs available,
 * 	      PLDM_REQUESTER_INSTANCE_IDS_EXHAUSTED is returned. Other failures
 * 	      are indicated by the return code PLDM_REQUESTER_INSTANCE_ID_FAIL.
 *
 * @return pldm_requester_rc_t.
 */
pldm_requester_rc_t
pldm_requester_allocate_instance_id(struct pldm_requester *ctx, pldm_tid_t tid,
				    uint8_t *instance_id);

/**
 * @brief Frees an instance id previously allocated by
 * 	  pldm_requester_allocate_instance_id
 *
 * @param[in] ctx - pldm requester instance
 * @param[in] tid - PLDM TID
 * @param[in] instance_id - If this instance ID was not previously allocated by
 * 	      pldm_requester_allocate_instance_id then the behaviour is
 * 	      undefined.
 *
 * @return pldm_requester_rc_t
 */
pldm_requester_rc_t pldm_requester_free_instance_id(struct pldm_requester *ctx,
						    pldm_tid_t tid,
						    uint8_t instance_id);

#ifdef __cplusplus
}
#endif

#endif /* REQUESTER_PLDM_H */
