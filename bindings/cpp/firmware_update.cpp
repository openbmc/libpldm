#include "libpldm/bindings/cpp/firmware_update.hpp"

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

	LIBPLDM_ABI_TESTING
	std::expected<Package, std::string>
	PackageParser::parse(const std::vector<uint8_t> &pkgHdr,
			     size_t pkgSize) noexcept
	{
		int rc;
		FirmwareDeviceIDRecords fwDeviceIDRecords;

		pldm_package_header_information_pad hdr{};
		struct pldm_package_iter packageItr{};
		DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR01H(pin);

		rc = decode_pldm_firmware_update_package(
			pkgHdr.data(), pkgSize, &pin, &hdr, &packageItr);

		if (rc) {
			return std::unexpected(std::format(
				"Failed to decode pldm package header: rc={}",
				rc));
		}

		pldm_package_firmware_device_id_record deviceIdRecordData{};

		foreach_pldm_package_firmware_device_id_record(
			packageItr, deviceIdRecordData, rc)
		{
			Descriptors descriptors{};
			struct pldm_descriptor descriptorData;

			foreach_pldm_package_firmware_device_id_record_descriptor(
				packageItr, deviceIdRecordData, descriptorData,
				rc)
			{
				auto result = helperParseFDDescriptor(
					&descriptorData, descriptors);

				if (!result.has_value()) {
					return std::unexpected(result.error());
				}
			}

			if (descriptors.size() !=
			    deviceIdRecordData.descriptor_count) {
				return std::unexpected(
					"error parsing firmware device descriptors");
			}

			DeviceUpdateOptionFlags deviceUpdateOptionFlags =
				deviceIdRecordData.device_update_option_flags
					.value;

			ApplicableComponents componentsList;
			for (size_t varBitfieldIdx = 0;
			     varBitfieldIdx <
			     deviceIdRecordData.applicable_components.bitmap
				     .length;
			     varBitfieldIdx++) {
				std::bitset<8> entry{ *(
					deviceIdRecordData.applicable_components
						.bitmap.ptr +
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

			struct variable_field fw_device_pkg_data =
				deviceIdRecordData.firmware_device_package_data;

			if (fw_device_pkg_data.length) {
				fwDevicePkgData = {
					fw_device_pkg_data.ptr,
					fw_device_pkg_data.ptr +
						fw_device_pkg_data.length
				};
			}

			fwDeviceIDRecords.emplace_back(std::make_tuple(
				deviceUpdateOptionFlags, componentsList,
				utils::toString(
					deviceIdRecordData
						.component_image_set_version_string),
				std::move(descriptors), fwDevicePkgData));
		}

		if (rc) {
			return std::unexpected(std::format(
				"could not iterate fw device descriptors, rc={}",
				rc));
		}

		pldm_package_downstream_device_id_record downstreamDeviceIdRec{};
		foreach_pldm_package_downstream_device_id_record(
			packageItr, downstreamDeviceIdRec, rc)
		{
			// TODO: look into pldm downstream devices
		}

		if (rc) {
			return std::unexpected(std::format(
				"could not iterate downstream device descriptors, rc={}",
				rc));
		}

		ComponentImageInfos componentImageInfos;

		struct pldm_package_component_image_information compImageInfo;
		foreach_pldm_package_component_image_information(
			packageItr, compImageInfo, rc)
		{
			const CompClassification compClassification =
				compImageInfo.component_classification;
			const CompIdentifier compIdentifier =
				compImageInfo.component_identifier;
			const CompComparisonStamp compComparisonTime =
				compImageInfo.component_comparison_stamp;
			const CompOptions compOptions =
				compImageInfo.component_options.value;
			const ReqCompActivationMethod reqCompActivationMethod =
				compImageInfo
					.requested_component_activation_method
					.value;

			const int64_t offset =
				compImageInfo.component_image.ptr -
				pkgHdr.data();

			if (offset < 0) {
				return std::unexpected(
					"component location offset negative");
			}

			if ((size_t)offset >= pkgSize) {
				return std::unexpected(
					"component location offset too large");
			}

			const CompLocationOffset compLocationOffset = offset;

			const CompSize compSize =
				compImageInfo.component_image.length;

			componentImageInfos.emplace_back(std::make_tuple(
				compClassification, compIdentifier,
				compComparisonTime, compOptions,
				reqCompActivationMethod, compLocationOffset,
				compSize,
				utils::toString(
					compImageInfo
						.component_version_string)));
		}

		if (rc) {
			return std::unexpected(std::format(
				"could not iterate component image area, rc={}",
				rc));
		}

		const Package package{ fwDeviceIDRecords, componentImageInfos };

		return package;
	}

} // namespace fw_update

} // namespace pldm
