/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_OEM_META_FILE_IO_H
#define LIBPLDM_OEM_META_FILE_IO_H

#include <libpldm/compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

struct pldm_msg;
/** @brief PLDM Commands in OEM META type
 */

enum pldm_oem_meta_file_io_commands {
	PLDM_OEM_META_FILE_IO_CMD_WRITE_FILE = 0x2,
	PLDM_OEM_META_FILE_IO_CMD_READ_FILE = 0x3,
};

/** @brief read options in read file io command
 */
enum pldm_oem_meta_file_io_read_option {
	// Read file attribute
	PLDM_OEM_META_FILE_IO_READ_ATTR = 0x00,
	// Read file data
	PLDM_OEM_META_FILE_IO_READ_DATA = 0x01,
};

struct pldm_oem_meta_file_io_write_req {
	uint8_t handle;
	uint32_t length;
#ifndef __cplusplus
	uint8_t data[] LIBPLDM_CC_COUNTED_BY(length);
#endif
};
#define PLDM_OEM_META_FILE_IO_WRITE_REQ_MIN_LENGTH 5u

/** @struct pldm_oem_meta_file_io_read_data_info
 *
 *  Structure representing PLDM read file data info
 */
struct pldm_oem_meta_file_io_read_data_info {
	uint8_t transferFlag;
	uint16_t offset;
};
#define PLDM_OEM_META_FILE_IO_READ_DATA_INFO_LENGTH 3u

/** @struct pldm_oem_meta_file_io_read_attr_info
 *
 *  Structure representing PLDM read file attribute info
 */
struct pldm_oem_meta_file_io_read_attr_info {
	uint16_t size;
	uint32_t crc32;
};
#define PLDM_OEM_META_FILE_IO_READ_ATTR_INFO_LENGTH 6u

/** @struct pldm_oem_meta_file_io_read_req
 *
 *  Structure representing PLDM read file request
 */
struct pldm_oem_meta_file_io_read_req {
	size_t version;
	uint8_t handle;
	uint8_t option;
	uint8_t length;
	union {
		struct pldm_oem_meta_file_io_read_data_info data;
	} info;
};
#define PLDM_OEM_META_FILE_IO_READ_REQ_MIN_LENGTH 3u

/** @brief Obtain the pointer to the data array of a write request
 *
 * @param[in] req - The pointer to the write request struct
 *
 * @return The write request data pointer.
 */
void *pldm_oem_meta_file_io_write_req_data(
	struct pldm_oem_meta_file_io_write_req *req);

/** @brief Decode OEM meta write file io req
 *
 *  @param[in] msg - Pointer to PLDM request message
 *  @param[in] payload_length - Length of request payload
 *  @param[out] req - Pointer to the structure to store the decoded response data
 *  @param[in] req_length - Length of request structure
 *  @return 0 on success, negative errno value on failure
 */
int decode_oem_meta_file_io_write_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_oem_meta_file_io_write_req *req, size_t req_length);

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
 *  @param[out] req - Pointer to the structure to store the decoded response data
 *  @return 0 on success, negative errno value on failure
 */
int decode_oem_meta_file_io_read_req(const struct pldm_msg *msg,
				     size_t payload_length,
				     struct pldm_oem_meta_file_io_read_req *req);

#ifdef __cplusplus
}
#endif

#endif /*LIBPLDM_OEM_META_FILE_IO_H*/
