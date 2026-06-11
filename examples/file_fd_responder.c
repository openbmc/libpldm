/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
/*
 * file_fd_responder - example PLDM File Transfer (type 7) responder
 *
 * Serves a single file (file_identifier 1), read from the path given as
 * argv[1], over PLDM type 7: DfOpen, DfClose, and DfHeartbeat, plus DfRead
 * which is realised as a PLDM_BASE MultipartReceive request (PLDM type 0)
 * carrying pldm_type==PLDM_FILE.
 *
 * Runs as a loop over stdin, so a whole session (DfOpen, one or more
 * DfReads, DfHeartbeat, DfClose, ...) can be driven in a single invocation.
 * Each message on stdin and stdout is framed as:
 *
 *   uint32_t length (little-endian) || raw PLDM message bytes
 *
 * A zero-length read of the length prefix (EOF) ends the session.
 *
 * Usage:
 *   python3 - <<'PY' | ./file_fd_responder some_file.txt | xxd
 *   import struct, sys
 *
 *   def frame(b):
 *       sys.stdout.buffer.write(struct.pack('<I', len(b)) + b)
 *
 *   # DfOpen: instance 0, request, type 7, DF_OPEN, file_identifier=1
 *   frame(bytes([0x80, 0x07, 0x01]) + struct.pack('<HH', 1, 0))
 *
 *   # DfRead part 1: MultipartReceive, XFER_FIRST_PART, transfer_ctx=1
 *   # (the FileDescriptor from the DfOpen response), section_offset=0,
 *   # section_length=8
 *   frame(bytes([0x80, 0x00, 0x09, 0x07, 0x00]) +
 *         struct.pack('<IIII', 1, 0, 0, 8))
 *
 *   # DfHeartbeat: instance 2, type 7, DF_HEARTBEAT, file_descriptor=1,
 *   # requester_max_interval=1000ms
 *   frame(bytes([0x82, 0x07, 0x03]) + struct.pack('<HI', 1, 1000))
 *
 *   # DfClose: instance 3, type 7, DF_CLOSE, file_descriptor=1,
 *   # df_close_options=0
 *   frame(bytes([0x83, 0x07, 0x02]) + struct.pack('<HH', 1, 0))
 *   PY
 */

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpldm/base.h>
#include <libpldm/file.h>
#include <libpldm/file_fd.h>

/* Maximum size for a single PLDM message on stdin / stdout */
#define MSG_BUF_SIZE 4096

/* Cap on the size of the file loaded from argv[1] */
#define MAX_FILE_SIZE (16ul * 1024ul * 1024ul)

/* Number of concurrently open files the responder supports */
#define NUM_FDS 4

/* The only file this example serves */
#define SERVED_FILE_ID 1

struct served_file {
	uint8_t *data;
	size_t size;
};

/*
 * open callback - called by pldm_file_fd_handle_msg when a DfOpen request
 * arrives.
 *
 * ctx           - the pointer supplied in pldm_file_fd_ops.ctx
 * file_id       - file_identifier from the DfOpen request
 * attr          - file_attribute from the DfOpen request
 * app_handle_out - opaque per-FD handle stored by the responder and passed
 *                  to subsequent close/read/heartbeat calls
 *
 * Returns a PLDM completion code (PLDM_SUCCESS on success).
 */
static uint8_t file_open(void *ctx, uint16_t file_id,
			 bitfield16_t attr __attribute__((unused)),
			 void **app_handle_out)
{
	if (file_id != SERVED_FILE_ID) {
		return PLDM_FILE_CC_INVALID_FILE_IDENTIFIER;
	}

	*app_handle_out = ctx;
	return PLDM_SUCCESS;
}

/*
 * close callback - called by pldm_file_fd_handle_msg when a DfClose request
 * arrives for a previously opened file.
 */
static void file_close(void *ctx __attribute__((unused)),
		       uint16_t file_id __attribute__((unused)),
		       void *app_handle __attribute__((unused)))
{
}

/*
 * read callback - called to service a DfRead (PLDM_BASE MultipartReceive)
 * request against a previously opened file.
 *
 * offset     - byte offset within the file
 * buf        - caller-provided buffer to fill
 * req_len    - maximum bytes to write into buf
 * actual_len - actual bytes written; may be < req_len at EOF
 */
