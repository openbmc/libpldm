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

	// users of derived classes must use 'HasMember' functions
	// before read/write member access to fields
	template <class Derived> struct GrowableStruct {
		// size of the derived class in bytes
		size_t struct_size;

		char *address() const
		{
			return (char *)this;
		}

		void Init()
		{
			struct_size = sizeof(Derived);
		}

		template <class T> bool HasMember(const T &member) const
		{
			return struct_size > (char *)&member - address();
		}

		template <class T> T Get(const T &member) const
		{
			return member;
		}

		template <class T>
		bool CompareEQ(const T &member, const Derived &other) const
		{
			if (!HasMember(member) || !other.HasMember(member)) {
				return true;
			}

			return member == other.Get(member);
		}

		template <class T>
		bool CompareNEQ(const T &member, const Derived &other) const
		{
			return !CompareEQ(member, other);
		}
	};

	// public api
	struct DescriptorData : GrowableStruct<struct DescriptorData> {
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

		bool operator==(const DescriptorData &other) const;

		std::optional<std::string> vendorDefinedDescriptorTitle;
		std::vector<uint8_t> data;
	};

	struct ComponentImageInfo : GrowableStruct<struct ComponentImageInfo> {
	    public:
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
		bool operator==(const ComponentImageInfo &other) const;

		uint16_t compClassification;
		uint16_t compIdentifier;
		uint32_t compComparisonStamp;
		std::bitset<16> compOptions;
		std::bitset<16> reqCompActivationMethod;

		// pointer to, and length of the component image
		variable_field compLocation;

		std::string compVersion;
	};

	struct FirmwareDeviceIDRecord
		: GrowableStruct<struct FirmwareDeviceIDRecord> {
	    public:
		LIBPLDM_ABI_TESTING
		FirmwareDeviceIDRecord(
			const std::bitset<32> &deviceUpdateOptionFlags,
			const std::vector<size_t> &applicableComponents,
			const std::string &componentImageSetVersion,
			const std::map<uint16_t, DescriptorData> &descriptors,
			const std::vector<uint8_t> &firmwareDevicePackageData);
		LIBPLDM_ABI_TESTING
		~FirmwareDeviceIDRecord();

		// descriptors
		LIBPLDM_ABI_TESTING
		std::vector<uint16_t> getDescriptorTypes();

		LIBPLDM_ABI_TESTING
		bool operator==(const FirmwareDeviceIDRecord &other) const;

		LIBPLDM_ABI_TESTING
		DescriptorData getDescriptor(uint16_t descriptorType);

		std::bitset<32> deviceUpdateOptionFlags;

		std::vector<size_t> applicableComponents;

		std::string componentImageSetVersion;

		// map descriptor type to descriptor data
		std::map<uint16_t, DescriptorData> descriptors;

		std::vector<uint8_t> firmwareDevicePackageData;
	};

	struct Package : GrowableStruct<struct Package> {
	    public:
		LIBPLDM_ABI_TESTING
		Package(const std::vector<FirmwareDeviceIDRecord>
				&fwDeviceIDRecords,
			const std::vector<ComponentImageInfo>
				&componentImageInfos);

		LIBPLDM_ABI_TESTING
		~Package();

		LIBPLDM_ABI_TESTING
		bool operator==(const Package &other) const;

		const std::vector<FirmwareDeviceIDRecord> fwDeviceIDRecords;
		const std::vector<ComponentImageInfo> componentImageInfos;
	};

} // namespace fw_update

} // namespace pldm
