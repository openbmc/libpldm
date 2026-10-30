#include "packagebuilder.hpp"
#include "testpkg-util.hpp"

#include <libpldm/api.h>
#include <libpldm/edac.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <libpldm++/firmware_update.hpp>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace pldm
{
namespace fw_update
{

#if HAVE_LIBPLDM_API_TESTING
static std::vector<uint8_t> makePackage(const std::string& version,
                                        PackagePin pin)
{
    std::vector<uint8_t> pkg;

    appendPackageHeaderIdentifier(pkg, pin);
    const size_t headerSizeOffset = pkg.size();

    appendLE16(pkg, PLACEHOLDER);
    appendTimestamp104(pkg);
    appendLE16(pkg, 0x08);
    appendTypeLengthString(pkg, version);
    pkg.push_back(0x01);
    appendFirmwareDeviceIdRecord1(pkg);
    if (pin == PackagePin::v1_1_0)
    {
        pkg.push_back(0x00); // No downstream device records.
    }

    const auto locations = appendComponentImageInfoArea1(pkg);
    const size_t headerSize = pkg.size() + sizeof(uint32_t);

    patchLE16(pkg, headerSizeOffset, headerSize);
    for (const size_t location : locations)
    {
        patchLE32(pkg, location, headerSize);
    }
    appendCRC(pkg);
    pkg.push_back(0x00); // Component image.
    return pkg;
}
#endif

#if HAVE_LIBPLDM_API_TESTING
static void updateHeaderChecksum(std::vector<uint8_t>& pkg)
{
    const size_t checksumOffset = pkg.size() - sizeof(uint32_t) - 1;

    patchLE32(pkg, checksumOffset, pldm_edac_crc32(pkg.data(), checksumOffset));
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageHeaderInformationTest, CopyPreservesAllFieldsAndOwnsVersion)
{
    const std::array<uint8_t, PLDM_TIMESTAMP104_SIZE> timestamp{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x19, 0x0C, 0xE5, 0x07, 0x00};
    const std::string version = "PackageVersionLongEnoughToRequireOwnedStorage";
    auto pkg = makePackage(version, PackagePin::v1_1_0);
    auto result = PackageParser::parse(pkg, PackagePin::v1_3_0);

    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result.value()->packageHeaderInformation, nullptr);
    const PackageHeaderInformation copy(
        *result.value()->packageHeaderInformation);

    EXPECT_NE(
        copy.packageVersionString.data(),
        result.value()->packageHeaderInformation->packageVersionString.data());
    result.value().reset();
    pkg.clear();

    EXPECT_EQ(copy.packageHeaderIdentifier, PackagePin::v1_1_0);
    EXPECT_EQ(copy.packageHeaderFormatRevision,
              PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR02H);
    EXPECT_EQ(copy.packageReleaseDateTime, timestamp);
    EXPECT_EQ(copy.packageVersionString, version);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageHeaderInformationTest, CopyPreservesEmptyVersion)
{
    auto pkg = makePackage("", PackagePin::v1);
    auto result = PackageParser::parse(pkg, PackagePin::v1_3_0);

    ASSERT_TRUE(result.has_value());
    const PackageHeaderInformation copy(
        *result.value()->packageHeaderInformation);

    result.value().reset();
    EXPECT_TRUE(copy.packageVersionString.empty());
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageHeaderInformationTest, CopyPreservesEmbeddedNullInVersion)
{
    const std::string version("v1\0suffix", 9);
    auto pkg = makePackage(version, PackagePin::v1);
    auto result = PackageParser::parse(pkg, PackagePin::v1_3_0);

    ASSERT_TRUE(result.has_value());
    const PackageHeaderInformation copy(
        *result.value()->packageHeaderInformation);

    result.value().reset();
    EXPECT_EQ(copy.packageVersionString, version);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageHeaderInformationTest, IdenticalHeadersCompareEqual)
{
    auto pkg = makePackage("PackageVersion", PackagePin::v1);
    auto lhs = PackageParser::parse(pkg, PackagePin::v1_3_0);
    auto rhs = PackageParser::parse(pkg, PackagePin::v1_3_0);

    ASSERT_TRUE(lhs.has_value());
    ASSERT_TRUE(rhs.has_value());
    const auto& left = *lhs.value()->packageHeaderInformation;
    const auto& right = *rhs.value()->packageHeaderInformation;

    EXPECT_EQ(left.packageHeaderIdentifier, PackagePin::v1);
    EXPECT_EQ(right.packageHeaderIdentifier, PackagePin::v1);
    EXPECT_EQ(left, left);
    EXPECT_EQ(left, right);
    EXPECT_EQ(right, left);

    const PackageHeaderInformation copy(left);

    EXPECT_EQ(copy, left);
    EXPECT_EQ(left, copy);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageHeaderInformationTest, DifferentIdentifiersCompareUnequal)
{
    const std::array<uint8_t, PLDM_FWUP_UUID_LENGTH> identifier =
        PLDM_PACKAGE_HEADER_IDENTIFIER_V1_1;
    auto first = makePackage("PackageVersion", PackagePin::v1);
    auto second = first;

    // Keep the format revision unchanged to isolate identifier comparison.
    std::copy(identifier.begin(), identifier.end(), second.begin());
    updateHeaderChecksum(second);
    auto lhs = PackageParser::parse(first, PackagePin::v1_3_0);
    auto rhs = PackageParser::parse(second, PackagePin::v1_3_0);

    ASSERT_TRUE(lhs.has_value());
    ASSERT_TRUE(rhs.has_value());
    const auto& left = *lhs.value()->packageHeaderInformation;
    const auto& right = *rhs.value()->packageHeaderInformation;

    EXPECT_EQ(left.packageHeaderIdentifier, PackagePin::v1);
    EXPECT_EQ(right.packageHeaderIdentifier, PackagePin::v1_1_0);
    ASSERT_EQ(left.packageHeaderFormatRevision,
              right.packageHeaderFormatRevision);
    EXPECT_FALSE(left == right);
    EXPECT_FALSE(right == left);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageHeaderInformationTest, DifferentFormatRevisionsCompareUnequal)
{
    const std::array<uint8_t, PLDM_FWUP_UUID_LENGTH> identifier =
        PLDM_PACKAGE_HEADER_IDENTIFIER_V1_1;
    auto first = makePackage("PackageVersion", PackagePin::v1);
    auto second = makePackage("PackageVersion", PackagePin::v1_1_0);

    // Keep the identifier unchanged to isolate format revision comparison.
    std::copy(identifier.begin(), identifier.end(), first.begin());
    updateHeaderChecksum(first);
    auto lhs = PackageParser::parse(first, PackagePin::v1_3_0);
    auto rhs = PackageParser::parse(second, PackagePin::v1_3_0);

    ASSERT_TRUE(lhs.has_value());
    ASSERT_TRUE(rhs.has_value());
    const auto& left = *lhs.value()->packageHeaderInformation;
    const auto& right = *rhs.value()->packageHeaderInformation;

    ASSERT_EQ(left.packageHeaderIdentifier, right.packageHeaderIdentifier);
    EXPECT_EQ(left.packageHeaderFormatRevision,
              PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR01H);
    EXPECT_EQ(right.packageHeaderFormatRevision,
              PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR02H);
    EXPECT_FALSE(left == right);
    EXPECT_FALSE(right == left);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageHeaderInformationTest, DifferentVersionsCompareUnequal)
{
    auto first = makePackage("Version1", PackagePin::v1);
    auto second = makePackage("Version2", PackagePin::v1);
    auto lhs = PackageParser::parse(first, PackagePin::v1_3_0);
    auto rhs = PackageParser::parse(second, PackagePin::v1_3_0);

    ASSERT_TRUE(lhs.has_value());
    ASSERT_TRUE(rhs.has_value());
    const auto& left = *lhs.value()->packageHeaderInformation;
    const auto& right = *rhs.value()->packageHeaderInformation;

    EXPECT_FALSE(left == right);
    EXPECT_FALSE(right == left);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageHeaderInformationTest, VersionComparisonIncludesBytesAfterNull)
{
    auto first = makePackage(std::string("v1\0first", 8), PackagePin::v1);
    auto second = makePackage(std::string("v1\0other", 8), PackagePin::v1);
    auto lhs = PackageParser::parse(first, PackagePin::v1_3_0);
    auto rhs = PackageParser::parse(second, PackagePin::v1_3_0);

    ASSERT_TRUE(lhs.has_value());
    ASSERT_TRUE(rhs.has_value());
    const auto& left = *lhs.value()->packageHeaderInformation;
    const auto& right = *rhs.value()->packageHeaderInformation;

    EXPECT_FALSE(left == right);
    EXPECT_FALSE(right == left);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageHeaderInformationTest, DifferentTimestampsStillCompareEqual)
{
    constexpr size_t timestampOffset =
        PLDM_FWUP_UUID_LENGTH + sizeof(uint8_t) + sizeof(uint16_t);
    auto first = makePackage("PackageVersion", PackagePin::v1);
    auto second = first;

    second[timestampOffset] = 1;
    updateHeaderChecksum(second);
    auto lhs = PackageParser::parse(first, PackagePin::v1_3_0);
    auto rhs = PackageParser::parse(second, PackagePin::v1_3_0);

    ASSERT_TRUE(lhs.has_value());
    ASSERT_TRUE(rhs.has_value());
    const auto& left = *lhs.value()->packageHeaderInformation;
    const auto& right = *rhs.value()->packageHeaderInformation;

    ASSERT_NE(left.packageReleaseDateTime, right.packageReleaseDateTime);
    EXPECT_EQ(left, right);
    EXPECT_EQ(right, left);
}
#endif

} // namespace fw_update
} // namespace pldm
