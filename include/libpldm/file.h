/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_FILE_H
#define LIBPLDM_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libpldm/pldm_types.h>

#include <asm/byteorder.h>
#include <stddef.h>
#include <stdint.h>

#define PLDM_DF_OPEN_REQ_BYTES	     4
#define PLDM_DF_OPEN_RESP_BYTES	     3
#define PLDM_DF_CLOSE_REQ_BYTES	     4
#define PLDM_DF_CLOSE_RESP_BYTES     1
#define PLDM_DF_HEARTBEAT_REQ_BYTES  6
#define PLDM_DF_HEARTBEAT_RESP_BYTES 5

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
	PLDM_FILE_CMD_DF_HEARTBEAT = 0x03,
	PLDM_FILE_CMD_DF_PROPERTIES = 0x10,
	PLDM_FILE_CMD_DF_GET_FILE_ATTRIBUTE = 0x11,
	PLDM_FILE_CMD_DF_SET_FILE_ATTRIBUTE = 0x12,
	PLDM_FILE_CMD_DF_READ = 0x20,
	PLDM_FILE_CMD_DF_FIFO_SEND = 0x21,
};

/** @struct pldm_file_df_open_req
 *
 *  Structure representing PLDM File DfOpen request.
 */
struct pldm_file_df_open_req {
	uint16_t file_identifier;
	bitfield16_t file_attribute;
};

/** @struct pldm_file_df_open_resp
 *
 *  Structure representing PLDM File DfOpen response.
 */
struct pldm_file_df_open_resp {
	uint8_t completion_code;
	uint16_t file_descriptor;
};

/** @struct pldm_file_df_close_req
 *
 *  Structure representing PLDM File DfClose request.
 */
struct pldm_file_df_close_req {
	uint16_t file_descriptor;
	bitfield16_t df_close_options;
};

/** @struct pldm_file_df_close_resp
 *
 *  Structure representing PLDM File DfClose response.
 */
struct pldm_file_df_close_resp {
	uint8_t completion_code;
};

/** @struct pldm_file_df_heartbeat_req
 *
 *  Structure representing PLDM File DfHeartbeat request.
 */
struct pldm_file_df_heartbeat_req {
	uint16_t file_descriptor;
	uint32_t requester_max_interval;
};

/** @struct pldm_file_df_heartbeat_resp
 *
 *  Structure representing PLDM File DfHearbeat response.
 */
struct pldm_file_df_heartbeat_resp {
	uint8_t completion_code;
	uint32_t responder_max_interval;
};

/** @brief Create a PLDM request message for DFOpen
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] req - The pointer to the request message to be encoded
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of the request message payload
 *  @return 0 on success
 *          -EINVAL if the input parameters' memory are not allocated,
 *          or message type or instance in request header is invalid
 *          -ENOMSG if the PLDM type in the request header is invalid
 *          -EOVERFLOW if the input message length is invalid
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_pldm_file_df_open_req(uint8_t instance_id,
				 const struct pldm_file_df_open_req *req,
				 struct pldm_msg *msg, size_t payload_length);

/** @brief Decode DFOpen response data
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] resp - pointer to the decoded response message
 *  @return 0 on success
 *          -EINVAL if the input parameters' memory are not allocated
 *          -EOVERFLOW if the input message buffer is too short for the output
 *          response struct
 *          -EBADMSG if the input message buffer is too large for the output
 *          response struct.
 */
int decode_pldm_file_df_open_resp(const struct pldm_msg *msg,
				  size_t payload_length,
				  struct pldm_file_df_open_resp *resp);

/** @brief Create a PLDM request message for DFClose
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] req - The pointer to the request message to be encoded
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of the request message payload
 *  @return 0 on success
 *          -EINVAL if the input parameters' memory are not allocated,
 *          or message type or instance in request header is invalid
 *          -ENOMSG if the PLDM type in the request header is invalid
 *          -EOVERFLOW if the input message length is invalid
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_pldm_file_df_close_req(uint8_t instance_id,
				  const struct pldm_file_df_close_req *req,
				  struct pldm_msg *msg, size_t payload_length);

/** @brief Decode DFClose response data
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] resp - pointer to the decoded response message
 *  @return 0 on success
 *          -EINVAL if the input parameters' memory are not allocated
 */
int decode_pldm_file_df_close_resp(const struct pldm_msg *msg,
				   size_t payload_length,
				   struct pldm_file_df_close_resp *resp);

/** @brief Create a PLDM request message for DFHeartbeat
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] req - The pointer to the request message to be encoded
 *  @param[in,out] msg - Message will be written to this
 *  @param[in] payload_length - Length of the request message payload
 *  @return 0 on success
 *          -EINVAL if the input parameters' memory are not allocated,
 *          or message type or instance in request header is invalid
 *          -ENOMSG if the PLDM type in the request header is invalid
 *          -EOVERFLOW if the input message length is invalid
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_pldm_file_df_heartbeat_req(
	uint8_t instance_id, const struct pldm_file_df_heartbeat_req *req,
	struct pldm_msg *msg, size_t payload_length);

/** @brief Decode DFHeartbeat response data
 *
 *  @param[in] msg - Response message
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] resp - pointer to the decoded response message
 *  @return 0 on success
 *          -EINVAL if the input parameters' memory are not allocated
 *          -EOVERFLOW if the input message buffer is too short for the output
 *          response struct
 *          -EBADMSG if the input message buffer is too large for the output
 *          response struct
*/
int decode_pldm_file_df_heartbeat_resp(const struct pldm_msg *msg,
				       size_t payload_length,
				       struct pldm_file_df_heartbeat_resp *resp);

#ifdef __cplusplus
}
#endif

#endif
