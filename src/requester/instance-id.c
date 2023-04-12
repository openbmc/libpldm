// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#define _GNU_SOURCE
#include "libpldm/requester/instance-id.h"
#include "libpldm/pldm.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PLDM_TID_MAX 256
#define PLDM_INST_ID_MAX 32

struct pldm_instance_id {
	pldm_inst_id_t prev[PLDM_TID_MAX];
	int lock_db_fd;
};

static inline int iid_next(pldm_inst_id_t cur)
{
	return (cur + 1) % PLDM_INST_ID_MAX;
}

int pldm_instance_id_init(struct pldm_instance_id **ctx, const char *dbpath)
{
	*ctx = calloc(sizeof(struct pldm_instance_id), 1);
	if (!*ctx) {
		return -1;
	}

	/* Initialise previous ID values so the next one is zero */
	for (int i = 0; i < PLDM_TID_MAX; i++) {
		(*ctx)->prev[i] = 31;
	}

	/* Lock database may be read-only, either by permissions or mountpoint
	 */
	(*ctx)->lock_db_fd = open(dbpath, O_RDONLY | O_CLOEXEC);
	if ((*ctx)->lock_db_fd < 0) {
		free(*ctx);
		return -errno;
	}

	return 0;
}

int pldm_instance_id_init_default(struct pldm_instance_id **ctx)
{
	return pldm_instance_id_init(ctx,
				     "/usr/share/libpldm/instance-db/default");
}

int pldm_instance_id_destroy(struct pldm_instance_id *ctx)
{
	if (!ctx) {
		return 0;
	}
	int rc = close(ctx->lock_db_fd);
	/* on Linux and most implementations, the file descriptor is guaranteed
	 * to be closed even if there was an error */
	free(ctx);
	return rc;
}

int pldm_instance_id_alloc(struct pldm_instance_id *ctx, pldm_tid_t tid,
			   pldm_inst_id_t *iid)
{
	static const struct flock cfls = {
	    .l_type = F_RDLCK,
	    .l_whence = SEEK_SET,
	    .l_len = 1,
	};
	static const struct flock cflx = {
	    .l_type = F_WRLCK,
	    .l_whence = SEEK_SET,
	    .l_len = 1,
	};
	uint8_t _iid;

	if (!iid) {
		return -EINVAL;
	}

	_iid = ctx->prev[tid];
	if (_iid >= PLDM_INST_ID_MAX) {
		return -EPROTO;
	}

	while ((_iid = iid_next(_iid)) != ctx->prev[tid]) {
		struct flock flop;
		off_t loff;
		int rc;

		/* Derive the instance ID offset in the lock database */
		loff = tid * PLDM_INST_ID_MAX + _iid;

		/* Reserving the TID's IID. Done via a shared lock */
		flop = cfls;
		flop.l_start = loff;
		rc = fcntl(ctx->lock_db_fd, F_OFD_SETLK, &flop);
		if (rc < 0) {
			return -errno;
		}

		/*
		 * If we *may* promote the lock to exclusive then this IID is
		 * only reserved by us. This is now our allocated IID.
		 *
		 * If we *may not* promote the lock to exclusive then this IID
		 * is also reserved on another file descriptor. Move on to the
		 * next IID index.
		 *
		 * Note that we cannot actually *perform* the promotion in
		 * practice because this is prevented by the lock database being
		 * opened O_RDONLY.
		 */
		flop = cflx;
		flop.l_start = loff;
		rc = fcntl(ctx->lock_db_fd, F_OFD_GETLK, &flop);
		if (rc < 0) {
			return -errno;
		}

		/* F_UNLCK is the type of the lock if we could successfully
		 * promote it to F_WRLCK */
		if (flop.l_type == F_UNLCK) {
			ctx->prev[tid] = _iid;
			*iid = (uint8_t)_iid;
			return 0;
		}
	}

	/* Failed to allocate an IID after a full loop. Make the caller try
	 * again */
	return -EAGAIN;
}

int pldm_instance_id_free(struct pldm_instance_id *ctx, pldm_tid_t tid,
			  pldm_inst_id_t iid)
{
	static const struct flock cflu = {
	    .l_type = F_UNLCK,
	    .l_whence = SEEK_SET,
	    .l_len = 1,
	};
	struct flock flop;
	int rc;

	if (ctx->prev[tid] != iid) {
		return -EINVAL;
	}

	flop = cflu;
	flop.l_start = tid * PLDM_INST_ID_MAX + iid;
	rc = fcntl(ctx->lock_db_fd, F_OFD_SETLK, &flop);
	if (rc < 0) {
		return -errno;
	}

	return 0;
}
