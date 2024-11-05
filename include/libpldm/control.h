#pragma once

#include <libpldm/pldm.h>
#include <libpldm/base.h>
#include <libpldm/utils.h>

// Static storage can be allocated with PLDM_SIZEOF_CONTROL macro */
struct pldm_control;

pldm_requester_rc_t pldm_control_handle_msg(struct pldm_control *control,
					    const void *req_msg, size_t req_len,
					    void *resp_msg, size_t *resp_len);

pldm_requester_rc_t pldm_control_setup(struct pldm_control *control,
				       size_t pldm_control_size);

pldm_requester_rc_t pldm_control_add_type(struct pldm_control *control,
					  uint8_t pldm_type,
					  const void *versions,
					  size_t versions_count,
					  const bitfield8_t *commands);
