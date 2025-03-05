/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include "api.h"
#include "dsp/base.h"
#include "msgbuf.h"
#include <libpldm/firmware_update.h>
#include <libpldm/utils.h>

#include <endian.h>
#include <stdbool.h>
#include <string.h>

static_assert(PLDM_FIRMWARE_MAX_STRING <= UINT8_MAX, "too large");

/** @brief Check whether string type value is valid
 *
 *  @return true if string type value is valid, false if not
 */
static bool is_string_type_valid(uint8_t string_type)
{
	switch (string_type) {
	case PLDM_STR_TYPE_UNKNOWN:
		return false;
	case PLDM_STR_TYPE_ASCII:
	case PLDM_STR_TYPE_UTF_8:
	case PLDM_STR_TYPE_UTF_16:
	case PLDM_STR_TYPE_UTF_16LE:
	case PLDM_STR_TYPE_UTF_16BE:
		return true;
	default:
		return false;
	}
}

/** @brief Return the length of the descriptor type described in firmware update
 *         specification
 *
 *  @return length of the descriptor type if descriptor type is valid else
 *          return 0
 */
static uint16_t get_descriptor_type_length(uint16_t descriptor_type)
{
	switch (descriptor_type) {
	case PLDM_FWUP_PCI_VENDOR_ID:
		return PLDM_FWUP_PCI_VENDOR_ID_LENGTH;
	case PLDM_FWUP_IANA_ENTERPRISE_ID:
		return PLDM_FWUP_IANA_ENTERPRISE_ID_LENGTH;
	case PLDM_FWUP_UUID:
		return PLDM_FWUP_UUID_LENGTH;
	case PLDM_FWUP_PNP_VENDOR_ID:
		return PLDM_FWUP_PNP_VENDOR_ID_LENGTH;
	case PLDM_FWUP_ACPI_VENDOR_ID:
		return PLDM_FWUP_ACPI_VENDOR_ID_LENGTH;
	case PLDM_FWUP_IEEE_ASSIGNED_COMPANY_ID:
		return PLDM_FWUP_IEEE_ASSIGNED_COMPANY_ID_LENGTH;
	case PLDM_FWUP_SCSI_VENDOR_ID:
		return PLDM_FWUP_SCSI_VENDOR_ID_LENGTH;
	case PLDM_FWUP_PCI_DEVICE_ID:
		return PLDM_FWUP_PCI_DEVICE_ID_LENGTH;
	case PLDM_FWUP_PCI_SUBSYSTEM_VENDOR_ID:
		return PLDM_FWUP_PCI_SUBSYSTEM_VENDOR_ID_LENGTH;
	case PLDM_FWUP_PCI_SUBSYSTEM_ID:
		return PLDM_FWUP_PCI_SUBSYSTEM_ID_LENGTH;
	case PLDM_FWUP_PCI_REVISION_ID:
		return PLDM_FWUP_PCI_REVISION_ID_LENGTH;
	case PLDM_FWUP_PNP_PRODUCT_IDENTIFIER:
		return PLDM_FWUP_PNP_PRODUCT_IDENTIFIER_LENGTH;
	case PLDM_FWUP_ACPI_PRODUCT_IDENTIFIER:
		return PLDM_FWUP_ACPI_PRODUCT_IDENTIFIER_LENGTH;
	case PLDM_FWUP_ASCII_MODEL_NUMBER_LONG_STRING:
		return PLDM_FWUP_ASCII_MODEL_NUMBER_LONG_STRING_LENGTH;
	case PLDM_FWUP_ASCII_MODEL_NUMBER_SHORT_STRING:
		return PLDM_FWUP_ASCII_MODEL_NUMBER_SHORT_STRING_LENGTH;
	case PLDM_FWUP_SCSI_PRODUCT_ID:
		return PLDM_FWUP_SCSI_PRODUCT_ID_LENGTH;
	case PLDM_FWUP_UBM_CONTROLLER_DEVICE_CODE:
		return PLDM_FWUP_UBM_CONTROLLER_DEVICE_CODE_LENGTH;
	default:
		return 0;
	}
}

static bool is_downstream_device_update_support_valid(uint8_t resp)
{
	switch (resp) {
	case PLDM_FWUP_DOWNSTREAM_DEVICE_UPDATE_NOT_SUPPORTED:
	case PLDM_FWUP_DOWNSTREAM_DEVICE_UPDATE_SUPPORTED:
		return true;
	default:
		return false;
	}
}

static bool
is_transfer_operation_flag_valid(enum transfer_op_flag transfer_op_flag)
{
	switch (transfer_op_flag) {
	case PLDM_GET_NEXTPART:
	case PLDM_GET_FIRSTPART:
		return true;
	default:
		return false;
	}
}

/** @brief Check whether ComponentResponse is valid
 *
 *  @return true if ComponentResponse is valid, false if not
 */
static bool is_comp_resp_valid(uint8_t comp_resp)
{
	switch (comp_resp) {
	case PLDM_CR_COMP_CAN_BE_UPDATED:
	case PLDM_CR_COMP_MAY_BE_UPDATEABLE:
		return true;

	default:
		return false;
	}
}

/** @brief Check whether ComponentResponseCode is valid
 *
 *  @return true if ComponentResponseCode is valid, false if not
 */
static bool is_comp_resp_code_valid(uint8_t comp_resp_code)
{
	switch (comp_resp_code) {
	case PLDM_CRC_COMP_CAN_BE_UPDATED:
	case PLDM_CRC_COMP_COMPARISON_STAMP_IDENTICAL:
	case PLDM_CRC_COMP_COMPARISON_STAMP_LOWER:
	case PLDM_CRC_INVALID_COMP_COMPARISON_STAMP:
	case PLDM_CRC_COMP_CONFLICT:
	case PLDM_CRC_COMP_PREREQUISITES_NOT_MET:
	case PLDM_CRC_COMP_NOT_SUPPORTED:
	case PLDM_CRC_COMP_SECURITY_RESTRICTIONS:
	case PLDM_CRC_INCOMPLETE_COMP_IMAGE_SET:
	case PLDM_CRC_ACTIVE_IMAGE_NOT_UPDATEABLE_SUBSEQUENTLY:
	case PLDM_CRC_COMP_VER_STR_IDENTICAL:
	case PLDM_CRC_COMP_VER_STR_LOWER:
		return true;

	default:
		if (comp_resp_code >=
			    PLDM_CRC_VENDOR_COMP_RESP_CODE_RANGE_MIN &&
		    comp_resp_code <=
			    PLDM_CRC_VENDOR_COMP_RESP_CODE_RANGE_MAX) {
			return true;
		}
		return false;
	}
}

/** @brief Check whether ComponentCompatibilityResponse is valid
 *
 *  @return true if ComponentCompatibilityResponse is valid, false if not
 */
static bool is_comp_compatibility_resp_valid(uint8_t comp_compatibility_resp)
{
	switch (comp_compatibility_resp) {
	case PLDM_CCR_COMP_CAN_BE_UPDATED:
	case PLDM_CCR_COMP_CANNOT_BE_UPDATED:
		return true;

	default:
		return false;
	}
}

/** @brief Check whether ComponentCompatibilityResponse Code is valid
 *
 *  @return true if ComponentCompatibilityResponse Code is valid, false if not
 */
static bool
is_comp_compatibility_resp_code_valid(uint8_t comp_compatibility_resp_code)
{
	switch (comp_compatibility_resp_code) {
	case PLDM_CCRC_NO_RESPONSE_CODE:
	case PLDM_CCRC_COMP_COMPARISON_STAMP_IDENTICAL:
	case PLDM_CCRC_COMP_COMPARISON_STAMP_LOWER:
	case PLDM_CCRC_INVALID_COMP_COMPARISON_STAMP:
	case PLDM_CCRC_COMP_CONFLICT:
	case PLDM_CCRC_COMP_PREREQUISITES_NOT_MET:
	case PLDM_CCRC_COMP_NOT_SUPPORTED:
	case PLDM_CCRC_COMP_SECURITY_RESTRICTIONS:
	case PLDM_CRC_INCOMPLETE_COMP_IMAGE_SET:
	case PLDM_CCRC_COMP_INFO_NO_MATCH:
	case PLDM_CCRC_COMP_VER_STR_IDENTICAL:
	case PLDM_CCRC_COMP_VER_STR_LOWER:
		return true;

	default:
		if (comp_compatibility_resp_code >=
			    PLDM_CCRC_VENDOR_COMP_RESP_CODE_RANGE_MIN &&
		    comp_compatibility_resp_code <=
			    PLDM_CCRC_VENDOR_COMP_RESP_CODE_RANGE_MAX) {
			return true;
		}
		return false;
	}
}

/** @brief Check whether SelfContainedActivationRequest is valid
 *
 *  @return true if SelfContainedActivationRequest is valid, false if not
 */
static bool
is_self_contained_activation_req_valid(bool8_t self_contained_activation_req)
{
	switch (self_contained_activation_req) {
	case PLDM_NOT_ACTIVATE_SELF_CONTAINED_COMPONENTS:
	case PLDM_ACTIVATE_SELF_CONTAINED_COMPONENTS:
		return true;

	default:
		return false;
	}
}

/** @brief Check if current or previous status in GetStatus command response is
 *         valid
 *
 *	@param[in] state - current or previous different state machine state of
 *                     the FD
 *	@return true if state is valid, false if not
 */
