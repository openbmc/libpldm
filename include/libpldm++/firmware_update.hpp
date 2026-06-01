#pragma once

#include <libpldm++/types.hpp>

#include <libpldm/firmware_update.h>

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <expected>
#include <span>
#include <memory>

// namespace pldm is for things which are (in)directly defined by PLDM specifications
namespace pldm
{

namespace fw_update
{
	// similar in concept to libpldm: 'struct pldm_package_format_pin',
	// this enum allows versioning of our 'struct Package'.
	//
	// The caller passes whichever version they want to have and will receive a
	// package containing at least those members which their requested version supports,
	// or an error, in case the linked libpldm++ can only support a lower version.
	// The caller cannot safely access any members beyond what they requested.
	enum class PackagePin {
		v1,
		v1_1_0,
		v1_3_0,
	};

	// forward declare structs and classes for our 'friend' declarations
	class PackageParser;
	struct DescriptorData;
	struct FirmwareDeviceIDRecord;
	struct DownstreamDeviceIDRecord;
	struct ComponentImageInfo;

	struct DescriptorData : libpldm::GrowableStruct<struct DescriptorData>,
				private libpldm::NonCopyableNonMoveable {
	    private:
		friend pldm::fw_update::PackageParser;

		// since it is holding a map of descriptors, it needs to construct
		// for the private copy constructor
		friend struct pldm::fw_update::FirmwareDeviceIDRecord;
		friend struct pldm::fw_update::DownstreamDeviceIDRecord;

		DescriptorData(const struct DescriptorData &ref);
		DescriptorData(const std::vector<uint8_t> &data);
		DescriptorData(const std::string &title,
			       const std::vector<uint8_t> &data);

	    public:
		~DescriptorData();

		bool operator==(const DescriptorData &other) const;

		// data members
		// introduced in PackagePin::v1
		const std::optional<std::string> vendorDefinedDescriptorTitle;
		// introduced in PackagePin::v1
		const std::vector<uint8_t> data;
	};

	struct ReferenceManifestData
		: libpldm::GrowableStruct<struct ReferenceManifestData>,
		  private libpldm::NonCopyableNonMoveable {
	    private:
		friend pldm::fw_update::PackageParser;

		ReferenceManifestData(uint8_t SVHID,
				      const std::vector<uint8_t> &vendorID,
				      const std::vector<uint8_t> &data);

	    public:
		~ReferenceManifestData();

		ReferenceManifestData(const ReferenceManifestData &ref);

		bool operator==(const ReferenceManifestData &other) const;

		// data members
		// introduced in PackagePin::v1_3_0
		const uint8_t SVHID;

		// introduced in PackagePin::v1_3_0
		const std::vector<uint8_t> vendorID;

		// introduced in PackagePin::v1_3_0
		const std::vector<uint8_t> data;
	};

	struct ComponentImageInfo
		: libpldm::GrowableStruct<struct ComponentImageInfo>,
		  private libpldm::NonCopyableNonMoveable {
	    private:
		friend pldm::fw_update::PackageParser;

		ComponentImageInfo(
			uint16_t componentClassification,
			uint16_t componentIdentifier,
			uint32_t componentComparisonStamp,
			std::bitset<16> componentOptions,
			std::bitset<16> requestedComponentActivationMethod,
			const variable_field &componentLocation,
			const std::string &componentVersion,
			const std::vector<uint8_t> &componentOpaqueData);

	    public:
		ComponentImageInfo(const ComponentImageInfo &ref);

		~ComponentImageInfo();

		// note: this function compares all members besides the
		// component image itself
		bool operator==(const ComponentImageInfo &other) const;

		// data members
		// introduced in PackagePin::v1
		const uint16_t componentClassification;
		// introduced in PackagePin::v1
		const uint16_t componentIdentifier;
		// introduced in PackagePin::v1
		const uint32_t compComparisonStamp;
		// introduced in PackagePin::v1
		const std::bitset<16> componentOptions;
		// introduced in PackagePin::v1
		const std::bitset<16> requestedComponentActivationMethod;

		// pointer to, and length of the component image.
		// The pointer becomes dangling when the
		// lifetime of the parsed buffer ends.
		// introduced in PackagePin::v1
		const variable_field componentLocation;

		// introduced in PackagePin::v1
		const std::string componentVersion;

		// introduced in PackagePin::v1_3_0
		const std::vector<uint8_t> componentOpaqueData;
	};

	struct FirmwareDeviceIDRecord
		: libpldm::GrowableStruct<struct FirmwareDeviceIDRecord>,
		  private libpldm::NonCopyableNonMoveable {
	    private:
		friend pldm::fw_update::PackageParser;

		FirmwareDeviceIDRecord(
			const std::bitset<32> &deviceUpdateOptionFlags,
			const std::vector<size_t> &applicableComponents,
			const std::string &componentImageSetVersionString,
			const std::map<uint16_t, std::unique_ptr<DescriptorData> >
				&descriptors,
			const std::vector<uint8_t> &firmwareDevicePackageData,
			const std::optional<ReferenceManifestData>
				&referenceManifestData);

	    public:
		~FirmwareDeviceIDRecord();

		FirmwareDeviceIDRecord(const FirmwareDeviceIDRecord &ref);

		bool operator==(const FirmwareDeviceIDRecord &other) const;

		// data members
		// introduced in PackagePin::v1
		const std::vector<uint16_t> getDescriptorTypes() const;

		// introduced in PackagePin::v1
		const std::bitset<32> deviceUpdateOptionFlags;

		// introduced in PackagePin::v1
		const std::vector<size_t> applicableComponents;

		// introduced in PackagePin::v1
		const std::string componentImageSetVersionString;

		// map descriptor type to descriptor data
		//
		// We cannot have a value-map since 'DescriptorData' is a growable struct
		// which cannot be (move) constructed by the library user or
		// anyone who is not a friend (including STL templates like construct_at).
		// To avoid any mismatch in usage due to layout difference of a value map
		// on struct growth, we store a unique_ptr.
		// introduced in PackagePin::v1
		const std::map<uint16_t, std::unique_ptr<DescriptorData> >
			recordDescriptors;

		// introduced in PackagePin::v1
		const std::vector<uint8_t> firmwareDevicePackageData;

		// introduced in PackagePin::v1_3_0
		const std::optional<ReferenceManifestData> referenceManifestData;
	};

