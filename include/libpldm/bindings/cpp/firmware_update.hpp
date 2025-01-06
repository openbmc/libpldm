#pragma once

#include "types.hpp"

#include <libpldm/firmware_update.h>

#include <cstdint>
#include <memory>
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

	struct PackageHeaderInfo {
		/** @brief Device identifiers of the managed FDs */
		const PackageHeaderSize pkgHeaderSize;

		/** @brief Package version string */
		const PackageVersion pkgVersion;

		/** @brief The number of bits that will be used to represent the bitmap in
		*   the ApplicableComponents field for matching device. The value
		*   shall be a multiple of 8 and be large enough to contain a bit
		*   for each component in the package.
		*/
		const ComponentBitmapBitLength componentBitmapBitLength;
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
		parse(const PackageHeaderInfo &headerInfo,
		      const std::vector<uint8_t> &pkgHdr,
		      size_t pkgSize) noexcept;
	};

	/** @brief Parse the package header information
	 *
	 *  @param[in] pkgHdrInfo - package header information section in the package
	 *
	 *  @return On success return the PackageParser for the header format version
	 *          on failure return nullptr
	 */
	LIBPLDM_ABI_TESTING
	std::expected<PackageHeaderInfo, std::string>
	parsePkgHeader(const std::vector<uint8_t> &pkgHdrInfo) noexcept;

} // namespace fw_update

} // namespace pldm
