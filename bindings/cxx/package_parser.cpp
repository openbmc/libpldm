#include "package_parser.hpp"

#include "utils.hpp"

#include <libpldm/firmware_update.h>
#include <libpldm/utils.h>
#include <expected>
#include <memory>

using namespace std;

namespace pldm
{

namespace fw_update
{

	static std::expected<void, std::string>
	helperParseFDDescriptor(variable_field &recordDescriptors,
				Descriptors &descriptors) noexcept
	{
		int rc;
		uint16_t descriptorType = 0;
		variable_field descriptorData{};

		rc = decode_descriptor_type_length_value(
			recordDescriptors.ptr, recordDescriptors.length,
			&descriptorType, &descriptorData);
		if (rc) {
			return std::unexpected(std::format(
				"Failed to decode descriptor type value of type '{}' and  length '{}', response code '{}'",
				descriptorType, recordDescriptors.length, rc));
		}

		if (descriptorType != PLDM_FWUP_VENDOR_DEFINED) {
			descriptors.emplace(
				descriptorType,
				DescriptorData{
					descriptorData.ptr,
					descriptorData.ptr +
						descriptorData.length });
		} else {
			uint8_t descTitleStrType = 0;
			variable_field descTitleStr{};
			variable_field vendorDefinedDescData{};

			rc = decode_vendor_defined_descriptor_value(
				descriptorData.ptr, descriptorData.length,
				&descTitleStrType, &descTitleStr,
				&vendorDefinedDescData);
			if (rc) {
				return std::unexpected(std::format(
					"Failed to decode vendor-defined descriptor value of type '{}' and  length '{}', response code '{}'",
					descriptorType,
					recordDescriptors.length, "RC", rc));
			}

			descriptors.emplace(
				descriptorType,
				std::make_tuple(
					utils::toString(descTitleStr),
					VendorDefinedDescriptorData{
						vendorDefinedDescData.ptr,
						vendorDefinedDescData.ptr +
							vendorDefinedDescData
								.length }));
		}

		auto nextDescriptorOffset =
			sizeof(pldm_descriptor_tlv().descriptor_type) +
			sizeof(pldm_descriptor_tlv().descriptor_length) +
			descriptorData.length;
		recordDescriptors.ptr += nextDescriptorOffset;
		recordDescriptors.length -= nextDescriptorOffset;

		return {};
	}

	static std::expected<Descriptors, std::string> helperParseFDDescriptors(
		variable_field recordDescriptors,
		pldm_firmware_device_id_record deviceIdRecHeader) noexcept
	{
		Descriptors descriptors{};
		while (deviceIdRecHeader.descriptor_count-- &&
		       (recordDescriptors.length > 0)) {
			auto result = helperParseFDDescriptor(recordDescriptors,
							      descriptors);

			if (!result.has_value()) {
				return std::unexpected(result.error());
			}
		}

		return descriptors;
	}

	static std::expected<void, std::string>
	helperParseFDIdRecord(const std::vector<uint8_t> &pkgHdr,
			      size_t &pkgHdrRemainingSize,
			      const PackageHeaderInfo &headerInfo,
			      FirmwareDeviceIDRecords &fwDeviceIDRecords,
			      size_t &offset) noexcept
	{
		pldm_firmware_device_id_record deviceIdRecHeader{};
		variable_field applicableComponents{};
		variable_field compImageSetVersionStr{};
		variable_field recordDescriptors{};
		variable_field fwDevicePkgData{};

		auto rc = decode_firmware_device_id_record(
			pkgHdr.data() + offset, pkgHdrRemainingSize,
			headerInfo.componentBitmapBitLength, &deviceIdRecHeader,
			&applicableComponents, &compImageSetVersionStr,
			&recordDescriptors, &fwDevicePkgData);
		if (rc) {
			return std::unexpected(std::format(
				"Failed to decode firmware device ID record, response code '{}'",
				rc));
		}

		auto result = helperParseFDDescriptors(recordDescriptors,
						       deviceIdRecHeader);

		if (!result.has_value()) {
			return std::unexpected(result.error());
		}

		auto descriptors = result.value();

		DeviceUpdateOptionFlags deviceUpdateOptionFlags =
			deviceIdRecHeader.device_update_option_flags.value;

		ApplicableComponents componentsList;

		for (size_t varBitfieldIdx = 0;
		     varBitfieldIdx < applicableComponents.length;
		     varBitfieldIdx++) {
			std::bitset<8> entry{ *(applicableComponents.ptr +
						varBitfieldIdx) };
			for (size_t idx = 0; idx < entry.size(); idx++) {
				if (entry[idx]) {
					componentsList.emplace_back(
						idx + (varBitfieldIdx *
						       entry.size()));
				}
			}
		}

		fwDeviceIDRecords.emplace_back(std::make_tuple(
			deviceUpdateOptionFlags, componentsList,
			utils::toString(compImageSetVersionStr),
			std::move(descriptors),
			FirmwareDevicePackageData{
				fwDevicePkgData.ptr,
				fwDevicePkgData.ptr + fwDevicePkgData.length }));
		offset += deviceIdRecHeader.record_length;
		pkgHdrRemainingSize -= deviceIdRecHeader.record_length;

		return {};
	}