	struct DownstreamDeviceIDRecord
		: libpldm::GrowableStruct<struct DownstreamDeviceIDRecord>,
		  private libpldm::NonCopyableNonMoveable {
	    private:
		friend pldm::fw_update::PackageParser;

		DownstreamDeviceIDRecord(
			const std::bitset<32> &downstreamDeviceUpdateOptionFlags,
			const std::optional<std::string> &
				downstreamDeviceSelfContainedActivationMinVersionString,
			const std::optional<uint32_t> &
				downstreamDeviceSelfContainedActivationMinVersionComparisonStamp,
			const std::vector<size_t> &applicableComponents,
			const std::map<uint16_t, std::unique_ptr<DescriptorData> >
				&recordDescriptors,

			const std::vector<uint8_t> &downstreamDevicePackageData,
			const std::optional<ReferenceManifestData>
				&downstreamDeviceReferenceManifestData);

	    public:
		~DownstreamDeviceIDRecord();

		DownstreamDeviceIDRecord(const DownstreamDeviceIDRecord &ref);

		bool operator==(const DownstreamDeviceIDRecord &other) const;

		// data members
		// introduced in PackagePin::v1_1_0
		const std::bitset<32> downstreamDeviceUpdateOptionFlags;

		// introduced in PackagePin::v1_1_0
		const std::optional<std::string>
			downstreamDeviceSelfContainedActivationMinVersionString;
		// introduced in PackagePin::v1_1_0
		const std::optional<uint32_t>
			downstreamDeviceSelfContainedActivationMinVersionComparisonStamp;
		// introduced in PackagePin::v1_1_0
		const std::vector<size_t> applicableComponents;

		// introduced in PackagePin::v1_1_0
		const std::map<uint16_t, std::unique_ptr<DescriptorData> >
			recordDescriptors;

		// introduced in PackagePin::v1_1_0
		const std::vector<uint8_t> downstreamDevicePackageData;

		// introduced in PackagePin::v1_3_0
		const std::optional<ReferenceManifestData>
			downstreamDeviceReferenceManifestData;
	};

	struct Package : libpldm::GrowableStruct<struct Package>,
			 private libpldm::NonCopyableNonMoveable {
	    private:
		friend pldm::fw_update::PackageParser;

		Package(const std::vector<FirmwareDeviceIDRecord>
				&firmwareDeviceIdRecords,
			const std::vector<DownstreamDeviceIDRecord>
				&downstreamDeviceIdRecords,
			const std::vector<ComponentImageInfo>
				&componentImageInformation);

	    public:
		~Package();

		Package(const Package &ref);

		bool operator==(const Package &other) const;

		// data members
		// introduced in PackagePin::v1
		const std::vector<FirmwareDeviceIDRecord>
			firmwareDeviceIdRecords;
		// introduced in PackagePin::v1
		const std::vector<ComponentImageInfo> componentImageInformation;
		// introduced in PackagePin::v1_1_0
		const std::vector<DownstreamDeviceIDRecord>
			downstreamDeviceIdRecords;
	};

	class PackageParserError {
	    public:
		explicit PackageParserError(std::string s);
		PackageParserError(std::string s, int rc);

		std::string msg;

		// error codes from libpldm
		std::optional<int> rc;
	};

	class PackageParser : private libpldm::NonCopyableNonMoveable {
	    public:
		PackageParser() = delete;
		~PackageParser();

		/** @brief Parse the firmware update package
		 *
		 *  @param[in] pkg - vector with the pldm fw update package
		 *  @param[in] pin - package struct version support
		 *
		 *  @returns an error value if parsing fails
		 *  @returns a unique_ptr to Package struct on success
		 */
		static std::expected<std::unique_ptr<Package>,
				     PackageParserError>
		parse(const std::span<const uint8_t> &pkg,
		      PackagePin pin) noexcept;

	    private:
		static std::expected<void, std::string> helperParseFDDescriptor(
			struct pldm_descriptor *desc,
			std::map<uint16_t,
				 std::unique_ptr<pldm::fw_update::DescriptorData> >
				&descriptors) noexcept;

		static std::expected<void, pldm::fw_update::PackageParserError>
		helperParseDownstreamDeviceIDRecord(
			std::vector<DownstreamDeviceIDRecord>
				&downstreamDeviceIdRecords,
			struct pldm_package &package,
			pldm_package_downstream_device_id_record
				&downstreamDeviceId) noexcept;

		static std::optional<pldm::fw_update::ReferenceManifestData>
		getReferenceManifestData(
			struct variable_field &reference_manifest_data) noexcept;
	};

} // namespace fw_update

} // namespace pldm
