#include "libpldm/bindings/cpp/package_parser.hpp"

#include "libpldm/bindings/cpp/utils.hpp"

#include <cassert>
#include <libpldm/firmware_update.h>
#include <libpldm/utils.h>
#include <expected>
#include <memory>
#include <iostream>

using namespace std;

namespace pldm
{

namespace fw_update
{

	static std::expected<void, std::string>
	helperParseFDDescriptor(struct pldm_descriptor *desc,
				Descriptors &descriptors) noexcept
	{
		int rc;
		variable_field descriptorData{};
		descriptorData.ptr = (const uint8_t *)desc->descriptor_data;
		descriptorData.length = desc->descriptor_length;

		if (desc->descriptor_type != PLDM_FWUP_VENDOR_DEFINED) {
			descriptors.emplace(
				desc->descriptor_type,
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
					desc->descriptor_type,
					descriptorData.length, "RC", rc));
			}

			descriptors.emplace(
				desc->descriptor_type,
				std::make_tuple(
					utils::toString(descTitleStr),
					VendorDefinedDescriptorData{
						vendorDefinedDescData.ptr,
						vendorDefinedDescData.ptr +
							vendorDefinedDescData
								.length }));
		}
		return {};
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

		FirmwareDeviceIDRecords fwDeviceIDRecords;

		struct pldm_firmware_device_id_iter iter;
		struct pldm_firmware_device_id_record dev;
		int rc;
		rc = decode_query_firmware_device_id_records(
			pkgHdr.data() + offset, pkgHdr.size(), &iter);

		if (rc != 0) {
			return std::unexpected(
				"Failed to parse firmware device id records");
		}

		auto deviceIdRecCount =
			static_cast<DeviceIDRecordCount>(pkgHdr[offset]);
		offset += sizeof(DeviceIDRecordCount);

		foreach_pldm_firmware_device_id_record(iter, dev, rc)
		{
			Descriptors descriptors{};
			struct variable_field field;
			struct pldm_descriptor_iter iterTLV;
			iterTLV.field = &field;
			// find ptr into the descriptors
			iterTLV.field->ptr =
				iter.field.ptr - dev.record_length + 2 + 1 + 4 +
				1 + 1 + 2 +
				dev.comp_image_set_version_string_length +
				(headerInfo.componentBitmapBitLength / 8);

			struct variable_field applicable_components;
			struct variable_field comp_image_set_version_str;
			struct variable_field record_descriptors;
			struct variable_field fw_device_pkg_data;

			rc = decode_firmware_device_id_record(
				iter.field.ptr - dev.record_length,
				iter.field.length + dev.record_length,
				headerInfo.componentBitmapBitLength, &dev,
				&applicable_components,
				&comp_image_set_version_str,
				&record_descriptors, &fw_device_pkg_data);

			if (rc != PLDM_SUCCESS) {
				return std::unexpected(std::format(
					"decode_firmware_device_id_record: rc {}",
					rc));
			}

			iterTLV.field->length = iter.field.length;
			iterTLV.count = dev.descriptor_count;

			struct pldm_descriptor tlv;

			foreach_pldm_firmware_descriptor_record(iterTLV, tlv,
								rc)
			{
				auto result = helperParseFDDescriptor(
					&tlv, descriptors);

				if (!result.has_value()) {
					return std::unexpected(result.error());
				}
			}

			if (descriptors.size() != dev.descriptor_count) {
				return std::unexpected(
					"error parsing firmware device descriptors");
			}

			DeviceUpdateOptionFlags deviceUpdateOptionFlags =
				dev.device_update_option_flags.value;

			ApplicableComponents componentsList;
			for (size_t varBitfieldIdx = 0;
			     varBitfieldIdx < applicable_components.length;
			     varBitfieldIdx++) {
				std::bitset<8> entry{ *(
					applicable_components.ptr +
					varBitfieldIdx) };
				for (size_t idx = 0; idx < entry.size();
				     idx++) {
					if (entry[idx]) {
						componentsList.emplace_back(
							idx + (varBitfieldIdx *
							       entry.size()));
					}
				}
			}

			FirmwareDevicePackageData fwDevicePkgData = {};

			if (dev.fw_device_pkg_data_length) {
				if (fw_device_pkg_data.length >=
				    headerInfo.pkgHeaderSize) {
					return std::unexpected(std::format(
						"fw_device_pkg_data.length == {}",
						fw_device_pkg_data.length));
				}
				fwDevicePkgData = {
					fw_device_pkg_data.ptr,
					fw_device_pkg_data.ptr +
						fw_device_pkg_data.length
				};
			}

			fwDeviceIDRecords.emplace_back(std::make_tuple(
				deviceUpdateOptionFlags, componentsList,
				utils::toString(comp_image_set_version_str),
				std::move(descriptors), fwDevicePkgData));

			offset += dev.record_length;
		}

		if (deviceIdRecCount != fwDeviceIDRecords.size()) {
			return std::unexpected(std::format(
				"Failed to find DeviceIDRecordCount {} entries",
				deviceIdRecCount));
		}
		if (offset + sizeof(ComponentImageCount) >=
		    headerInfo.pkgHeaderSize) {
			return std::unexpected(std::format(
				"Failed to parsing package header of size '{}'. (offset = {})",
				headerInfo.pkgHeaderSize, offset));
		}

		ComponentImageInfos componentImageInfos;

		auto compImageCount = static_cast<ComponentImageCount>(
			le16toh(pkgHdr[offset] | (pkgHdr[offset + 1] << 8)));

		offset += sizeof(ComponentImageCount);

		size_t pkgHdrRemainingSize = pkgHdr.size() - offset;

		auto tmpCompImageCount = compImageCount;

		struct pldm_component_image_information_iter iterComp;
		iterComp.count = tmpCompImageCount;
		iterComp.field.ptr = pkgHdr.data() + offset;
		iterComp.field.length = pkgHdrRemainingSize;

		struct pldm_component_image_information compImageInfo;
		foreach_pldm_component_image_information(iterComp,
							 compImageInfo, rc)
		{
			variable_field compVersion;
			compVersion.length =
				compImageInfo.comp_version_string_length;

			// we are behind the current element, just go back the length of the string
			compVersion.ptr =
				iterComp.field.ptr - compVersion.length;

			CompClassification compClassification =
				compImageInfo.comp_classification;
			CompIdentifier compIdentifier =
				compImageInfo.comp_identifier;
			CompComparisonStamp compComparisonTime =
				compImageInfo.comp_comparison_stamp;
			CompOptions compOptions =
				compImageInfo.comp_options.value;
			ReqCompActivationMethod reqCompActivationMethod =
				compImageInfo.requested_comp_activation_method
					.value;
			CompLocationOffset compLocationOffset =
				compImageInfo.comp_location_offset;
			CompSize compSize = compImageInfo.comp_size;

			componentImageInfos.emplace_back(std::make_tuple(
				compClassification, compIdentifier,
				compComparisonTime, compOptions,
				reqCompActivationMethod, compLocationOffset,
				compSize, utils::toString(compVersion)));

			offset += sizeof(pldm_component_image_information) +
				  compImageInfo.comp_version_string_length;
		}

		if (compImageCount != componentImageInfos.size()) {
			return std::unexpected(std::format(
				"Failed to find ComponentImageCount '{}' entries",
				compImageCount));
		}

		if (offset + sizeof(PackageHeaderChecksum) !=
		    headerInfo.pkgHeaderSize) {
			return std::unexpected(std::format(
				"Failed to parse package header of size '{}'. (offset = {})",
				headerInfo.pkgHeaderSize, offset));
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
