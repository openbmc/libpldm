#include "libpldm/transport.h"
#include "base.h"
#include "libpldm/requester/pldm.h"
#include "transport.h"

#include <errno.h>
#include <limits.h>
#ifdef PLDM_HAS_POLL
#include <poll.h>
#endif
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef PLDM_HAS_POLL
struct pollfd {
	int fd;	       /* file descriptor */
	short events;  /* requested events */
	short revents; /* returned events */
};

static inline int poll(struct pollfd *fds __attribute__((unused)),
		       int nfds __attribute__((unused)),
		       int timeout __attribute__((unused)))
{
	return 0;
}
#endif

LIBPLDM_ABI_TESTING
pldm_requester_rc_t pldm_transport_poll(struct pldm_transport *transport,
					int timeout)
{
	struct pollfd pollfd;
	int rc = 0;
	if (!transport) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}
	if (!transport->init_pollfd) {
		return PLDM_REQUESTER_SUCCESS;
	}

	rc = transport->init_pollfd(transport, &pollfd);
	if (rc < 0) {
		return PLDM_REQUESTER_POLL_FAIL;
	}

	rc = poll(&pollfd, 1, timeout);
	if (rc < 0) {
		return PLDM_REQUESTER_POLL_FAIL;
	}

	return PLDM_REQUESTER_SUCCESS;
}

LIBPLDM_ABI_TESTING
pldm_requester_rc_t pldm_transport_send_msg(struct pldm_transport *transport,
					    pldm_tid_t tid,
					    const void *pldm_req_msg,
					    size_t req_msg_len)
{
	if (!transport || !pldm_req_msg) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}

	if (req_msg_len < sizeof(struct pldm_msg_hdr)) {
		return PLDM_REQUESTER_NOT_REQ_MSG;
	}

	return transport->send(transport, tid, pldm_req_msg, req_msg_len);
}

LIBPLDM_ABI_TESTING
pldm_requester_rc_t pldm_transport_recv_msg(struct pldm_transport *transport,
					    pldm_tid_t tid,
					    void **pldm_resp_msg,
					    size_t *resp_msg_len)
{
	if (!transport || !resp_msg_len) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}

	pldm_requester_rc_t rc =
		transport->recv(transport, tid, pldm_resp_msg, resp_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}

	if (*resp_msg_len < sizeof(struct pldm_msg_hdr)) {
		free(*pldm_resp_msg);
		*pldm_resp_msg = NULL;
		return PLDM_REQUESTER_INVALID_RECV_LEN;
	}
	return PLDM_REQUESTER_SUCCESS;
}

static void timespec_to_timeval(const struct timespec *ts, struct timeval *tv)
{
	tv->tv_sec = ts->tv_sec;
	tv->tv_usec = ts->tv_nsec / 1000;
}

/* Overflow safety must be upheld before call */
static long timeval_to_msec(const struct timeval *tv)
{
	return tv->tv_sec * 1000 + tv->tv_usec / 1000;
}

/* If calculations on `tv` don't overflow then operations on derived
 * intervals can't either.
 */
static bool timeval_is_valid(const struct timeval *tv)
{
	if (tv->tv_sec < 0 || tv->tv_usec < 0 || tv->tv_usec >= 1000000) {
		return false;
	}

	if (tv->tv_sec > (LONG_MAX - tv->tv_usec / 1000) / 1000) {
		return false;
	}

	return true;
}

static int clock_gettimeval(clockid_t clockid, struct timeval *tv)
{
	struct timespec now;
	int rc;

	rc = clock_gettime(clockid, &now);
	if (rc < 0) {
		return rc;
	}

	timespec_to_timeval(&now, tv);

	return 0;
}

LIBPLDM_ABI_TESTING
pldm_requester_rc_t
pldm_transport_send_recv_msg(struct pldm_transport *transport, pldm_tid_t tid,
			     const void *pldm_req_msg, size_t req_msg_len,
			     void **pldm_resp_msg, size_t *resp_msg_len)

{
	/**
	 * Section "Requirements for requesters" in DSP0240, define the Time-out
	 * waiting for a response of the requester.
	 * PT2max = PT3min - 2*PT4max = 4800ms
	 */
	static const struct timeval max_response_interval = {
		.tv_sec = 4, .tv_usec = 800000
	};
	const struct pldm_msg_hdr *req_hdr;
	struct timeval remaining;
	pldm_requester_rc_t rc;
	struct timeval now;
	struct timeval end;
	int ret;

	if (req_msg_len < sizeof(*req_hdr) || !resp_msg_len) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}

	req_hdr = pldm_req_msg;

	rc = pldm_transport_send_msg(transport, tid, pldm_req_msg, req_msg_len);
	if (rc != PLDM_REQUESTER_SUCCESS) {
		return rc;
	}

	ret = clock_gettimeval(CLOCK_MONOTONIC, &now);
	if (ret < 0) {
		return PLDM_REQUESTER_POLL_FAIL;
	}

	timeradd(&now, &max_response_interval, &end);
	if (!timeval_is_valid(&end)) {
		return PLDM_REQUESTER_POLL_FAIL;
	}

	do {
		timersub(&end, &now, &remaining);
		/* 0 <= `timeval_to_msec()` <= 4800, and 4800 < INT_MAX */
		rc = pldm_transport_poll(transport,
					 (int)(timeval_to_msec(&remaining)));
		if (rc != PLDM_REQUESTER_SUCCESS) {
			return rc;
		}

		rc = pldm_transport_recv_msg(transport, tid, pldm_resp_msg,
					     resp_msg_len);
		if (rc == PLDM_REQUESTER_SUCCESS) {
			const struct pldm_msg_hdr *resp_hdr = *pldm_resp_msg;
			if (req_hdr->instance_id == resp_hdr->instance_id) {
				return rc;
			}

			/* This isn't the message we wanted */
			free(*pldm_resp_msg);
		}

		ret = clock_gettimeval(CLOCK_MONOTONIC, &now);
		if (ret < 0) {
			return PLDM_REQUESTER_POLL_FAIL;
		}
	} while (!timercmp(&now, &end, <));

	return PLDM_REQUESTER_RECV_FAIL;
}
