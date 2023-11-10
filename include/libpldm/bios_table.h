/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef LIBPLDM_BIOS_TABLE_H
#define LIBPLDM_BIOS_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libpldm/bios.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
struct variable_field;

/** @struct pldm_bios_table_iter
 *  structure representing bios table iterator
 */
struct pldm_bios_table_iter;

/** @brief Create a bios table iterator
 *  @param[in] table - Pointer to table data
 *  @param[in] length - Length of table data
 *  @param[in] type - Type of pldm bios table
 *  @return Iterator to the beginning on success. Returns NULL on failure.
 */
struct pldm_bios_table_iter *
pldm_bios_table_iter_create(const void *table, size_t length,
			    enum pldm_bios_table_types type);

/** @brief Release a bios table iterator
 *  @param[in] iter - Pointer to bios table iterator
 */
void pldm_bios_table_iter_free(struct pldm_bios_table_iter *iter);

/** @brief Check if the iterator reaches the end of the bios table
 *  @param[in] iter - Pointer to the bios table iterator
 *  @return true if iterator reaches the end
 *  @note *end* is a position after the last entry.
 */
bool pldm_bios_table_iter_is_end(const struct pldm_bios_table_iter *iter);

/** @brief Get iterator to next entry
 *  @param[in] iter - Pointer the bios table iterator
 */
void pldm_bios_table_iter_next(struct pldm_bios_table_iter *iter);

/** @brief Get the bios table entry that the iterator points to
 *  @param[in] iter - Pointer to the bios table iterator
 *  @return Pointer to an entry in bios table
 */
const void *pldm_bios_table_iter_value(struct pldm_bios_table_iter *iter);

/** @brief Get the bios attribute table entry that the iterator points to
 *  @param[in] iter - Pointer the bios attribute table iterator
 *  @return Pointer to an entry in bios attribute table
 */
static inline const struct pldm_bios_attr_table_entry *
pldm_bios_table_iter_attr_entry_value(struct pldm_bios_table_iter *iter)
{
	return (const struct pldm_bios_attr_table_entry *)
		pldm_bios_table_iter_value(iter);
}

/** @brief Get the bios string table entry that the iterator ponit to
 *  @param[in] iter - Pointer the bios string table iterator
 *  @return Pointer to an entry in bios string table
 */
static inline const struct pldm_bios_string_table_entry *
pldm_bios_table_iter_string_entry_value(struct pldm_bios_table_iter *iter)
{
	return (const struct pldm_bios_string_table_entry *)
		pldm_bios_table_iter_value(iter);
}

/** @brief Get the bios attribute value table entry that the iterator ponit to
 *  @param[in] iter - Pointer the bios attribute value table iterator
 *  @return Pointer to an entry in bios attribute value table
 */
static inline const struct pldm_bios_attr_val_table_entry *
pldm_bios_table_iter_attr_value_entry_value(struct pldm_bios_table_iter *iter)
{
	return (const struct pldm_bios_attr_val_table_entry *)
		pldm_bios_table_iter_value(iter);
}

/** @brief Get the length of an entry in the BIOS String Table
 *  @param[in] string_length - Length of string
 *  @return Length of an entry in bytes
 */
size_t pldm_bios_table_string_entry_encode_length(uint16_t string_length);

/** @brief Create an entry of BIOS String Table and check the validity of the
 * parameters
 *
 *  @param[out] entry - Pointer to a buffer to create an entry
 *  @param[in] entry_length - Length of the buffer to create an entry
 *  @param[in] str - String itself
 *  @param[in] str_length - Length of the string
 *  @return PLDM_SUCCESS on success, PLDM_ERROR_INVALID_DATA if entry or str are NULL, or
 *          PLDM_ERROR_INVALID_LENGTH if entry_length is insufficient for encoding str. An
 *          appropriate value for entry_length can be determined using
 *          @ref pldm_bios_table_string_entry_encode_length
 */
int pldm_bios_table_string_entry_encode_check(void *entry, size_t entry_length,
					      const char *str,
					      uint16_t str_length);

/** @brief Get the string handle for the entry
 *  @param[in] entry - Pointer to a bios string table entry
 *  @return Handle to identify a string in the bios string table
 */
