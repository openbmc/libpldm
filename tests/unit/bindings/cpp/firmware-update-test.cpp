#include "packagebuilder.hpp"
#include "testpkg-util.hpp"

#include <libpldm/api.h>
#include <libpldm/edac.h>

#include <algorithm>
#include <array>
#include <expected>
#include <libpldm++/firmware_update.hpp>
#include <libpldm++/types.hpp>
#include <span>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace pldm::fw_update;

namespace pldm
{
namespace fw_update
{

static std::vector<uint8_t> makePkgWithHeader(const std::string& version)
{
    std::vector<uint8_t> pkg;

    appendPackageHeaderIdentifier(pkg, PackagePin::v1);
    const size_t headerSizeOffset = pkg.size();
    appendLE16(pkg, PLACEHOLDER);
    appendTimestamp104(pkg);
    appendLE16(pkg, 0x08);
    appendTypeLengthString(pkg, version);
    pkg.push_back(0x01);
    appendFirmwareDeviceIdRecord1(pkg);
    const auto locations = appendComponentImageInfoArea1(pkg);
    const size_t headerSize = pkg.size() + sizeof(uint32_t);
    patchLE16(pkg, headerSizeOffset, headerSize);
    for (const size_t location : locations)
    {
        patchLE32(pkg, location, headerSize);
    }
    appendCRC(pkg);
    pkg.push_back(0x00); // Component Image
    return pkg;
}

TEST(PackageHeaderTest, RetainsHeaderInformation)
{
    const std::array<uint8_t, PLDM_TIMESTAMP104_SIZE> timestamp{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x19, 0x0C, 0xE5, 0x07, 0x00};
    auto pkg = makePkgWithHeader("PackageVersion");
    auto result = PackageParser::parse(pkg, PackagePin::v1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value()->packageHeaderIdentifier, PackagePin::v1);
    EXPECT_EQ(result.value()->packageHeaderFormatRevision,
              PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR01H);
    EXPECT_EQ(result.value()->packageReleaseDateTime, timestamp);
    EXPECT_EQ(result.value()->packageVersionString, "PackageVersion");

    pkg.assign(pkg.size(), 0);
    EXPECT_EQ(result.value()->packageReleaseDateTime, timestamp);
    EXPECT_EQ(result.value()->packageVersionString, "PackageVersion");
}

TEST(PackageHeaderTest, RejectsUnknownHeaderIdentifier)
{
    auto pkg = makePkgWithHeader("PackageVersion");

    std::fill_n(pkg.begin(), PLDM_FWUP_UUID_LENGTH, 0);
    auto result = PackageParser::parse(pkg, PackagePin::v1);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().msg, "Failed to decode pldm package header");
}

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageHeaderTest, CopiesHeaderInformation)
{
    auto pkg = makePkgWithHeader("PackageVersion");
    auto result = PackageParser::parse(pkg, PackagePin::v1);

    ASSERT_TRUE(result.has_value());
    const Package copy(*result.value());
    EXPECT_EQ(copy.packageHeaderIdentifier, PackagePin::v1);
    EXPECT_EQ(copy.packageHeaderFormatRevision,
              PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR01H);
    EXPECT_EQ(copy.packageReleaseDateTime,
              result.value()->packageReleaseDateTime);
    EXPECT_EQ(copy.packageVersionString, "PackageVersion");
    EXPECT_EQ(copy, *result.value());
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageHeaderTest, ComparesPackageVersion)
{
    auto first = makePkgWithHeader("Version1");
    auto second = makePkgWithHeader("Version2");
    auto lhs = PackageParser::parse(first, PackagePin::v1);
    auto rhs = PackageParser::parse(second, PackagePin::v1);

    ASSERT_TRUE(lhs.has_value());
    ASSERT_TRUE(rhs.has_value());
    EXPECT_FALSE(*lhs.value() == *rhs.value());
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageHeaderTest, ComparesReleaseTimestamp)
{
    auto first = makePkgWithHeader("PackageVersion");
    auto second = first;
    const size_t checksumOffset = second.size() - 1 - sizeof(uint32_t);
    const size_t timestampOffset =
        PLDM_FWUP_UUID_LENGTH + sizeof(uint8_t) + sizeof(uint16_t);

    second[timestampOffset] = 1;
    patchLE32(second, checksumOffset,
              pldm_edac_crc32(second.data(), checksumOffset));
    auto lhs = PackageParser::parse(first, PackagePin::v1);
    auto rhs = PackageParser::parse(second, PackagePin::v1);

    ASSERT_TRUE(lhs.has_value());
    ASSERT_TRUE(rhs.has_value());
    EXPECT_FALSE(*lhs.value() == *rhs.value());
}
#endif

