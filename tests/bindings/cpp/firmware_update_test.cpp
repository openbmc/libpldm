#include "libpldm/bindings/cpp/firmware_update.hpp"
#include "libpldm/bindings/cpp/types.hpp"

#include <expected>
#include <typeinfo>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace pldm::fw_update;

// friend tests must be in the same namespace as the class they are testing
// https://google.github.io/googletest/reference/testing.html#FRIEND_TEST
namespace pldm
{
namespace fw_update
{

#ifdef LIBPLDM_API_TESTING

class PackageParserTest : public testing::Test
{
    // need this empty class so we can be friends with our implementation
};
#endif

#ifdef LIBPLDM_API_TESTING

const std::vector<uint8_t> fwPkgHdrSingleComponent{
    // clang-format off
    0xF0, 0x18, 0x87, 0x8C, 0xCB, 0x7D, 0x49, 0x43,
    0x98, 0x00, 0xA0, 0x2F, 0x05, 0x9A, 0xCA, 0x02, // UUID

    0x01,       // pkg header format revision
    0x8b, 0x00, // pkg header size

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x19, 0x0C, 0xE5, 0x07,
    0x00,       // pkg release date time (13 bytes, timestamp104)

    0x08, 0x00, // component bitmap bit length

    0x01,       // package version string type

    0x0E,       // package version string length

    0x56, 0x65, 0x72, 0x73, 0x69, 0x6F,
    0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
    0x67, 0x31, // package version string

    0x01,       // device id record count

    0x2E, 0x00, // record 0: record length

    0x01,                   // record 0: descriptor count
    0x01, 0x00, 0x00, 0x00, // record 0: device update options flags

    0x01,       // record 0: component image set version string type
    0x0E,       // record 0: component image set version string length
    0x00, 0x00, // record 0: firmware device package data length

    0x01,       // applicable components

    0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E,
    0x53, 0x74, 0x72, 0x69, 0x6E, 0x67,
    0x32,       // component image set version string (14 bytes)

    // record descriptors below
    0x02, 0x00, // record 0: descriptor type: UUID

    0x10, 0x00, // record 0: InitialDescriptorLength (16 bytes)

    0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41,
    0x15, 0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49,
    0xD6, 0x75, // record 0: InitialDescriptorData (UUID)

    // firmware device package data (empty here)

    // component image info area
    0x01, 0x00,             // component image count

    0x0A, 0x00,             // component classification
    0x64, 0x00,             // component identifier
    0xFF, 0xFF, 0xFF, 0xFF, // component comparison stamp
    0x00, 0x00,             // component options

    0x00, 0x00,             // requested component activation method
    0x8B, 0x00, 0x00, 0x00, // component location offset
    0x01, 0x00, 0x00, 0x00, // component size
    0x01,                   // component version string type
    0x0E,                   // component version string length

    0x56, 0x65, 0x72, 0x73, 0x69, 0x6F,
    0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
    0x67, 0x33,             // component version string

    0x54, 0x9d, 0x7d, 0xe1, // package header checksum

    0x00,                   // component image
    // clang-format on
};

TEST(PackageParserTest, ValidPkgSingleDescriptorSingleComponent)
{

    uintmax_t pkgSize = fwPkgHdrSingleComponent.size();

    std::cout << pkgSize << std::endl;

    DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR01H(pin);
    auto res = PackageParser::parse(fwPkgHdrSingleComponent, pin);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;
    }

    ASSERT_TRUE(res.has_value());

    std::vector<ComponentImageInfo> compImageInfos{
        {10, 100, 0xFFFFFFFF, 0, 0, variable_field{(uint8_t*)139, 1},
         "VersionString3"}};

    auto outfwDeviceIDRecords = res.value()->fwDeviceIDRecords;

    std::unique_ptr<DescriptorData> dd1 =
        std::unique_ptr<DescriptorData>(new DescriptorData{std::vector<uint8_t>{
            0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15, 0x95, 0xF4, 0x48,
            0x70, 0x1D, 0x49, 0xD6, 0x75}});

    std::map<uint16_t, std::unique_ptr<DescriptorData>> map1;
    map1.emplace(PLDM_FWUP_UUID, std::move(dd1));

    std::vector<FirmwareDeviceIDRecord> fwDeviceIDRecords{
        FirmwareDeviceIDRecord(std::bitset<32>(1), {0}, "VersionString2",
                               std::move(map1), {}),
    };

    ASSERT_EQ(outfwDeviceIDRecords.size(), fwDeviceIDRecords.size());
    ASSERT_EQ(outfwDeviceIDRecords.size(), 1);

    // we cannot compare the applicable components here since their memory
    // address will be different
    EXPECT_EQ(outfwDeviceIDRecords[0].deviceUpdateOptionFlags,
              fwDeviceIDRecords[0].deviceUpdateOptionFlags);
    EXPECT_EQ(outfwDeviceIDRecords[0].componentImageSetVersion,
              fwDeviceIDRecords[0].componentImageSetVersion);

    EXPECT_EQ(outfwDeviceIDRecords[0].getDescriptorTypes(),
              fwDeviceIDRecords[0].getDescriptorTypes());

    for (uint16_t type : fwDeviceIDRecords[0].getDescriptorTypes())
    {
        const auto& d1 = outfwDeviceIDRecords[0].descriptors.at(type);
        const auto& d2 = fwDeviceIDRecords[0].descriptors.at(type);
        EXPECT_TRUE(*d1 == *d2);
    }

    EXPECT_EQ(outfwDeviceIDRecords[0].firmwareDevicePackageData,
              fwDeviceIDRecords[0].firmwareDevicePackageData);

    auto outCompImageInfos = res.value()->componentImageInfos;

    ASSERT_EQ(outCompImageInfos.size(), compImageInfos.size());
    ASSERT_EQ(outCompImageInfos.size(), 1);

    EXPECT_EQ(outCompImageInfos[0].compClassification,
              compImageInfos[0].compClassification);
    EXPECT_EQ(outCompImageInfos[0].compIdentifier,
              compImageInfos[0].compIdentifier);
    EXPECT_EQ(outCompImageInfos[0].compComparisonStamp,
              compImageInfos[0].compComparisonStamp);
    EXPECT_EQ(outCompImageInfos[0].compOptions, compImageInfos[0].compOptions);
    EXPECT_EQ(outCompImageInfos[0].reqCompActivationMethod,
              compImageInfos[0].reqCompActivationMethod);
    EXPECT_EQ(outCompImageInfos[0].compLocation.length,
              compImageInfos[0].compLocation.length);
    EXPECT_EQ(outCompImageInfos[0].compVersion, compImageInfos[0].compVersion);
}

#endif

} // namespace fw_update
} // namespace pldm