uint16_t pldm_bios_table_string_entry_decode_handle(
	const struct pldm_bios_string_table_entry *entry);

/** @brief Get the string length for the entry
 *  @param[in] entry - Pointer to a bios string table entry
 *  @return Length of string in bytes
 */
uint16_t pldm_bios_table_string_entry_decode_string_length(
	const struct pldm_bios_string_table_entry *entry);

/** @brief Get the string from the entry and check the validity of the
 * parameters
 *  @param[in] entry - Pointer to a bios string table entry
 *  @param[out] buffer - Pointer to a buffer to store the string
 *  @param[in] size - Size of the buffer to store the string
 *  @return PLDM_SUCCESS on success, PLDM_ERROR_INVALID_DATA if entry or buffer are NULL, or
 *          PLDM_ERROR_INVALID_LENGTH if size is insufficient for NUL termination. An
 *          appropriate value for size can be determined using the expression
 *          `pldm_bios_table_string_entry_decode_string_length(entry) + 1`. The provided size value
 *          may be smaller than the entry's string, in which case the string placed in buffer will
 *          be truncated (but still NUL terminated).
 */
int pldm_bios_table_string_entry_decode_string_check(
	const struct pldm_bios_string_table_entry *entry, char *buffer,
	size_t size);

/** @brief Find an entry in bios string table by string
 *  @param[in] table - The BIOS String Table
 *  @param[in] length - Length of the BIOS String Table
 *  @param[in] str - String itself
 *  @return Pointer to an entry in the bios string table
 */
const struct pldm_bios_string_table_entry *
pldm_bios_table_string_find_by_string(const void *table, size_t length,
				      const char *str);
/** @brief Find an entry in bios string table by handle
 *  @param[in] table - The BIOS String Table
 *  @param[in] length - Length of the BIOS String Table
 *  @param[in] handle - Handle to identify a string in the bios string table
 *  @return Pointer to an entry in the bios string table
 */
const struct pldm_bios_string_table_entry *
pldm_bios_table_string_find_by_handle(const void *table, size_t length,
				      uint16_t handle);

/** @brief Get the attribute handle from the attribute table entry
 *  @param[in] entry - Pointer to bios attribute table entry
 *  @return handle to identify the attribute in the attribute table
 */
uint16_t pldm_bios_table_attr_entry_decode_attribute_handle(
	const struct pldm_bios_attr_table_entry *entry);

/** @brief Get the attribute type of the attribute table entry
 *  @param[in] entry - Pointer to bios attribute table entry
 *  @return Type of the attribute table entry
 */
uint8_t pldm_bios_table_attr_entry_decode_attribute_type(
	const struct pldm_bios_attr_table_entry *entry);

/** @brief Get the attribute name handle from the attribute table entry
 *  @param[in] entry - Pointer to bios attribute table entry
 *  @return handle to identify the name of the attribute, this handle points
 *          to a string in the bios string table.
 */
uint16_t pldm_bios_table_attr_entry_decode_string_handle(
	const struct pldm_bios_attr_table_entry *entry);

/** @brief Find an entry in attribute table by handle
 *  @param[in] table - The BIOS Attribute Table
 *  @param[in] length - Length of the BIOS Attribute Table
 *  @param[in] handle - handle to identify the attribute in the attribute table
 *  @return Pointer to the entry
 */
const struct pldm_bios_attr_table_entry *
pldm_bios_table_attr_find_by_handle(const void *table, size_t length,
				    uint16_t handle);

/** @brief Find an entry in attribute table by string handle
 *  @param[in] table - The BIOS Attribute Table
 *  @param[in] length - Length of the BIOS Attribute Table
 *  @param[in] handle - The string handle
 *  @return Pointer to the entry
 */
const struct pldm_bios_attr_table_entry *
pldm_bios_table_attr_find_by_string_handle(const void *table, size_t length,
					   uint16_t handle);

/** @struct pldm_bios_table_attr_entry_enum_info
 *
 *  An auxiliary structure for passing parameters to @ref
 * pldm_bios_table_attr_entry_enum_encode_check
 *
 */
