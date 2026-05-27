/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include "compiler.h"
#include "pd-internal.h"

#include <libpldm/pldm.h>
#include <libpldm/base.h>
#include <libpldm/platform.h>
#include <libpldm/pdr.h>
#include <libpldm/platform_device.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdalign.h>
#include <endian.h>
#include <errno.h>

/* PLDM Platform Monitoring and Control 1.2.0 (DSP0248) */
#define PLDM_PD_VERSIONS_COUNT 2
static const uint32_t PLDM_PD_VERSIONS[PLDM_PD_VERSIONS_COUNT] = {
	0xf1f2f000,
	/* CRC: hex(crccheck.crc.Crc32.calc(struct.pack('<I', 0xf1f2f000))) */
	0x78b0ed79,
};

/* Command bitmap for PLDM_PLATFORM (type 0x02).
 * byte[n] covers commands n*8 to n*8+7.
 * Supported: GET_SENSOR_READING(0x11),
 *            GET_PDR_REPOSITORY_INFO(0x50), GET_PDR(0x51)
 * Update this bitmap if adding new command handlers. */
const bitfield8_t PLDM_PD_COMMANDS[32] = {
	/* bytes 0..1: no supported commands */
	{ 0 },
	{ 0 },
	/* byte 2: commands 0x10..0x17, GET_SENSOR_READING=0x11 */
	{ .byte = 1u << (PLDM_GET_SENSOR_READING & 7u) },
	/* bytes 3..9: no supported commands */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	/* byte 10: commands 0x50..0x57
	 * GET_PDR_REPOSITORY_INFO=0x50, GET_PDR=0x51 */
	{ .byte = (1u << (PLDM_GET_PDR_REPOSITORY_INFO & 7u) |
		   1u << (PLDM_GET_PDR & 7u)) },
	/* bytes 11..31: no supported commands */
};

/* Ensure the public alignment constant matches the actual struct */
static_assert(alignof(struct pldm_platform_pd) == PLDM_ALIGNOF_PLDM_PLATFORM_PD,
	      "PLDM_ALIGNOF_PLDM_PLATFORM_PD wrong");

/* Maybe called with success or failure completion codes, though
 * success only makes sense for responses without a body.
 * Returns 0 or negative errno. */
LIBPLDM_CC_NONNULL
static int pldm_platform_pd_reply_cc(uint8_t ccode,
				     const struct pldm_header_info *req_hdr,
				     struct pldm_msg *resp,
				     size_t *resp_payload_len)
{
	int status;

	/* 1 byte completion code */
	if (*resp_payload_len < 1) {
		return -EOVERFLOW;
	}
	*resp_payload_len = 1;

	status = encode_cc_only_resp(req_hdr->instance, PLDM_PLATFORM,
				     req_hdr->command, ccode, resp);
	if (status != PLDM_SUCCESS) {
		return -EINVAL;
	}
	return 0;
}

/* Must be called with a negative errno.
 * Returns 0 or negative errno. */
LIBPLDM_CC_NONNULL
static int pldm_platform_pd_reply_errno(int err,
					const struct pldm_header_info *req_hdr,
					struct pldm_msg *resp,
					size_t *resp_payload_len)
{
	uint8_t ccode = PLDM_ERROR;

	assert(err < 0);
	switch (err) {
	case -EINVAL:
		// internal error, shouldn't occur.
		ccode = PLDM_ERROR;
		break;
	case -EPROTO:
		ccode = PLDM_ERROR_INVALID_DATA;
		break;
	case -EOVERFLOW:
	case -EBADMSG:
		ccode = PLDM_ERROR_INVALID_LENGTH;
		break;
	default:
		ccode = PLDM_ERROR;
	}

	return pldm_platform_pd_reply_cc(ccode, req_hdr, resp,
					 resp_payload_len);
}

LIBPLDM_CC_NONNULL
static int pldm_platform_pd_get_sensor_reading(
	struct pldm_platform_pd *pd, const struct pldm_header_info *hdr,
	const struct pldm_msg *req, size_t req_payload_len,
	struct pldm_msg *resp, size_t *resp_payload_len)
{
	struct pldm_platform_pd_sensor_state state = { 0 };
	struct pldm_numeric_sensor_value_pdr pdr;
	const pldm_pdr_record *rec = NULL;
	bool8_t rearm_event_state;
	uint8_t sensor_data_size;
	size_t payload_len;
	bool found = false;
	uint8_t *pdr_data;
	uint32_t pdr_size;
	uint16_t sensor_id;
	uint8_t ccode;
	int rc;