TEST(PackageHeaderTest, PreservesEmbeddedNullInVersion)
{
    const std::string version("v1\0suffix", 9);
    auto pkg = makePkgWithHeader(version);
    auto result = PackageParser::parse(pkg, PackagePin::v1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value()->packageVersionString, version);
}

TEST(PackageHeaderTest, RejectsNonAsciiVersionEncoding)
{
    const std::string version("v\xc3\xa9rsion");
    auto pkg = makePkgWithHeader(version);
    const size_t checksumOffset = pkg.size() - 1 - sizeof(uint32_t);
    const size_t stringTypeOffset = PLDM_FWUP_UUID_LENGTH + sizeof(uint8_t) +
                                    sizeof(uint16_t) + PLDM_TIMESTAMP104_SIZE +
                                    sizeof(uint16_t);

    pkg[stringTypeOffset] = PLDM_STR_TYPE_UTF_8;
    patchLE32(pkg, checksumOffset, pldm_edac_crc32(pkg.data(), checksumOffset));
    auto result = PackageParser::parse(pkg, PackagePin::v1);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().msg, "not an ascii string");
}

TEST(PackageHeaderTest, PreservesEmptyVersion)
{
    auto pkg = makePkgWithHeader("");
    auto result = PackageParser::parse(pkg, PackagePin::v1);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value()->packageVersionString.empty());
}

TEST(PackageParserTest, ValidPkgSingleDescriptorSingleComponent)
{
    std::vector<uint8_t> pkg;

    appendPackageHeaderIdentifier(pkg, pldm::fw_update::PackagePin::v1);

    const size_t hdrSizeOffset = pkg.size();
    // pkg header size
    appendLE16(pkg, PLACEHOLDER);

    // pkg release date time (13 bytes, timestamp104)
    appendTimestamp104(pkg);

    // component bitmap bit length
    appendLE16(pkg, 0x08);

    // package version string
    appendTypeLengthString(pkg, "VersionString1");

    // device id record count
    pkg.push_back(0x01);

    appendFirmwareDeviceIdRecord1(pkg);

    const auto clos = appendComponentImageInfoArea1(pkg);

    for (const size_t clo : clos)
    {
        patchLE32(pkg, clo, pkg.size() + 4);
    }

    patchLE16(pkg, hdrSizeOffset, pkg.size() + 4);

    appendCRC(pkg);

    // component image
    pkg.push_back(0x00);

    auto res = PackageParser::parse(pkg, PackagePin::v1);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;
    }

    ASSERT_TRUE(res.has_value());

    const auto& outfwDeviceIDRecords = res.value()->firmwareDeviceIdRecords;

    std::vector<uint8_t> dd1Data{
        // clang-format off
        0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
        0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75
        // clang-format on
    };

    ASSERT_EQ(outfwDeviceIDRecords.size(), 1);

    // we cannot compare the applicable components here since their memory
    // address will be different
    EXPECT_EQ(outfwDeviceIDRecords[0].deviceUpdateOptionFlags,
              std::bitset<32>(1));
    EXPECT_EQ(outfwDeviceIDRecords[0].componentImageSetVersionString,
              "VersionString2");

#if HAVE_LIBPLDM_API_TESTING
    EXPECT_EQ(outfwDeviceIDRecords[0].getDescriptorTypes(),
              std::vector<uint16_t>({PLDM_FWUP_UUID}));
#endif

    // assert for descriptor type PLDM_FWUP_UUID
    const auto& d1 =
        outfwDeviceIDRecords[0].recordDescriptors.find(PLDM_FWUP_UUID)->second;
    EXPECT_EQ(d1->data, dd1Data);
    EXPECT_EQ(d1->vendorDefinedDescriptorTitle, std::nullopt);

    EXPECT_EQ(outfwDeviceIDRecords[0].firmwareDevicePackageData,
              std::vector<uint8_t>{});

    const auto& outCompImageInfos = res.value()->componentImageInformation;

    ASSERT_EQ(outCompImageInfos.size(), 1);

    EXPECT_EQ(outCompImageInfos[0].componentClassification, 10);
    EXPECT_EQ(outCompImageInfos[0].componentIdentifier, 100);
    EXPECT_EQ(outCompImageInfos[0].compComparisonStamp, 0xFFFFFFFF);
    EXPECT_EQ(outCompImageInfos[0].componentOptions, 0);
    EXPECT_EQ(outCompImageInfos[0].requestedComponentActivationMethod, 0);
    EXPECT_EQ(outCompImageInfos[0].componentLocation.length, 1);
    EXPECT_EQ(outCompImageInfos[0].componentVersion, "VersionString3");
}

