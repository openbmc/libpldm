#include "utils.hpp"

#include <libpldm/pldm_types.h>

#include <string>

namespace pldm
{
namespace utils
{

	std::string toString(const uint8_t pldm_string_type,
			     const struct variable_field &var)
	{
		if (var.ptr == nullptr || !var.length) {
			return "";
		}

		if (pldm_string_type != 1) {
			// unsupported string encoding
			return "";
		}

		// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
		std::string str(reinterpret_cast<const char *>(var.ptr),
				var.length);
		// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
		return str;
	}

} // namespace utils
} // namespace pldm