	rc = decode_get_sensor_reading_req(req, req_payload_len, &sensor_id,
					   &rearm_event_state);
	if (rc != PLDM_SUCCESS) {
		return pldm_platform_pd_reply_errno(-EBADMSG, hdr, resp,
						    resp_payload_len);
	}

	if (!pd->get_sensor_reading) {
		return pldm_platform_pd_reply_cc(
			PLDM_ERROR_UNSUPPORTED_PLDM_CMD, hdr, resp,
			resp_payload_len);
	}

	while ((rec = pldm_pdr_find_record_by_type(pd->pdr,
						   PLDM_NUMERIC_SENSOR_PDR, rec,
						   &pdr_data, &pdr_size))) {
		if (decode_numeric_sensor_pdr_data(pdr_data, pdr_size, &pdr) ==
			    0 &&
		    pdr.sensor_id == sensor_id) {
			found = true;
			break;
		}
	}
	if (!found) {
		return pldm_platform_pd_reply_cc(
			PLDM_PLATFORM_INVALID_SENSOR_ID, hdr, resp,
			resp_payload_len);
	}

	sensor_data_size = pdr.sensor_data_size;
	ccode = pd->get_sensor_reading(pd->sensor_ops_ctx, &pdr,
				       rearm_event_state, &state);

	if (ccode != PLDM_SUCCESS) {
		return pldm_platform_pd_reply_cc(ccode, hdr, resp,
						 resp_payload_len);
	}

	switch (sensor_data_size) {
	case PLDM_SENSOR_DATA_SIZE_UINT8:
	case PLDM_SENSOR_DATA_SIZE_SINT8:
		payload_len = PLDM_GET_SENSOR_READING_MIN_RESP_BYTES;
		break;
	case PLDM_SENSOR_DATA_SIZE_UINT16:
	case PLDM_SENSOR_DATA_SIZE_SINT16:
		payload_len = PLDM_GET_SENSOR_READING_MIN_RESP_BYTES + 1;
		break;
	case PLDM_SENSOR_DATA_SIZE_UINT32:
	case PLDM_SENSOR_DATA_SIZE_SINT32:
		payload_len = PLDM_GET_SENSOR_READING_MIN_RESP_BYTES + 3;
		break;
	default:
		return pldm_platform_pd_reply_cc(PLDM_ERROR_INVALID_DATA, hdr,
						 resp, resp_payload_len);
	}
	if (*resp_payload_len < payload_len) {
		return pldm_platform_pd_reply_errno(-EOVERFLOW, hdr, resp,
						    resp_payload_len);
	}

	rc = encode_get_sensor_reading_resp(
		hdr->instance, PLDM_SUCCESS, sensor_data_size,
		state.operational_state, state.event_enable,
		state.present_state, state.previous_state, state.event_state,
		(const uint8_t *)&state.current_reading, resp, payload_len);
	if (rc != PLDM_SUCCESS) {
		return pldm_platform_pd_reply_errno(-EINVAL, hdr, resp,
						    resp_payload_len);
	}