TEST(PackageParserTest, ValidPkgMultipleDescriptorsMultipleComponents)
{
    std::vector<uint8_t> pkg;

    appendPackageHeaderIdentifier(pkg, pldm::fw_update::PackagePin::v1);

    const size_t hdrSizeOffset = pkg.size();

    // pkg header size
    appendLE16(pkg, PLACEHOLDER);

    // pkg release date time, 13 bytes, timestamp104
    appendTimestamp104(pkg);

    // component bitmap bit length
    appendLE16(pkg, 0x08);

    // package version string
    appendTypeLengthString(pkg, "VersionString1");

    appendFirmwareDeviceIdArea1(pkg);

    const auto clos = appendComponentImageInfoArea2(pkg);

    // set PackageHeaderSize
    patchLE16(pkg, hdrSizeOffset, pkg.size() + 4);

    // patch CLO
    for (const size_t clo : clos)
    {
        patchLE32(pkg, clo, pkg.size() + 4);
    }

    appendCRC(pkg);

    // component image
    pkg.push_back(0x00);

    auto res = PackageParser::parse(pkg, PackagePin::v1);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;
    }

    ASSERT_TRUE(res.has_value());

    const std::vector<FirmwareDeviceIDRecord>& outfwDeviceIDRecords =
        res.value()->firmwareDeviceIdRecords;

    std::vector<uint8_t> dd1Data{
        // clang-format off
        0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18,
        0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5B
        // clang-format on
    };

    std::vector<uint8_t> dd2Data{0x47, 0x16, 0x00, 0x00};

    std::vector<uint8_t> dd3Data{0x12, 0x34};

    std::vector<uint8_t> dd4Data{
        // clang-format off
        0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18, 0xA0,
        0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5C, 0x11
        // clang-format on
    };

    std::vector<uint8_t> dd5Data{
        // clang-format off
        0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18, 0xA0,
        0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5D, 0x12
        // clang-format on
    };

    ASSERT_EQ(outfwDeviceIDRecords.size(), 3);

    // start asserting fw device id records
    // fw device id records index 0 (record 1)

    // we cannot compare the applicable components here since their memory
    // address will be different
    EXPECT_EQ(outfwDeviceIDRecords[0].deviceUpdateOptionFlags,
              std::bitset<32>(1));

    EXPECT_EQ(outfwDeviceIDRecords[0].componentImageSetVersionString,
              "VersionString2");

#if HAVE_LIBPLDM_API_TESTING
    // assert record descriptor types
    const auto types = outfwDeviceIDRecords[0].getDescriptorTypes();
    EXPECT_THAT(types, ::testing::UnorderedElementsAre(
                           PLDM_FWUP_UUID, PLDM_FWUP_IANA_ENTERPRISE_ID,
                           PLDM_FWUP_VENDOR_DEFINED));
#endif

    // assert record descriptor contents
    EXPECT_EQ(outfwDeviceIDRecords[0]
                  .recordDescriptors.find(PLDM_FWUP_UUID)
                  ->second->data,
              dd1Data);
    EXPECT_EQ(outfwDeviceIDRecords[0]
                  .recordDescriptors.find(PLDM_FWUP_UUID)
                  ->second->vendorDefinedDescriptorTitle,
              std::nullopt);

    EXPECT_EQ(outfwDeviceIDRecords[0]
                  .recordDescriptors.find(PLDM_FWUP_IANA_ENTERPRISE_ID)
                  ->second->data,
              dd2Data);
    EXPECT_EQ(outfwDeviceIDRecords[0]
                  .recordDescriptors.find(PLDM_FWUP_IANA_ENTERPRISE_ID)
                  ->second->vendorDefinedDescriptorTitle,
              std::nullopt);

    EXPECT_EQ(outfwDeviceIDRecords[0]
                  .recordDescriptors.find(PLDM_FWUP_VENDOR_DEFINED)
                  ->second->data,
              dd3Data);
    EXPECT_EQ(outfwDeviceIDRecords[0]
                  .recordDescriptors.find(PLDM_FWUP_VENDOR_DEFINED)
                  ->second->vendorDefinedDescriptorTitle,
              "OpenBMC");

    EXPECT_EQ(outfwDeviceIDRecords[0].firmwareDevicePackageData,
              std::vector<uint8_t>{});

    // fw device id records index 1 (record 2)

    // we cannot compare the applicable components here since their memory
    // address will be different
    EXPECT_EQ(outfwDeviceIDRecords[1].deviceUpdateOptionFlags,
              std::bitset<32>(0));
    EXPECT_EQ(outfwDeviceIDRecords[1].componentImageSetVersionString,
              "VersionString3");
