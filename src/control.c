#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <libpldm/pldm.h>
#include <libpldm/utils.h>
#include <libpldm/platform.h>
#include <libpldm/control.h>
#include <compiler.h>
#include <msgbuf.h>

#include "control-internal.h"

#define PLDM_BASE_VERSIONS_COUNT 2
static const uint32_t PLDM_BASE_VERSIONS[PLDM_BASE_VERSIONS_COUNT] = {
	/* PLDM 1.1.0 is current implemented. */
	0xf1f1f000,
	/* CRC. Calculated with python:
	hex(crccheck.crc.Crc32.calc(struct.pack('<I', 0xf1f1f000)))
	*/
	0x539dbeba,
};
const bitfield8_t PLDM_CONTROL_COMMANDS[32] = {
	// 0x00..0x07
	{ .byte = (1 << PLDM_GET_TID | 1 << PLDM_GET_PLDM_VERSION |
		   1 << PLDM_GET_PLDM_TYPES | 1 << PLDM_GET_PLDM_COMMANDS) }
};

static int pldm_control_reply_error(uint8_t ccode,
				    const struct pldm_header_info *req_hdr,
				    struct pldm_msg *resp,
				    size_t *resp_payload_len)
{
	int rc;

	/* 1 byte completion code */
	if (*resp_payload_len < 1) {
		return -EOVERFLOW;
	}
	*resp_payload_len = 1;

	rc = encode_cc_only_resp(req_hdr->instance, PLDM_FWUP, req_hdr->command,
				 ccode, resp);
	if (rc != PLDM_SUCCESS) {
		return -EINVAL;
	}
	return 0;
}

static int pldm_control_get_tid(const struct pldm_header_info *hdr,
				const struct pldm_msg *req LIBPLDM_CC_UNUSED,
				size_t req_payload_len, struct pldm_msg *resp,
				size_t *resp_payload_len)
{
	if (req_payload_len != PLDM_GET_TID_REQ_BYTES) {
		return pldm_control_reply_error(PLDM_ERROR_INVALID_LENGTH, hdr,
						resp, resp_payload_len);
	}

	if (*resp_payload_len <= PLDM_GET_TID_RESP_BYTES) {
		return -EOVERFLOW;
	}
	*resp_payload_len = PLDM_GET_TID_RESP_BYTES;

	uint8_t cc = encode_get_tid_resp(hdr->instance, PLDM_SUCCESS,
					 PLDM_TID_UNASSIGNED, resp);
	if (cc) {
		return pldm_control_reply_error(cc, hdr, resp,
						resp_payload_len);
	}
	return 0;
}

static int pldm_control_get_version(struct pldm_control *control,
				    const struct pldm_header_info *hdr,
				    const struct pldm_msg *req,
				    size_t req_payload_len,
				    struct pldm_msg *resp,
				    size_t *resp_payload_len)
{
	uint8_t cc;

	uint32_t handle;
	uint8_t opflag;
	uint8_t type;
	cc = decode_get_version_req(req, req_payload_len, &handle, &opflag,
				    &type);
	if (cc) {
		return pldm_control_reply_error(cc, hdr, resp,
						resp_payload_len);
	}

	/* Response is always sent as a single transfer */
	if (opflag != PLDM_GET_FIRSTPART) {
		return pldm_control_reply_error(
			PLDM_PLATFORM_INVALID_TRANSFER_OPERATION_FLAG, hdr,
			resp, resp_payload_len);
	}

	const struct pldm_type_versions *v = NULL;
	for (int i = 0; i < PLDM_CONTROL_MAX_VERSION_TYPES; i++) {
		if (control->types[i].pldm_type == type &&
		    control->types[i].versions) {
			v = &control->types[i];
			break;
		}
	}

	if (!v) {
		return pldm_control_reply_error(
			PLDM_PLATFORM_INVALID_PLDM_TYPE_IN_REQUEST_DATA, hdr,
			resp, resp_payload_len);
	}

	/* encode_get_version_resp doesn't have length checking */
	uint32_t required_resp_payload =
		1 + 4 + 1 + v->versions_count * sizeof(ver32_t);
	if (*resp_payload_len < required_resp_payload) {
		return -EOVERFLOW;
	}
	*resp_payload_len = required_resp_payload;

	/* crc32 is included in the versions buffer */
	cc = encode_get_version_resp(hdr->instance, PLDM_SUCCESS, 0,
				     PLDM_START_AND_END, v->versions,
				     v->versions_count * sizeof(ver32_t), resp);
	if (cc) {
		return pldm_control_reply_error(cc, hdr, resp,
						resp_payload_len);
	}
	return 0;
}

static int pldm_control_get_types(struct pldm_control *control,
				  const struct pldm_header_info *hdr,
				  const struct pldm_msg *req LIBPLDM_CC_UNUSED,
				  size_t req_payload_len, struct pldm_msg *resp,
				  size_t *resp_payload_len)
{
	uint8_t cc;

	if (req_payload_len != PLDM_GET_TYPES_REQ_BYTES) {
		return pldm_control_reply_error(PLDM_ERROR_INVALID_LENGTH, hdr,
						resp, resp_payload_len);
	}

	bitfield8_t types[8];
	memset(types, 0, sizeof(types));
	for (int i = 0; i < PLDM_CONTROL_MAX_VERSION_TYPES; i++) {
		uint8_t ty = control->types[i].pldm_type;
		if (ty < 64 && control->types[i].versions) {
			uint8_t bit = 1 << (ty % 8);
			types[ty / 8].byte |= bit;
		}
	}

	/* encode_get_types_resp doesn't have length checking */
	uint32_t required_resp_payload = 1 + 8;
	if (*resp_payload_len < required_resp_payload) {
		return -EOVERFLOW;
	}
	*resp_payload_len = required_resp_payload;

	cc = encode_get_types_resp(hdr->instance, PLDM_SUCCESS, types, resp);
	if (cc) {
		return pldm_control_reply_error(cc, hdr, resp,
						resp_payload_len);
	}
	return 0;
}

