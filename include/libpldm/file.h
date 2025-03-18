/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef FILE_H
#define FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libpldm/base.h>
#include <libpldm/utils.h>

#include <asm/byteorder.h>
#include <stddef.h>
#include <stdint.h>

#define PLDM_DF_OPEN_REQ_BYTES	4
#define PLDM_DF_OPEN_RESP_BYTES 3

/** @brief PLDM File Transfer Completion Code */
enum pldm_file_cc {
	PLDM_FILE_CC_INVALID_FILE_DESCRIPTOR = 0x80,
	PLDM_FILE_CC_INVALID_DF_ATTRIBUTE = 0x81,
	PLDM_FILE_CC_ZEROLENGTH_NOT_ALLOWED = 0x82,
	PLDM_FILE_CC_EXCLUSIVE_OWNERSHIP_NOT_ESTABLISHED = 0x83,
	PLDM_FILE_CC_EXCLUSIVE_OWNERSHIP_NOT_ALLOWED = 0x84,
	PLDM_FILE_CC_EXCLUSIVE_OWNERSHIP_NOT_AVAILABLE = 0x85,
	PLDM_FILE_CC_INVALID_FILE_IDENTIFIER = 0x86,
	PLDM_FILE_CC_DFOPEN_DIR_NOT_ALLOWED = 0x87,
	PLDM_FILE_CC_MAX_NUM_FDS_EXCEEDED = 0x88,
	PLDM_FILE_CC_FILE_OPEN = 0x89,
	PLDM_FILE_CC_UNABLE_TO_OPEN_FILE = 0x8A,
};

/** @brief PLDM File Transfer Command */
enum pldm_file_cmd {
	PLDM_FILE_CMD_DF_OPEN = 0x01,
	PLDM_FILE_CMD_DF_CLOSE = 0x02,
	PLDM_FILE_CMD_DF_HEARBEAT = 0x03,
	PLDM_FILE_CMD_DF_PROPERTIES = 0x10,
	PLDM_FILE_CMD_DF_GET_FILE_ATTRIBUTE = 0x11,
	PLDM_FILE_CMD_DF_SET_FILE_ATTRIBUTE = 0x12,
	PLDM_FILE_CMD_DF_READ = 0x20,
	PLDM_FILE_CMD_DF_FIFO_SEND = 0x21,
};

/** @struct pldm_df_open_req
 *
 *  Structure representing PLDM File DfOpen request.
 */
struct pldm_df_open_req {
	uint16_t file_identifier;
	bitfield16_t file_attribute;
} __attribute__((packed));

/** @struct pldm_df_open_resp
 *
 *  Structure representing PLDM File DfOpen response.
 */
struct pldm_df_open_resp {
	uint8_t completion_code;
	uint16_t file_descriptor;
};

/** @brief Create a PLDM request message for DFOpen
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] file_identifier - File Identifier
 *  @param[in] file_attribute - File Attribute
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of the request message payload
 *  @return 0 on success, -EINVAL if the input parameters' memory
 *         are not allocated
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_df_open_req(uint8_t instance_id, uint16_t file_identifier,
		       bitfield16_t file_attribute, struct pldm_msg *msg,
		       size_t payload_length);

/** @brief Decode DFOpen response data
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] resp - pointer to the decoded response message
 *  @return 0 on success, -EINVAL if the input parameters' memory
 *          are not allocated
 */
int decode_df_open_resp(const struct pldm_msg *msg, size_t payload_length,
			struct pldm_df_open_resp *resp);

#ifdef __cplusplus
}
#endif

#endif
