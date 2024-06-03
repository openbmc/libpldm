// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
#ifndef LIBPLDM_SRC_DSP_BASE_H
#define LIBPLDM_SRC_DSP_BASE_H

/* Internal functions */

#include <libpldm/base.h>

int pack_pldm_header_errno(const struct pldm_header_info *hdr,
			   struct pldm_msg_hdr *msg);

int unpack_pldm_header_errno(const struct pldm_msg_hdr *msg,
			     struct pldm_header_info *hdr);

#endif
