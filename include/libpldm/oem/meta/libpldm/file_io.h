#ifndef LIBPLDM_OEM_META_FILE_IO_H
#define LIBPLDM_OEM_META_FILE_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "utils.h"

struct pldm_msg;
/** @brief PLDM Commands in OEM META type
 */

enum pldm_oem_meta_fileio_commands {
	PLDM_WRITE_FILE = 0x2,
	PLDM_READ_FILE = 0x3,
};

enum pldm_oem_meta_file_io_type {
	POST_CODE = 0x00,
};

#define PLDM_OEM_META_DECODE_WRITE_FILE_IO_MIN_SIZE 6

struct pldm_write_file_req {
	uint8_t file_handle;
	uint32_t length;
	uint8_t file_data[1];
};

int decode_write_file_io_req_oem_meta(const struct pldm_msg *msg,
				      size_t payload_length,
				      uint8_t *file_handle, uint32_t *length,
				      uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /*LIBPLDM_OEM_META_FILE_IO_H*/
