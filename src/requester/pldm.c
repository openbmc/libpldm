#include "libpldm/requester/pldm.h"
#include "../transport/transport.h"
#include "base.h"

#include <bits/types/struct_iovec.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define PLDM_INSTANCE_ID_FILE_PATH "/var/lib/libpldm/"
#define PLDM_INSTANCE_ID_FILE_PREFIX                                           \
	PLDM_INSTANCE_ID_FILE_PATH "pldm_instance_ids_tid_"

/**
 * @brief struct that contains the required elements to do range locking on a
 *	  file that are unique to a file
 *
 * @var fd - file descriptor to the id file
 * @var last_instance_id - the last allocated instance id. This is to make sure
 *	a new request has a different id from the previous request
 * @var setup - indicates if the fd is ready to use
 */
struct pldm_requester_id_file {
	int fd;
	int last_instance_id;
};

/**
 * @brief PLDM requester struct
 *
 * @var pldm_transport - pointer to the transport
 * @var id_files - array of files for ids per TID
 * @var flock - flock struct used to do locking
 */
struct pldm_requester {
	struct pldm_transport *transport;
	struct pldm_requester_id_file id_files[PLDM_MAX_TIDS];
	struct flock *flock;
};

static struct flock *pldm_requester_init_flock(void)
{
	struct flock *flock = malloc(sizeof(struct flock));
	if (flock == NULL) {
		return NULL;
	}
	flock->l_type = F_WRLCK;
	flock->l_whence = SEEK_SET;
	flock->l_start = 0;
	flock->l_len = 1;
	flock->l_pid = getpid();

	return flock;
}

static struct pldm_requester_id_file *
pldm_requester_init_id_file(struct pldm_requester *ctx, pldm_tid_t tid)
{
	int size = strlen(PLDM_INSTANCE_ID_FILE_PREFIX) + 4;
	char *file_name = malloc(size);
	if (!file_name) {
		return NULL;
	}
	snprintf(file_name, size, "%s%d", PLDM_INSTANCE_ID_FILE_PREFIX, tid);

	if (mkdir(PLDM_INSTANCE_ID_FILE_PATH, 0755) == -1 && errno != EEXIST) {
	}

	if ((ctx->id_files[tid].fd =
		 open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
		free(file_name);
		return NULL;
	}
	ctx->id_files[tid].last_instance_id = -1;

	free(file_name);
	return &(ctx->id_files[tid]);
}

static struct pldm_requester_id_file *
pldm_requester_get_id_file(struct pldm_requester *ctx, pldm_tid_t tid)
{
	if (ctx->id_files[tid].fd >= 0) {
		return &(ctx->id_files[tid]);
	}

	return pldm_requester_init_id_file(ctx, tid);
}

static void pldm_requester_close_id_fds(struct pldm_requester *ctx)
{
	for (int i = 0; i < PLDM_MAX_TIDS; i++) {
		if (ctx->id_files[i].fd >= 0) {
			close(ctx->id_files[i].fd);
		}
	}
}

struct pldm_requester *pldm_requester_init(void)
{
	struct pldm_requester *ctx = calloc(sizeof(struct pldm_requester), 1);
	if (!ctx) {
		return NULL;
	}
	ctx->flock = pldm_requester_init_flock();
	for (int i = 0; i < PLDM_MAX_TIDS; i++) {
		ctx->id_files[i].fd = -1;
	}
	if (!ctx->flock) {
		free(ctx);
		return NULL;
	}
	return ctx;
}

pldm_requester_rc_t pldm_requester_destroy(struct pldm_requester *ctx)
{

	if (ctx->transport) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}
	pldm_requester_close_id_fds(ctx);
	free(ctx->flock);
	free(ctx);
	return PLDM_REQUESTER_SUCCESS;
}

pldm_requester_rc_t
pldm_requester_register_transport(struct pldm_requester *ctx,
				  struct pldm_transport *transport)
{
	if (!ctx || ctx->transport != NULL || !transport) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}
	ctx->transport = transport;

	return PLDM_REQUESTER_SUCCESS;
}

pldm_requester_rc_t
pldm_requester_unregister_transports(struct pldm_requester *ctx)
{
	if (!ctx) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}

	ctx->transport = NULL;
	return PLDM_REQUESTER_SUCCESS;
}

pldm_requester_rc_t
pldm_requester_allocate_instance_id(struct pldm_requester *ctx, pldm_tid_t tid,
				    uint8_t *instance_id)
{

	if (!ctx || !ctx->flock || !instance_id) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}

	struct pldm_requester_id_file *id_file =
	    pldm_requester_get_id_file(ctx, tid);
	if (!id_file) {
		return PLDM_REQUESTER_INSTANCE_ID_FAIL;
	}

	ctx->flock->l_type = F_WRLCK;
	int idx = (id_file->last_instance_id + 1) % 32;
	while (idx != id_file->last_instance_id) {
		ctx->flock->l_start = idx;
		if (fcntl(id_file->fd, F_SETLK, ctx->flock) == 0) {
			*instance_id = idx;
			break;
		}
		idx = (idx + 1) % 32;
	}
	if (*instance_id != idx) {
		return PLDM_REQUESTER_INSTANCE_IDS_EXHAUSTED;
	}
	id_file->last_instance_id = idx;
	return PLDM_REQUESTER_SUCCESS;
}

