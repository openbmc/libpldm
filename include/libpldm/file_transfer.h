#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "pldm_types.h"

/**
 * @brief PLDM Command Codes for File Transfer
*/
enum pldm_df_commands {
	PLDM_DF_OPEN = 0x01,
	PLDM_DF_CLOSE = 0x02,
	PLDM_DF_READ = 0x20,
};

enum pldm_file_transfer_completion_codes {
	INVALID_FILE_DESCRIPTOR = 0x80,
	INVALID_DF_ATTRIBUTE = 0x81,
	ZEROLENGTH_NOT_ALLOWED = 0x82,
	EXCLUSIVE_OWNERSHIP_NOT_ESTABLISHED = 0x83,
	EXCLUSIVE_OWNERSHIP_NOT_ALLOWED = 0x84,
	EXCLUSIVE_OWNERSHIP_NOT_AVAILABLE = 0x85,
	INVALID_FILE_IDENTIFIER = 0x86,
	DFOPEN_DIR_NOT_ALLOWED = 0x87,
	MAX_NUM_FDS_EXCEEDED = 0x88,
	FILE_OPEN = 0x89,
	UNABLE_TO_OPEN_FILE = 0x8A,
};

/**
 * @struct Structure for DfOpen request
 */
struct pldm_df_open_req {
	uint16_t file_identifier;
	bitfield16_t df_open_attribute;
} __attribute__((packed));

/**
 * @brief Encode a PLDM DF Open request message
 *
 * @param[in] instance_id - The instance ID of the PLDM endpoint
 * @param[in] file_identifier - Identifier for the file to be read
 * @param[in] df_open_attribute - File open attributes
 * @param[in] payload_length - The length of the msg payload buffer
 * @param[out] msg - Message will be written to this
 * @return 0 on success or else errno error code
 */
int encode_pldm_df_open_req(uint8_t instance_id, uint16_t file_identifier,
			    const bitfield16_t *df_open_attribute,
			    size_t payload_length, struct pldm_msg *msg);

/**
 * @brief Decode a PLDM DF Open request message
 *
 * @param[in] msg - The PLDM message to be decoded
 * @param[in] payload_length - The length of the message payload
 * @param[out] req - A pointer to pldm_df_open_req struct
 * @return 0 on success or else errno error code
 */
int decode_pldm_df_open_req(const struct pldm_msg *msg, size_t payload_length,
			    struct pldm_df_open_req *req);

#ifdef __cplusplus
}
#endif

#endif /* FILE_TRANSFER_H */
