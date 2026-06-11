/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

#include <libpldm/api.h>
#include <libpldm/pldm_types.h>
#include <libpldm/base.h>
#include <libpldm/control.h>

/** @struct pldm_file_fd_ops
 *
 * Application-provided file I/O operations.  All callbacks are invoked from
 * within pldm_file_fd_handle_msg().
 */
struct pldm_file_fd_ops {
	/** Opaque context passed as ctx to all callbacks. */
	void *ctx;

	/** @brief Open a file.
	 *
	 *  @param[in]  ctx            - ops.ctx
	 *  @param[in]  file_id        - file_identifier from the DfOpen request
	 *  @param[in]  attr           - file_attribute from the DfOpen request
	 *  @param[out] app_handle_out - opaque per-FD handle stored by the
	 *                               responder and passed to subsequent ops
	 *
	 *  @return PLDM_SUCCESS or a pldm_file_cc completion code on failure.
	 */
	uint8_t (*open)(void *ctx, uint16_t file_id, bitfield16_t attr,
			void **app_handle_out);

	/** @brief Close a file.
	 *
	 *  @param[in] ctx        - ops.ctx
	 *  @param[in] file_id    - file_identifier passed to open() for this FD
	 *  @param[in] app_handle - handle returned by open()
	 */
	void (*close)(void *ctx, uint16_t file_id, void *app_handle);

	/** @brief Read data from an open file.
	 *
	 *  @param[in]  ctx        - ops.ctx
	 *  @param[in]  file_id    - file_identifier passed to open() for this FD
	 *  @param[in]  app_handle - handle returned by open()
	 *  @param[in]  offset     - byte offset within the file
	 *  @param[out] buf        - caller-provided buffer to fill
	 *  @param[in]  req_len    - maximum bytes to write into buf
	 *  @param[out] actual_len - actual bytes written; may be < req_len at EOF
	 *
	 *  @return PLDM_SUCCESS or a pldm_file_cc completion code on failure.
	 */
	uint8_t (*read)(void *ctx, uint16_t file_id, void *app_handle,
			uint32_t offset, void *buf, uint32_t req_len,
			uint32_t *actual_len);

	/** @brief Optional: notified of a DfHeartbeat from the requester.
	 *
	 *  On entry, *interval_ms is the requester's proposed max interval
	 *  between DfRead calls. The callback may lower it to whatever
	 *  interval the application requires; the (possibly-adjusted) value
	 *  is echoed back to the requester as-is, with no further clamping
	 *  by the responder.
	 *
	 *  @param[in]     ctx        - ops.ctx
	 *  @param[in]     file_id    - file_identifier passed to open() for
	 *                              this FD
	 *  @param[in]     app_handle - handle returned by open()
	 *  @param[in,out] interval_ms - requester's proposed interval on entry;
	 *                              may be adjusted by the callback
	 */
	void (*heartbeat)(void *ctx, uint16_t file_id, void *app_handle,
			  uint32_t *interval_ms);
};

/* Static storage can be allocated with the PLDM_FILE_FD_BUFFER /
 * PLDM_FILE_FD_SIZE macros below, which require <libpldm/sizes.h>. */
#define PLDM_ALIGNOF_PLDM_FILE_FD __SIZEOF_POINTER__

/** @brief Determine the underlying object size for a struct pldm_file_fd
 *  sized to hold @p n concurrent open files.
 *
 * @pre @p n must be a constant expression
 * @pre <libpldm/sizes.h> must be included for PLDM_SIZEOF_PLDM_FILE_FD and
 *      PLDM_SIZEOF_PLDM_FILE_FD_SLOT
 *
 * @param n The desired number of file-descriptor slots
 */
#define PLDM_FILE_FD_SIZE(n)                                                   \
	((sizeof(char[(__builtin_constant_p(n)) ? 1 : -1])) *                  \
	 (PLDM_SIZEOF_PLDM_FILE_FD + (n) * PLDM_SIZEOF_PLDM_FILE_FD_SLOT))

