#include "libpldm++/firmware_update.hpp"
#include "libpldm++/types.hpp"

#include <expected>
#include <span>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace pldm::fw_update;

namespace pldm
{
namespace fw_update
{

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

#endif

#ifdef LIBPLDM_API_TESTING

TEST(PackageParserTest, ValidPkgSingleDescriptorSingleComponent)
{

    uintmax_t pkgSize = fwPkgHdrSingleComponent.size();

    std::cout << pkgSize << std::endl;

    DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR01H(pin);
    auto span = std::span<const uint8_t>(fwPkgHdrSingleComponent);
    auto res = PackageParser::parse(span, pin);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;
    }

    ASSERT_TRUE(res.has_value());

    auto outfwDeviceIDRecords = res.value()->firmwareDeviceIdRecords;

    std::vector<uint8_t> dd1Data{0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5,
                                 0x41, 0x15, 0x95, 0xF4, 0x48, 0x70,
                                 0x1D, 0x49, 0xD6, 0x75};

    ASSERT_EQ(outfwDeviceIDRecords.size(), 1);

    // we cannot compare the applicable components here since their memory
    // address will be different
    EXPECT_EQ(outfwDeviceIDRecords[0].deviceUpdateOptionFlags,
              std::bitset<32>(1));
    EXPECT_EQ(outfwDeviceIDRecords[0].componentImageSetVersionString,
              "VersionString2");

    EXPECT_EQ(outfwDeviceIDRecords[0].getDescriptorTypes(),
              std::vector<uint16_t>({PLDM_FWUP_UUID}));

    // assert for descriptor type PLDM_FWUP_UUID
    const auto& d1 =
        outfwDeviceIDRecords[0].recordDescriptors.at(PLDM_FWUP_UUID);
    EXPECT_EQ(d1->data, dd1Data);
    EXPECT_EQ(d1->vendorDefinedDescriptorTitle, std::nullopt);

    EXPECT_EQ(outfwDeviceIDRecords[0].firmwareDevicePackageData,
              std::vector<uint8_t>{});

    auto outCompImageInfos = res.value()->componentImageInformation;

    ASSERT_EQ(outCompImageInfos.size(), 1);