static int pldm_control_get_commands(struct pldm_control *control,
				     const struct pldm_header_info *hdr,
				     const struct pldm_msg *req,
				     size_t req_payload_len,
				     struct pldm_msg *resp,
				     size_t *resp_payload_len)
{
	uint8_t cc;

	uint8_t ty;
	// version in request is ignored, since SelectPLDMVersion isn't supported currently
	ver32_t version;
	cc = decode_get_commands_req(req, req_payload_len, &ty, &version);
	if (cc) {
		return pldm_control_reply_error(cc, hdr, resp,
						resp_payload_len);
	}

	const struct pldm_type_versions *v = NULL;
	for (int i = 0; i < PLDM_CONTROL_MAX_VERSION_TYPES; i++) {
		if (control->types[i].pldm_type == ty &&
		    control->types[i].versions && control->types[i].commands) {
			v = &control->types[i];
			break;
		}
	}

	if (!v) {
		return pldm_control_reply_error(
			PLDM_PLATFORM_INVALID_PLDM_TYPE_IN_REQUEST_DATA, hdr,
			resp, resp_payload_len);
	}

	/* encode_get_commands_resp doesn't have length checking */
	uint32_t required_resp_payload = 1 + 32;
	if (*resp_payload_len < required_resp_payload) {
		return -EOVERFLOW;
	}
	*resp_payload_len = required_resp_payload;

	cc = encode_get_commands_resp(hdr->instance, PLDM_SUCCESS, v->commands,
				      resp);
	if (cc) {
		return pldm_control_reply_error(cc, hdr, resp,
						resp_payload_len);
	}
	return 0;
}

/* A response should only be used when this returns 0, and *resp_len > 0 */
LIBPLDM_ABI_TESTING
int pldm_control_handle_msg(struct pldm_control *control, const void *req_msg,
			    size_t req_len, void *resp_msg, size_t *resp_len)
{
	int rc;

	/* Space for header plus completion code */
	if (*resp_len < sizeof(struct pldm_msg_hdr) + 1) {
		return -EOVERFLOW;
	}
	size_t resp_payload_len = *resp_len - sizeof(struct pldm_msg_hdr);
	struct pldm_msg *resp = resp_msg;

	if (req_len < sizeof(struct pldm_msg_hdr)) {
		return -EOVERFLOW;
	}
	size_t req_payload_len = req_len - sizeof(struct pldm_msg_hdr);
	const struct pldm_msg *req = req_msg;

	struct pldm_header_info hdr;
	rc = unpack_pldm_header(&req->hdr, &hdr);
	if (rc != PLDM_SUCCESS) {
		return -EINVAL;
	}

	if (hdr.pldm_type != PLDM_BASE) {
		/* Caller should not have passed non-control */
		return -ENOMSG;
	}

	if (hdr.msg_type != PLDM_REQUEST) {
		return -EINVAL;
	}

	/* Dispatch command */
	switch (hdr.command) {
	case PLDM_GET_TID:
		rc = pldm_control_get_tid(&hdr, req, req_payload_len, resp,
					  &resp_payload_len);
		break;
	case PLDM_GET_PLDM_VERSION:
		rc = pldm_control_get_version(control, &hdr, req,
					      req_payload_len, resp,
					      &resp_payload_len);
		break;
	case PLDM_GET_PLDM_TYPES:
		rc = pldm_control_get_types(control, &hdr, req, req_payload_len,
					    resp, &resp_payload_len);
		break;
	case PLDM_GET_PLDM_COMMANDS:
		rc = pldm_control_get_commands(control, &hdr, req,
					       req_payload_len, resp,
					       &resp_payload_len);
		break;
	default:
		rc = pldm_control_reply_error(PLDM_ERROR_UNSUPPORTED_PLDM_CMD,
					      &hdr, resp, &resp_payload_len);
	}

	if (rc == 0) {
		*resp_len = resp_payload_len + sizeof(struct pldm_msg_hdr);
	}

	return rc;
}

LIBPLDM_ABI_TESTING
int pldm_control_setup(struct pldm_control *control, size_t pldm_control_size)
{
	int rc;

	if (pldm_control_size < sizeof(struct pldm_control)) {
		return -EINVAL;
	}

	memset(control, 0, sizeof(struct pldm_control));

	rc = pldm_control_add_type(control, PLDM_BASE, &PLDM_BASE_VERSIONS,
				   PLDM_BASE_VERSIONS_COUNT,
				   PLDM_CONTROL_COMMANDS);
	if (rc) {
		return rc;
	}

	return 0;
}

LIBPLDM_ABI_TESTING
int pldm_control_add_type(struct pldm_control *control, uint8_t pldm_type,
			  const void *versions, size_t versions_count,
			  const bitfield8_t *commands)
{
	if (versions_count < 2) {
		/* At least one version must be provided, along with a CRC32 */
		return -EINVAL;
	}

	struct pldm_type_versions *v = NULL;
	for (int i = 0; i < PLDM_CONTROL_MAX_VERSION_TYPES; i++) {
		if (control->types[i].versions == NULL ||
		    (control->types[i].versions != NULL &&
		     control->types[i].pldm_type == pldm_type)) {
			v = &control->types[i];
			break;
		}
	}

	if (!v) {
		return -ENOMEM;
		// No spare slots
	}

	v->pldm_type = pldm_type;
	v->versions = versions;
	v->versions_count = versions_count;
	v->commands = commands;

	return 0;
}
