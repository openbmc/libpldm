#pragma once

#include <bitset>
#include <cstdint>
#include <libpldm/firmware_update.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <optional>

#ifdef LIBPLDM_GTEST_FRIENDS
#include <gtest/gtest.h>
#define LIBPLDM_GTEST_ONLY LIBPLDM_ABI_TESTING
#else
#define LIBPLDM_GTEST_ONLY
#endif

namespace pldm
{

namespace fw_update
{
	// forward declare package parser for our 'friend' declarations
	class PackageParser;
	struct DescriptorData;
	struct FirmwareDeviceIDRecord;
	struct ComponentImageInfo;

#ifdef LIBPLDM_GTEST_FRIENDS
	class PackageParserTest;
#endif

	// define this base class to avoid library users having
	// copy constructors and move constructors auto-generated, which could mismatch
	// with the library ABI
	struct NonCopyableNonMoveable {
		NonCopyableNonMoveable() = default;
		NonCopyableNonMoveable(const NonCopyableNonMoveable &) = delete;
		NonCopyableNonMoveable &
		operator=(const NonCopyableNonMoveable &) = delete;
		NonCopyableNonMoveable(NonCopyableNonMoveable &&) = delete;
		NonCopyableNonMoveable &
		operator=(NonCopyableNonMoveable &&) = delete;

	    protected:
		~NonCopyableNonMoveable() = default;
	};

	// users of derived classes must use 'HasMember' functions
	// before read/write member access to fields
	template <class Derived> struct GrowableStruct {
	    protected:
		// size of the derived class in bytes
		const size_t struct_size = sizeof(Derived);

		// reserved for future use
		uint64_t reserved_1 = 0;

		char *address() const
		{
			return (char *)this;
		}

		template <class T>
		bool HasMember(const T Derived::*member) const
		{
			return struct_size >
			       (char *)&(static_cast<const Derived *>(this)
						 ->*member) -
				       address();
		}

		// unchecked access
		// checked access
		template <class T>
		bool CompareEQ(const T Derived::*member,
			       const Derived &other) const
		{
			if (!HasMember(member) || !other.HasMember(member)) {
				return true;
			}

			return *(GetPtr(member)) == *(other.GetPtr(member));
		}

		// checked access
		template <class T>
		bool CompareNEQ(const T Derived::*member,
				const Derived &other) const
		{
			if (!HasMember(member) || !other.HasMember(member)) {
				return false;
			}

			return *(GetPtr(member)) != *(other.GetPtr(member));
		}

	    private:
		// unchecked access
		template <class T> T Get(const T Derived::*member) const
		{
			return static_cast<const Derived *>(this)->*member;
		}

		// unchecked access
		template <class T>
		const T *GetPtr(const T Derived::*member) const
		{
			return &((static_cast<const Derived *>(this))->*member);
		}
	};

	// public api
	struct DescriptorData : GrowableStruct<struct DescriptorData>,
				private NonCopyableNonMoveable {
	    private:
		friend pldm::fw_update::PackageParser;

		// since it is holding a map of descriptors, it needs to construct
		// for the private copy constructor
		friend struct pldm::fw_update::FirmwareDeviceIDRecord;

#ifdef LIBPLDM_GTEST_FRIENDS
		friend class pldm::fw_update::PackageParserTest;
		FRIEND_TEST(PackageParserTest,
			    ValidPkgSingleDescriptorSingleComponent);
		FRIEND_TEST(PackageParserTest,
			    ValidPkgMultipleDescriptorsMultipleComponents);
		FRIEND_TEST(PackageParserTest, InvalidPkgBadChecksum);
#endif

		LIBPLDM_GTEST_ONLY
		DescriptorData(const struct DescriptorData &ref);
		LIBPLDM_GTEST_ONLY
		DescriptorData(const std::vector<uint8_t> &data);
		LIBPLDM_GTEST_ONLY
		DescriptorData(const std::string &title,
			       const std::vector<uint8_t> &data);

	    public:
		LIBPLDM_ABI_TESTING
		~DescriptorData();

		bool operator==(const DescriptorData &other) const;

		// data members
		const std::optional<std::string> vendorDefinedDescriptorTitle;
		const std::vector<uint8_t> data;
	};

