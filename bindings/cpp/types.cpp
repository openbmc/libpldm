#include <bitset>
#include <cstdint>
#include <libpldm/firmware_update.h>
#include <libpldm/bindings/cpp/types.hpp>
#include <map>
#include <ranges>
#include <string>
#include <vector>
#include <optional>
#include <memory>

LIBPLDM_ABI_TESTING
pldm::fw_update::DescriptorData::DescriptorData(const struct DescriptorData &ref)
	: vendorDefinedDescriptorTitle(ref.vendorDefinedDescriptorTitle)
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::DescriptorData::DescriptorData(const std::vector<uint8_t> &data)
	: data(data)
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::DescriptorData::DescriptorData(
	const std::string &title, const std::vector<uint8_t> &data)
	: vendorDefinedDescriptorTitle(title), data(data)
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::DescriptorData::~DescriptorData()
{
}

LIBPLDM_ABI_TESTING
bool pldm::fw_update::DescriptorData::operator==(
	const DescriptorData &other) const
{
	if (CompareNEQ(vendorDefinedDescriptorTitle, other)) {
		return false;
	}
	if (CompareNEQ(data, other)) {
		return false;
	}
	return true;
}

LIBPLDM_ABI_TESTING
pldm::fw_update::ComponentImageInfo::ComponentImageInfo(
	uint16_t compClassification, uint16_t compIdentifier,
	uint32_t compComparisonStamp, std::bitset<16> compOptions,
	std::bitset<16> reqCompActivationMethod,
	const variable_field &compLocation, const std::string &compVersion)
	: compClassification(compClassification),
	  compIdentifier(compIdentifier),
	  compComparisonStamp(compComparisonStamp), compOptions(compOptions),
	  reqCompActivationMethod(reqCompActivationMethod),
	  compLocation(compLocation), compVersion(compVersion)
{
}

LIBPLDM_ABI_TESTING
bool pldm::fw_update::ComponentImageInfo::operator==(
	const ComponentImageInfo &other) const
{
	if (CompareNEQ(compClassification, other)) {
		return false;
	}
	if (CompareNEQ(compIdentifier, other)) {
		return false;
	}
	if (CompareNEQ(compComparisonStamp, other)) {
		return false;
	}
	if (CompareNEQ(compOptions, other)) {
		return false;
	}
	if (CompareNEQ(reqCompActivationMethod, other)) {
		return false;
	}
	if (HasMember(compLocation) && other.HasMember(compLocation)) {
		if (CompareNEQ(compLocation.length, other)) {
			return false;
		}
	}
	if (CompareNEQ(compVersion, other)) {
		return false;
	}
	return true;
}

LIBPLDM_ABI_TESTING
pldm::fw_update::ComponentImageInfo::~ComponentImageInfo()
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::FirmwareDeviceIDRecord::FirmwareDeviceIDRecord(
	const std::bitset<32> &deviceUpdateOptionFlags,
	const std::vector<size_t> &applicableComponents,
	const std::string &componentImageSetVersion,
	const std::map<uint16_t, DescriptorData> &descriptors,
	const std::vector<uint8_t> &firmwareDevicePackageData)
	: deviceUpdateOptionFlags(deviceUpdateOptionFlags),
	  applicableComponents(applicableComponents),
	  componentImageSetVersion(componentImageSetVersion),
	  descriptors(descriptors),
	  firmwareDevicePackageData(firmwareDevicePackageData)
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::FirmwareDeviceIDRecord::~FirmwareDeviceIDRecord()
{
}

LIBPLDM_ABI_TESTING
std::vector<uint16_t>
pldm::fw_update::FirmwareDeviceIDRecord::getDescriptorTypes()
{
	std::vector<uint16_t> res;
	res.reserve(descriptors.size());

	for (auto &[k, _] : descriptors) {
		res.emplace_back(k);
	}
	return res;
}

LIBPLDM_ABI_TESTING
bool pldm::fw_update::FirmwareDeviceIDRecord::operator==(
	const FirmwareDeviceIDRecord &other) const
{
	if (CompareNEQ(deviceUpdateOptionFlags, other)) {
		return false;
	}
	if (CompareNEQ(applicableComponents, other)) {
		return false;
	}
	if (CompareNEQ(componentImageSetVersion, other)) {
		return false;
	}
	if (CompareNEQ(descriptors, other)) {
		return false;
	}
	if (CompareNEQ(firmwareDevicePackageData, other)) {
		return false;
	}
	return true;
}

LIBPLDM_ABI_TESTING
pldm::fw_update::DescriptorData
pldm::fw_update::FirmwareDeviceIDRecord::getDescriptor(uint16_t descriptorType)
{
	return descriptors.at(descriptorType);
}

LIBPLDM_ABI_TESTING
pldm::fw_update::Package::Package(
	const std::vector<FirmwareDeviceIDRecord> &fwDeviceIDRecords,
	const std::vector<ComponentImageInfo> &componentImageInfos)
	: fwDeviceIDRecords(fwDeviceIDRecords),
	  componentImageInfos(componentImageInfos)
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::Package::~Package()
{
}

LIBPLDM_ABI_TESTING
bool pldm::fw_update::Package::operator==(const Package &other) const
{
	if (CompareNEQ(fwDeviceIDRecords, other)) {
		return false;
	}
	if (CompareNEQ(componentImageInfos, other)) {
		return false;
	}
	return true;
}
