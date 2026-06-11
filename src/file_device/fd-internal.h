/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <libpldm/compiler.h>
#include <libpldm/pldm_types.h>
#include <libpldm/file_fd.h>

struct pldm_control;

struct pldm_file_fd_slot {
	uint16_t file_descriptor;
	uint16_t file_id;
	bitfield16_t attributes;
	void *app_handle;

	uint32_t read_xfr_handle;
};

struct pldm_file_fd {
	struct pldm_file_fd_ops ops;
	struct pldm_control *control;
	uint32_t next_xfr_handle;
	uint16_t last_fd;
	uint16_t num_fds;
	struct pldm_file_fd_slot fds[] LIBPLDM_CC_COUNTED_BY(num_fds);
};
