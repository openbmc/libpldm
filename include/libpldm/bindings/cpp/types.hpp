#pragma once

#include <bitset>
#include <cstdint>
#include <libpldm/firmware_update.h>
#include <map>
#include <string>
#include <vector>
#include <optional>

namespace pldm
{

namespace fw_update
{
	struct DescriptorData {
		std::optional<std::string> vendorDefinedDescriptorTitle;
		std::vector<uint8_t> data;

		bool operator==(const DescriptorData &other) const = default;
	};

	struct ComponentImageInfo;

	struct FirmwareDeviceIDRecord {
		std::bitset<32> deviceUpdateOptionFlags;
		std::vector<std::reference_wrapper<ComponentImageInfo> >
			applicableComponents;
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

		// pointer to, and length of the component image
		variable_field compLocation;

		std::string compVersion;
	};

} // namespace fw_update

} // namespace pldm
