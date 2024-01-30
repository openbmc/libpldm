#include "libpldm/rde/pldm_rde.h"
#include "libpldm/base.h"
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LIBPLDM_ABI_STABLE
int encode_negotiate_redfish_parameters_req(uint8_t instance_id,
					    uint8_t concurrency_support,
					    bitfield16_t *feature_support,
					    struct pldm_msg *msg)
{
	if ((msg == NULL) || (feature_support == NULL) ||
	    (concurrency_support == 0)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_NEGOTIATE_REDFISH_PARAMETERS;

	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_rde_negotiate_redfish_parameters_req *req =
		(struct pldm_rde_negotiate_redfish_parameters_req *)msg->payload;
	req->mc_concurrency_support = concurrency_support;
	req->mc_feature_support.value = htole16(feature_support->value);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_negotiate_redfish_parameters_req(const struct pldm_msg *msg,
					    size_t payload_length,
					    uint8_t *mc_concurrency_support,
					    bitfield16_t *mc_feature_support)
{
	if ((msg == NULL) || (mc_concurrency_support == NULL) ||
	    (mc_feature_support == NULL)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (payload_length !=
	    sizeof(struct pldm_rde_negotiate_redfish_parameters_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_rde_negotiate_redfish_parameters_req *request =
		(struct pldm_rde_negotiate_redfish_parameters_req *)msg->payload;
	if (request->mc_concurrency_support == 0) {
		fprintf(stderr, "Invalid concurrency support: 0\n");
		return PLDM_ERROR_INVALID_DATA;
	}
	*mc_concurrency_support = request->mc_concurrency_support;
	mc_feature_support->value = le16toh(request->mc_feature_support.value);
	return PLDM_SUCCESS;
}
