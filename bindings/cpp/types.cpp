#include "types_private.hpp"

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

// impl for private types
//
bool pldm::fw_update::ComponentImageInfoPrivate::operator==(
	const ComponentImageInfoPrivate &other) const
{
	return compClassification == other.compClassification &&
	       compIdentifier == other.compIdentifier &&
	       compComparisonStamp == other.compComparisonStamp &&
	       compOptions == other.compOptions &&
	       reqCompActivationMethod == other.reqCompActivationMethod &&
	       compLocation.ptr == other.compLocation.ptr &&
	       compLocation.length == other.compLocation.length &&
	       compVersion == other.compVersion;
}

// impl for struct DescriptorData

LIBPLDM_ABI_TESTING
pldm::fw_update::DescriptorData::DescriptorData(const struct DescriptorData &ref)
	: d(std::make_unique<DescriptorDataPrivate>(*ref.d))
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::DescriptorData::DescriptorData(const std::vector<uint8_t> &data)
{
	d = std::make_unique<DescriptorDataPrivate>(std::nullopt, data);
}

LIBPLDM_ABI_TESTING
pldm::fw_update::DescriptorData::DescriptorData(
	const std::string &title, const std::vector<uint8_t> &data)
{
	d = std::make_unique<DescriptorDataPrivate>(title, data);
}

LIBPLDM_ABI_TESTING
pldm::fw_update::DescriptorData::~DescriptorData()
{
}

LIBPLDM_ABI_TESTING
bool pldm::fw_update::DescriptorData::operator==(const DescriptorData &other)
{
	return *d == *other.d;
}

LIBPLDM_ABI_TESTING
std::optional<std::string>
pldm::fw_update::DescriptorData::getVendorDefinedDescriptorTitle()
{
	return d->vendorDefinedDescriptorTitle;
}

LIBPLDM_ABI_TESTING
std::vector<uint8_t> pldm::fw_update::DescriptorData::getData()
{
	return d->data;
}

// impl for struct ComponentInfo

LIBPLDM_ABI_TESTING
pldm::fw_update::ComponentImageInfo::ComponentImageInfo(
	const struct ComponentImageInfo &ref)
	: d(std::make_unique<ComponentImageInfoPrivate>(*ref.d))
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::ComponentImageInfo::ComponentImageInfo(
	uint16_t compClassification, uint16_t compIdentifier,
	uint32_t compComparisonStamp, std::bitset<16> compOptions,
	std::bitset<16> reqCompActivationMethod,
	const variable_field &compLocation, const std::string &compVersion)
{
	d = std::make_unique<ComponentImageInfoPrivate>(
		compClassification, compIdentifier, compComparisonStamp,
		compOptions, reqCompActivationMethod, compLocation,
		compVersion);
}

LIBPLDM_ABI_TESTING
pldm::fw_update::ComponentImageInfo::~ComponentImageInfo()
{
}

LIBPLDM_ABI_TESTING
bool pldm::fw_update::ComponentImageInfo::operator==(
	const ComponentImageInfo &other)
{
	return *d == *other.d;
}

LIBPLDM_ABI_TESTING
uint16_t pldm::fw_update::ComponentImageInfo::getCompClassification()
{
	return d->compClassification;
}

LIBPLDM_ABI_TESTING
uint16_t pldm::fw_update::ComponentImageInfo::getCompIdentifier()
{
	return d->compIdentifier;
}

LIBPLDM_ABI_TESTING
uint32_t pldm::fw_update::ComponentImageInfo::getCompComparisonStamp()
{
	return d->compComparisonStamp;
}

LIBPLDM_ABI_TESTING
std::bitset<16> pldm::fw_update::ComponentImageInfo::getCompOptions()
{
	return d->compOptions;
}

LIBPLDM_ABI_TESTING
std::bitset<16>
pldm::fw_update::ComponentImageInfo::getReqCompActivationMethod()
{
	return d->reqCompActivationMethod;
}

