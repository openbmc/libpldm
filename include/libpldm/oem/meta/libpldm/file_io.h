#ifndef FILEIO_H
#define FILEIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "utils.h"

struct pldm_msg;
/** @brief PLDM Commands in OEM META type
 */

enum pldm_fileio_commands {
	PLDM_WRITE_FILE = 0x2,
	PLDM_READ_FILE = 0x3,
};

enum file_io_type {
	POST_CODE = 0x00,
};

struct pldm_write_file_req {
	uint8_t file_handle;
	uint32_t length;
	uint8_t file_data[1];
} __attribute__((packed));

int decode_write_file_io_req(const struct pldm_msg *msg, size_t payload_length,
			     uint8_t *file_handle, uint32_t *length,
			     struct variable_field *data);

#ifdef __cplusplus
}
#endif

#endif /* FILEIO_H */
