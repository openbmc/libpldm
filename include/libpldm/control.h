#pragma once

#include <libpldm/pldm.h>
#include <libpldm/base.h>
#include <libpldm/utils.h>

// Static storage can be allocated with PLDM_SIZEOF_CONTROL macro */
struct pldm_control;

pldm_requester_rc_t pldm_control_handle_msg(struct pldm_control *control,
					    const void *req_msg, size_t req_len,
					    void *resp_msg, size_t *resp_len);
