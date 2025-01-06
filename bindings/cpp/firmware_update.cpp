#include "libpldm/bindings/cpp/firmware_update.hpp"

#include "utils.hpp"

#include <cassert>
#include <libpldm/firmware_update.h>
#include <libpldm/utils.h>
#include <expected>
#include <format>

using namespace std;

pldm::fw_update::PackageParserError::PackageParserError(std::string s)
	: msg(std::move(s))
{
}

pldm::fw_update::PackageParserError::PackageParserError(std::string s, int rc)
	: msg(std::move(s)), rc(rc)
{
}

static std::expected<void, std::string>
helperParseFDDescriptor(struct pldm_descriptor *desc,
			std::map<uint16_t, pldm::fw_update::DescriptorData>
				&descriptors) noexcept
{
	int rc;
	variable_field descriptorData{};
	descriptorData.ptr = (const uint8_t *)desc->descriptor_data;
	descriptorData.length = desc->descriptor_length;

	if (desc->descriptor_type != PLDM_FWUP_VENDOR_DEFINED) {
		descriptors.emplace(
			desc->descriptor_type,
			pldm::fw_update::DescriptorData{ std::vector(
				descriptorData.ptr,
				descriptorData.ptr + descriptorData.length) });
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
				desc->descriptor_type, descriptorData.length,
				"RC", rc));
		}

		auto descrTitleStr =
			pldm::utils::toString(descTitleStrType, descTitleStr);

		if (!descrTitleStr.has_value()) {
			return std::unexpected(descrTitleStr.error());
		}
		descriptors.emplace(
			desc->descriptor_type,
			pldm::fw_update::DescriptorData(
				descrTitleStr.value(),
				std::vector<uint8_t>{
					vendorDefinedDescData.ptr,
					vendorDefinedDescData.ptr +
						vendorDefinedDescData.length }));
	}
	return {};
}

// @param[out] compList  The list of indices into the component array
// @param[in]  bitmap    The bitmap of applicable components
static void getApplicableComponents(std::vector<size_t> &compList,
				    struct variable_field &bitmap)
{
	for (size_t bitIdx = 0; bitIdx < bitmap.length * 8; bitIdx++) {
		const uint8_t applicable =
			((*(bitmap.ptr + (bitIdx / 8))) >> (bitIdx % 8)) & 0x1;

		if (applicable) {
			compList.emplace_back(bitIdx);
		}
	}
}

pldm::fw_update::PackageParser::~PackageParser()
{
}

LIBPLDM_ABI_TESTING
std::expected<std::unique_ptr<pldm::fw_update::Package>,
	      pldm::fw_update::PackageParserError>
pldm::fw_update::PackageParser::parse(
	const std::vector<uint8_t> &pkg,
	struct pldm_package_format_pin &pin) noexcept
{
	const size_t pkgSize = pkg.size();

	struct pldm_package package = {};
	std::vector<FirmwareDeviceIDRecord> fwDeviceIDRecords = {};
	pldm_package_header_information_pad hdr = {};
	int rc;

	if (pin.format.revision > PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR01H) {
		return std::unexpected(
			PackageParserError("unsupported format revision"));
	}

	rc = decode_pldm_firmware_update_package(pkg.data(), pkgSize, &pin,
						 &hdr, &package, 0);

	if (rc) {
		return std::unexpected(PackageParserError(
			"Failed to decode pldm package header", rc));
	}

	pldm_package_firmware_device_id_record deviceIdRecordData{};

	std::vector<std::vector<size_t> > componentsListList = {};

	foreach_pldm_package_firmware_device_id_record(package,
						       deviceIdRecordData, rc)
	{
		std::map<uint16_t, DescriptorData> descriptors{};
		struct pldm_descriptor descriptorData;

		foreach_pldm_package_firmware_device_id_record_descriptor(
			package, deviceIdRecordData, descriptorData, rc)
		{
			auto result = helperParseFDDescriptor(&descriptorData,
							      descriptors);

			if (!result.has_value()) {
				return std::unexpected(
					PackageParserError(result.error()));
			}
		}

		if (rc) {
			return std::unexpected(PackageParserError(
				"Failed to decode pldm package firmware device id record",
				rc));
		}

		std::bitset<32> deviceUpdateOptionFlags =
			deviceIdRecordData.device_update_option_flags.value;

		std::vector<size_t> componentsList;

		getApplicableComponents(
			componentsList,
			deviceIdRecordData.applicable_components.bitmap);

		componentsListList.push_back(componentsList);

		std::vector<uint8_t> fwDevicePkgData = {};

		struct variable_field fw_device_pkg_data =
			deviceIdRecordData.firmware_device_package_data;

		if (fw_device_pkg_data.length) {
			fwDevicePkgData = { fw_device_pkg_data.ptr,
					    fw_device_pkg_data.ptr +
						    fw_device_pkg_data.length };
		}

		auto imageSetVerStr = utils::toString(
			deviceIdRecordData
				.component_image_set_version_string_type,
			deviceIdRecordData.component_image_set_version_string);

		if (!imageSetVerStr.has_value()) {
			return std::unexpected(
				PackageParserError(imageSetVerStr.error()));
		}

		fwDeviceIDRecords.emplace_back(FirmwareDeviceIDRecord(
			deviceUpdateOptionFlags, {}, imageSetVerStr.value(),
			std::move(descriptors), fwDevicePkgData));
	}

	if (rc) {
		return std::unexpected(PackageParserError(
			"could not iterate fw device descriptors", rc));
	}

	std::vector<ComponentImageInfo> componentImageInfos = {};

	struct pldm_package_component_image_information imageInfo;
	foreach_pldm_package_component_image_information(package, imageInfo, rc)
	{
		const std::bitset<16> compOptions =
			imageInfo.component_options.value;
		const std::bitset<16> reqCompActivationMethod =
			imageInfo.requested_component_activation_method.value;

		auto compVerStr =
			utils::toString(imageInfo.component_version_string_type,
					imageInfo.component_version_string);

		if (!compVerStr.has_value()) {
			return std::unexpected(
				PackageParserError(compVerStr.error()));
		}

		componentImageInfos.emplace_back(ComponentImageInfo(
			imageInfo.component_classification,
			imageInfo.component_identifier,
			imageInfo.component_comparison_stamp, compOptions,
			reqCompActivationMethod, imageInfo.component_image,
			compVerStr.value()));
	}

	if (rc) {
		return std::unexpected(PackageParserError(
			"could not iterate component image area", rc));
	}

	return std::make_unique<Package>(fwDeviceIDRecords,
					 componentImageInfos);
}
