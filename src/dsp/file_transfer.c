#include "dsp/base.h"
#include "msgbuf.h"

#include <libpldm/file_transfer.h>

LIBPLDM_ABI_TESTING
int encode_pldm_df_open_req(uint8_t instance_id, uint16_t file_identifier,
			    const bitfield16_t *df_open_attribute,
			    size_t payload_length, struct pldm_msg *msg)
{
	PLDM_MSGBUF_DEFINE_P(buf);

	if ((msg == NULL) || (df_open_attribute == NULL)) {
		return -EINVAL;
	}

	struct pldm_header_info header = { 0 };
	header.msg_type = PLDM_REQUEST;
	header.instance = instance_id;
	header.pldm_type = PLDM_FILE;
	header.command = PLDM_DF_OPEN;

	int rc = pack_pldm_header_errno(&header, &msg->hdr);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(buf, sizeof(struct pldm_df_open_req),
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, file_identifier);
	pldm_msgbuf_insert(buf, df_open_attribute->value);

	return pldm_msgbuf_complete(buf);
}

LIBPLDM_ABI_TESTING
int decode_pldm_df_open_req(const struct pldm_msg *msg, size_t payload_length,
			    struct pldm_df_open_req *req)
{
	PLDM_MSGBUF_DEFINE_P(buf);

	if ((msg == NULL) || (req == NULL)) {
		return -EINVAL;
	}

	int rc = pldm_msgbuf_init_errno(buf, sizeof(struct pldm_df_open_req),
					msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_extract(buf, req->file_identifier);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	rc = pldm_msgbuf_extract(buf, req->df_open_attribute.value);
	if (rc) {
		return pldm_msgbuf_discard(buf, rc);
	}

	return pldm_msgbuf_complete_consumed(buf);
}
