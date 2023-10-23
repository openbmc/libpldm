#include "libpldm/file_io.h"
#include "base.h"
#include <endian.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"

LIBPLDM_ABI_STABLE
int decode_write_file_req(const struct pldm_msg *msg, size_t payload_length,
			  uint8_t *file_handle, uint32_t *length,
			  struct variable_field *data)
{
	if (msg == NULL || file_handle == NULL || length == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_write_file_req *request =
		(struct pldm_write_file_req *)msg->payload;

	*file_handle = request->file_handle;
	*length = le32toh(request->length);

	data->length = payload_length -
		       (sizeof(request->file_handle) + sizeof(request->length));

	data->ptr = request->file_data;

	return PLDM_SUCCESS;
}