struct pldm_bios_table_attr_entry_enum_info {
	uint16_t name_handle; //!< attribute name handle
	bool read_only;	      //!< indicate whether the attribute is read-only
	uint8_t pv_num;	      //!< number of possible values
	const uint16_t *pv_handle; //!< handles of possible values
	uint8_t def_num;	   //!< nnumber of default values
	const uint8_t *def_index;  //!< indices of default values.
};

/** @brief Get length that an attribute entry(type: enum) will take
 *  @param[in] pv_num - Number of possible values
 *  @param[in] def_num - Number of default values
 *  @return The length that an entry(type: enum) will take
 */
size_t pldm_bios_table_attr_entry_enum_encode_length(uint8_t pv_num,
						     uint8_t def_num);

/** @brief Create an entry of BIOS Attribute Table (type: enum) and check the
 * validity of the parameters
 *  @param[out] entry - Pointer to a buffer to create an entry
 *  @param[in] entry_length - Length of the buffer to create an entry
 *  @param[in] info - Pointer to an auxiliary structure @ref
 * pldm_bios_table_attr_entry_enum_info
 *  @return PLDM_SUCCESS on success, otherwise PLDM_ERROR_INVALID_DATA if entry or info are NULL, or
 *          PLDM_ERROR_INVALID_LENGTH if entry_length is insufficient to encode info. An appropriate
 *          value for entry_length can be determined using @ref
 *          pldm_bios_table_attr_entry_enum_encode_length.
 */
int pldm_bios_table_attr_entry_enum_encode_check(
	void *entry, size_t entry_length,
	const struct pldm_bios_table_attr_entry_enum_info *info);

/** @brief Get the total number of possible values for the entry and check the
 * validity of the parameters
 *  @param[in] entry - Pointer to bios attribute table entry
 *  @param[out] pv_num - Pointer to total number of possible values
 *  @return PLDM_SUCCESS on success, PLDM_ERROR_INVALID_DATA if entry or pv_num are NULL, or
 *          PLDM_ERROR_INVALID_DATA if entry is not a valid PLDM_BIOS_ENUMERATION
 */
int pldm_bios_table_attr_entry_enum_decode_pv_num_check(
	const struct pldm_bios_attr_table_entry *entry, uint8_t *pv_num);

/** @brief Get the total number of default values for the entry and check the
 * validity of the parameters
 *  @param[in] entry - Pointer to bios attribute table entry
 *  @param[out] def_num - Pointer to total number of default values
 *  @return PLDM_SUCCESS on success, otherwise PLDM_ERROR_INVALID_DATA if entry or def_num are NULL,
 *          or entry is not of type PLDM_BIOS_ENUMERATION.
 */
int pldm_bios_table_attr_entry_enum_decode_def_num_check(
	const struct pldm_bios_attr_table_entry *entry, uint8_t *def_num);

/** @brief Get possible values string handles and check the validity of the
 * parameters
 *  @param[in] entry - Pointer to bios attribute table entry
 *  @param[out] pv_hdls - Pointer to a buffer to store
 * PossibleValuesStringHandles
 *  @param[in] pv_num - Number of PossibleValuesStringHandles the buffer can
 * store
 *  @return PLDM_SUCCESS on success, otherwise PLDM_ERROR_INVALID_DATA if entry or pv_hdls are NULL,
 *          or entry is not of type PLDM_BIOS_ENUMERATION. An appropriate value for pv_num can be
 *          determined using @ref pldm_bios_table_attr_entry_enum_decode_pv_num_check. pv_num may be
 *          less than the value provided by @ref
 *          pldm_bios_table_attr_entry_enum_decode_pv_num_check, in which case only the first pv_num
 *          handles will be decoded.
 */
int pldm_bios_table_attr_entry_enum_decode_pv_hdls_check(
	const struct pldm_bios_attr_table_entry *entry, uint16_t *pv_hdls,
	uint8_t pv_num);

/** @brief Get Indices of default values
 *  @param[in] entry - Pointer to bios attribute table entry
 *  @param[out] def_indices - Pointer to a buffer to store
 *                            default value indices
 *  @param[in] def_num - Number of DefaultValues the buffer can
 *                       store
 *  @return Number of default values decoded
 */