static uint8_t file_read(void *ctx __attribute__((unused)),
			 uint16_t file_id __attribute__((unused)),
			 void *app_handle, uint32_t offset, void *buf,
			 uint32_t req_len, uint32_t *actual_len)
{
	const struct served_file *file = app_handle;
	size_t avail;

	if (offset >= file->size) {
		*actual_len = 0;
		return PLDM_SUCCESS;
	}

	avail = file->size - offset;
	*actual_len = avail < req_len ? (uint32_t)avail : req_len;
	memcpy(buf, file->data + offset, *actual_len);

	return PLDM_SUCCESS;
}

/*
 * heartbeat callback - called when a DfHeartbeat request arrives. This
 * example accepts whatever interval the requester proposes.
 */
static void file_heartbeat(void *ctx __attribute__((unused)),
			   uint16_t file_id __attribute__((unused)),
			   void *app_handle __attribute__((unused)),
			   uint32_t *interval_ms __attribute__((unused)))
{
}

/* Read the whole file at path into a heap buffer, capped at MAX_FILE_SIZE. */
static int load_file(const char *path, struct served_file *file)
{
	long size;
	FILE *f;

	f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "error: fopen %s: %s\n", path, strerror(errno));
		return -1;
	}

	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return -1;
	}
	size = ftell(f);
	if (size < 0 || (size_t)size > MAX_FILE_SIZE) {
		fprintf(stderr, "error: %s too large (max %lu bytes)\n", path,
			MAX_FILE_SIZE);
		fclose(f);
		return -1;
	}
	rewind(f);

	file->data = malloc((size_t)size);
	if (!file->data) {
		fclose(f);
		return -1;
	}

	if (fread(file->data, 1, (size_t)size, f) != (size_t)size) {
		fprintf(stderr, "error: short read on %s\n", path);
		free(file->data);
		fclose(f);
		return -1;
	}
	file->size = (size_t)size;

	fclose(f);
	return 0;
}

/* Read a length-prefixed frame from stdin into buf (capacity buf_size).
 * Returns the frame length on success, 0 on clean EOF, -1 on error. */
static long read_frame(uint8_t *buf, size_t buf_size)
{
	uint32_t len;

	if (fread(&len, sizeof(len), 1, stdin) != 1) {
		return feof(stdin) ? 0 : -1;
	}

	if (len > buf_size) {
		fprintf(stderr,
			"error: frame length %" PRIu32 " exceeds buffer size\n",
			len);
		return -1;
	}

	if (len > 0 && fread(buf, 1, len, stdin) != len) {
		fprintf(stderr, "error: short read on frame body\n");
		return -1;
	}

	return (long)len;
}

/* Write a length-prefixed frame to stdout. */
static int write_frame(const uint8_t *buf, size_t len)
{
	uint32_t len32 = (uint32_t)len;

	if (fwrite(&len32, sizeof(len32), 1, stdout) != 1) {
		return -1;
	}
	if (len > 0 && fwrite(buf, 1, len, stdout) != len) {
		return -1;
	}
	if (fflush(stdout) != 0) {
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	uint8_t in_buf[MSG_BUF_SIZE];
	uint8_t out_buf[MSG_BUF_SIZE];
	struct served_file file;
	struct pldm_file_fd *fd;
	struct pldm_file_fd_ops ops;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (load_file(argv[1], &file) != 0) {
		return EXIT_FAILURE;
	}

	/* Populate the ops table with our file I/O callbacks */
	memset(&ops, 0, sizeof(ops));
	ops.ctx = &file;
	ops.open = file_open;
	ops.close = file_close;
	ops.read = file_read;
	ops.heartbeat = file_heartbeat;

	/* Allocate and initialise the File Transfer responder */
	fd = pldm_file_fd_new(NUM_FDS, &ops, sizeof(ops), NULL);
	if (!fd) {
		fprintf(stderr, "error: pldm_file_fd_new failed\n");
		free(file.data);
		return EXIT_FAILURE;
	}

	for (;;) {
		size_t out_len;
		long in_len;
		int rc;

		in_len = read_frame(in_buf, sizeof(in_buf));
		if (in_len < 0) {
			break;
		}
		if (in_len == 0) {
			/* Clean EOF: end of session */
			break;
		}

		out_len = sizeof(out_buf);
		rc = pldm_file_fd_handle_msg(fd, in_buf, (size_t)in_len,
					     out_buf, &out_len);
		if (rc) {
			fprintf(stderr, "error: pldm_file_fd_handle_msg: %s\n",
				strerror(-rc));
			continue;
		}

		if (out_len > 0) {
			if (write_frame(out_buf, out_len) != 0) {
				fprintf(stderr, "error: write_frame\n");
				break;
			}
		}
	}

	free(fd);
	free(file.data);
	return EXIT_SUCCESS;
}
