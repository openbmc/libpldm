#pragma once

#include <bitset>
#include <cstdint>
#include <libpldm/firmware_update.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace pldm
{

namespace fw_update
{
	// forward declare private impl
	struct PackagePrivate;
	struct FirmwareDeviceIDRecordPrivate;
	struct ComponentImageInfoPrivate;

	// public api
	struct DescriptorData {
	    public:
		LIBPLDM_ABI_TESTING
		DescriptorData(const struct DescriptorData &ref);
		LIBPLDM_ABI_TESTING
		DescriptorData(const std::vector<uint8_t> &data);
		LIBPLDM_ABI_TESTING
		DescriptorData(const std::string &title,
			       const std::vector<uint8_t> &data);
		LIBPLDM_ABI_TESTING
		~DescriptorData();
		LIBPLDM_ABI_TESTING
		bool operator==(const DescriptorData &other);

		LIBPLDM_ABI_TESTING
		std::optional<std::string> getVendorDefinedDescriptorTitle();
		LIBPLDM_ABI_TESTING
		std::vector<uint8_t> getData();

	    private:
		std::unique_ptr<struct DescriptorDataPrivate> d;
	};

	struct ComponentImageInfo {
	    public:
		LIBPLDM_ABI_TESTING
		ComponentImageInfo(const struct ComponentImageInfo &ref);
		LIBPLDM_ABI_TESTING
		ComponentImageInfo(uint16_t compClassification,
				   uint16_t compIdentifier,
				   uint32_t compComparisonStamp,
				   std::bitset<16> compOptions,
				   std::bitset<16> reqCompActivationMethod,
				   const variable_field &compLocation,
				   const std::string &compVersion);
		LIBPLDM_ABI_TESTING
		~ComponentImageInfo();
		LIBPLDM_ABI_TESTING
		bool operator==(const ComponentImageInfo &other);

		LIBPLDM_ABI_TESTING
		uint16_t getCompClassification();
		LIBPLDM_ABI_TESTING
		uint16_t getCompIdentifier();
		LIBPLDM_ABI_TESTING
		uint32_t getCompComparisonStamp();
		LIBPLDM_ABI_TESTING
		std::bitset<16> getCompOptions();
		LIBPLDM_ABI_TESTING
		std::bitset<16> getReqCompActivationMethod();
		LIBPLDM_ABI_TESTING
		variable_field getCompLocation();
		LIBPLDM_ABI_TESTING
		std::string getCompVersion();

	    private:
		std::unique_ptr<struct ComponentImageInfoPrivate> d;
	};

	struct FirmwareDeviceIDRecord {
	    public:
		LIBPLDM_ABI_TESTING
		FirmwareDeviceIDRecord(const struct FirmwareDeviceIDRecord &ref);
		LIBPLDM_ABI_TESTING
		FirmwareDeviceIDRecord(
			const std::bitset<32> &deviceUpdateOptionFlags,
			const std::vector<size_t> &applicableComponents,
			const std::string &componentImageSetVersion,
			const std::map<uint16_t, DescriptorData> &descriptors,
			const std::vector<uint8_t> &firmwareDevicePackageData);
		LIBPLDM_ABI_TESTING
		~FirmwareDeviceIDRecord();
		LIBPLDM_ABI_TESTING
		bool operator==(const FirmwareDeviceIDRecord &other);

		LIBPLDM_ABI_TESTING
		std::bitset<32> getDeviceUpdateOptionFlags();

		LIBPLDM_ABI_TESTING
		std::vector<size_t> getApplicableComponents();

		LIBPLDM_ABI_TESTING
		std::string getComponentImageSetVersion();

		// descriptors
		LIBPLDM_ABI_TESTING
		std::vector<uint16_t> getDescriptorTypes();
		LIBPLDM_ABI_TESTING
		DescriptorData getDescriptor(uint16_t descriptorType);

		LIBPLDM_ABI_TESTING
		std::vector<uint8_t> getFirmwareDevicePackageData();

	    private:
		std::unique_ptr<struct FirmwareDeviceIDRecordPrivate> d;
	};

	struct Package {
	    public:
		LIBPLDM_ABI_TESTING
		Package(const std::vector<FirmwareDeviceIDRecord>
				&fwDeviceIDRecords,
			const std::vector<ComponentImageInfo>
				&componentImageInfos);
		LIBPLDM_ABI_TESTING
		std::vector<FirmwareDeviceIDRecord> getFwDeviceIDRecords();
		LIBPLDM_ABI_TESTING
		std::vector<ComponentImageInfo> getComponentImageInfos();
		LIBPLDM_ABI_TESTING
		~Package();

	    private:
		std::unique_ptr<struct PackagePrivate> d;
	};

} // namespace fw_update

} // namespace pldm
