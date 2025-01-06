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

	class Package {
	    public:
		/** @brief Firmware Device ID Records in the package */
		const FirmwareDeviceIDRecords fwDeviceIDRecords;

		/** @brief Component Image Information in the package */
		const ComponentImageInfos componentImageInfos;
	};

	class PackageHeaderInfo {
	    public:
		/** @brief Device identifiers of the managed FDs */
		const PackageHeaderSize pkgHeaderSize;

		/** @brief Package version string */
		const PackageVersion pkgVersion;

		/** @brief The number of bits that will be used to represent the bitmap in
     *         the ApplicableComponents field for matching device. The value
     *         shall be a multiple of 8 and be large enough to contain a bit
     *         for each component in the package.
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
     *  @param[in] pkgHdr - Package header
     *  @param[in] pkgSize - Size of the firmware update package
     *
     *  @note Throws exception is parsing fails
     */
		static std::expected<Package, std::string>
		parse(const PackageHeaderInfo &headerInfo,
		      const std::vector<uint8_t> &pkgHdr,
		      uintmax_t pkgSize) noexcept;

	    private:
		/** @brief Parse the firmware device identification area
     *
     *  @param[in] deviceIdRecCount - count of firmware device ID records
     *  @param[in] pkgHdr - firmware package header
     *  @param[in] offset - offset in package header which is the start of the
     *                      firmware device identification area
     *
     *  @return On success return the offset which is the end of the firmware
     *          device identification area.
     */
		static std::expected<size_t, std::string>
		parseFDIdentificationArea(
			const PackageHeaderInfo &headerInfo,
			FirmwareDeviceIDRecords &fwDeviceIDRecords,
			DeviceIDRecordCount deviceIdRecCount,
			const std::vector<uint8_t> &pkgHdr,
			size_t offset) noexcept;

		/** @brief Parse the component image information area
     *
     *  @param[in] compImageCount - component image count
     *  @param[in] pkgHdr - firmware package header
     *  @param[in] offset - offset in package header which is the start of the
     *                      component image information area
     *
     *  @return On success return the offset which is the end of the component
     *          image information area.
     */
		static std::expected<size_t, std::string>
		parseCompImageInfoArea(ComponentImageInfos &componentImageInfos,
				       ComponentImageCount compImageCount,
				       const std::vector<uint8_t> &pkgHdr,
				       size_t offset) noexcept;

		/** @brief Validate the total size of the package
     *
     *  Verify the total size of the package is the sum of package header and
     *  the size of each component.
     *
     *  @param[in] pkgSize - firmware update package size
     *
     *  @note Throws exception if validation fails
     */
		static std::expected<void, std::string>
		validatePkgTotalSize(const PackageHeaderInfo &headerInfo,
				     const Package &package,
				     uintmax_t pkgSize) noexcept;
	};

	/** @brief Parse the package header information
 *
 *  @param[in] pkgHdrInfo - package header information section in the package
 *
 *  @return On success return the PackageParser for the header format version
 *          on failure return nullptr
 */
	std::expected<PackageHeaderInfo, std::string>
	parsePkgHeader(const std::vector<uint8_t> &pkgHdrInfo) noexcept;

} // namespace fw_update

} // namespace pldm