	*resp_payload_len = payload_len;
	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_platform_pd_get_pdr_repository_info(
	struct pldm_platform_pd *pd, const struct pldm_header_info *hdr,
	size_t req_payload_len, struct pldm_msg *resp, size_t *resp_payload_len)
{
	static const uint8_t zeroes[13] = { 0 };
	uint32_t record_count;
	uint32_t largest = 0;
	uint32_t repo_size;
	uint32_t size;
	uint32_t next;
	uint8_t *data;
	int rc;

	if (req_payload_len != 0) {
		return pldm_platform_pd_reply_cc(PLDM_ERROR_INVALID_LENGTH, hdr,
						 resp, resp_payload_len);
	}

	if (*resp_payload_len < PLDM_GET_PDR_REPOSITORY_INFO_RESP_BYTES) {
		return pldm_platform_pd_reply_errno(-EOVERFLOW, hdr, resp,
						    resp_payload_len);
	}

	record_count = pldm_pdr_get_record_count(pd->pdr);
	repo_size = pldm_pdr_get_repo_size(pd->pdr);

	/* Walk repo to find the largest record */
	if (pldm_pdr_find_record(pd->pdr, 0, &data, &size, &next)) {
		if (size > largest) {
			largest = size;
		}
		while (next != 0) {
			if (!pldm_pdr_find_record(pd->pdr, next, &data, &size,
						  &next)) {
				break;
			}
			if (size > largest) {
				largest = size;
			}
		}
	}

	rc = encode_get_pdr_repository_info_resp(
		hdr->instance, PLDM_SUCCESS, PLDM_AVAILABLE, zeroes, zeroes,
		record_count, repo_size, largest, PLDM_NO_TIMEOUT, resp);
	if (rc != PLDM_SUCCESS) {
		return pldm_platform_pd_reply_errno(-EINVAL, hdr, resp,
						    resp_payload_len);
	}

	*resp_payload_len = PLDM_GET_PDR_REPOSITORY_INFO_RESP_BYTES;
	return 0;
}

LIBPLDM_CC_NONNULL
static int pldm_platform_pd_get_pdr(struct pldm_platform_pd *pd,
				    const struct pldm_header_info *hdr,
				    const struct pldm_msg *req,
				    size_t req_payload_len,
				    struct pldm_msg *resp,
				    size_t *resp_payload_len)
{
	uint32_t data_transfer_handle;
	uint32_t next_record_handle;
	uint8_t transfer_op_flag;
	uint16_t record_chg_num;
	uint32_t record_handle;
	uint8_t *record_data;
	uint32_t record_size;
	uint16_t request_cnt;
	uint16_t resp_cnt;
	size_t max_resp;
	int rc;

	rc = decode_get_pdr_req(req, req_payload_len, &record_handle,
				&data_transfer_handle, &transfer_op_flag,
				&request_cnt, &record_chg_num);
	if (rc != PLDM_SUCCESS) {
		return pldm_platform_pd_reply_errno(-EBADMSG, hdr, resp,
						    resp_payload_len);
	}

	if (transfer_op_flag != PLDM_GET_FIRSTPART) {
		return pldm_platform_pd_reply_cc(
			PLDM_PLATFORM_INVALID_TRANSFER_OPERATION_FLAG, hdr,
			resp, resp_payload_len);
	}

	if (!pldm_pdr_find_record(pd->pdr, record_handle, &record_data,
				  &record_size, &next_record_handle)) {
		return pldm_platform_pd_reply_cc(
			PLDM_PLATFORM_INVALID_RECORD_HANDLE, hdr, resp,
			resp_payload_len);
	}

	if (*resp_payload_len < PLDM_GET_PDR_MIN_RESP_BYTES) {
		return pldm_platform_pd_reply_errno(-EOVERFLOW, hdr, resp,
						    resp_payload_len);
	}

	/* Cap response to what the requester asked for and what fits */
	max_resp = *resp_payload_len - PLDM_GET_PDR_MIN_RESP_BYTES;
	if (max_resp > UINT16_MAX) {
		max_resp = UINT16_MAX;
	}
	resp_cnt = (uint16_t)record_size;
	if (resp_cnt > (uint16_t)max_resp) {
		resp_cnt = (uint16_t)max_resp;
	}
	if (request_cnt < resp_cnt) {
		resp_cnt = request_cnt;
	}

	rc = encode_get_pdr_resp(hdr->instance, PLDM_SUCCESS,
				 next_record_handle, 0,
				 PLDM_PLATFORM_TRANSFER_START_AND_END, resp_cnt,
				 record_data, 0, resp);
	if (rc != PLDM_SUCCESS) {
		return pldm_platform_pd_reply_errno(-EINVAL, hdr, resp,
						    resp_payload_len);
	}

	*resp_payload_len = PLDM_GET_PDR_MIN_RESP_BYTES + resp_cnt;
	return 0;
}

/* ------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------ */

LIBPLDM_ABI_TESTING
int pldm_platform_pd_handle_msg(struct pldm_platform_pd *pd, const void *in_msg,
				size_t in_len, void *out_msg, size_t *out_len)
{
	const struct pldm_msg *req = in_msg;
	struct pldm_msg *resp = out_msg;
	struct pldm_header_info hdr;
	size_t resp_payload_len;
	size_t req_payload_len;
	int rc;

	if (pd == NULL || in_msg == NULL || out_msg == NULL ||
	    out_len == NULL) {
		return -EINVAL;
	}

	if (in_len < sizeof(struct pldm_msg_hdr)) {
		return -EOVERFLOW;
	}
	req_payload_len = in_len - sizeof(struct pldm_msg_hdr);

	rc = unpack_pldm_header(&req->hdr, &hdr);
	if (rc != PLDM_SUCCESS) {
		return -EINVAL;
	}

	if (hdr.pldm_type != PLDM_PLATFORM) {
		/* Caller should have passed platform and control/type2 message */
		return -ENOMSG;
	}

	if (hdr.msg_type == PLDM_RESPONSE) {
		/* PD does not send requests; responses are unexpected */
		return -EPROTO;
	}

	if (hdr.msg_type != PLDM_REQUEST) {
		return -EPROTO;
	}

	/* Space for header plus at least completion code */
	if (*out_len < sizeof(struct pldm_msg_hdr) + 1) {
		return -EOVERFLOW;
	}
	resp_payload_len = *out_len - sizeof(struct pldm_msg_hdr);

	/* Dispatch command. Update PLDM_PD_COMMANDS if adding new handlers. */
	switch (hdr.command) {
	case PLDM_GET_SENSOR_READING:
		rc = pldm_platform_pd_get_sensor_reading(pd, &hdr, req,
							 req_payload_len, resp,
							 &resp_payload_len);
		break;
	case PLDM_GET_PDR_REPOSITORY_INFO:
		rc = pldm_platform_pd_get_pdr_repository_info(
			pd, &hdr, req_payload_len, resp, &resp_payload_len);
		break;
	case PLDM_GET_PDR:
		rc = pldm_platform_pd_get_pdr(pd, &hdr, req, req_payload_len,
					      resp, &resp_payload_len);
		break;
	default:
		rc = pldm_platform_pd_reply_cc(PLDM_ERROR_UNSUPPORTED_PLDM_CMD,
					       &hdr, resp, &resp_payload_len);
	}

	if (rc == 0) {
		*out_len = resp_payload_len + sizeof(struct pldm_msg_hdr);
	}

	return rc;
}

LIBPLDM_ABI_TESTING
int pldm_platform_pd_set_sensor_ops(
	struct pldm_platform_pd *pd,
	uint8_t (*get_sensor_reading)(
		void *ctx, const struct pldm_numeric_sensor_value_pdr *pdr,
		bool8_t rearm_event_state,
		struct pldm_platform_pd_sensor_state *state),
	void *opt_ctx)
{
	if (pd == NULL || get_sensor_reading == NULL) {
		return -EINVAL;
	}

	pd->get_sensor_reading = get_sensor_reading;
	pd->sensor_ops_ctx = opt_ctx;
	return 0;
}

LIBPLDM_ABI_TESTING
struct pldm_platform_pd *pldm_platform_pd_new(const pldm_pdr *pdr,
					      struct pldm_control *control)
{
	struct pldm_platform_pd *pd = malloc(sizeof(*pd));
	if (pd) {
		if (pldm_platform_pd_setup(pd, sizeof(*pd), pdr, control) ==
		    0) {
			return pd;
		}
		free(pd);
		pd = NULL;
	}
	return pd;
}

LIBPLDM_ABI_TESTING
int pldm_platform_pd_setup(struct pldm_platform_pd *pd,
			   size_t pldm_platform_pd_size, const pldm_pdr *pdr,
			   struct pldm_control *control)
{
	int rc;

	if (pd == NULL || pdr == NULL) {
		return -EINVAL;
	}

	if (pldm_platform_pd_size < sizeof(struct pldm_platform_pd)) {
		/* Safety check that sufficient storage was provided for *pd,
		 * in case PLDM_SIZEOF_PLDM_PLATFORM_PD is incorrect */
		return -EINVAL;
	}

	memset(pd, 0x0, sizeof(*pd));
	pd->pdr = pdr;

	if (control) {
		rc = pldm_control_add_type(control, PLDM_PLATFORM,
					   &PLDM_PD_VERSIONS,
					   PLDM_PD_VERSIONS_COUNT,
					   PLDM_PD_COMMANDS);
		if (rc) {
			return rc;
		}
	}

	return 0;
}