LIBPLDM_ABI_TESTING
variable_field pldm::fw_update::ComponentImageInfo::getCompLocation()
{
	return d->compLocation;
}

LIBPLDM_ABI_TESTING
std::string pldm::fw_update::ComponentImageInfo::getCompVersion()
{
	return d->compVersion;
}

// impl for struct FirmwareDeviceIDRecord

LIBPLDM_ABI_TESTING
pldm::fw_update::FirmwareDeviceIDRecord::FirmwareDeviceIDRecord(
	const struct FirmwareDeviceIDRecord &ref)
	: d(std::make_unique<FirmwareDeviceIDRecordPrivate>(*ref.d))
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::FirmwareDeviceIDRecord::FirmwareDeviceIDRecord(
	const std::bitset<32> &deviceUpdateOptionFlags,
	const std::vector<size_t> &applicableComponents,
	const std::string &componentImageSetVersion,
	const std::map<uint16_t, DescriptorData> &descriptors,
	const std::vector<uint8_t> &firmwareDevicePackageData)
{
	d = std::make_unique<FirmwareDeviceIDRecordPrivate>(
		deviceUpdateOptionFlags, applicableComponents,
		componentImageSetVersion, descriptors,
		firmwareDevicePackageData);
}

LIBPLDM_ABI_TESTING
pldm::fw_update::FirmwareDeviceIDRecord::~FirmwareDeviceIDRecord()
{
}

LIBPLDM_ABI_TESTING
bool pldm::fw_update::FirmwareDeviceIDRecord::operator==(
	const FirmwareDeviceIDRecord &other)
{
	return d == other.d;
}

LIBPLDM_ABI_TESTING
std::bitset<32>
pldm::fw_update::FirmwareDeviceIDRecord::getDeviceUpdateOptionFlags()
{
	return d->deviceUpdateOptionFlags;
}

LIBPLDM_ABI_TESTING
std::vector<size_t>
pldm::fw_update::FirmwareDeviceIDRecord::getApplicableComponents()
{
	return d->applicableComponents;
}

LIBPLDM_ABI_TESTING
std::string
pldm::fw_update::FirmwareDeviceIDRecord::getComponentImageSetVersion()
{
	return d->componentImageSetVersion;
}

// descriptors
LIBPLDM_ABI_TESTING
std::vector<uint16_t>
pldm::fw_update::FirmwareDeviceIDRecord::getDescriptorTypes()
{
	std::vector<uint16_t> res;
	for (const auto &key : std::views::keys(d->descriptors)) {
		res.emplace_back(key);
	}
	return res;
}

LIBPLDM_ABI_TESTING
pldm::fw_update::DescriptorData
pldm::fw_update::FirmwareDeviceIDRecord::getDescriptor(uint16_t descriptorType)
{
	return d->descriptors.at(descriptorType);
}

LIBPLDM_ABI_TESTING
std::vector<uint8_t>
pldm::fw_update::FirmwareDeviceIDRecord::getFirmwareDevicePackageData()
{
	return d->firmwareDevicePackageData;
}

// impl for struct Package

LIBPLDM_ABI_TESTING
pldm::fw_update::Package::Package(
	const std::vector<FirmwareDeviceIDRecord> &fwDeviceIDRecords,
	const std::vector<ComponentImageInfo> &componentImageInfos)
	: d(std::make_unique<PackagePrivate>(fwDeviceIDRecords,
					     componentImageInfos))
{
}

LIBPLDM_ABI_TESTING
pldm::fw_update::Package::~Package()
{
}

LIBPLDM_ABI_TESTING
std::vector<pldm::fw_update::FirmwareDeviceIDRecord>
pldm::fw_update::Package::getFwDeviceIDRecords()
{
	return d->fwDeviceIDRecords;
}

LIBPLDM_ABI_TESTING
std::vector<pldm::fw_update::ComponentImageInfo>
pldm::fw_update::Package::getComponentImageInfos()
{
	return d->componentImageInfos;
}
