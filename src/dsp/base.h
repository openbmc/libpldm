// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
#ifndef LIBPLDM_SRC_DSP_BASE_H
#define LIBPLDM_SRC_DSP_BASE_H

/* Internal functions */

#include "compiler.h"
#include <libpldm/base.h>

#define PLDM_BASE_MULTIPART_RECEIVE_RESP_MIN_BYTES 10

int pack_pldm_header_errno(const struct pldm_header_info *hdr,
			   struct pldm_msg_hdr *msg);

int unpack_pldm_header_errno(const struct pldm_msg_hdr *msg,
			     struct pldm_header_info *hdr);

int encode_pldm_header_only_errno(uint8_t msg_type, uint8_t instance_id,
				  uint8_t pldm_type, uint8_t command,
				  struct pldm_msg *msg);

LIBPLDM_CC_ALWAYS_INLINE
int pldm_msg_has_error(const struct pldm_msg *msg, size_t payload_length)
{
	static_assert(PLDM_SUCCESS == 0, "Rework required");
	return payload_length < 1 ? 0 : msg->payload[0];
}

#endif