uint8_t pldm_bios_table_attr_entry_enum_decode_def_indices(
	const struct pldm_bios_attr_table_entry *entry, uint8_t *def_indices,
	uint8_t def_num);

/** @struct pldm_bios_table_attr_entry_string_info
 *
 *  An auxiliary structure for passing parameters to @ref
 * pldm_bios_table_attr_entry_string_encode
 *
 */
struct pldm_bios_table_attr_entry_string_info {
	uint16_t name_handle;	//!< attribute name handle
	bool read_only;		//!< indicate whether the attribute is read-only
	uint8_t string_type;	//!< The type of the string
	uint16_t min_length;	//!< The minimum length of the string in bytes
	uint16_t max_length;	//!< The maximum length of the string in bytes
	uint16_t def_length;	//!< The length of the defaut string in bytes
	const char *def_string; //!< The default string itself
};

/** @brief Check fields in @ref pldm_bios_table_attr_entry_string_info
 *  @param[in] info - Pointer to the pldm_bios_table_attr_entry_string_info
 *  @param[out] errmsg - Pointer to an errmsg stored in the statically allocated
 * memory
 *  @return pldm_completion_codes
 */
int pldm_bios_table_attr_entry_string_info_check(
	const struct pldm_bios_table_attr_entry_string_info *info,
	const char **errmsg);

/** @brief Get length that an attribute entry(type: string) will take
 *  @param[in] def_str_len - Length of default string
 *  @return The length that an entry(type: string) will take
 */
size_t pldm_bios_table_attr_entry_string_encode_length(uint16_t def_str_len);

/** @brief Create an entry of BIOS Attribute Table (type: string) and check the
 * validity of the parameters
 *  @param[out] entry - Pointer to a buffer to create an entry
 *  @param[in] entry_length - Length of the buffer to create an entry
 *  @param[in] info - Pointer to an auxiliary structure @ref
 * pldm_bios_table_attr_entry_string_info
 *  @return PLDM_SUCCESS on success, PLDM_ERROR_INVALID_DATA if entry or info are NULL or info
 *          contains logically inconsistent data, or PLDM_ERROR_INVALID_LENGTH if entry_length is
 *          not sufficient to encode info. An appropriate value for entry_length can be determined
 *          using @ref pldm_bios_table_attr_entry_string_encode_length
 */
int pldm_bios_table_attr_entry_string_encode_check(
	void *entry, size_t entry_length,
	const struct pldm_bios_table_attr_entry_string_info *info);

/** @brief Get the length of default string in bytes for the entry and check the
 * validity of the parameters
 *  @param[in] entry - Pointer to bios attribute table entry
 *  @param[out] def_string_length Pointer to length of default string in bytes
 *  @return PLDM_SUCCESS on success, otherwise PLDM_ERROR_INVALID_DATA if entry or def_string_length
 *          are NULL, or entry is not of type PLDM_BIOS_STRING
 */
int pldm_bios_table_attr_entry_string_decode_def_string_length_check(
	const struct pldm_bios_attr_table_entry *entry,
	uint16_t *def_string_length);

/** @brief Get the type of string of bios attribute table entry
 *  @param[in] entry - Pointer to bios attribute table entry
 *  @return Type of the string
 */
uint8_t pldm_bios_table_attr_entry_string_decode_string_type(
	const struct pldm_bios_attr_table_entry *entry);

/** @brief Get maximum length of the string from a bios attribute table entry in
 * bytes
 *  @param[in] entry - Pointer to a bios attribute table entry
 *  @return Maximum length of the string
 */
uint16_t pldm_bios_table_attr_entry_string_decode_max_length(
	const struct pldm_bios_attr_table_entry *entry);

/** @brief Get minimum length of the string from a bios attribute table entry in
 * bytes
 *  @param[in] entry - Pointer to a bios attribute table entry
 *  @return Minimum length of the string
 */
uint16_t pldm_bios_table_attr_entry_string_decode_min_length(
	const struct pldm_bios_attr_table_entry *entry);

/** @brief Get the default string from a bios attribute table entry
 *  @param[out] buffer - Pointer to a buffer to store the string
 *  @param[in] size - Size of the buffer to store the string
 *  @return Length of the string decoded
 */