static bool is_state_valid(uint8_t state)
{
	switch (state) {
	case PLDM_FD_STATE_IDLE:
	case PLDM_FD_STATE_LEARN_COMPONENTS:
	case PLDM_FD_STATE_READY_XFER:
	case PLDM_FD_STATE_DOWNLOAD:
	case PLDM_FD_STATE_VERIFY:
	case PLDM_FD_STATE_APPLY:
	case PLDM_FD_STATE_ACTIVATE:
		return true;

	default:
		return false;
	}
}

/** @brief Check if aux state in GetStatus command response is valid
 *
 *  @param[in] aux_state - provides additional information to the UA to describe
 *                         the current operation state of the FD/FDP
 *
 *	@return true if aux state is valid, false if not
 */
static bool is_aux_state_valid(uint8_t aux_state)
{
	switch (aux_state) {
	case PLDM_FD_OPERATION_IN_PROGRESS:
	case PLDM_FD_OPERATION_SUCCESSFUL:
	case PLDM_FD_OPERATION_FAILED:
	case PLDM_FD_IDLE_LEARN_COMPONENTS_READ_XFER:
		return true;

	default:
		return false;
	}
}

/** @brief Check if aux state status in GetStatus command response is valid
 *
 *	@param[in] aux_state_status - aux state status
 *
 *	@return true if aux state status is valid, false if not
 */
static bool is_aux_state_status_valid(uint8_t aux_state_status)
{
	if (aux_state_status == PLDM_FD_AUX_STATE_IN_PROGRESS_OR_SUCCESS ||
	    aux_state_status == PLDM_FD_TIMEOUT ||
	    aux_state_status == PLDM_FD_GENERIC_ERROR ||
	    (aux_state_status >= PLDM_FD_VENDOR_DEFINED_STATUS_CODE_START &&
	     aux_state_status <= PLDM_FD_VENDOR_DEFINED_STATUS_CODE_END)) {
		return true;
	}

	return false;
}

/** @brief Check if reason code in GetStatus command response is valid
 *
 *	@param[in] reason_code - provides the reason for why the current state
 *                           entered the IDLE state
 *
 *	@return true if reason code is valid, false if not
 */
static bool is_reason_code_valid(uint8_t reason_code)
{
	switch (reason_code) {
	case PLDM_FD_INITIALIZATION:
	case PLDM_FD_ACTIVATE_FW:
	case PLDM_FD_CANCEL_UPDATE:
	case PLDM_FD_TIMEOUT_LEARN_COMPONENT:
	case PLDM_FD_TIMEOUT_READY_XFER:
	case PLDM_FD_TIMEOUT_DOWNLOAD:
	case PLDM_FD_TIMEOUT_VERIFY:
	case PLDM_FD_TIMEOUT_APPLY:
		return true;

	default:
		if (reason_code >= PLDM_FD_STATUS_VENDOR_DEFINED_MIN) {
			return true;
		}
		return false;
	}
}

/** @brief Check if non functioning component indication in CancelUpdate
 *         response is valid
 *
 *  @return true if non functioning component indication is valid, false if not
 */
static bool is_non_functioning_component_indication_valid(
	bool8_t non_functioning_component_indication)
{
	switch (non_functioning_component_indication) {
	case PLDM_FWUP_COMPONENTS_FUNCTIONING:
	case PLDM_FWUP_COMPONENTS_NOT_FUNCTIONING:
		return true;

	default:
		return false;
	}
}

static int decode_pldm_package_header_info_errno(
	const uint8_t *data, size_t length,
	struct pldm_package_header_information *package_header_info,
	struct variable_field *package_version_str)
{
	if (data == NULL || package_header_info == NULL ||
	    package_version_str == NULL) {
		return -EINVAL;
	}

	if (length < sizeof(struct pldm_package_header_information)) {
		return -EOVERFLOW;
	}

	struct pldm_package_header_information *data_header =
		(struct pldm_package_header_information *)(data);

	if (!is_string_type_valid(data_header->package_version_string_type) ||
	    (data_header->package_version_string_length == 0)) {
		return -EBADMSG;
	}

	if (length < sizeof(struct pldm_package_header_information) +
			     data_header->package_version_string_length) {
		return -EOVERFLOW;
	}

	if ((data_header->component_bitmap_bit_length %
	     PLDM_FWUP_COMPONENT_BITMAP_MULTIPLE) != 0) {
		return -EBADMSG;
	}

	memcpy(package_header_info->uuid, data_header->uuid,
	       sizeof(data_header->uuid));
	package_header_info->package_header_format_version =
		data_header->package_header_format_version;
	package_header_info->package_header_size =
		le16toh(data_header->package_header_size);
	memcpy(package_header_info->package_release_date_time,
	       data_header->package_release_date_time,
	       sizeof(data_header->package_release_date_time));
	package_header_info->component_bitmap_bit_length =
		le16toh(data_header->component_bitmap_bit_length);
	package_header_info->package_version_string_type =
		data_header->package_version_string_type;
	package_header_info->package_version_string_length =
		data_header->package_version_string_length;
	package_version_str->ptr =
		data + sizeof(struct pldm_package_header_information);
	package_version_str->length =
		package_header_info->package_version_string_length;

	return 0;
}

LIBPLDM_ABI_STABLE
int decode_pldm_package_header_info(
	const uint8_t *data, size_t length,
	struct pldm_package_header_information *package_header_info,
	struct variable_field *package_version_str)
{
	int rc;

	rc = decode_pldm_package_header_info_errno(
		data, length, package_header_info, package_version_str);
	if (rc < 0) {
		return pldm_xlate_errno(rc);
	}

	return PLDM_SUCCESS;
}

static int decode_firmware_device_id_record_errno(
	const uint8_t *data, size_t length,
	uint16_t component_bitmap_bit_length,
	struct pldm_firmware_device_id_record *fw_device_id_record,
	struct variable_field *applicable_components,
	struct variable_field *comp_image_set_version_str,
	struct variable_field *record_descriptors,
	struct variable_field *fw_device_pkg_data)
{
	if (data == NULL || fw_device_id_record == NULL ||
	    applicable_components == NULL ||
	    comp_image_set_version_str == NULL || record_descriptors == NULL ||
	    fw_device_pkg_data == NULL) {
		return -EINVAL;
	}

	if (length < sizeof(struct pldm_firmware_device_id_record)) {
		return -EOVERFLOW;
	}

	if ((component_bitmap_bit_length %
	     PLDM_FWUP_COMPONENT_BITMAP_MULTIPLE) != 0) {
		return -EBADMSG;
	}

	struct pldm_firmware_device_id_record *data_record =
		(struct pldm_firmware_device_id_record *)(data);

	if (!is_string_type_valid(
		    data_record->comp_image_set_version_string_type) ||
	    (data_record->comp_image_set_version_string_length == 0)) {
		return -EBADMSG;
	}

	fw_device_id_record->record_length =
		le16toh(data_record->record_length);
	fw_device_id_record->descriptor_count = data_record->descriptor_count;
	fw_device_id_record->device_update_option_flags.value =
		le32toh(data_record->device_update_option_flags.value);
	fw_device_id_record->comp_image_set_version_string_type =
		data_record->comp_image_set_version_string_type;
	fw_device_id_record->comp_image_set_version_string_length =
		data_record->comp_image_set_version_string_length;
	fw_device_id_record->fw_device_pkg_data_length =
		le16toh(data_record->fw_device_pkg_data_length);

	if (length < fw_device_id_record->record_length) {
		return -EOVERFLOW;
	}

	uint16_t applicable_components_length =
		component_bitmap_bit_length /
		PLDM_FWUP_COMPONENT_BITMAP_MULTIPLE;
	size_t calc_min_record_length =
		sizeof(struct pldm_firmware_device_id_record) +
		applicable_components_length +
		data_record->comp_image_set_version_string_length +
		PLDM_FWUP_DEVICE_DESCRIPTOR_MIN_LEN +
		fw_device_id_record->fw_device_pkg_data_length;

	if (fw_device_id_record->record_length < calc_min_record_length) {
		return -EOVERFLOW;
	}

	applicable_components->ptr =
		data + sizeof(struct pldm_firmware_device_id_record);
	applicable_components->length = applicable_components_length;

	comp_image_set_version_str->ptr =
		applicable_components->ptr + applicable_components->length;
	comp_image_set_version_str->length =
		fw_device_id_record->comp_image_set_version_string_length;

	record_descriptors->ptr = comp_image_set_version_str->ptr +
				  comp_image_set_version_str->length;
	record_descriptors->length =
		fw_device_id_record->record_length -
		sizeof(struct pldm_firmware_device_id_record) -
		applicable_components_length -
		fw_device_id_record->comp_image_set_version_string_length -
		fw_device_id_record->fw_device_pkg_data_length;

	if (fw_device_id_record->fw_device_pkg_data_length) {
		fw_device_pkg_data->ptr =
			record_descriptors->ptr + record_descriptors->length;
		fw_device_pkg_data->length =
			fw_device_id_record->fw_device_pkg_data_length;
	}

	return 0;
}

LIBPLDM_ABI_STABLE
int decode_firmware_device_id_record(
	const uint8_t *data, size_t length,
	uint16_t component_bitmap_bit_length,
	struct pldm_firmware_device_id_record *fw_device_id_record,
	struct variable_field *applicable_components,
	struct variable_field *comp_image_set_version_str,
	struct variable_field *record_descriptors,
	struct variable_field *fw_device_pkg_data)
{
	int rc;