    EXPECT_EQ(outCompImageInfos[0].componentClassification, 10);
    EXPECT_EQ(outCompImageInfos[0].componentIdentifier, 100);
    EXPECT_EQ(outCompImageInfos[0].compComparisonStamp, 0xFFFFFFFF);
    EXPECT_EQ(outCompImageInfos[0].componentOptions, 0);
    EXPECT_EQ(outCompImageInfos[0].requestedComponentActivationMethod, 0);
    EXPECT_EQ(outCompImageInfos[0].componentLocation.length, 1);
    EXPECT_EQ(outCompImageInfos[0].componentVersion, "VersionString3");
}

#endif

#ifdef LIBPLDM_API_TESTING

TEST(PackageParserTest, ValidPkgMultipleDescriptorsMultipleComponents)
{
    std::vector<uint8_t> fwPkgHdr{
        // clang-format off
        0xF0, 0x18, 0x87, 0x8C, 0xCB, 0x7D, 0x49, 0x43, 0x98, 0x00, 0xA0, 0x2F,
        0x05, 0x9A, 0xCA, 0x02, // UUID

        0x01,       // header format revision
        0x46, 0x01, // pkg header size

        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x0C, 0xE5, 0x07,
        0x00, // pkg release date time, 13 bytes, timestamp104

        0x08, 0x00, // component bitmake bit length
        0x01,       // package version string type
        0x0E,       // package version string length

        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x31, // package version string

        0x03, // device id record count

        0x45, 0x00, // record 0: record length

        0x03,                   // record 0: descriptor countn
        0x01, 0x00, 0x00, 0x00, // record 0: device update options flags
        0x01,       // record 0: component image set version string type
        0x0E,       // record 0: component image set version string length
        0x00, 0x00, // record 0: firmware device package data length
        0x03,       // applicable components:

        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x32, // component image set version string (14 bytes)

        0x02, 0x00, // record 0: record descriptor type: uuid
        0x10, 0x00, 0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18, 0xA0, 0x30,
        0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5B, 0x01, 0x00, 0x04, 0x00, 0x47, 0x16,
        0x00, 0x00, 0xFF, 0xFF, 0x0B, 0x00, 0x01, 0x07, 0x4F, 0x70, 0x65, 0x6E,
        0x42, 0x4D, 0x43, 0x12, 0x34, 0x2E, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x0E, 0x00, 0x00, 0x07, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E,
        0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x33, 0x02, 0x00, 0x10, 0x00, 0x12,
        0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18, 0xA0, 0x30, 0xFC, 0x8A, 0x56,
        0x58, 0x7D, 0x5C, 0x2E, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0E,
        0x00, 0x00, 0x01, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74,
        0x72, 0x69, 0x6E, 0x67, 0x34, 0x02, 0x00, 0x10, 0x00, 0x12, 0x44, 0xD2,
        0x64, 0x8D, 0x7D, 0x47, 0x18, 0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D,
        0x5D,

        0x03, 0x00, // component image count

        0x0A, 0x00,             // component classification
        0x64, 0x00,             // component identifier
        0xFF, 0xFF, 0xFF, 0xFF, // component comparison stamp
        0x00, 0x00,             // component options

        0x00, 0x00,             // requested component activation method
        0x46, 0x01, 0x00, 0x00, // component location offset
        0x01, 0x00, 0x00, 0x00, // component size
        0x01,                   // component version string type
        0x0E,                   // component version string length

        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x35, // component version string (14 bytes)

        0x0A, 0x00,             // component classification
        0xC8, 0x00,             // component identifier
        0xFF, 0xFF, 0xFF, 0xFF, // component comparison stamp
        0x00, 0x00,             // component options

        0x01, 0x00,             // requested component activation method
        0x46, 0x01, 0x00, 0x00, // component location offset
        0x01, 0x00, 0x00, 0x00, // component size
        0x01,                   // component version string type
        0x0E,                   // component version string length
        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x36, // component version string

        0xB,  0x00,             // component classification
        0x2C, 0x01,             // component identifier
        0xFF, 0xFF, 0xFF, 0xFF, // component comparison stamp
        0x01, 0x00,             // component options

        0x0C, 0x00,             // requested component activation method
        0x46, 0x01, 0x00, 0x00, // component location offset
        0x01, 0x00, 0x00, 0x00, // component size
        0x01,                   // component version string type
        0x0E,                   // component version string length
        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x37, // component version string

        0x14, 0xa9, 0xbf, 0x39, // package checksum
        0x00                    // component image
        // clang-format on
    };

    const uintmax_t pkgSize = fwPkgHdr.size();

    std::cout << pkgSize << std::endl;

    DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR01H(pin);
    auto span = std::span<const uint8_t>(fwPkgHdr);
    auto res = PackageParser::parse(span, pin);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;
    }

    ASSERT_TRUE(res.has_value());

    std::vector<FirmwareDeviceIDRecord> outfwDeviceIDRecords =
        res.value()->firmwareDeviceIdRecords;

    std::vector<uint8_t> dd1Data{0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D,
                                 0x47, 0x18, 0xA0, 0x30, 0xFC, 0x8A,
                                 0x56, 0x58, 0x7D, 0x5B};

    std::vector<uint8_t> dd2Data{0x47, 0x16, 0x00, 0x00};

    std::vector<uint8_t> dd3Data{0x12, 0x34};

    std::vector<uint8_t> dd4Data{0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D,
                                 0x47, 0x18, 0xA0, 0x30, 0xFC, 0x8A,
                                 0x56, 0x58, 0x7D, 0x5C};

    std::vector<uint8_t> dd5Data{0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D,
                                 0x47, 0x18, 0xA0, 0x30, 0xFC, 0x8A,
                                 0x56, 0x58, 0x7D, 0x5D};

    EXPECT_EQ(outfwDeviceIDRecords.size(), 3);

