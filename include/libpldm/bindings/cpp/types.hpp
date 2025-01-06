#pragma once

#include <bitset>
#include <cstdint>
#include <libpldm/firmware_update.h>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <optional>

namespace pldm
{

using Availability = bool;
using eid = uint8_t;
using UUID = std::string;

namespace fw_update
{
	struct DescriptorData {
		std::optional<std::string> vendorDefinedDescriptorTitle;
		std::vector<uint8_t> data;

		bool operator==(const DescriptorData &other) const = default;
	};

	struct FirmwareDeviceIDRecord {
		std::bitset<32> deviceUpdateOptionFlags;
		std::vector<size_t> applicableComponents;
		std::string componentImageSetVersion;

		// map descriptor type to descriptor data
		std::map<uint16_t, DescriptorData> descriptors;

		std::vector<uint8_t> firmwareDevicePackageData;

		bool
		operator==(const FirmwareDeviceIDRecord &other) const = default;
	};

	struct ComponentImageInfo {
		uint16_t compClassification;
		uint16_t compIdentifier;
		uint32_t compComparisonStamp;
		std::bitset<16> compOptions;
		std::bitset<16> reqCompActivationMethod;
		uint32_t compLocationOffset;
		uint32_t compSize;
		std::string compVersion;

		bool
		operator==(const ComponentImageInfo &other) const = default;
	};

} // namespace fw_update

} // namespace pldm