	struct ComponentImageInfo : GrowableStruct<struct ComponentImageInfo>,
				    private NonCopyableNonMoveable {
	    private:
		friend pldm::fw_update::PackageParser;

#ifdef LIBPLDM_GTEST_FRIENDS
		friend class pldm::fw_update::PackageParserTest;
		FRIEND_TEST(PackageParserTest,
			    ValidPkgSingleDescriptorSingleComponent);
		FRIEND_TEST(PackageParserTest,
			    ValidPkgMultipleDescriptorsMultipleComponents);
		FRIEND_TEST(PackageParserTest, InvalidPkgBadChecksum);
#endif

		LIBPLDM_GTEST_ONLY
		ComponentImageInfo(uint16_t compClassification,
				   uint16_t compIdentifier,
				   uint32_t compComparisonStamp,
				   std::bitset<16> compOptions,
				   std::bitset<16> reqCompActivationMethod,
				   const variable_field &compLocation,
				   const std::string &compVersion);

	    public:
		LIBPLDM_ABI_TESTING
		ComponentImageInfo(const ComponentImageInfo &ref);

		LIBPLDM_ABI_TESTING
		~ComponentImageInfo();

		LIBPLDM_ABI_TESTING
		bool operator==(const ComponentImageInfo &other) const;

		// data members
		const uint16_t compClassification;
		const uint16_t compIdentifier;
		const uint32_t compComparisonStamp;
		const std::bitset<16> compOptions;
		const std::bitset<16> reqCompActivationMethod;

		// pointer to, and length of the component image
		const variable_field compLocation;

		const std::string compVersion;
	};

	struct FirmwareDeviceIDRecord
		: GrowableStruct<struct FirmwareDeviceIDRecord>,
		  private NonCopyableNonMoveable {
	    private:
		friend pldm::fw_update::PackageParser;

#ifdef LIBPLDM_GTEST_FRIENDS
		friend class pldm::fw_update::PackageParserTest;
		FRIEND_TEST(PackageParserTest,
			    ValidPkgSingleDescriptorSingleComponent);
		FRIEND_TEST(PackageParserTest,
			    ValidPkgMultipleDescriptorsMultipleComponents);
		FRIEND_TEST(PackageParserTest, InvalidPkgBadChecksum);
#endif

		LIBPLDM_GTEST_ONLY
		FirmwareDeviceIDRecord(
			const std::bitset<32> &deviceUpdateOptionFlags,
			const std::vector<size_t> &applicableComponents,
			const std::string &componentImageSetVersion,
			const std::map<uint16_t, std::unique_ptr<DescriptorData> >
				&descriptors,
			const std::vector<uint8_t> &firmwareDevicePackageData);

	    public:
		LIBPLDM_ABI_TESTING
		~FirmwareDeviceIDRecord();

		LIBPLDM_ABI_TESTING
		FirmwareDeviceIDRecord(const FirmwareDeviceIDRecord &ref);

		LIBPLDM_ABI_TESTING
		bool operator==(const FirmwareDeviceIDRecord &other) const;

		// data members
		LIBPLDM_ABI_TESTING
		const std::vector<uint16_t> getDescriptorTypes() const;

		const std::bitset<32> deviceUpdateOptionFlags;

		const std::vector<size_t> applicableComponents;

		const std::string componentImageSetVersion;

		// map descriptor type to descriptor data
		//
		// We cannot have a value-map since 'DescriptorData' is a growable struct
		// which cannot be (move) constructed by the library user or
		// anyone who is not a friend (including STL templates like construct_at).
		// To avoid any mismatch in usage due to layout difference of a value map
		// on struct growth, we store a unique_ptr.
		const std::map<uint16_t, std::unique_ptr<DescriptorData> >
			descriptors;

		const std::vector<uint8_t> firmwareDevicePackageData;
	};

	struct Package : GrowableStruct<struct Package>,
			 private NonCopyableNonMoveable {
	    private:
		friend pldm::fw_update::PackageParser;

#ifdef LIBPLDM_GTEST_FRIENDS
		friend class pldm::fw_update::PackageParserTest;
		FRIEND_TEST(PackageParserTest,
			    ValidPkgSingleDescriptorSingleComponent);
		FRIEND_TEST(PackageParserTest,
			    ValidPkgMultipleDescriptorsMultipleComponents);
		FRIEND_TEST(PackageParserTest, InvalidPkgBadChecksum);
#endif

		LIBPLDM_GTEST_ONLY
		Package(const std::vector<FirmwareDeviceIDRecord>
				&fwDeviceIDRecords,
			const std::vector<ComponentImageInfo>
				&componentImageInfos);

	    public:
		LIBPLDM_ABI_TESTING
		~Package();

		LIBPLDM_ABI_TESTING
		Package(const Package &ref);

		LIBPLDM_ABI_TESTING
		bool operator==(const Package &other) const;

		// data members
		const std::vector<FirmwareDeviceIDRecord> fwDeviceIDRecords;
		const std::vector<ComponentImageInfo> componentImageInfos;
	};

} // namespace fw_update

} // namespace pldm