	std::expected<size_t, std::string>
	PackageParser::parseFDIdentificationArea(
		const PackageHeaderInfo &headerInfo,
		FirmwareDeviceIDRecords &fwDeviceIDRecords,
		DeviceIDRecordCount deviceIdRecCount,
		const std::vector<uint8_t> &pkgHdr, size_t offset) noexcept
	{
		size_t pkgHdrRemainingSize = pkgHdr.size() - offset;

		while (deviceIdRecCount-- && (pkgHdrRemainingSize > 0)) {
			auto result = helperParseFDIdRecord(
				pkgHdr, pkgHdrRemainingSize, headerInfo,
				fwDeviceIDRecords, offset);

			if (!result.has_value()) {
				return std::unexpected(result.error());
			}
		}

		return offset;
	}

	static std::expected<void, std::string>
	helperParseCompImageInfoArea(ComponentImageInfos &componentImageInfos,
				     const std::vector<uint8_t> &pkgHdr,
				     size_t &pkgHdrRemainingSize,
				     size_t &offset) noexcept
	{
		pldm_component_image_information compImageInfo{};
		variable_field compVersion{};

		auto rc = decode_pldm_comp_image_info(pkgHdr.data() + offset,
						      pkgHdrRemainingSize,
						      &compImageInfo,
						      &compVersion);
		if (rc) {
			return std::unexpected(std::format(
				"Failed to decode component image information, response code '{}'",
				rc));
		}

		CompClassification compClassification =
			compImageInfo.comp_classification;
		CompIdentifier compIdentifier = compImageInfo.comp_identifier;
		CompComparisonStamp compComparisonTime =
			compImageInfo.comp_comparison_stamp;
		CompOptions compOptions = compImageInfo.comp_options.value;
		ReqCompActivationMethod reqCompActivationMethod =
			compImageInfo.requested_comp_activation_method.value;
		CompLocationOffset compLocationOffset =
			compImageInfo.comp_location_offset;
		CompSize compSize = compImageInfo.comp_size;

		componentImageInfos.emplace_back(std::make_tuple(
			compClassification, compIdentifier, compComparisonTime,
			compOptions, reqCompActivationMethod,
			compLocationOffset, compSize,
			utils::toString(compVersion)));
		offset += sizeof(pldm_component_image_information) +
			  compImageInfo.comp_version_string_length;
		pkgHdrRemainingSize -=
			sizeof(pldm_component_image_information) +
			compImageInfo.comp_version_string_length;

		return {};
	}

	std::expected<size_t, std::string>
	PackageParser::parseCompImageInfoArea(
		ComponentImageInfos &componentImageInfos,
		ComponentImageCount compImageCount,
		const std::vector<uint8_t> &pkgHdr, size_t offset) noexcept
	{
		size_t pkgHdrRemainingSize = pkgHdr.size() - offset;

		while (compImageCount-- && (pkgHdrRemainingSize > 0)) {
			auto result = helperParseCompImageInfoArea(
				componentImageInfos, pkgHdr,
				pkgHdrRemainingSize, offset);
			if (!result.has_value()) {
				return std::unexpected(result.error());
			}
		}

		return offset;
	}

	std::expected<void, std::string>
	PackageParser::validatePkgTotalSize(const PackageHeaderInfo &headerInfo,
					    const Package &package,
					    size_t pkgSize) noexcept
	{
		size_t calcPkgSize = headerInfo.pkgHeaderSize;
		for (const auto &componentImageInfo :
		     package.componentImageInfos) {
			CompLocationOffset compLocOffset = std::get<static_cast<
				size_t>(
				ComponentImageInfoPos::CompLocationOffsetPos)>(
				componentImageInfo);
			CompSize compSize = std::get<static_cast<size_t>(
				ComponentImageInfoPos::CompSizePos)>(
				componentImageInfo);

			if (compLocOffset != calcPkgSize) {
				auto cmpVersion = std::get<static_cast<size_t>(
					ComponentImageInfoPos::CompVersionPos)>(
					componentImageInfo);
				return std::unexpected(std::format(
					"Failed to validate the component location offset '{}' for version '{}' and package size '{}'",
					compLocOffset, cmpVersion,
					calcPkgSize));
			}

			calcPkgSize += compSize;
		}

		if (calcPkgSize != pkgSize) {
			return std::unexpected(std::format(
				"Failed to match package size '{}' to calculated package size '{}'.",
				pkgSize, calcPkgSize));
		}

		return {};
	}

