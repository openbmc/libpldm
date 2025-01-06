#pragma once

#include <bitset>
#include <cstdint>
#include <libpldm/bindings/cpp/types.hpp>
#include <libpldm/firmware_update.h>
#include <map>
#include <string>
#include <vector>
#include <optional>

namespace pldm
{

namespace fw_update
{
	struct DescriptorDataPrivate {
		std::optional<std::string> vendorDefinedDescriptorTitle;
		std::vector<uint8_t> data;

		bool
		operator==(const DescriptorDataPrivate &other) const = default;
	};

	struct ComponentImageInfoPrivate {
		uint16_t compClassification;
		uint16_t compIdentifier;
		uint32_t compComparisonStamp;
		std::bitset<16> compOptions;
		std::bitset<16> reqCompActivationMethod;

		// pointer to, and length of the component image
		variable_field compLocation;

		std::string compVersion;

		// cannot use default impl. due to reference member
		bool operator==(const ComponentImageInfoPrivate &other) const;
	};

	struct FirmwareDeviceIDRecordPrivate {
		std::bitset<32> deviceUpdateOptionFlags;

		// We can get the reference to an applicable component on-demand
		// via a method. So we just store indices here.
		std::vector<size_t> applicableComponents;

		std::string componentImageSetVersion;

		// map descriptor type to descriptor data
		std::map<uint16_t, DescriptorData> descriptors;

		std::vector<uint8_t> firmwareDevicePackageData;

		bool operator==(const FirmwareDeviceIDRecordPrivate &other)
			const = default;
	};

	struct PackagePrivate {
		const std::vector<FirmwareDeviceIDRecord> fwDeviceIDRecords;
		const std::vector<ComponentImageInfo> componentImageInfos;
	};

} // namespace fw_update

} // namespace pldm
