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

pldm::fw_update::DescriptorData::DescriptorData(const struct DescriptorData &ref)
	: vendorDefinedDescriptorTitle(ref.vendorDefinedDescriptorTitle),
	  data(ref.data)
{
}

pldm::fw_update::DescriptorData::DescriptorData(const std::vector<uint8_t> &data)
	: data(data)
{
}

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
	if (CompareNEQ(&DescriptorData::vendorDefinedDescriptorTitle, other)) {
		return false;
	}
	if (CompareNEQ(&DescriptorData::data, other)) {
		return false;
	}
	return true;
}

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
pldm::fw_update::ComponentImageInfo::ComponentImageInfo(
	const ComponentImageInfo &ref)
	: compClassification(ref.compClassification),
	  compIdentifier(ref.compIdentifier),
	  compComparisonStamp(ref.compComparisonStamp),
	  compOptions(ref.compOptions),
	  reqCompActivationMethod(ref.reqCompActivationMethod),
	  compLocation(ref.compLocation), compVersion(ref.compVersion)
{
}

LIBPLDM_ABI_TESTING
bool pldm::fw_update::ComponentImageInfo::operator==(
	const ComponentImageInfo &other) const
{
	if (CompareNEQ(&ComponentImageInfo::compClassification, other)) {
		return false;
	}
	if (CompareNEQ(&ComponentImageInfo::compIdentifier, other)) {
		return false;
	}
	if (CompareNEQ(&ComponentImageInfo::compComparisonStamp, other)) {
		return false;
	}
	if (CompareNEQ(&ComponentImageInfo::compOptions, other)) {
		return false;
	}
	if (CompareNEQ(&ComponentImageInfo::reqCompActivationMethod, other)) {
		return false;
	}
	if (HasMember(&ComponentImageInfo::compLocation) &&
	    other.HasMember(&ComponentImageInfo::compLocation)) {
		if (compLocation.length != other.compLocation.length) {
			return false;
		}
	}
	if (CompareNEQ(&ComponentImageInfo::compVersion, other)) {
		return false;
	}
	return true;
}

LIBPLDM_ABI_TESTING
pldm::fw_update::ComponentImageInfo::~ComponentImageInfo()
{
}

pldm::fw_update::FirmwareDeviceIDRecord::FirmwareDeviceIDRecord(
	const std::bitset<32> &deviceUpdateOptionFlags,
	const std::vector<size_t> &applicableComponents,
	const std::string &componentImageSetVersion,
	const std::map<uint16_t, std::unique_ptr<DescriptorData> >
		&descriptorsIn,
	const std::vector<uint8_t> &firmwareDevicePackageData)
	: deviceUpdateOptionFlags(deviceUpdateOptionFlags),
	  applicableComponents(applicableComponents),
	  componentImageSetVersion(componentImageSetVersion),
	  descriptors([&descriptorsIn]() {
		  std::map<uint16_t, std::unique_ptr<DescriptorData> > res;
		  // We have to init the map here manually since the descriptor constructor
		  // is not a friend of the template which would otherwise be able to construct it.
		  for (const auto &[key, desc] : descriptorsIn) {
			  res[key] = std::unique_ptr<DescriptorData>(
				  new DescriptorData(*desc));
		  }

		  return res;
	  }()),
	  firmwareDevicePackageData(firmwareDevicePackageData)
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::FirmwareDeviceIDRecord::FirmwareDeviceIDRecord(
	const FirmwareDeviceIDRecord &ref)
	: deviceUpdateOptionFlags(ref.deviceUpdateOptionFlags),
	  applicableComponents(ref.applicableComponents),
	  componentImageSetVersion(ref.componentImageSetVersion),
	  descriptors([&ref]() {
		  std::map<uint16_t, std::unique_ptr<DescriptorData> > res;
		  // We have to init the map here manually since the descriptor constructor
		  // is not a friend of the template which would otherwise be able to construct it.
		  for (const auto &[key, desc] : ref.descriptors) {
			  res[key] = std::unique_ptr<DescriptorData>(
				  new DescriptorData(*desc));
		  }

		  return res;
	  }()),
	  firmwareDevicePackageData(ref.firmwareDevicePackageData)
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::FirmwareDeviceIDRecord::~FirmwareDeviceIDRecord()
{
}

LIBPLDM_ABI_TESTING
const std::vector<uint16_t>
pldm::fw_update::FirmwareDeviceIDRecord::getDescriptorTypes() const
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
	if (CompareNEQ(&FirmwareDeviceIDRecord::deviceUpdateOptionFlags,
		       other)) {
		return false;
	}
	if (CompareNEQ(&FirmwareDeviceIDRecord::applicableComponents, other)) {
		return false;
	}
	if (CompareNEQ(&FirmwareDeviceIDRecord::componentImageSetVersion,
		       other)) {
		return false;
	}
	if (CompareNEQ(&FirmwareDeviceIDRecord::descriptors, other)) {
		return false;
	}
	if (CompareNEQ(&FirmwareDeviceIDRecord::firmwareDevicePackageData,
		       other)) {
		return false;
	}
	return true;
}

pldm::fw_update::Package::Package(
	const std::vector<FirmwareDeviceIDRecord> &fwDeviceIDRecords,
	const std::vector<ComponentImageInfo> &componentImageInfos)
	: fwDeviceIDRecords(fwDeviceIDRecords),
	  componentImageInfos(componentImageInfos)
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::Package::Package(const Package &ref)
	: fwDeviceIDRecords(ref.fwDeviceIDRecords),
	  componentImageInfos(ref.componentImageInfos)
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::Package::~Package()
{
}

LIBPLDM_ABI_TESTING
bool pldm::fw_update::Package::operator==(const Package &other) const
{
	if (CompareNEQ(&Package::fwDeviceIDRecords, other)) {
		return false;
	}
	if (CompareNEQ(&Package::componentImageInfos, other)) {
		return false;
	}
	return true;
}