    // start asserting fw device id records
    // fw device id records index 0 (record 1)

    // we cannot compare the applicable components here since their memory
    // address will be different
    EXPECT_EQ(outfwDeviceIDRecords[0].deviceUpdateOptionFlags,
              std::bitset<32>(1));

    EXPECT_EQ(outfwDeviceIDRecords[0].componentImageSetVersionString,
              "VersionString2");

    EXPECT_EQ(outfwDeviceIDRecords[0].getDescriptorTypes().size(), 3);

    // assert record descriptor types
    EXPECT_EQ(outfwDeviceIDRecords[0].getDescriptorTypes()[1], PLDM_FWUP_UUID);
    EXPECT_EQ(outfwDeviceIDRecords[0].getDescriptorTypes()[0],
              PLDM_FWUP_IANA_ENTERPRISE_ID);
    EXPECT_EQ(outfwDeviceIDRecords[0].getDescriptorTypes()[2],
              PLDM_FWUP_VENDOR_DEFINED);

    // assert record descriptor contents
    EXPECT_EQ(
        outfwDeviceIDRecords[0].recordDescriptors.at(PLDM_FWUP_UUID)->data,
        dd1Data);
    EXPECT_EQ(outfwDeviceIDRecords[0]
                  .recordDescriptors.at(PLDM_FWUP_UUID)
                  ->vendorDefinedDescriptorTitle,
              std::nullopt);

    EXPECT_EQ(outfwDeviceIDRecords[0]
                  .recordDescriptors.at(PLDM_FWUP_IANA_ENTERPRISE_ID)
                  ->data,
              dd2Data);
    EXPECT_EQ(outfwDeviceIDRecords[0]
                  .recordDescriptors.at(PLDM_FWUP_IANA_ENTERPRISE_ID)
                  ->vendorDefinedDescriptorTitle,
              std::nullopt);

    EXPECT_EQ(outfwDeviceIDRecords[0]
                  .recordDescriptors.at(PLDM_FWUP_VENDOR_DEFINED)
                  ->data,
              dd3Data);
    EXPECT_EQ(outfwDeviceIDRecords[0]
                  .recordDescriptors.at(PLDM_FWUP_VENDOR_DEFINED)
                  ->vendorDefinedDescriptorTitle,
              "OpenBMC");

    EXPECT_EQ(outfwDeviceIDRecords[0].firmwareDevicePackageData,
              std::vector<uint8_t>{});

    // fw device id records index 1 (record 2)

    // we cannot compare the applicable components here since their memory
    // address will be different
    EXPECT_EQ(outfwDeviceIDRecords[1].deviceUpdateOptionFlags, 0);
    EXPECT_EQ(outfwDeviceIDRecords[1].componentImageSetVersionString,
              "VersionString3");
    EXPECT_EQ(outfwDeviceIDRecords[1].getDescriptorTypes(),
              std::vector<uint16_t>{PLDM_FWUP_UUID});

    for (uint16_t type : {PLDM_FWUP_UUID})
    {
        const auto& d1 = outfwDeviceIDRecords[1].recordDescriptors.at(type);

        EXPECT_EQ(d1->data, dd4Data);
        EXPECT_EQ(d1->vendorDefinedDescriptorTitle, std::nullopt);
    }
    EXPECT_EQ(outfwDeviceIDRecords[1].firmwareDevicePackageData,
              std::vector<uint8_t>{});

    // fw device id records index 2 (record 3)

    // we cannot compare the applicable components here since their memory
    // address will be different
    EXPECT_EQ(outfwDeviceIDRecords[2].deviceUpdateOptionFlags, 0);
    EXPECT_EQ(outfwDeviceIDRecords[2].componentImageSetVersionString,
              "VersionString4");
    EXPECT_EQ(outfwDeviceIDRecords[2].getDescriptorTypes(),
              std::vector<uint16_t>{PLDM_FWUP_UUID});

    // assert for descriptor type PLDM_FWUP_UUID
    const auto& d1 =
        outfwDeviceIDRecords[2].recordDescriptors.at(PLDM_FWUP_UUID);