#if HAVE_LIBPLDM_API_TESTING
    EXPECT_EQ(outfwDeviceIDRecords[1].getDescriptorTypes(),
              std::vector<uint16_t>{PLDM_FWUP_UUID});
#endif

    const auto& d1 =
        outfwDeviceIDRecords[1].recordDescriptors.find(PLDM_FWUP_UUID)->second;

    EXPECT_EQ(d1->data, dd4Data);
    EXPECT_EQ(d1->vendorDefinedDescriptorTitle, std::nullopt);

    EXPECT_EQ(outfwDeviceIDRecords[1].firmwareDevicePackageData,
              std::vector<uint8_t>{});

    // fw device id records index 2 (record 3)

    // we cannot compare the applicable components here since their memory
    // address will be different
    EXPECT_EQ(outfwDeviceIDRecords[2].deviceUpdateOptionFlags,
              std::bitset<32>(0));
    EXPECT_EQ(outfwDeviceIDRecords[2].componentImageSetVersionString,
              "VersionString4");
#if HAVE_LIBPLDM_API_TESTING
    EXPECT_EQ(outfwDeviceIDRecords[2].getDescriptorTypes(),
              std::vector<uint16_t>{PLDM_FWUP_UUID});
#endif

    // assert for descriptor type PLDM_FWUP_UUID
    const auto& d2 =
        outfwDeviceIDRecords[2].recordDescriptors.find(PLDM_FWUP_UUID)->second;

    EXPECT_EQ(d2->data, dd5Data);
    EXPECT_EQ(d2->vendorDefinedDescriptorTitle, std::nullopt);

    EXPECT_EQ(outfwDeviceIDRecords[2].firmwareDevicePackageData,
              std::vector<uint8_t>{});

    // end asserting fw device id records

    const auto& outCompImageInfos = res.value()->componentImageInformation;

    ASSERT_EQ(outCompImageInfos.size(), 3);

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

TEST(PackageParserTest,
     InValidPkgSingleDescriptorSingleComponentApplicableComponentOOB)
{
    // failure: the applicable component is out of bounds
    std::vector<uint8_t> pkg;

    appendPackageHeaderIdentifier(pkg, pldm::fw_update::PackagePin::v1);

    const size_t hdrSizeOffset = pkg.size();
    // pkg header size
    appendLE16(pkg, PLACEHOLDER);

    // pkg release date time (13 bytes, timestamp104)
    appendTimestamp104(pkg);

    // component bitmap bit length
    appendLE16(pkg, 0x08);

    // package version string
    appendTypeLengthString(pkg, "VersionString1");

    // device id record count
    pkg.push_back(0x01);

    // describes an applicable component which does not exist in the package
    appendFirmwareDeviceIdRecord1InvalidApplicableComponentOOB(pkg);

    const auto clos = appendComponentImageInfoArea1(pkg);

    for (size_t clo : clos)
    {
        patchLE32(pkg, clo, pkg.size() + 4);
    }

    patchLE16(pkg, hdrSizeOffset, pkg.size() + 4);

    appendCRC(pkg);

    // component image
    pkg.push_back(0x00);

    auto res = PackageParser::parse(pkg, PackagePin::v1);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;

        EXPECT_EQ(res.error().msg,
                  "applicable component index 3 is out of bounds");
    }

    ASSERT_FALSE(res.has_value());
}

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

    std::expected<std::unique_ptr<Package>, PackageParserError> result =
        PackageParser::parse(fwPkgHdr, PackagePin::v1);

    EXPECT_FALSE(result.has_value());
}

} // namespace fw_update
} // namespace pldm