uint16_t pldm_bios_table_attr_entry_string_decode_def_string(
	const struct pldm_bios_attr_table_entry *entry, char *buffer,
	size_t size);

/** @struct pldm_bios_table_attr_entry_integer_info
 *
 *  An auxiliary structure for passing parameters to @ref
 * pldm_bios_table_attr_entry_integer_encode
 *
 */
struct pldm_bios_table_attr_entry_integer_info {
	uint16_t name_handle; //!< attribute name handle
	bool read_only;	      //!< indicate whether the attribute is read-only
	uint64_t lower_bound; //!< The lower bound on the integer value
	uint64_t upper_bound; //!< The upper bound on the integer value
	uint32_t scalar_increment; //!< The scalar value that is used for the
				   //!< increments to this integer
	uint64_t default_value;	   //!< The default value of the integer
};

/** @brief Check fields in @ref pldm_bios_table_attr_entry_integer_info
 *  @param[in] info - Pointer to the pldm_bios_table_attr_entry_integer_info
 *  @param[out] errmsg - Pointer to an errmsg stored in the statically allocated
 * memory
 *  @return pldm_completion_codes
 */
int pldm_bios_table_attr_entry_integer_info_check(
	const struct pldm_bios_table_attr_entry_integer_info *info,
	const char **errmsg);

/** @brief Get length that an attribute entry(type: integer) will take
 *  @return The length that an entry(type: integer) will take
 */
size_t pldm_bios_table_attr_entry_integer_encode_length(void);

/** @brief Create an entry of BIOS Attribute Table (type: integer) and check the
 * validity of the parameters
 *  @param[out] entry - Pointer to a buffer to create an entry
 *  @param[in] entry_length - Length of the buffer to create an entry
 *  @param[in] info - Pointer to an auxiliary structure @ref
 * pldm_bios_table_attr_entry_integer_info
 *  @return PLDM_SUCCESS on success. PLDM_ERROR_INVALID_DATA if entry or info are null, or the data
 *          in info is not logically consistent. PLDM_ERROR_INVALID_LENGTH if entry_length lacks
 *          capacity to encode the attribute.
 */
int pldm_bios_table_attr_entry_integer_encode_check(
	void *entry, size_t entry_length,
	const struct pldm_bios_table_attr_entry_integer_info *info);

/** @brief Decode the specific fields(integer) of attribute table entry
 *  @param[in] entry - Pointer to an entry of attribute table
 *  @param[out] lower - The lower bound on the integer value
 *  @param[out] upper - The upper bound on the integer value
 *  @param[out] scalar - The scalar value that is used for the increments to
 *                       this integer
 *  @param[out] def - The default value of the integer
 */
void pldm_bios_table_attr_entry_integer_decode(
	const struct pldm_bios_attr_table_entry *entry, uint64_t *lower,
	uint64_t *upper, uint32_t *scalar, uint64_t *def);

/** @brief Get the attribute handle from the attribute value table entry
 *  @param[in] entry - Pointer to bios attribute value table entry
 *  @return handle to identify the attribute in the attribute value table
 */
uint16_t pldm_bios_table_attr_value_entry_decode_attribute_handle(
	const struct pldm_bios_attr_val_table_entry *entry);

/** @brief Get the attribute type from the attribute value table entry
 *  @param[in] entry - Pointer to bios attribute value table entry
 *  @return Type of the attribute value entry
 */
uint8_t pldm_bios_table_attr_value_entry_decode_attribute_type(
	const struct pldm_bios_attr_val_table_entry *entry);

/** @brief Get length that an attribute value entry(type: enum) will take
 *  @param[in] count - Total number of current values for this enumeration
 *  @return The length that an entry(type: enum) will take
 */
size_t pldm_bios_table_attr_value_entry_encode_enum_length(uint8_t count);

/** @brief Get number of current values for the enum entry
 *  @param[in] entry - Pointer to bios attribute value table entry
 *  @return Total number of current values for this enumeration
 */
uint8_t pldm_bios_table_attr_value_entry_enum_decode_number(
	const struct pldm_bios_attr_val_table_entry *entry);

