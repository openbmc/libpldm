/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_OEM_META_FILE_IO_H
#define LIBPLDM_OEM_META_FILE_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

struct pldm_msg;
/** @brief PLDM Commands in OEM META type
 */

enum pldm_oem_meta_fileio_commands {
	PLDM_OEM_META_FILEIO_CMD_WRITE_FILE = 0x2,
	PLDM_OEM_META_FILEIO_CMD_READ_FILE = 0x3,
};

/** @brief read options in read file io command
 */
enum pldm_oem_meta_file_io_read_option {
	// Read file attribute
	PLDM_OEM_META_FILEIO_READ_ATTR,
	// Read file data
	PLDM_OEM_META_FILEIO_READ_DATA,
};

struct pldm_oem_meta_write_file_req {
	uint8_t file_handle;
	uint32_t length;
	uint8_t file_data[1];
};

/** @struct pldm_oem_meta_file_io_read_req
 *
 *  Structure representing PLDM read file request
 */
struct pldm_oem_meta_file_io_read_req {
	uint8_t file_handle;
	uint8_t read_option;
	uint8_t length;
	uint8_t *read_info;
};

/** @struct pldm_oem_meta_file_io_read_data_info
 *
 *  Structure representing PLDM read file data info
 */
struct pldm_oem_meta_file_io_read_data_info {
	uint8_t length;
	uint8_t transferFlag;
	uint8_t highOffset;
	uint8_t lowOffset;
};

/** @brief Decode OEM meta write file io req
 *
 *  @param[in] msg - Pointer to PLDM request message
 *  @param[in] payload_length - Length of request payload
 *  @param[out] file_handle - The handle of data
 *  @param[out] length - Total size of data
 *  @param[out] data - Message will be written to this
 *  @return 0 on success, negative errno value on failure
 */
int decode_oem_meta_file_io_write_req(const struct pldm_msg *msg,
				      size_t payload_length,
				      uint8_t *file_handle, uint32_t *length,
				      uint8_t *data);

/** @brief Deprecated decoder for OEM meta write file io req
 *
 *  @param[in] msg - Pointer to PLDM request message
 *  @param[in] payload_length - Length of request payload
 *  @param[out] file_handle - The handle of data
 *  @param[out] length - Total size of data
 *  @param[out] data - Message will be written to this
 *  @return pldm_completion_codes
 */
int decode_oem_meta_file_io_req(const struct pldm_msg *msg,
				size_t payload_length, uint8_t *file_handle,
				uint32_t *length, uint8_t *data);

/** @brief Decode OEM meta read file io req
 *
 *  @param[in] msg - Pointer to PLDM request message
 *  @param[in] payload_length - Length of request payload
 *  @param[in] req_length - Length of request structure
 *  @param[out] req - Pointer to the structure to store the decoded response data
 *  @return 0 on success, negative errno value on failure
 */
int decode_oem_meta_file_io_read_req(const struct pldm_msg *msg,
				     size_t payload_length, size_t req_length,
				     struct pldm_oem_meta_read_file_req *req);

/**
 * @brief Encode OEM meta read file io resp
 *
 * @param[in] instance_id - The instance ID of the PLDM entity
 * @param[out] completion_code - The completion code of response
 * @param[out] responseMsg - Pointer to the buffer to store the response data
 * @return 0 on success, negative errno value on failure
 */
int encode_oem_meta_file_io_read_resp(uint8_t instance_id,
				      uint8_t completion_code,
				      struct pldm_msg *responseMsg);

#ifdef __cplusplus
}
#endif

#endif /*LIBPLDM_OEM_META_FILE_IO_H*/
