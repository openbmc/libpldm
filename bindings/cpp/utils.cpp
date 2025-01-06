#include "libpldm/bindings/cpp/utils.hpp"

#include <libpldm/pldm_types.h>

#include <algorithm>
#include <string>

namespace pldm
{
namespace utils
{

	std::string toString(const struct variable_field &var)
	{
		if (var.ptr == nullptr || !var.length) {
			return "";
		}

		// NOLINTBEGIN
		std::string str(reinterpret_cast<const char *>(var.ptr),
				var.length);
		// NOLINTEND
		return str;
	}

} // namespace utils
} // namespace pldm