	rc = decode_firmware_device_id_record_errno(
		data, length, component_bitmap_bit_length, fw_device_id_record,
		applicable_components, comp_image_set_version_str,
		record_descriptors, fw_device_pkg_data);
	if (rc < 0) {
		return pldm_xlate_errno(rc);
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_pldm_descriptor_from_iter(struct pldm_descriptor_iter *iter,
				     struct pldm_descriptor *desc)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	int rc;

	if (!iter || !iter->field || !desc) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_FWUP_DEVICE_DESCRIPTOR_MIN_LEN,
				    iter->field->ptr, iter->field->length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, desc->descriptor_type);
	rc = pldm_msgbuf_extract(buf, desc->descriptor_length);
	if (rc) {
		return rc;
	}

	desc->descriptor_data = NULL;
	pldm_msgbuf_span_required(buf, desc->descriptor_length,
				  (void **)&desc->descriptor_data);
	iter->field->ptr = NULL;
	pldm_msgbuf_span_remaining(buf, (void **)&iter->field->ptr,
				   &iter->field->length);

	return pldm_msgbuf_complete(buf);
}

static int decode_descriptor_type_length_value_errno(
	const uint8_t *data, size_t length, uint16_t *descriptor_type,
	struct variable_field *descriptor_data)
{
	uint16_t descriptor_length = 0;

	if (data == NULL || descriptor_type == NULL ||
	    descriptor_data == NULL) {
		return -EINVAL;
	}

	if (length < PLDM_FWUP_DEVICE_DESCRIPTOR_MIN_LEN) {
		return -EOVERFLOW;
	}

	struct pldm_descriptor_tlv *entry =
		(struct pldm_descriptor_tlv *)(data);

	*descriptor_type = le16toh(entry->descriptor_type);
	descriptor_length = le16toh(entry->descriptor_length);
	if (*descriptor_type != PLDM_FWUP_VENDOR_DEFINED) {
		if (descriptor_length !=
		    get_descriptor_type_length(*descriptor_type)) {
			return -EBADMSG;
		}
	}

	if (length < (sizeof(*descriptor_type) + sizeof(descriptor_length) +
		      descriptor_length)) {
		return -EOVERFLOW;
	}

	descriptor_data->ptr = entry->descriptor_data;
	descriptor_data->length = descriptor_length;

	return 0;
}

LIBPLDM_ABI_STABLE
int decode_descriptor_type_length_value(const uint8_t *data, size_t length,
					uint16_t *descriptor_type,
					struct variable_field *descriptor_data)
{
	int rc;

	rc = decode_descriptor_type_length_value_errno(
		data, length, descriptor_type, descriptor_data);
	if (rc < 0) {
		return pldm_xlate_errno(rc);
	}

	return PLDM_SUCCESS;
}

static int decode_vendor_defined_descriptor_value_errno(
	const uint8_t *data, size_t length, uint8_t *descriptor_title_str_type,
	struct variable_field *descriptor_title_str,
	struct variable_field *descriptor_data)
{
	if (data == NULL || descriptor_title_str_type == NULL ||
	    descriptor_title_str == NULL || descriptor_data == NULL) {
		return -EINVAL;
	}

	if (length < sizeof(struct pldm_vendor_defined_descriptor_title_data)) {
		return -EOVERFLOW;
	}

	struct pldm_vendor_defined_descriptor_title_data *entry =
		(struct pldm_vendor_defined_descriptor_title_data *)(data);
	if (!is_string_type_valid(
		    entry->vendor_defined_descriptor_title_str_type) ||
	    (entry->vendor_defined_descriptor_title_str_len == 0)) {
		return -EBADMSG;
	}

	// Assuming at least 1 byte of VendorDefinedDescriptorData
	if (length < (sizeof(struct pldm_vendor_defined_descriptor_title_data) +
		      entry->vendor_defined_descriptor_title_str_len)) {
		return -EOVERFLOW;
	}

	*descriptor_title_str_type =
		entry->vendor_defined_descriptor_title_str_type;
	descriptor_title_str->ptr = entry->vendor_defined_descriptor_title_str;
	descriptor_title_str->length =
		entry->vendor_defined_descriptor_title_str_len;

	descriptor_data->ptr =
		descriptor_title_str->ptr + descriptor_title_str->length;
	descriptor_data->length =
		length -
		sizeof(entry->vendor_defined_descriptor_title_str_type) -
		sizeof(entry->vendor_defined_descriptor_title_str_len) -
		descriptor_title_str->length;

	return 0;
}

LIBPLDM_ABI_STABLE
int decode_vendor_defined_descriptor_value(
	const uint8_t *data, size_t length, uint8_t *descriptor_title_str_type,
	struct variable_field *descriptor_title_str,
	struct variable_field *descriptor_data)
{
	int rc;

	rc = decode_vendor_defined_descriptor_value_errno(
		data, length, descriptor_title_str_type, descriptor_title_str,
		descriptor_data);
	if (rc < 0) {
		return pldm_xlate_errno(rc);
	}

	return PLDM_SUCCESS;
}

static int decode_pldm_comp_image_info_errno(
	const uint8_t *data, size_t length,
	struct pldm_component_image_information *pldm_comp_image_info,
	struct variable_field *comp_version_str)
{
	if (data == NULL || pldm_comp_image_info == NULL ||
	    comp_version_str == NULL) {
		return -EINVAL;
	}

	if (length < sizeof(struct pldm_component_image_information)) {
		return -EOVERFLOW;
	}

	struct pldm_component_image_information *data_header =
		(struct pldm_component_image_information *)(data);

	if (!is_string_type_valid(data_header->comp_version_string_type) ||
	    (data_header->comp_version_string_length == 0)) {
		return -EBADMSG;
	}

	if (length < sizeof(struct pldm_component_image_information) +
			     data_header->comp_version_string_length) {
		return -EOVERFLOW;
	}

	pldm_comp_image_info->comp_classification =
		le16toh(data_header->comp_classification);
	pldm_comp_image_info->comp_identifier =
		le16toh(data_header->comp_identifier);
	pldm_comp_image_info->comp_comparison_stamp =
		le32toh(data_header->comp_comparison_stamp);
	pldm_comp_image_info->comp_options.value =
		le16toh(data_header->comp_options.value);
	pldm_comp_image_info->requested_comp_activation_method.value =
		le16toh(data_header->requested_comp_activation_method.value);
	pldm_comp_image_info->comp_location_offset =
		le32toh(data_header->comp_location_offset);
	pldm_comp_image_info->comp_size = le32toh(data_header->comp_size);
	pldm_comp_image_info->comp_version_string_type =
		data_header->comp_version_string_type;
	pldm_comp_image_info->comp_version_string_length =
		data_header->comp_version_string_length;

	if ((pldm_comp_image_info->comp_options.bits.bit1 == false &&
	     pldm_comp_image_info->comp_comparison_stamp !=
		     PLDM_FWUP_INVALID_COMPONENT_COMPARISON_TIMESTAMP)) {
		return -EBADMSG;
	}

	if (pldm_comp_image_info->comp_location_offset == 0 ||
	    pldm_comp_image_info->comp_size == 0) {
		return -EBADMSG;
	}

	comp_version_str->ptr =
		data + sizeof(struct pldm_component_image_information);
	comp_version_str->length =
		pldm_comp_image_info->comp_version_string_length;

	return 0;
}

LIBPLDM_ABI_STABLE
int decode_pldm_comp_image_info(
	const uint8_t *data, size_t length,
	struct pldm_component_image_information *pldm_comp_image_info,
	struct variable_field *comp_version_str)
{
	int rc;