/** @brief Get CurrentValueStringHandleIndex
 *  @param[in] entry - Pointer to bios attribute value table entry
 *  @param[in, out] handles - Pointer to a buffer to store
 *                            CurrentValueStringHandleIndex
 *  @param[in] number - Number of PossibleValuesStringHandles expected
 *  @return Number of CurrentValueStringHandleIndex decoded.
 */
uint8_t pldm_bios_table_attr_value_entry_enum_decode_handles(
	const struct pldm_bios_attr_val_table_entry *entry, uint8_t *handles,
	uint8_t number);

/** @brief Create an attribute value entry(type: enum) and check the validity of
 * the parameters
 *  @param[out] entry - Pointer to bios attribute value entry
 *  @param[in] entry_length - Length of attribute value entry
 *  @param[in] attr_handle - This handle points to an attribute in the
 *  BIOS Attribute Vlaue Table.
 *  @param[in] attr_type - Type of this attribute in the BIOS Attribute Value
 * Table
 *  @param[in] count - Total number of current values for this enum attribute
 *  @param[in] handle_indexes - Index into the array(provided in the BIOS
 * Attribute Table) of the possible values of string handles for this attribute.
 *  @return PLDM_SUCCESS on success. PLDM_ERROR_INVALID_DATA if entry or handles are NULL, or if
 *          attr_type is not a PLDM_BIOS_ENUMERATION. PLDM_ERROR_INVALID_LENGTH if entry_length
 *          lacks capacity to encode handles into entry.
 */
int pldm_bios_table_attr_value_entry_encode_enum_check(
	void *entry, size_t entry_length, uint16_t attr_handle,
	uint8_t attr_type, uint8_t count, const uint8_t *handles);

/** @brief Get length that an attribute value entry(type: string) will take
 *  @param[in] string_length - Length of the current string in byte, 0 indicates
 *  that the current string value is not set.
 *  @return The length that an entry(type: string) will take
 */
size_t
pldm_bios_table_attr_value_entry_encode_string_length(uint16_t string_length);

/** @brief Get length of the current string in bytes
 *  @param [in] entry - Pointer to bios attribute value table entry
 *  @return The length of the current string in bytes
 */
uint16_t pldm_bios_table_attr_value_entry_string_decode_length(
	const struct pldm_bios_attr_val_table_entry *entry);

/** @brief Get Current String Itself
 *  @param[in] entry - Pointer to bios attribute value table entry
 *  @param[in, out] current_string - Struct variable_field, contains a pointer
 *                                   to the CurrentString field in the buffer of
 *                                    \p entry, \p entry must be valid
 *                                    when \p current_string is used.
 */
void pldm_bios_table_attr_value_entry_string_decode_string(
	const struct pldm_bios_attr_val_table_entry *entry,
	struct variable_field *current_string);

/** @brief Create an attribute value entry(type: string) and check the validity
 * of the parameters
 *  @param[out] entry - Pointer to bios attribute value entry
 *  @param[in] entry_length - Length of attribute value entry
 *  @param[in] attr_handle - This handle points to an attribute in the
 *  BIOS Attribute Vlaue Table.
 *  @param[in] attr_type - Type of this attribute in the BIOS Attribute Value
 * Table
 *  @param[in] string_length - Length of current string in bytes. 0 indicates
 * that the current string value is not set.
 *  @param[in] string - The current string itsel
 *  @return PLDM_SUCCESS on success. PLDM_ERROR_INVALID_DATA if entry is NULL, str is NULL while
 *  	    str_length is non-zero, or attr_type is not PLDM_BIOS_STRING. PLDM_ERROR_INVALID_LENGTH
 *  	    if entry_length lacks capacity to encode str into entry.
 */
int pldm_bios_table_attr_value_entry_encode_string_check(
	void *entry, size_t entry_length, uint16_t attr_handle,
	uint8_t attr_type, uint16_t str_length, const char *string);

/** @brief Get length that an attribute value entry(type: integer) will take
 *  @return The length that an entry(type: integer) will take
 */
size_t pldm_bios_table_attr_value_entry_encode_integer_length(void);

/** @brief Get current values for the integer entry
 *  @param[in] entry - Pointer to bios attribute value table entry
 *  @return Current Value
 */
