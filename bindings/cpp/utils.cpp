#include "utils.hpp"

#include <libpldm/firmware_update.h>
#include <libpldm/pldm_types.h>

#include <string>

namespace pldm
{
namespace utils
{

	std::expected<std::string, std::string>
	toString(const uint8_t pldm_string_type,
		 const struct variable_field &var)
	{
		if (var.ptr == nullptr || !var.length) {
			return std::unexpected("null pointer");
		}

		if (pldm_string_type != PLDM_STR_TYPE_ASCII) {
			// unsupported string encoding
			return std::unexpected("not an ascii string");
		}

		std::string s(var.length, ' ');
		for (size_t i = 0; i < var.length; i++) {
			s[i] = static_cast<char>(var.ptr[i]);
		}

		return s;
	}

} // namespace utils
} // namespace pldm
