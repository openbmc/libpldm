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

struct pldm_oem_meta_write_file_req {
	uint8_t file_handle;
	uint32_t length;
	uint8_t file_data[1];
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
int decode_oem_meta_write_file_io_req(const struct pldm_msg *msg,
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

#ifdef __cplusplus
}
#endif

#endif /*LIBPLDM_OEM_META_FILE_IO_H*/
