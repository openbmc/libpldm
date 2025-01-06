#pragma once

#include <libpldm/base.h>
#include <libpldm/utils.h>

#include <string>

namespace pldm
{
namespace utils
{

	/** @brief Convert the buffer to std::string
 *
 *  If there are characters that are not printable characters, it is replaced
 *  with space(0x20).
 *
 *  In case the string encoding is not ASCII, an empty string is returned.
 *
 *  @param[in] pldm_string_type - DSP0267, Table 20
 *  @param[in] var              - pointer to data and length of the data
 *
 *  @return std::string equivalent of variable field
 */
	std::string toString(uint8_t pldm_string_type,
			     const struct variable_field &var);

} // namespace utils
} // namespace pldm
