#pragma once

#include <expected>
#include <libpldm/base.h>
#include <libpldm/utils.h>

#include <string>

namespace pldm
{
namespace utils
{

	/** @brief Convert the buffer to std::string
	 *
	 *  In case the string encoding is not ASCII, an error string is returned.
	 *
	 *  @param[in] pldm_string_type - DSP0267, Table 20
	 *  @param[in] var              - pointer to data and length of the data
	 *
	 *  @return[expected] std::string equivalent of variable field
	 *  @return[unexpected] error message string
	 */
	std::expected<std::string, std::string>
	toString(uint8_t pldm_string_type, const struct variable_field &var);

} // namespace utils
} // namespace pldm