pldm_requester_rc_t pldm_requester_free_instance_id(struct pldm_requester *ctx,
						    pldm_tid_t tid,
						    uint8_t instance_id)
{
	if (!ctx || !ctx->flock) {
		return PLDM_REQUESTER_INVALID_SETUP;
	}

	struct pldm_requester_id_file *id_file =
	    pldm_requester_get_id_file(ctx, tid);
	if (!id_file) {
		return PLDM_REQUESTER_INSTANCE_ID_FAIL;
	}
	ctx->flock->l_type = F_UNLCK;
	ctx->flock->l_start = instance_id;

	if (fcntl(id_file->fd, F_SETLK, ctx->flock) == -1) {
		return PLDM_REQUESTER_INSTANCE_ID_FAIL;
	}

	return PLDM_REQUESTER_SUCCESS;
}

/* Temporary for old api */
#include "libpldm/transport/mctp-demux.h"
#include "libpldm/transport/transport.h"
extern int
pldm_transport_mctp_demux_get_socket_fd(struct pldm_transport_mctp_demux *ctx);
extern struct pldm_transport_mctp_demux *
pldm_transport_mctp_demux_init_with_fd(int mctp_fd);

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

	if (open_transport) {
		return -1;
	}

	struct pldm_transport_mctp_demux *demux =
	    pldm_transport_mctp_demux_init();
	if (!demux) {
		return -1;
	}

	fd = pldm_transport_mctp_demux_get_socket_fd(demux);

	open_transport = demux;

	return fd;
}

/* This macro does the setup and teardown required for the old API to use the
 * new API. Since the setup/teardown logic is the same for all four send/recv
 * functions, it makes sense to only define it once. */
#define PLDM_REQ_FN(eid, fd, fn, ...)                                          \
	do {                                                                   \
		struct pldm_transport_mctp_demux *demux;                       \
		bool using_open_transport = false;                             \
		pldm_requester_rc_t rc;                                        \
		pldm_tid_t tid = 1;                                            \
		struct pldm_transport *ctx;                                    \
		/* The fd can be for a socket we opened or one the consumer    \
		 * opened. */                                                  \
		if (open_transport &&                                          \
		    mctp_fd == pldm_transport_mctp_demux_get_socket_fd(        \
				   open_transport)) {                          \
			using_open_transport = true;                           \
			demux = open_transport;                                \
		} else {                                                       \
			demux = pldm_transport_mctp_demux_init_with_fd(fd);    \
			if (!demux) {                                          \
				rc = PLDM_REQUESTER_OPEN_FAIL;                 \
				goto transport_out;                            \
			}                                                      \
		}                                                              \
		ctx = pldm_transport_mctp_demux_core(demux);                   \
		rc = pldm_transport_mctp_demux_map_tid(demux, tid, eid);       \
		if (rc) {                                                      \
			rc = PLDM_REQUESTER_OPEN_FAIL;                         \
			goto transport_out;                                    \
		}                                                              \
		rc = fn(ctx, tid, __VA_ARGS__);                                \
	transport_out:                                                         \
		if (!using_open_transport) {                                   \
			pldm_transport_mctp_demux_destroy(demux);              \
		}                                                              \
		return rc;                                                     \
	} while (0)

pldm_requester_rc_t pldm_recv_any(mctp_eid_t eid, int mctp_fd,
				  uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	PLDM_REQ_FN(eid, mctp_fd, pldm_transport_recv_msg,
		    (void **)pldm_resp_msg, resp_msg_len);
}

pldm_requester_rc_t pldm_recv(mctp_eid_t eid, int mctp_fd,
			      __attribute__((unused)) uint8_t instance_id,
			      uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
	pldm_requester_rc_t rc =
	    pldm_recv_any(eid, mctp_fd, pldm_resp_msg, resp_msg_len);
	struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)(*pldm_resp_msg);
	if (hdr->instance_id != instance_id) {
		free(*pldm_resp_msg);
		*pldm_resp_msg = NULL;
		return PLDM_REQUESTER_INSTANCE_ID_MISMATCH;
	}
	return rc;
}

pldm_requester_rc_t pldm_send_recv(mctp_eid_t eid, int mctp_fd,
				   const uint8_t *pldm_req_msg,
				   size_t req_msg_len, uint8_t **pldm_resp_msg,
				   size_t *resp_msg_len)
{
	PLDM_REQ_FN(eid, mctp_fd, pldm_transport_send_recv_msg, pldm_req_msg,
		    req_msg_len, (void **)pldm_resp_msg, resp_msg_len);
}

pldm_requester_rc_t pldm_send(mctp_eid_t eid, int mctp_fd,
			      const uint8_t *pldm_req_msg, size_t req_msg_len)
{
	PLDM_REQ_FN(eid, mctp_fd, pldm_transport_send_msg, (void *)pldm_req_msg,
		    req_msg_len);
}

/* Adding this here for completeness in the case we can't smoothly
 * transition apps over to the new api */
void pldm_close()
{
	if (open_transport) {
		pldm_transport_mctp_demux_destroy(open_transport);
	}
	open_transport = NULL;
	return;
}
