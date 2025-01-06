#include <bitset>
#include <cstdint>
#include <libpldm/firmware_update.h>
#include <libpldm++/types.hpp>
#include <libpldm++/firmware_update.hpp>
#include <map>
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
	uint16_t componentClassification, uint16_t componentIdentifier,
	uint32_t componentComparisonStamp, std::bitset<16> componentOptions,
	std::bitset<16> requestedComponentActivationMethod,
	const variable_field &componentLocation,
	const std::string &componentVersion)
	: componentClassification(componentClassification),
	  componentIdentifier(componentIdentifier),
	  compComparisonStamp(componentComparisonStamp),
	  componentOptions(componentOptions),
	  requestedComponentActivationMethod(
		  requestedComponentActivationMethod),
	  componentLocation(componentLocation),
	  componentVersion(componentVersion)
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::ComponentImageInfo::ComponentImageInfo(
	const ComponentImageInfo &ref)
	: componentClassification(ref.componentClassification),
	  componentIdentifier(ref.componentIdentifier),
	  compComparisonStamp(ref.compComparisonStamp),
	  componentOptions(ref.componentOptions),
	  requestedComponentActivationMethod(
		  ref.requestedComponentActivationMethod),
	  componentLocation(ref.componentLocation),
	  componentVersion(ref.componentVersion)
{
}

LIBPLDM_ABI_TESTING
bool pldm::fw_update::ComponentImageInfo::operator==(
	const ComponentImageInfo &other) const
{
	if (CompareNEQ(&ComponentImageInfo::componentClassification, other)) {
		return false;
	}
	if (CompareNEQ(&ComponentImageInfo::componentIdentifier, other)) {
		return false;
	}
	if (CompareNEQ(&ComponentImageInfo::compComparisonStamp, other)) {
		return false;
	}
	if (CompareNEQ(&ComponentImageInfo::componentOptions, other)) {
		return false;
	}
	if (CompareNEQ(&ComponentImageInfo::requestedComponentActivationMethod,
		       other)) {
		return false;
	}
	if (HasMember(&ComponentImageInfo::componentLocation) &&
	    other.HasMember(&ComponentImageInfo::componentLocation)) {
		if (componentLocation.length !=
		    other.componentLocation.length) {
			return false;
		}
	}
	if (CompareNEQ(&ComponentImageInfo::componentVersion, other)) {
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
	  componentImageSetVersionString(componentImageSetVersion),
	  recordDescriptors([&descriptorsIn]() {
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
	  componentImageSetVersionString(ref.componentImageSetVersionString),
	  recordDescriptors([&ref]() {
		  std::map<uint16_t, std::unique_ptr<DescriptorData> > res;
		  // We have to init the map here manually since the descriptor constructor
		  // is not a friend of the template which would otherwise be able to construct it.
		  for (const auto &[key, desc] : ref.recordDescriptors) {
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
	res.reserve(recordDescriptors.size());

	for (auto &[k, _] : recordDescriptors) {
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
	if (CompareNEQ(&FirmwareDeviceIDRecord::componentImageSetVersionString,
		       other)) {
		return false;
	}
	if (CompareNEQ(&FirmwareDeviceIDRecord::firmwareDevicePackageData,
		       other)) {
		return false;
	}

	// need to manually compare the map since otherwise unique_ptr
	// default comparison would only compare pointer values
	if (HasMember(&FirmwareDeviceIDRecord::recordDescriptors) !=
	    other.HasMember(&FirmwareDeviceIDRecord::recordDescriptors)) {
		// different size structs compare not equal
		return false;
	}
	if (!HasMember(&FirmwareDeviceIDRecord::recordDescriptors) &&
	    !other.HasMember(&FirmwareDeviceIDRecord::recordDescriptors)) {
		// none of them has record descriptors field
		return true;
	}

	// both have record descriptors field
	if (recordDescriptors.size() != other.recordDescriptors.size()) {
		return false;
	}

	for (const auto &[k, v] : recordDescriptors) {
		if (!other.recordDescriptors.contains(k)) {
			return false;
		}
		const auto &otherDesc = other.recordDescriptors.at(k);

		if (!v.get() && !otherDesc.get()) {
			continue;
		}
		if (!v.get() || !otherDesc.get()) {
			return false;
		}

		// descriptor value comparison
		if (*v != *otherDesc) {
			return false;
		}
	}

	return true;
}

pldm::fw_update::Package::Package(
	const std::vector<FirmwareDeviceIDRecord> &firmwareDeviceIdRecords,
	const std::vector<ComponentImageInfo> &componentImageInformation)
	: firmwareDeviceIdRecords(firmwareDeviceIdRecords),
	  componentImageInformation(componentImageInformation)
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::Package::Package(const Package &ref)
	: firmwareDeviceIdRecords(ref.firmwareDeviceIdRecords),
	  componentImageInformation(ref.componentImageInformation)
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::Package::~Package()
{
}

LIBPLDM_ABI_TESTING
bool pldm::fw_update::Package::operator==(const Package &other) const
{
	if (CompareNEQ(&Package::firmwareDeviceIdRecords, other)) {
		return false;
	}
	if (CompareNEQ(&Package::componentImageInformation, other)) {
		return false;
	}
	return true;
}