uint64_t pldm_bios_table_attr_value_entry_integer_decode_cv(
	const struct pldm_bios_attr_val_table_entry *entry);

/** @brief Create an attribute value entry(type: integer) and check the validity
 * of the parameters
 *  @param[out] entry - Pointer to bios attribute value entry
 *  @param[in] entry_length - Length of attribute value entry
 *  @param[in] attr_handle - This handle points to an attribute in the
 *  BIOS Attribute Vlaue Table.
 *  @param[in] attr_type - Type of this attribute in the BIOS Attribute Value
 * Table
 *  @param[in] cv - Current Value
 *  @return PLDM_SUCCESS on success. PLDM_ERROR_INVALID_DATA if entry is NULL or attr_type is not
 *  	    PLDM_BIOS_INTEGER. PLDM_ERROR_INVALID_LENGTH if entry_length lacks capacity to encode cv
 *  	    in entry.
 */
int pldm_bios_table_attr_value_entry_encode_integer_check(void *entry,
							  size_t entry_length,
							  uint16_t attr_handle,
							  uint8_t attr_type,
							  uint64_t cv);

/** @brief Get the handle from the attribute value entry
 *  @param[in] entry - Pointer to bios attribute value entry
 *  @return handle to identify the attribute in the attribute value table
 */
uint16_t pldm_bios_table_attr_value_entry_decode_handle(
	const struct pldm_bios_attr_val_table_entry *entry);

/** @brief Get the length of the attribute value entry
 *  @param[in] entry - Pointer to bios attribute value entry
 *  @return Length of the entry
 */
size_t pldm_bios_table_attr_value_entry_length(
	const struct pldm_bios_attr_val_table_entry *entry);

/** @brief Find an entry in attribute value table by handle
 *  @param[in] table - The BIOS Attribute Value Table
 *  @param[in] length - Length of the BIOS Attribute Value Table
 *  @param[in] handle - handle to identify the attribute in the attribute value
 * table
 *  @return Pointer to the entry
 */
const struct pldm_bios_attr_val_table_entry *
pldm_bios_table_attr_value_find_by_handle(const void *table, size_t length,
					  uint16_t handle);

/** @brief Get the size of pad and checksum
 *  @param[in] size_without_pad - Table size without pad
 *  @return The size of pad and checksum
 */
size_t pldm_bios_table_pad_checksum_size(size_t size_without_pad);

/** @brief Append pad and checksum at the end of the table or return an error
 *  @param[in,out] table - Pointer to a buffer of a bios table
 *  @param[in] capacity - Size of the buffer of a bios table
 *  @param[in,out] size - On input, the table size without pad and checksum, on output, the table
 *  			  with the padding and checksum appended
 *  @return PLDM_SUCCESS on success, PLDM_INVALID_DATA if table or size are NULL, or
 *  	    PLDM_ERROR_INVALID_LENGTH if size lacks capacity to encode the padded checksum in the
 *  	    buffer provided by table. The appropriate buffer capacity can be determined with the
 *  	    help of @ref pldm_bios_table_pad_checksum_size
 */
int pldm_bios_table_append_pad_checksum_check(void *table, size_t capacity,
					      size_t *size);

/** @brief Build a new table and update an entry
 *  @param[in] src_table - Pointer to the source table
 *  @param[in] src_length - Size of the source table
 *  @param[out] dest_table - Pointer to the buffer of destination table
 *  @param[in,out] dest_length - Buffer size of the destination table as input
 *                               parameter and will be assigned the length of
 *                               the new table, if the function returns
 * 				 PLDM_SUCCESS
 *  @param[in] entry - Pointer to an entry
 *  @param[in] entry_length - Size of the entry
 *  @return pldm_completion_codes
 */
int pldm_bios_table_attr_value_copy_and_update(
	const void *src_table, size_t src_length, void *dest_table,
	size_t *dest_length, const void *entry, size_t entry_length);

/** @brief Verify the crc value of the complete table
 *  @param[in] table - Pointer to a buffer of a bios table
 *  @param[in] size - Size of the buffer of a bios table
 *  @return true: crc value is correct
 */
bool pldm_bios_table_checksum(const uint8_t *table, size_t size);

#ifdef __cplusplus
}
#endif

#endif