/** @brief Stack-allocate a buffer to hold a struct pldm_file_fd with @p n
 *  file-descriptor slots.
 *
 * @param name - The variable name used to define the buffer
 * @param n    - The desired number of file-descriptor slots
 */
#define PLDM_FILE_FD_BUFFER(name, n)                                           \
	alignas(PLDM_ALIGNOF_PLDM_FILE_FD) unsigned char(                      \
		name)[PLDM_FILE_FD_SIZE(n)]

/* Opaque — allocate with PLDM_FILE_FD_BUFFER(name, n) from file_fd.h, or
 * PLDM_FILE_FD_SIZE(n) bytes aligned to PLDM_ALIGNOF_PLDM_FILE_FD. */
struct pldm_file_fd;

/** @brief Allocate and initialise a File IO responder.
 *
 *  @param[in] num_fds  - Number of concurrent open-file slots to provide.
 *  @param[in] ops      - Required file operation callbacks. Set ops.ctx to
 *                        the opaque context to pass to each callback.
 *  @param[in] ops_size - sizeof(*ops) as seen by the caller. Pass
 *                        sizeof(struct pldm_file_fd_ops). This enables
 *                        forward and backward compatibility as the struct grows.
 *  @param[in] control  - Optional pldm_control; if non-NULL, registers PLDM
 *                        type 7 (File Transfer) with its commands.
 *
 *  @return malloc-allocated responder owned by the caller; release with
 *          free(). Returns NULL on failure.
 */
struct pldm_file_fd *pldm_file_fd_new(size_t num_fds,
				      const struct pldm_file_fd_ops *ops,
				      size_t ops_size,
				      struct pldm_control *control);

/** @brief Initialise a File IO responder in caller-provided storage.
 *
 *  @param[in] fd              - Pointer to storage of at least the size
 *                               returned by PLDM_FILE_FD_SIZE(num_fds),
 *                               aligned to PLDM_ALIGNOF_PLDM_FILE_FD. May be
 *                               obtained via PLDM_FILE_FD_BUFFER(name, num_fds).
 *  @param[in] pldm_file_fd_size - Pass PLDM_FILE_FD_SIZE(num_fds).
 *  @param[in] num_fds  - Number of concurrent open-file slots to provide.
 *  @param[in] ops      - Required file operation callbacks. Set ops.ctx to
 *                        the opaque context to pass to each callback.
 *  @param[in] ops_size - sizeof(*ops) as seen by the caller. Pass
 *                        sizeof(struct pldm_file_fd_ops).
 *  @param[in] control  - Optional pldm_control.
 *
 *  @return 0 on success, a negative errno value on failure.
 */
int pldm_file_fd_setup(struct pldm_file_fd *fd, size_t pldm_file_fd_size,
		       size_t num_fds, const struct pldm_file_fd_ops *ops,
		       size_t ops_size, struct pldm_control *control);

/** @brief Handle an incoming PLDM message.
 *
 *  Handles PLDM_FILE (type 7) messages for DfOpen, DfClose, and DfHeartbeat,
 *  and PLDM_BASE (type 0) MultipartReceive requests with pldm_type==PLDM_FILE
 *  (DfRead).
 *
 *  @param[in]     fd      - File IO responder context
 *  @param[in]     in_msg  - Incoming PLDM message buffer
 *  @param[in]     in_len  - Length of in_msg
 *  @param[out]    out_msg - Buffer for the outgoing response
 *  @param[in,out] out_len - Buffer size on entry; bytes written on return
 *
 *  @return 0 on success (out_len > 0 means a response was written),
 *          -ENOMSG if the message type/command is not handled here,
 *          a negative errno value on other failure.
 */
int pldm_file_fd_handle_msg(struct pldm_file_fd *fd, const void *in_msg,
			    size_t in_len, void *out_msg, size_t *out_len);

#ifdef __cplusplus
}
#endif
