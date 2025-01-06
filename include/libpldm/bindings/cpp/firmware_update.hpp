#pragma once

#include "types.hpp"

#include <libpldm/firmware_update.h>

#include <cstdint>
#include <vector>
#include <expected>

namespace pldm
{

namespace fw_update
{

	struct Package {
		/** @brief Firmware Device ID Records in the package */
		const FirmwareDeviceIDRecords fwDeviceIDRecords;

		/** @brief Component Image Information in the package */
		const ComponentImageInfos componentImageInfos;
	};

	/** @class PackageParser
	 *
	 *  PackageParser is the abstract base class for parsing the PLDM firmware
	 *  update package. The PLDM firmware update contains two major sections; the
	 *  firmware package header, and the firmware package payload. Each package
	 *  header version will have a concrete implementation of the PackageParser.
	 *  The concrete implementation understands the format of the package header and
	 *  will implement the parse API.
	 */
	class PackageParser {
	    public:
		PackageParser() = delete;
		PackageParser(const PackageParser &) = delete;
		PackageParser(PackageParser &&) = default;
		PackageParser &operator=(const PackageParser &) = delete;
		PackageParser &operator=(PackageParser &&) = delete;
		virtual ~PackageParser() = default;

		/** @brief Parse the firmware update package header
		 *
		 *  @param[in] headerInfo - Package Header Info (calculated elsewhere)
		 *  @param[in] pkgHdr - Package header
		 *  @param[in] pkgSize - Size of the firmware update package
		 *
		 *  @note Throws exception is parsing fails
		 */
		LIBPLDM_ABI_TESTING
		static std::expected<Package, std::string>
		parse(const std::vector<uint8_t> &pkgHdr,
		      size_t pkgSize) noexcept;
	};

} // namespace fw_update

} // namespace pldm
