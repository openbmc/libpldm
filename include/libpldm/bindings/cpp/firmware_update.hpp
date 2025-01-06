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

	class PackageParserError {
	    public:
		PackageParserError(std::string s);

		std::string msg;
	};

	struct Package {
		const std::vector<FirmwareDeviceIDRecord> fwDeviceIDRecords;

		const std::vector<ComponentImageInfo> componentImageInfos;
	};

	class PackageParser {
	    public:
		PackageParser() = delete;
		PackageParser(const PackageParser &) = delete;
		PackageParser(PackageParser &&) = default;
		PackageParser &operator=(const PackageParser &) = delete;
		PackageParser &operator=(PackageParser &&) = delete;
		virtual ~PackageParser() = default;

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
	};

} // namespace fw_update

} // namespace pldm