	LIBPLDM_ABI_TESTING
	std::expected<Package, std::string>
	PackageParser::parse(const PackageHeaderInfo &headerInfo,
			     const std::vector<uint8_t> &pkgHdr,
			     size_t pkgSize) noexcept
	{
		if (headerInfo.pkgHeaderSize != pkgHdr.size()) {
			return std::unexpected(
				std::format("Invalid package header size '{}' ",
					    headerInfo.pkgHeaderSize));
		}

		size_t offset = sizeof(pldm_package_header_information) +
				headerInfo.pkgVersion.size();
		if (offset + sizeof(DeviceIDRecordCount) >=
		    headerInfo.pkgHeaderSize) {
			return std::unexpected(std::format(
				"Failed to parse package header of size '{}'",
				headerInfo.pkgHeaderSize));
		}

		auto deviceIdRecCount =
			static_cast<DeviceIDRecordCount>(pkgHdr[offset]);
		offset += sizeof(DeviceIDRecordCount);

		FirmwareDeviceIDRecords fwDeviceIDRecords;

		auto result = parseFDIdentificationArea(headerInfo,
							fwDeviceIDRecords,
							deviceIdRecCount,
							pkgHdr, offset);

		if (!result.has_value()) {
			return std::unexpected(result.error());
		}

		offset = result.value();
		if (deviceIdRecCount != fwDeviceIDRecords.size()) {
			return std::unexpected(std::format(
				"Failed to find DeviceIDRecordCount {} entries",
				deviceIdRecCount));
		}
		if (offset + sizeof(ComponentImageCount) >=
		    headerInfo.pkgHeaderSize) {
			return std::unexpected(std::format(
				"Failed to parsing package header of size '{}'",
				headerInfo.pkgHeaderSize));
		}

		ComponentImageInfos componentImageInfos;

		auto compImageCount = static_cast<ComponentImageCount>(
			le16toh(pkgHdr[offset] | (pkgHdr[offset + 1] << 8)));
		offset += sizeof(ComponentImageCount);

		result = parseCompImageInfoArea(componentImageInfos,
						compImageCount, pkgHdr, offset);

		if (!result.has_value()) {
			return std::unexpected(result.error());
		}

		offset = result.value();

		if (compImageCount != componentImageInfos.size()) {
			return std::unexpected(std::format(
				"Failed to find ComponentImageCount '{}' entries",
				compImageCount));
		}

		if (offset + sizeof(PackageHeaderChecksum) !=
		    headerInfo.pkgHeaderSize) {
			return std::unexpected(std::format(
				"Failed to parse package header of size '{}'",
				headerInfo.pkgHeaderSize));
		}

		auto calcChecksum = crc32(pkgHdr.data(), offset);
		auto checksum = static_cast<PackageHeaderChecksum>(
			le32toh(pkgHdr[offset] | (pkgHdr[offset + 1] << 8) |
				(pkgHdr[offset + 2] << 16) |
				(pkgHdr[offset + 3] << 24)));
		if (calcChecksum != checksum) {
			return std::unexpected(std::format(
				"Failed to parse package header for calculated checksum '{}' and header checksum '{}'",
				calcChecksum, checksum));
		}

		const Package package{ fwDeviceIDRecords, componentImageInfos };

		auto validateResult =
			validatePkgTotalSize(headerInfo, package, pkgSize);
		if (!validateResult.has_value()) {
			return std::unexpected(validateResult.error());
		}

		return package;
	}

	LIBPLDM_ABI_TESTING
	std::expected<PackageHeaderInfo, std::string>
	parsePkgHeader(const std::vector<uint8_t> &pkgHdrInfo) noexcept
	{
		constexpr std::array<uint8_t, PLDM_FWUP_UUID_LENGTH>
			hdrIdentifierv1{ 0xF0, 0x18, 0x87, 0x8C, 0xCB, 0x7D,
					 0x49, 0x43, 0x98, 0x00, 0xA0, 0x2F,
					 0x05, 0x9A, 0xCA, 0x02 };
		constexpr uint8_t pkgHdrVersion1 = 0x01;

		pldm_package_header_information pkgHeader{};
		variable_field pkgVersion{};
		auto rc = decode_pldm_package_header_info(pkgHdrInfo.data(),
							  pkgHdrInfo.size(),
							  &pkgHeader,
							  &pkgVersion);
		if (rc) {
			return std::unexpected(std::format(
				"Failed to decode PLDM package header information, response code '{}'",
				rc));
		}

		if (std::equal(pkgHeader.uuid,
			       pkgHeader.uuid + PLDM_FWUP_UUID_LENGTH,
			       hdrIdentifierv1.begin(),
			       hdrIdentifierv1.end()) &&
		    (pkgHeader.package_header_format_version ==
		     pkgHdrVersion1)) {
			PackageHeaderSize pkgHdrSize =
				pkgHeader.package_header_size;
			ComponentBitmapBitLength componentBitmapBitLength =
				pkgHeader.component_bitmap_bit_length;

			return PackageHeaderInfo(pkgHdrSize,
						 utils::toString(pkgVersion),
						 componentBitmapBitLength);
		}

		return std::unexpected("Failed to parse PLDM package header.");
	}

} // namespace fw_update

} // namespace pldm