	rc = decode_pldm_comp_image_info_errno(
		data, length, pldm_comp_image_info, comp_version_str);
	if (rc < 0) {
		return pldm_xlate_errno(rc);
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_query_device_identifiers_req(uint8_t instance_id,
					size_t payload_length,
					struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_QUERY_DEVICE_IDENTIFIERS_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	return encode_pldm_header_only(PLDM_REQUEST, instance_id, PLDM_FWUP,
				       PLDM_QUERY_DEVICE_IDENTIFIERS, msg);
}

LIBPLDM_ABI_STABLE
int decode_query_device_identifiers_resp(const struct pldm_msg *msg,
					 size_t payload_length,
					 uint8_t *completion_code,
					 uint32_t *device_identifiers_len,
					 uint8_t *descriptor_count,
					 uint8_t **descriptor_data)
{
	if (msg == NULL || completion_code == NULL ||
	    device_identifiers_len == NULL || descriptor_count == NULL ||
	    descriptor_data == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length <
	    sizeof(struct pldm_query_device_identifiers_resp)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_query_device_identifiers_resp *response =
		(struct pldm_query_device_identifiers_resp *)msg->payload;
	*device_identifiers_len = le32toh(response->device_identifiers_len);

	if (*device_identifiers_len < PLDM_FWUP_DEVICE_DESCRIPTOR_MIN_LEN) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	if (payload_length !=
	    sizeof(struct pldm_query_device_identifiers_resp) +
		    *device_identifiers_len) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	*descriptor_count = response->descriptor_count;

	if (*descriptor_count == 0) {
		return PLDM_ERROR_INVALID_DATA;
	}
	*descriptor_data =
		(uint8_t *)(msg->payload +
			    sizeof(struct pldm_query_device_identifiers_resp));
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_query_device_identifiers_resp(
	uint8_t instance_id, uint8_t descriptor_count,
	const struct pldm_descriptor *descriptors, struct pldm_msg *msg,
	size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (descriptors == NULL || msg == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	if (descriptor_count < 1) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only(PLDM_RESPONSE, instance_id, PLDM_FWUP,
				     PLDM_QUERY_DEVICE_IDENTIFIERS, msg);
	if (rc) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	/* Determine total length */
	uint32_t device_identifiers_len = 0;
	for (uint8_t i = 0; i < descriptor_count; i++) {
		const struct pldm_descriptor *d = &descriptors[i];
		device_identifiers_len +=
			2 * sizeof(uint16_t) + d->descriptor_length;
	}

	pldm_msgbuf_insert_uint8(buf, PLDM_SUCCESS);
	pldm_msgbuf_insert(buf, device_identifiers_len);
	pldm_msgbuf_insert(buf, descriptor_count);

	for (uint8_t i = 0; i < descriptor_count; i++) {
		const struct pldm_descriptor *d = &descriptors[i];
		pldm_msgbuf_insert(buf, d->descriptor_type);
		pldm_msgbuf_insert(buf, d->descriptor_length);
		if (d->descriptor_data == NULL) {
			return -EINVAL;
		}
		rc = pldm_msgbuf_insert_array(
			buf, d->descriptor_length,
			(const uint8_t *)d->descriptor_data,
			d->descriptor_length);
		if (rc) {
			return rc;
		}
	}

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_STABLE
int encode_get_firmware_parameters_req(uint8_t instance_id,
				       size_t payload_length,
				       struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_GET_FIRMWARE_PARAMETERS_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	return encode_pldm_header_only(PLDM_REQUEST, instance_id, PLDM_FWUP,
				       PLDM_GET_FIRMWARE_PARAMETERS, msg);
}

LIBPLDM_ABI_STABLE
int decode_get_firmware_parameters_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_get_firmware_parameters_resp *resp_data,
	struct variable_field *active_comp_image_set_ver_str,
	struct variable_field *pending_comp_image_set_ver_str,
	struct variable_field *comp_parameter_table)
{
	if (msg == NULL || resp_data == NULL ||
	    active_comp_image_set_ver_str == NULL ||
	    pending_comp_image_set_ver_str == NULL ||
	    comp_parameter_table == NULL || !payload_length) {
		return PLDM_ERROR_INVALID_DATA;
	}

	resp_data->completion_code = msg->payload[0];
	if (PLDM_SUCCESS != resp_data->completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length < sizeof(struct pldm_get_firmware_parameters_resp)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_firmware_parameters_resp *response =
		(struct pldm_get_firmware_parameters_resp *)msg->payload;

	if (!is_string_type_valid(
		    response->active_comp_image_set_ver_str_type) ||
	    (response->active_comp_image_set_ver_str_len == 0)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (response->pending_comp_image_set_ver_str_len == 0) {
		if (response->pending_comp_image_set_ver_str_type !=
		    PLDM_STR_TYPE_UNKNOWN) {
			return PLDM_ERROR_INVALID_DATA;
		}
	} else {
		if (!is_string_type_valid(
			    response->pending_comp_image_set_ver_str_type)) {
			return PLDM_ERROR_INVALID_DATA;
		}
	}

	size_t partial_response_length =
		sizeof(struct pldm_get_firmware_parameters_resp) +
		response->active_comp_image_set_ver_str_len +
		response->pending_comp_image_set_ver_str_len;

	if (payload_length < partial_response_length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	resp_data->capabilities_during_update.value =
		le32toh(response->capabilities_during_update.value);
	resp_data->comp_count = le16toh(response->comp_count);
	resp_data->active_comp_image_set_ver_str_type =
		response->active_comp_image_set_ver_str_type;
	resp_data->active_comp_image_set_ver_str_len =
		response->active_comp_image_set_ver_str_len;
	resp_data->pending_comp_image_set_ver_str_type =
		response->pending_comp_image_set_ver_str_type;
	resp_data->pending_comp_image_set_ver_str_len =
		response->pending_comp_image_set_ver_str_len;

	active_comp_image_set_ver_str->ptr =
		msg->payload + sizeof(struct pldm_get_firmware_parameters_resp);
	active_comp_image_set_ver_str->length =
		resp_data->active_comp_image_set_ver_str_len;

	if (resp_data->pending_comp_image_set_ver_str_len != 0) {
		pending_comp_image_set_ver_str->ptr =
			msg->payload +
			sizeof(struct pldm_get_firmware_parameters_resp) +
			resp_data->active_comp_image_set_ver_str_len;
		pending_comp_image_set_ver_str->length =
			resp_data->pending_comp_image_set_ver_str_len;
	} else {
		pending_comp_image_set_ver_str->ptr = NULL;
		pending_comp_image_set_ver_str->length = 0;
	}

	if (payload_length > partial_response_length && resp_data->comp_count) {
		comp_parameter_table->ptr =
			msg->payload +
			sizeof(struct pldm_get_firmware_parameters_resp) +
			resp_data->active_comp_image_set_ver_str_len +
			resp_data->pending_comp_image_set_ver_str_len;
		comp_parameter_table->length =
			payload_length - partial_response_length;
	} else {
		comp_parameter_table->ptr = NULL;
		comp_parameter_table->length = 0;
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_get_firmware_parameters_resp(
	uint8_t instance_id,
	const struct pldm_get_firmware_parameters_resp_full *resp_data,
	struct pldm_msg *msg, size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (resp_data == NULL || msg == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only(PLDM_RESPONSE, instance_id, PLDM_FWUP,
				     PLDM_GET_FIRMWARE_PARAMETERS, msg);
	if (rc) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, resp_data->completion_code);
	pldm_msgbuf_insert(buf, resp_data->capabilities_during_update.value);
	pldm_msgbuf_insert(buf, resp_data->comp_count);
	pldm_msgbuf_insert(buf,
			   resp_data->active_comp_image_set_ver_str.str_type);
	pldm_msgbuf_insert(buf,
			   resp_data->active_comp_image_set_ver_str.str_len);
	pldm_msgbuf_insert(buf,
			   resp_data->pending_comp_image_set_ver_str.str_type);
	pldm_msgbuf_insert(buf,
			   resp_data->pending_comp_image_set_ver_str.str_len);
	/* String data appended */
	rc = pldm_msgbuf_insert_array(
		buf, resp_data->active_comp_image_set_ver_str.str_len,
		resp_data->active_comp_image_set_ver_str.str_data,
		resp_data->active_comp_image_set_ver_str.str_len);
	if (rc) {
		return rc;
	}
	rc = pldm_msgbuf_insert_array(
		buf, resp_data->pending_comp_image_set_ver_str.str_len,
		resp_data->pending_comp_image_set_ver_str.str_data,
		resp_data->pending_comp_image_set_ver_str.str_len);
	if (rc) {
		return rc;
	}

	/* Further calls to encode_get_firmware_parameters_resp_comp_entry
	 * will populate the remainder */

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_TESTING
int encode_get_firmware_parameters_resp_comp_entry(
	const struct pldm_component_parameter_entry_full *comp,
	uint8_t *payload, size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (comp == NULL || payload == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, payload, *payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, comp->comp_classification);
	pldm_msgbuf_insert(buf, comp->comp_identifier);
	pldm_msgbuf_insert(buf, comp->comp_classification_index);

	pldm_msgbuf_insert(buf, comp->active_ver.comparison_stamp);
	pldm_msgbuf_insert(buf, (uint8_t)comp->active_ver.str.str_type);
	pldm_msgbuf_insert(buf, comp->active_ver.str.str_len);
	rc = pldm_msgbuf_insert_array(buf, PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN,
				      comp->active_ver.date,
				      PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, comp->pending_ver.comparison_stamp);
	pldm_msgbuf_insert(buf, (uint8_t)comp->pending_ver.str.str_type);
	pldm_msgbuf_insert(buf, comp->pending_ver.str.str_len);
	rc = pldm_msgbuf_insert_array(buf, PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN,
				      comp->pending_ver.date,
				      PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, comp->comp_activation_methods.value);
	pldm_msgbuf_insert(buf, comp->capabilities_during_update.value);

	rc = pldm_msgbuf_insert_array(buf, comp->active_ver.str.str_len,
				      comp->active_ver.str.str_data,
				      comp->active_ver.str.str_len);
	if (rc) {
		return rc;
	}
	rc = pldm_msgbuf_insert_array(buf, comp->pending_ver.str.str_len,
				      comp->pending_ver.str.str_data,
				      comp->pending_ver.str.str_len);
	if (rc) {
		return rc;
	}

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_STABLE
int decode_get_firmware_parameters_resp_comp_entry(
	const uint8_t *data, size_t length,
	struct pldm_component_parameter_entry *component_data,
	struct variable_field *active_comp_ver_str,
	struct variable_field *pending_comp_ver_str)
{
	if (data == NULL || component_data == NULL ||
	    active_comp_ver_str == NULL || pending_comp_ver_str == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (length < sizeof(struct pldm_component_parameter_entry)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_component_parameter_entry *entry =
		(struct pldm_component_parameter_entry *)(data);

	size_t entry_length = sizeof(struct pldm_component_parameter_entry) +
			      entry->active_comp_ver_str_len +
			      entry->pending_comp_ver_str_len;

	if (length < entry_length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	component_data->comp_classification =
		le16toh(entry->comp_classification);
	component_data->comp_identifier = le16toh(entry->comp_identifier);
	component_data->comp_classification_index =
		entry->comp_classification_index;
	component_data->active_comp_comparison_stamp =
		le32toh(entry->active_comp_comparison_stamp);
	component_data->active_comp_ver_str_type =
		entry->active_comp_ver_str_type;
	component_data->active_comp_ver_str_len =
		entry->active_comp_ver_str_len;
	memcpy(component_data->active_comp_release_date,
	       entry->active_comp_release_date,
	       sizeof(entry->active_comp_release_date));
	component_data->pending_comp_comparison_stamp =
		le32toh(entry->pending_comp_comparison_stamp);
	component_data->pending_comp_ver_str_type =
		entry->pending_comp_ver_str_type;
	component_data->pending_comp_ver_str_len =
		entry->pending_comp_ver_str_len;
	memcpy(component_data->pending_comp_release_date,
	       entry->pending_comp_release_date,
	       sizeof(entry->pending_comp_release_date));
	component_data->comp_activation_methods.value =
		le16toh(entry->comp_activation_methods.value);
	component_data->capabilities_during_update.value =
		le32toh(entry->capabilities_during_update.value);

	if (entry->active_comp_ver_str_len != 0) {
		active_comp_ver_str->ptr =
			data + sizeof(struct pldm_component_parameter_entry);
		active_comp_ver_str->length = entry->active_comp_ver_str_len;
	} else {
		active_comp_ver_str->ptr = NULL;
		active_comp_ver_str->length = 0;
	}

	if (entry->pending_comp_ver_str_len != 0) {
		pending_comp_ver_str->ptr =
			data + sizeof(struct pldm_component_parameter_entry) +
			entry->active_comp_ver_str_len;
		pending_comp_ver_str->length = entry->pending_comp_ver_str_len;
	} else {
		pending_comp_ver_str->ptr = NULL;
		pending_comp_ver_str->length = 0;
	}
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_query_downstream_devices_req(uint8_t instance_id,
					struct pldm_msg *msg)
{
	if (msg == NULL) {
		return -EINVAL;
	}

	return encode_pldm_header_only_errno(PLDM_REQUEST, instance_id,
					     PLDM_FWUP,
					     PLDM_QUERY_DOWNSTREAM_DEVICES,
					     msg);
}

LIBPLDM_ABI_STABLE
int decode_query_downstream_devices_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_query_downstream_devices_resp *resp_data)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	int rc;

	if (msg == NULL || resp_data == NULL || !payload_length) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_OPTIONAL_COMMAND_RESP_MIN_LEN,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_extract(buf, resp_data->completion_code);
	if (rc) {
		return rc;
	}
	if (PLDM_SUCCESS != resp_data->completion_code) {
		// Return the CC directly without decoding the rest of the payload
		return 0;
	}

	if (payload_length < PLDM_QUERY_DOWNSTREAM_DEVICES_RESP_BYTES) {
		return -EBADMSG;
	}

	rc = pldm_msgbuf_extract(buf,
				 resp_data->downstream_device_update_supported);
	if (rc) {
		return rc;
	}

	if (!is_downstream_device_update_support_valid(
		    resp_data->downstream_device_update_supported)) {
		return -EINVAL;
	}

	pldm_msgbuf_extract(buf, resp_data->number_of_downstream_devices);
	pldm_msgbuf_extract(buf, resp_data->max_number_of_downstream_devices);
	pldm_msgbuf_extract(buf, resp_data->capabilities.value);

	return pldm_msgbuf_complete_consumed(buf);
}

LIBPLDM_ABI_STABLE
int encode_query_downstream_identifiers_req(
	uint8_t instance_id,
	const struct pldm_query_downstream_identifiers_req *params_req,
	struct pldm_msg *msg, size_t payload_length)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	int rc;

	if (!msg || !params_req) {
		return -EINVAL;
	}

	if (!is_transfer_operation_flag_valid(
		    (enum transfer_op_flag)
			    params_req->transfer_operation_flag)) {
		return -EINVAL;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_QUERY_DOWNSTREAM_IDENTIFIERS;
	rc = pack_pldm_header_errno(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(buf,
				    PLDM_QUERY_DOWNSTREAM_IDENTIFIERS_REQ_BYTES,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, params_req->data_transfer_handle);
	// Data correctness has been verified, cast it to 1-byte data directly.
	pldm_msgbuf_insert(buf, params_req->transfer_operation_flag);

	return pldm_msgbuf_complete(buf);
}

LIBPLDM_ABI_STABLE
int decode_query_downstream_identifiers_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_query_downstream_identifiers_resp *resp_data,
	struct pldm_downstream_device_iter *iter)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	void *remaining = NULL;
	int rc = 0;

	if (msg == NULL || resp_data == NULL || iter == NULL ||
	    !payload_length) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_OPTIONAL_COMMAND_RESP_MIN_LEN,
				    msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_extract(buf, resp_data->completion_code);
	if (rc) {
		return rc;
	}
	if (PLDM_SUCCESS != resp_data->completion_code) {
		return 0;
	}

	if (payload_length < PLDM_QUERY_DOWNSTREAM_IDENTIFIERS_RESP_MIN_LEN) {
		return -EBADMSG;
	}

	pldm_msgbuf_extract(buf, resp_data->next_data_transfer_handle);
	pldm_msgbuf_extract(buf, resp_data->transfer_flag);

	rc = pldm_msgbuf_extract(buf, resp_data->downstream_devices_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, resp_data->number_of_downstream_devices);
	rc = pldm_msgbuf_span_required(
		buf, resp_data->downstream_devices_length, &remaining);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_complete(buf);
	if (rc) {
		return rc;
	}

	iter->field.ptr = remaining;
	iter->field.length = resp_data->downstream_devices_length;
	iter->devs = resp_data->number_of_downstream_devices;

	return 0;
}

LIBPLDM_ABI_STABLE
int decode_pldm_downstream_device_from_iter(
	struct pldm_downstream_device_iter *iter,
	struct pldm_downstream_device *dev)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	int rc;

	if (!iter || !dev) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 3, iter->field.ptr,
				    iter->field.length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, dev->downstream_device_index);
	pldm_msgbuf_extract(buf, dev->downstream_descriptor_count);
	iter->field.ptr = NULL;
	pldm_msgbuf_span_remaining(buf, (void **)&iter->field.ptr,
				   &iter->field.length);

	return pldm_msgbuf_complete(buf);
}

LIBPLDM_ABI_STABLE
int encode_get_downstream_firmware_parameters_req(
	uint8_t instance_id,
	const struct pldm_get_downstream_firmware_parameters_req *params_req,
	struct pldm_msg *msg, size_t payload_length)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	int rc;

	if (!msg || !params_req) {
		return -EINVAL;
	}

	if (!is_transfer_operation_flag_valid(
		    (enum transfer_op_flag)
			    params_req->transfer_operation_flag)) {
		return -EBADMSG;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_QUERY_DOWNSTREAM_FIRMWARE_PARAMETERS;
	rc = pack_pldm_header_errno(&header, &msg->hdr);
	if (rc < 0) {
		return rc;
	}

	rc = pldm_msgbuf_init_errno(
		buf, PLDM_GET_DOWNSTREAM_FIRMWARE_PARAMETERS_REQ_BYTES,
		msg->payload, payload_length);
	if (rc < 0) {
		return rc;
	}

	pldm_msgbuf_insert(buf, params_req->data_transfer_handle);
	// Data correctness has been verified, cast it to 1-byte data directly.
	pldm_msgbuf_insert(buf, params_req->transfer_operation_flag);

	return pldm_msgbuf_complete(buf);
}

LIBPLDM_ABI_STABLE
int decode_get_downstream_firmware_parameters_resp(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_get_downstream_firmware_parameters_resp *resp_data,
	struct pldm_downstream_device_parameters_iter *iter)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	void *remaining = NULL;
	size_t length;
	int rc;

	if (msg == NULL || resp_data == NULL || iter == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, PLDM_OPTIONAL_COMMAND_RESP_MIN_LEN,
				    msg->payload, payload_length);
	if (rc < 0) {
		return rc;
	}

	rc = pldm_msgbuf_extract(buf, resp_data->completion_code);
	if (rc < 0) {
		return rc;
	}
	if (PLDM_SUCCESS != resp_data->completion_code) {
		return 0;
	}

	if (payload_length <
	    PLDM_GET_DOWNSTREAM_FIRMWARE_PARAMETERS_RESP_MIN_LEN) {
		return -EBADMSG;
	}

	pldm_msgbuf_extract(buf, resp_data->next_data_transfer_handle);
	pldm_msgbuf_extract(buf, resp_data->transfer_flag);
	pldm_msgbuf_extract(buf,
			    resp_data->fdp_capabilities_during_update.value);
	pldm_msgbuf_extract(buf, resp_data->downstream_device_count);

	rc = pldm_msgbuf_span_remaining(buf, &remaining, &length);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_complete(buf);
	if (rc) {
		return rc;
	}

	iter->field.ptr = remaining;
	iter->field.length = length;
	iter->entries = resp_data->downstream_device_count;

	return 0;
}

LIBPLDM_ABI_STABLE
int decode_pldm_downstream_device_parameters_entry_from_iter(
	struct pldm_downstream_device_parameters_iter *iter,
	struct pldm_downstream_device_parameters_entry *entry)
{
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;
	void *comp_ver_str;
	size_t remaining;
	void *cursor;
	int rc;

	if (iter == NULL || entry == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(
		buf, PLDM_DOWNSTREAM_DEVICE_PARAMETERS_ENTRY_MIN_LEN,
		iter->field.ptr, iter->field.length);
	if (rc < 0) {
		return rc;
	}

	pldm_msgbuf_extract(buf, entry->downstream_device_index);
	pldm_msgbuf_extract(buf, entry->active_comp_comparison_stamp);
	pldm_msgbuf_extract(buf, entry->active_comp_ver_str_type);
	rc = pldm_msgbuf_extract(buf, entry->active_comp_ver_str_len);
	if (rc < 0) {
		return rc;
	}
	rc = pldm_msgbuf_extract_array(buf,
				       PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN,
				       entry->active_comp_release_date,
				       sizeof(entry->active_comp_release_date));
	if (rc < 0) {
		return rc;
	}

	// Fill the last byte with NULL character
	entry->active_comp_release_date[PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN] =
		'\0';

	pldm_msgbuf_extract(buf, entry->pending_comp_comparison_stamp);
	pldm_msgbuf_extract(buf, entry->pending_comp_ver_str_type);
	rc = pldm_msgbuf_extract(buf, entry->pending_comp_ver_str_len);
	if (rc < 0) {
		return rc;
	}

	rc = pldm_msgbuf_extract_array(
		buf, PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN,
		entry->pending_comp_release_date,
		sizeof(entry->pending_comp_release_date));
	if (rc < 0) {
		return rc;
	}

	// Fill the last byte with NULL character
	entry->pending_comp_release_date[PLDM_FWUP_COMPONENT_RELEASE_DATA_LEN] =
		'\0';

	pldm_msgbuf_extract(buf, entry->comp_activation_methods.value);
	pldm_msgbuf_extract(buf, entry->capabilities_during_update.value);

	comp_ver_str = NULL;
	pldm_msgbuf_span_required(buf, entry->active_comp_ver_str_len,
				  &comp_ver_str);
	entry->active_comp_ver_str = comp_ver_str;

	comp_ver_str = NULL;
	pldm_msgbuf_span_required(buf, entry->pending_comp_ver_str_len,
				  &comp_ver_str);
	entry->pending_comp_ver_str = comp_ver_str;

	cursor = NULL;
	rc = pldm_msgbuf_span_remaining(buf, &cursor, &remaining);
	if (rc < 0) {
		return rc;
	}

	iter->field.ptr = cursor;
	iter->field.length = remaining;

	return 0;
}

LIBPLDM_ABI_STABLE
int encode_request_update_req(uint8_t instance_id, uint32_t max_transfer_size,
			      uint16_t num_of_comp,
			      uint8_t max_outstanding_transfer_req,
			      uint16_t pkg_data_len,
			      uint8_t comp_image_set_ver_str_type,
			      uint8_t comp_image_set_ver_str_len,
			      const struct variable_field *comp_img_set_ver_str,
			      struct pldm_msg *msg, size_t payload_length)
{
	if (comp_img_set_ver_str == NULL || comp_img_set_ver_str->ptr == NULL ||
	    msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(struct pldm_request_update_req) +
				      comp_img_set_ver_str->length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	if ((comp_image_set_ver_str_len == 0) ||
	    (comp_image_set_ver_str_len != comp_img_set_ver_str->length)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if ((max_transfer_size < PLDM_FWUP_BASELINE_TRANSFER_SIZE) ||
	    (max_outstanding_transfer_req < PLDM_FWUP_MIN_OUTSTANDING_REQ)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!is_string_type_valid(comp_image_set_ver_str_type)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_REQUEST_UPDATE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	struct pldm_request_update_req *request =
		(struct pldm_request_update_req *)msg->payload;

	request->max_transfer_size = htole32(max_transfer_size);
	request->num_of_comp = htole16(num_of_comp);
	request->max_outstanding_transfer_req = max_outstanding_transfer_req;
	request->pkg_data_len = htole16(pkg_data_len);
	request->comp_image_set_ver_str_type = comp_image_set_ver_str_type;
	request->comp_image_set_ver_str_len = comp_image_set_ver_str_len;

	memcpy(msg->payload + sizeof(struct pldm_request_update_req),
	       comp_img_set_ver_str->ptr, comp_img_set_ver_str->length);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int decode_request_update_req(const struct pldm_msg *msg, size_t payload_length,
			      struct pldm_request_update_req_full *req)
{
	int rc;
	uint8_t t;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || req == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, req->max_transfer_size);
	pldm_msgbuf_extract(buf, req->num_of_comp);
	pldm_msgbuf_extract(buf, req->max_outstanding_transfer_req);
	pldm_msgbuf_extract(buf, req->pkg_data_len);
	rc = pldm_msgbuf_extract(buf, t);
	if (rc) {
		return rc;
	}
	if (t > PLDM_STR_TYPE_UTF_16BE) {
		return -EBADMSG;
	}
	req->image_set_ver.str_type = (enum pldm_firmware_update_string_type)t;
	pldm_msgbuf_extract(buf, req->image_set_ver.str_len);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_extract_array(buf, req->image_set_ver.str_len,
				       req->image_set_ver.str_data,
				       PLDM_FIRMWARE_MAX_STRING);
	if (rc) {
		return rc;
	}

	return pldm_msgbuf_complete_consumed(buf);
}

LIBPLDM_ABI_STABLE
int decode_request_update_resp(const struct pldm_msg *msg,
			       size_t payload_length, uint8_t *completion_code,
			       uint16_t *fd_meta_data_len,
			       uint8_t *fd_will_send_pkg_data)
{
	if (msg == NULL || completion_code == NULL ||
	    fd_meta_data_len == NULL || fd_will_send_pkg_data == NULL ||
	    !payload_length) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (*completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}

	if (payload_length != sizeof(struct pldm_request_update_resp)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_request_update_resp *response =
		(struct pldm_request_update_resp *)msg->payload;

	*fd_meta_data_len = le16toh(response->fd_meta_data_len);
	*fd_will_send_pkg_data = response->fd_will_send_pkg_data;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_request_update_resp(uint8_t instance_id,
			       const struct pldm_request_update_resp *resp_data,
			       struct pldm_msg *msg, size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	struct pldm_header_info header = {
		.instance = instance_id,
		.msg_type = PLDM_RESPONSE,
		.pldm_type = PLDM_FWUP,
		.command = PLDM_REQUEST_UPDATE,
	};
	rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert_uint8(buf, PLDM_SUCCESS);
	pldm_msgbuf_insert(buf, resp_data->fd_meta_data_len);
	pldm_msgbuf_insert(buf, resp_data->fd_will_send_pkg_data);

	/* TODO: DSP0267 1.3.0 adds GetPackageDataMaximumTransferSize */

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_STABLE
int encode_pass_component_table_req(uint8_t instance_id, uint8_t transfer_flag,
				    uint16_t comp_classification,
				    uint16_t comp_identifier,
				    uint8_t comp_classification_index,
				    uint32_t comp_comparison_stamp,
				    uint8_t comp_ver_str_type,
				    uint8_t comp_ver_str_len,
				    const struct variable_field *comp_ver_str,
				    struct pldm_msg *msg, size_t payload_length)
{
	if (comp_ver_str == NULL || comp_ver_str->ptr == NULL || msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(struct pldm_pass_component_table_req) +
				      comp_ver_str->length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	if ((comp_ver_str_len == 0) ||
	    (comp_ver_str_len != comp_ver_str->length)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!is_transfer_flag_valid(transfer_flag)) {
		return PLDM_INVALID_TRANSFER_OPERATION_FLAG;
	}

	if (!is_string_type_valid(comp_ver_str_type)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_PASS_COMPONENT_TABLE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	struct pldm_pass_component_table_req *request =
		(struct pldm_pass_component_table_req *)msg->payload;

	request->transfer_flag = transfer_flag;
	request->comp_classification = htole16(comp_classification);
	request->comp_identifier = htole16(comp_identifier);
	request->comp_classification_index = comp_classification_index;
	request->comp_comparison_stamp = htole32(comp_comparison_stamp);
	request->comp_ver_str_type = comp_ver_str_type;
	request->comp_ver_str_len = comp_ver_str_len;

	memcpy(msg->payload + sizeof(struct pldm_pass_component_table_req),
	       comp_ver_str->ptr, comp_ver_str->length);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int decode_pass_component_table_req(
	const struct pldm_msg *msg, size_t payload_length,
	struct pldm_pass_component_table_req_full *pcomp)
{
	int rc;
	uint8_t t;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || pcomp == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, pcomp->transfer_flag);
	pldm_msgbuf_extract(buf, pcomp->comp_classification);
	pldm_msgbuf_extract(buf, pcomp->comp_identifier);
	pldm_msgbuf_extract(buf, pcomp->comp_classification_index);
	pldm_msgbuf_extract(buf, pcomp->comp_comparison_stamp);
	rc = pldm_msgbuf_extract(buf, t);
	if (rc) {
		return rc;
	}
	if (t > PLDM_STR_TYPE_UTF_16BE) {
		return -EBADMSG;
	}
	pcomp->version.str_type = (enum pldm_firmware_update_string_type)t;
	rc = pldm_msgbuf_extract(buf, pcomp->version.str_len);
	if (rc) {
		return rc;
	}
	rc = pldm_msgbuf_extract_array(buf, pcomp->version.str_len,
				       pcomp->version.str_data,
				       PLDM_FIRMWARE_MAX_STRING);
	if (rc) {
		return rc;
	}

	return pldm_msgbuf_complete_consumed(buf);
}

LIBPLDM_ABI_STABLE
int decode_pass_component_table_resp(const struct pldm_msg *msg,
				     const size_t payload_length,
				     uint8_t *completion_code,
				     uint8_t *comp_resp,
				     uint8_t *comp_resp_code)
{
	if (msg == NULL || completion_code == NULL || comp_resp == NULL ||
	    comp_resp_code == NULL || !payload_length) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (*completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}

	if (payload_length != sizeof(struct pldm_pass_component_table_resp)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_pass_component_table_resp *response =
		(struct pldm_pass_component_table_resp *)msg->payload;

	if (!is_comp_resp_valid(response->comp_resp)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!is_comp_resp_code_valid(response->comp_resp_code)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*comp_resp = response->comp_resp;
	*comp_resp_code = response->comp_resp_code;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_pass_component_table_resp(
	uint8_t instance_id,
	const struct pldm_pass_component_table_resp *resp_data,
	struct pldm_msg *msg, size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only(PLDM_RESPONSE, instance_id, PLDM_FWUP,
				     PLDM_PASS_COMPONENT_TABLE, msg);
	if (rc) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert_uint8(buf, PLDM_SUCCESS);
	pldm_msgbuf_insert(buf, resp_data->comp_resp);
	pldm_msgbuf_insert(buf, resp_data->comp_resp_code);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_STABLE
int encode_update_component_req(
	uint8_t instance_id, uint16_t comp_classification,
	uint16_t comp_identifier, uint8_t comp_classification_index,
	uint32_t comp_comparison_stamp, uint32_t comp_image_size,
	bitfield32_t update_option_flags, uint8_t comp_ver_str_type,
	uint8_t comp_ver_str_len, const struct variable_field *comp_ver_str,
	struct pldm_msg *msg, size_t payload_length)
{
	if (comp_ver_str == NULL || comp_ver_str->ptr == NULL || msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length !=
	    sizeof(struct pldm_update_component_req) + comp_ver_str->length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	if (!comp_image_size) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if ((comp_ver_str_len == 0) ||
	    (comp_ver_str_len != comp_ver_str->length)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!is_string_type_valid(comp_ver_str_type)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_UPDATE_COMPONENT;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	struct pldm_update_component_req *request =
		(struct pldm_update_component_req *)msg->payload;

	request->comp_classification = htole16(comp_classification);
	request->comp_identifier = htole16(comp_identifier);
	request->comp_classification_index = comp_classification_index;
	request->comp_comparison_stamp = htole32(comp_comparison_stamp);
	request->comp_image_size = htole32(comp_image_size);
	request->update_option_flags.value = htole32(update_option_flags.value);
	request->comp_ver_str_type = comp_ver_str_type;
	request->comp_ver_str_len = comp_ver_str_len;

	memcpy(msg->payload + sizeof(struct pldm_update_component_req),
	       comp_ver_str->ptr, comp_ver_str->length);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int decode_update_component_req(const struct pldm_msg *msg,
				size_t payload_length,
				struct pldm_update_component_req_full *up)
{
	int rc;
	uint8_t t;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || up == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_extract(buf, up->comp_classification);
	pldm_msgbuf_extract(buf, up->comp_identifier);
	pldm_msgbuf_extract(buf, up->comp_classification_index);
	pldm_msgbuf_extract(buf, up->comp_comparison_stamp);
	pldm_msgbuf_extract(buf, up->comp_image_size);
	pldm_msgbuf_extract(buf, up->update_option_flags.value);
	rc = pldm_msgbuf_extract(buf, t);
	if (rc) {
		return rc;
	}
	if (t > PLDM_STR_TYPE_UTF_16BE) {
		return -EBADMSG;
	}
	up->version.str_type = (enum pldm_firmware_update_string_type)t;
	rc = pldm_msgbuf_extract(buf, up->version.str_len);
	if (rc) {
		return rc;
	}
	rc = pldm_msgbuf_extract_array(buf, up->version.str_len,
				       up->version.str_data,
				       PLDM_FIRMWARE_MAX_STRING);
	if (rc) {
		return rc;
	}

	if (buf->remaining != 0) {
		return -EINVAL;
	}

	return 0;
}

LIBPLDM_ABI_STABLE
int decode_update_component_resp(const struct pldm_msg *msg,
				 size_t payload_length,
				 uint8_t *completion_code,
				 uint8_t *comp_compatibility_resp,
				 uint8_t *comp_compatibility_resp_code,
				 bitfield32_t *update_option_flags_enabled,
				 uint16_t *time_before_req_fw_data)
{
	if (msg == NULL || completion_code == NULL ||
	    comp_compatibility_resp == NULL ||
	    comp_compatibility_resp_code == NULL ||
	    update_option_flags_enabled == NULL ||
	    time_before_req_fw_data == NULL || !payload_length) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (*completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}

	if (payload_length != sizeof(struct pldm_update_component_resp)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_update_component_resp *response =
		(struct pldm_update_component_resp *)msg->payload;

	if (!is_comp_compatibility_resp_valid(
		    response->comp_compatibility_resp)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!is_comp_compatibility_resp_code_valid(
		    response->comp_compatibility_resp_code)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*comp_compatibility_resp = response->comp_compatibility_resp;
	*comp_compatibility_resp_code = response->comp_compatibility_resp_code;
	update_option_flags_enabled->value =
		le32toh(response->update_option_flags_enabled.value);
	*time_before_req_fw_data = le16toh(response->time_before_req_fw_data);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_update_component_resp(
	uint8_t instance_id, const struct pldm_update_component_resp *resp_data,
	struct pldm_msg *msg, size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only(PLDM_RESPONSE, instance_id, PLDM_FWUP,
				     PLDM_UPDATE_COMPONENT, msg);
	if (rc) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert_uint8(buf, PLDM_SUCCESS);
	pldm_msgbuf_insert(buf, resp_data->comp_compatibility_resp);
	pldm_msgbuf_insert(buf, resp_data->comp_compatibility_resp_code);
	pldm_msgbuf_insert(buf, resp_data->update_option_flags_enabled.value);
	pldm_msgbuf_insert(buf, resp_data->time_before_req_fw_data);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_STABLE
int decode_request_firmware_data_req(const struct pldm_msg *msg,
				     size_t payload_length, uint32_t *offset,
				     uint32_t *length)
{
	if (msg == NULL || offset == NULL || length == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (payload_length != sizeof(struct pldm_request_firmware_data_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_request_firmware_data_req *request =
		(struct pldm_request_firmware_data_req *)msg->payload;
	*offset = le32toh(request->offset);
	*length = le32toh(request->length);

	if (*length < PLDM_FWUP_BASELINE_TRANSFER_SIZE) {
		return PLDM_FWUP_INVALID_TRANSFER_LENGTH;
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_request_firmware_data_req(
	uint8_t instance_id,
	const struct pldm_request_firmware_data_req *req_params,
	struct pldm_msg *msg, size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only(PLDM_REQUEST, instance_id, PLDM_FWUP,
				     PLDM_REQUEST_FIRMWARE_DATA, msg);
	if (rc) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, req_params->offset);
	pldm_msgbuf_insert(buf, req_params->length);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_STABLE
int encode_request_firmware_data_resp(uint8_t instance_id,
				      uint8_t completion_code,
				      struct pldm_msg *msg,
				      size_t payload_length)
{
	if (msg == NULL || !payload_length) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_RESPONSE;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_REQUEST_FIRMWARE_DATA;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	msg->payload[0] = completion_code;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_transfer_complete_req(const struct pldm_msg *msg,
				 size_t payload_length,
				 uint8_t *transfer_result)
{
	if (msg == NULL || transfer_result == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(*transfer_result)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*transfer_result = msg->payload[0];
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_transfer_complete_req(uint8_t instance_id, uint8_t transfer_result,
				 struct pldm_msg *msg, size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only(PLDM_REQUEST, instance_id, PLDM_FWUP,
				     PLDM_TRANSFER_COMPLETE, msg);
	if (rc) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_insert(buf, transfer_result);
	if (rc) {
		return rc;
	}

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_STABLE
int encode_transfer_complete_resp(uint8_t instance_id, uint8_t completion_code,
				  struct pldm_msg *msg, size_t payload_length)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(completion_code)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_RESPONSE;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_TRANSFER_COMPLETE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	msg->payload[0] = completion_code;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_verify_complete_req(const struct pldm_msg *msg,
			       size_t payload_length, uint8_t *verify_result)
{
	if (msg == NULL || verify_result == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(*verify_result)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*verify_result = msg->payload[0];
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_verify_complete_req(uint8_t instance_id, uint8_t verify_result,
			       struct pldm_msg *msg, size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only(PLDM_REQUEST, instance_id, PLDM_FWUP,
				     PLDM_VERIFY_COMPLETE, msg);
	if (rc) {
		return EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	rc = pldm_msgbuf_insert(buf, verify_result);
	if (rc) {
		return rc;
	}

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_STABLE
int encode_verify_complete_resp(uint8_t instance_id, uint8_t completion_code,
				struct pldm_msg *msg, size_t payload_length)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(completion_code)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_RESPONSE;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_VERIFY_COMPLETE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	msg->payload[0] = completion_code;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_apply_complete_req(const struct pldm_msg *msg, size_t payload_length,
			      uint8_t *apply_result,
			      bitfield16_t *comp_activation_methods_modification)
{
	if (msg == NULL || apply_result == NULL ||
	    comp_activation_methods_modification == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(struct pldm_apply_complete_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_apply_complete_req *request =
		(struct pldm_apply_complete_req *)msg->payload;

	*apply_result = request->apply_result;
	comp_activation_methods_modification->value =
		le16toh(request->comp_activation_methods_modification.value);

	if ((*apply_result != PLDM_FWUP_APPLY_SUCCESS_WITH_ACTIVATION_METHOD) &&
	    comp_activation_methods_modification->value) {
		return PLDM_ERROR_INVALID_DATA;
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_apply_complete_req(uint8_t instance_id,
			      const struct pldm_apply_complete_req *req_data,
			      struct pldm_msg *msg, size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only(PLDM_REQUEST, instance_id, PLDM_FWUP,
				     PLDM_APPLY_COMPLETE, msg);
	if (rc) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert(buf, req_data->apply_result);
	pldm_msgbuf_insert(
		buf, req_data->comp_activation_methods_modification.value);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_STABLE
int encode_apply_complete_resp(uint8_t instance_id, uint8_t completion_code,
			       struct pldm_msg *msg, size_t payload_length)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(completion_code)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_RESPONSE;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_APPLY_COMPLETE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	msg->payload[0] = completion_code;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int decode_activate_firmware_req(const struct pldm_msg *msg,
				 size_t payload_length, bool *self_contained)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || self_contained == NULL) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, payload_length);
	if (rc) {
		return 0;
	}

	uint8_t self_contained_u8 = 0;
	rc = pldm_msgbuf_extract(buf, self_contained_u8);
	if (rc) {
		return rc;
	}
	if (buf->remaining != 0) {
		return -EOVERFLOW;
	}
	*self_contained = (bool)self_contained_u8;
	return 0;
}

LIBPLDM_ABI_STABLE
int encode_activate_firmware_req(uint8_t instance_id,
				 bool8_t self_contained_activation_req,
				 struct pldm_msg *msg, size_t payload_length)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(struct pldm_activate_firmware_req)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	if (!is_self_contained_activation_req_valid(
		    self_contained_activation_req)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_ACTIVATE_FIRMWARE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	struct pldm_activate_firmware_req *request =
		(struct pldm_activate_firmware_req *)msg->payload;

	request->self_contained_activation_req = self_contained_activation_req;

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_activate_firmware_resp(const struct pldm_msg *msg,
				  size_t payload_length,
				  uint8_t *completion_code,
				  uint16_t *estimated_time_activation)
{
	if (msg == NULL || completion_code == NULL ||
	    estimated_time_activation == NULL || !payload_length) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (*completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}

	if (payload_length != sizeof(struct pldm_activate_firmware_resp)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_activate_firmware_resp *response =
		(struct pldm_activate_firmware_resp *)msg->payload;

	*estimated_time_activation =
		le16toh(response->estimated_time_activation);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_activate_firmware_resp(
	uint8_t instance_id,
	const struct pldm_activate_firmware_resp *resp_data,
	struct pldm_msg *msg, size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only(PLDM_RESPONSE, instance_id, PLDM_FWUP,
				     PLDM_ACTIVATE_FIRMWARE, msg);
	if (rc) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert_uint8(buf, PLDM_SUCCESS);
	pldm_msgbuf_insert(buf, resp_data->estimated_time_activation);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_STABLE
int encode_get_status_req(uint8_t instance_id, struct pldm_msg *msg,
			  size_t payload_length)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_GET_STATUS_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_GET_STATUS;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_get_status_resp(const struct pldm_msg *msg, size_t payload_length,
			   uint8_t *completion_code, uint8_t *current_state,
			   uint8_t *previous_state, uint8_t *aux_state,
			   uint8_t *aux_state_status, uint8_t *progress_percent,
			   uint8_t *reason_code,
			   bitfield32_t *update_option_flags_enabled)
{
	if (msg == NULL || completion_code == NULL || current_state == NULL ||
	    previous_state == NULL || aux_state == NULL ||
	    aux_state_status == NULL || progress_percent == NULL ||
	    reason_code == NULL || update_option_flags_enabled == NULL ||
	    !payload_length) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (*completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}

	if (payload_length != sizeof(struct pldm_get_status_resp)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_get_status_resp *response =
		(struct pldm_get_status_resp *)msg->payload;

	if (!is_state_valid(response->current_state)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (!is_state_valid(response->previous_state)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (!is_aux_state_valid(response->aux_state)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (!is_aux_state_status_valid(response->aux_state_status)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (response->progress_percent > PLDM_FWUP_MAX_PROGRESS_PERCENT) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (!is_reason_code_valid(response->reason_code)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if ((response->current_state == PLDM_FD_STATE_IDLE) ||
	    (response->current_state == PLDM_FD_STATE_LEARN_COMPONENTS) ||
	    (response->current_state == PLDM_FD_STATE_READY_XFER)) {
		if (response->aux_state !=
		    PLDM_FD_IDLE_LEARN_COMPONENTS_READ_XFER) {
			return PLDM_ERROR_INVALID_DATA;
		}
	}

	*current_state = response->current_state;
	*previous_state = response->previous_state;
	*aux_state = response->aux_state;
	*aux_state_status = response->aux_state_status;
	*progress_percent = response->progress_percent;
	*reason_code = response->reason_code;
	update_option_flags_enabled->value =
		le32toh(response->update_option_flags_enabled.value);

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_get_status_resp(uint8_t instance_id,
			   const struct pldm_get_status_resp *status,
			   struct pldm_msg *msg, size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (status == NULL || msg == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	if (status->completion_code != PLDM_SUCCESS) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only(PLDM_RESPONSE, instance_id, PLDM_FWUP,
				     PLDM_GET_STATUS, msg);
	if (rc) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert_uint8(buf, PLDM_SUCCESS);
	pldm_msgbuf_insert(buf, status->current_state);
	pldm_msgbuf_insert(buf, status->previous_state);
	pldm_msgbuf_insert(buf, status->aux_state);
	pldm_msgbuf_insert(buf, status->aux_state_status);
	pldm_msgbuf_insert(buf, status->progress_percent);
	pldm_msgbuf_insert(buf, status->reason_code);
	pldm_msgbuf_insert(buf, status->update_option_flags_enabled.value);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}

LIBPLDM_ABI_STABLE
int encode_cancel_update_component_req(uint8_t instance_id,
				       struct pldm_msg *msg,
				       size_t payload_length)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_CANCEL_UPDATE_COMPONENT_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_CANCEL_UPDATE_COMPONENT;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_cancel_update_component_resp(const struct pldm_msg *msg,
					size_t payload_length,
					uint8_t *completion_code)
{
	if (msg == NULL || completion_code == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(*completion_code)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*completion_code = msg->payload[0];
	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int encode_cancel_update_req(uint8_t instance_id, struct pldm_msg *msg,
			     size_t payload_length)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_CANCEL_UPDATE_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_header_info header = { 0 };
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_CANCEL_UPDATE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_STABLE
int decode_cancel_update_resp(const struct pldm_msg *msg, size_t payload_length,
			      uint8_t *completion_code,
			      bool8_t *non_functioning_component_indication,
			      bitfield64_t *non_functioning_component_bitmap)
{
	if (msg == NULL || completion_code == NULL ||
	    non_functioning_component_indication == NULL ||
	    non_functioning_component_bitmap == NULL || !payload_length) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (*completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}

	if (payload_length != sizeof(struct pldm_cancel_update_resp)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	struct pldm_cancel_update_resp *response =
		(struct pldm_cancel_update_resp *)msg->payload;

	if (!is_non_functioning_component_indication_valid(
		    response->non_functioning_component_indication)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*non_functioning_component_indication =
		response->non_functioning_component_indication;

	if (*non_functioning_component_indication) {
		non_functioning_component_bitmap->value =
			le64toh(response->non_functioning_component_bitmap);
	}

	return PLDM_SUCCESS;
}

LIBPLDM_ABI_TESTING
int encode_cancel_update_resp(uint8_t instance_id,
			      const struct pldm_cancel_update_resp *resp_data,
			      struct pldm_msg *msg, size_t *payload_length)
{
	int rc;
	struct pldm_msgbuf _buf;
	struct pldm_msgbuf *buf = &_buf;

	if (msg == NULL || payload_length == NULL) {
		return -EINVAL;
	}

	rc = encode_pldm_header_only(PLDM_RESPONSE, instance_id, PLDM_FWUP,
				     PLDM_CANCEL_UPDATE, msg);
	if (rc) {
		return -EINVAL;
	}

	rc = pldm_msgbuf_init_errno(buf, 0, msg->payload, *payload_length);
	if (rc) {
		return rc;
	}

	pldm_msgbuf_insert_uint8(buf, PLDM_SUCCESS);
	pldm_msgbuf_insert(buf,
			   resp_data->non_functioning_component_indication);
	pldm_msgbuf_insert(buf, resp_data->non_functioning_component_bitmap);

	return pldm_msgbuf_complete_used(buf, *payload_length, payload_length);
}
