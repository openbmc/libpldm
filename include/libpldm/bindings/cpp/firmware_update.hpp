#pragma once

#include "types.hpp"

#include <libpldm/firmware_update.h>

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <expected>
#include <memory>

namespace pldm
{

namespace fw_update
{
	// To avoid issues like "ERROR: no symbols info in the ABI dump"
	// with the ABI tooling, expose a do-nothing stable symbol.
	// Which ensures we always have a non-empty stable ABI.
	LIBPLDM_ABI_STABLE
	void stable_nop();

	class PackageParserError {
	    public:
		explicit PackageParserError(std::string s);
		PackageParserError(std::string s, int rc);

		std::string msg;

		// error codes from libpldm
		std::optional<int> rc;
	};

	class PackageParser : private NonCopyableNonMoveable {
	    public:
		PackageParser() = delete;
		~PackageParser();

		/** @brief Parse the firmware update package
		 *
		 *  @param[in] pkg - vector with the pldm fw update package
		 *  @param[in] pin - pldm package format support
		 *
		 *  @returns an error value if parsing fails
		 *  @returns a unique_ptr to Package struct on success
		 */
		LIBPLDM_ABI_TESTING
		static std::expected<std::unique_ptr<Package>,
				     PackageParserError>
		parse(const std::vector<uint8_t> &pkg,
		      struct pldm_package_format_pin &pin) noexcept;

	    private:
		static std::expected<void, std::string> helperParseFDDescriptor(
			struct pldm_descriptor *desc,
			std::map<uint16_t,
				 std::unique_ptr<pldm::fw_update::DescriptorData> >
				&descriptors) noexcept;
	};

} // namespace fw_update

} // namespace pldm
