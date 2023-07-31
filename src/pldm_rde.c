#include "pldm_rde.h"

#include "base.h"

#include <endian.h>
#include <stdio.h>
#include <string.h>

LIBPLDM_ABI_TESTING
int encode_negotiate_redfish_parameters_req(uint8_t instance_id,
					    uint8_t concurrency_support,
					    bitfield16_t *feature_support,
					    struct pldm_msg *msg)
{
	if (msg == NULL || feature_support == NULL ||
	    concurrency_support == 0) {
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

LIBPLDM_ABI_TESTING
int decode_negotiate_redfish_parameters_req(const struct pldm_msg *msg,
					    size_t payload_length,
					    uint8_t *mc_concurrency_support,
					    bitfield16_t *mc_feature_support)
{
	if (msg == NULL || mc_concurrency_support == NULL ||
	    mc_feature_support == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length !=
	    sizeof(struct pldm_rde_negotiate_redfish_parameters_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_rde_negotiate_redfish_parameters_req *request =
		(struct pldm_rde_negotiate_redfish_parameters_req *)msg->payload;

	if (request->mc_concurrency_support == 0) {
		fprintf(stderr,
			"Concurrency support has to be greater than 0\n");
		return PLDM_ERROR_INVALID_DATA;
	}

	*mc_concurrency_support = request->mc_concurrency_support;
	mc_feature_support->value = le16toh(request->mc_feature_support.value);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_negotiate_redfish_parameters_resp(
	uint8_t instance_id, uint8_t completion_code,
	uint8_t device_concurrency_support,
	bitfield8_t device_capabilities_flags,
	bitfield16_t device_feature_support,
	uint32_t device_configuration_signature,
	const char *device_provider_name,
	enum pldm_rde_varstring_format name_format, struct pldm_msg *msg)
{
	if (NULL == msg || NULL == device_provider_name) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.command = PLDM_NEGOTIATE_REDFISH_PARAMETERS;

	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_rde_negotiate_redfish_parameters_resp *response =
		(struct pldm_rde_negotiate_redfish_parameters_resp *)
			msg->payload;
	response->completion_code = completion_code;
	if (response->completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}

	response->device_concurrency_support = device_concurrency_support;
	response->device_capabilities_flags.byte =
		device_capabilities_flags.byte;
	response->device_feature_support.value =
		htole16(device_feature_support.value);
	response->device_configuration_signature =
		htole32(device_configuration_signature);

	response->device_provider_name.string_format = name_format;
	// length should include NULL terminator.
	response->device_provider_name.string_length_bytes =
		strlen(device_provider_name) + 1;
	// Copy including NULL terminator.
	memcpy(response->device_provider_name.string_data, device_provider_name,
	       response->device_provider_name.string_length_bytes);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_negotiate_medium_parameters_req(uint8_t instance_id,
					   uint32_t maximum_transfer_size,
					   struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_NEGOTIATE_MEDIUM_PARAMETERS;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_rde_negotiate_medium_parameters_req *req =
		(struct pldm_rde_negotiate_medium_parameters_req *)msg->payload;
	req->mc_maximum_transfer_chunk_size_bytes =
		htole32(maximum_transfer_size);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int decode_negotiate_medium_parameters_req(const struct pldm_msg *msg,
					   size_t payload_length,
					   uint32_t *mc_maximum_transfer_size)
{
	if (msg == NULL || mc_maximum_transfer_size == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length !=
	    sizeof(struct pldm_rde_negotiate_medium_parameters_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_rde_negotiate_medium_parameters_req *request =
		(struct pldm_rde_negotiate_medium_parameters_req *)msg->payload;

	*mc_maximum_transfer_size =
		le32toh(request->mc_maximum_transfer_chunk_size_bytes);
	if (*mc_maximum_transfer_size < PLDM_RDE_MIN_TRANSFER_SIZE_BYTES) {
		return PLDM_ERROR_INVALID_DATA;
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_negotiate_medium_parameters_resp(
	uint8_t instance_id, uint8_t completion_code,
	uint32_t device_maximum_transfer_bytes, struct pldm_msg *msg)
{
	if (NULL == msg) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.command = PLDM_NEGOTIATE_MEDIUM_PARAMETERS;

	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_rde_negotiate_medium_parameters_resp *response =
		(struct pldm_rde_negotiate_medium_parameters_resp *)msg->payload;
	response->completion_code = completion_code;
	if (response->completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}

	response->device_maximum_transfer_chunk_size_bytes =
		htole32(device_maximum_transfer_bytes);
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_get_schema_dictionary_req(uint8_t instance_id, uint32_t resource_id,
				     uint8_t schema_class, struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.msg_type = PLDM_REQUEST;
	header.command = PLDM_GET_SCHEMA_DICTIONARY;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_rde_get_schema_dictionary_req *req =
		(struct pldm_rde_get_schema_dictionary_req *)msg->payload;
	req->resource_id = htole32(resource_id);
	req->requested_schema_class = schema_class;
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int decode_get_schema_dictionary_req(const struct pldm_msg *msg,
				     size_t payload_length,
				     uint32_t *resource_id,
				     uint8_t *requested_schema_class)
{
	if (msg == NULL || resource_id == NULL ||
	    requested_schema_class == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length !=
	    sizeof(struct pldm_rde_get_schema_dictionary_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_rde_get_schema_dictionary_req *request =
		(struct pldm_rde_get_schema_dictionary_req *)msg->payload;

	*resource_id = le32toh(request->resource_id);
	*requested_schema_class = request->requested_schema_class;
	if (*requested_schema_class > PLDM_RDE_SCHEMA_REGISTRY) {
		return PLDM_ERROR_INVALID_DATA;
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_get_schema_dictionary_resp(uint8_t instance_id,
				      uint8_t completion_code,
				      uint8_t dictionary_format,
				      uint32_t transfer_handle,
				      struct pldm_msg *msg)
{
	if (NULL == msg) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_RDE;
	header.command = PLDM_GET_SCHEMA_DICTIONARY;

	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc != PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_rde_get_schema_dictionary_resp *response =
		(struct pldm_rde_get_schema_dictionary_resp *)msg->payload;
	response->completion_code = completion_code;
	if (response->completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}

	response->dictionary_format = dictionary_format;
	response->transfer_handle = htole32(transfer_handle);
	return PLDM_SUCCESS;
}