    EXPECT_TRUE(d1->data == dd5Data);
    EXPECT_TRUE(d1->vendorDefinedDescriptorTitle == std::nullopt);

    EXPECT_EQ(outfwDeviceIDRecords[2].firmwareDevicePackageData,
              std::vector<uint8_t>{});

    // end asserting fw device id records

    auto outCompImageInfos = res.value()->componentImageInformation;

    EXPECT_EQ(outCompImageInfos.size(), 3);

    // start asserting component image info
    // component image info index 0

    EXPECT_EQ(outCompImageInfos[0].componentClassification, 10);
    EXPECT_EQ(outCompImageInfos[0].componentIdentifier, 100);
    EXPECT_EQ(outCompImageInfos[0].compComparisonStamp, 0xFFFFFFFF);
    EXPECT_EQ(outCompImageInfos[0].componentOptions, 0);
    EXPECT_EQ(outCompImageInfos[0].requestedComponentActivationMethod, 0);
    EXPECT_EQ(outCompImageInfos[0].componentLocation.length, 1);
    EXPECT_EQ(outCompImageInfos[0].componentVersion, "VersionString5");

    // component image info index 1

    EXPECT_EQ(outCompImageInfos[1].componentClassification, 10);
    EXPECT_EQ(outCompImageInfos[1].componentIdentifier, 200);
    EXPECT_EQ(outCompImageInfos[1].compComparisonStamp, 0xFFFFFFFF);
    EXPECT_EQ(outCompImageInfos[1].componentOptions, 0);
    EXPECT_EQ(outCompImageInfos[1].requestedComponentActivationMethod, 1);
    EXPECT_EQ(outCompImageInfos[1].componentLocation.length, 1);
    EXPECT_EQ(outCompImageInfos[1].componentVersion, "VersionString6");

    // component image info index 2

    EXPECT_EQ(outCompImageInfos[2].componentClassification, 11);
    EXPECT_EQ(outCompImageInfos[2].componentIdentifier, 300);
    EXPECT_EQ(outCompImageInfos[2].compComparisonStamp, 0xFFFFFFFF);
    EXPECT_EQ(outCompImageInfos[2].componentOptions, 1);
    EXPECT_EQ(outCompImageInfos[2].requestedComponentActivationMethod, 12);
    EXPECT_EQ(outCompImageInfos[2].componentLocation.length, 1);
    EXPECT_EQ(outCompImageInfos[2].componentVersion, "VersionString7");

    // end asserting component image info
}

#endif

#ifdef LIBPLDM_API_TESTING

TEST(PackageParserTest, InvalidPkgBadChecksum)
{
    std::vector<uint8_t> fwPkgHdr{
        0xF0, 0x18, 0x87, 0x8C, 0xCB, 0x7D, 0x49, 0x43, 0x98, 0x00, 0xA0, 0x2F,
        0x05, 0x9A, 0xCA, 0x02, 0x01, 0x8B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x19, 0x0C, 0xE5, 0x07, 0x00, 0x08, 0x00, 0x01, 0x0E,
        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x31, 0x01, 0x2E, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0E,
        0x00, 0x00, 0x01, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74,
        0x72, 0x69, 0x6E, 0x67, 0x32, 0x02, 0x00, 0x10, 0x00, 0x16, 0x20, 0x23,
        0xC9, 0x3E, 0xC5, 0x41, 0x15, 0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6,
        0x75, 0x01, 0x00, 0x0A, 0x00, 0x64, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
        0x00, 0x00, 0x00, 0x8B, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x01,
        0x0E, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69,
        0x6E, 0x67, 0x33, 0x4F, 0x96, 0xAE, 0x57};

    DEFINE_PLDM_PACKAGE_FORMAT_PIN_FR01H(pin);

    auto span = std::span<const uint8_t>(fwPkgHdr);
    std::expected<std::unique_ptr<Package>, PackageParserError> result =
        PackageParser::parse(span, pin);

    EXPECT_FALSE(result.has_value());
}

#endif

} // namespace fw_update
} // namespace pldm
