#include "testpkg-util.hpp"

#include <libpldm/api.h>
#include <libpldm/firmware_update.h>

#include <expected>
#include <libpldm++/firmware_update.hpp>
#include <libpldm++/types.hpp>
#include <span>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace pldm::fw_update;

namespace pldm
{
namespace fw_update
{

struct TestDescriptor
{
    uint16_t type;
    std::vector<uint8_t> data;
};

static void appendDescriptors(std::vector<uint8_t>& pkg,
                              const std::vector<TestDescriptor>& descriptors)
{
    for (const auto& desc : descriptors)
    {
        appendLE16(pkg, desc.type);
        appendLE16(pkg, static_cast<uint16_t>(desc.data.size()));
        pkg.insert(pkg.end(), desc.data.begin(), desc.data.end());
    }
}

static const std::vector<uint8_t> uuid16{0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5,
                                         0x41, 0x15, 0x95, 0xF4, 0x48, 0x70,
                                         0x1D, 0x49, 0xD6, 0x75};

static const std::vector<uint8_t> iana4{0x47, 0x16, 0x00, 0x00};

// ASCII title "OpenBMC", 2 bytes of data
static const std::vector<uint8_t> vendorDefined{
    0x01, 0x07, 0x4F, 0x70, 0x65, 0x6E, 0x42, 0x4D, 0x43, 0x12, 0x34};

// FR01h package, one FD record with the given descriptors, one component
static std::vector<uint8_t>
    makePkgWithFDDescriptors(const std::vector<TestDescriptor>& descriptors,
                             uint8_t descriptorCount)
{
    std::vector<uint8_t> pkg;

    appendPackageHeaderIdentifier(pkg, PackagePin::v1);

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

    const size_t recordLengthOffset = pkg.size();

    // record length
    appendLE16(pkg, PLACEHOLDER);

    // descriptor count
    pkg.push_back(descriptorCount);

    // device update option flags
    appendLE32(pkg, 0x01);

    // component image set version string type
    pkg.push_back(0x01);

    // component image set version string length
    pkg.push_back(0x0E);

    // firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x01);

    // component image set version string
    appendString(pkg, "VersionString2");

    appendDescriptors(pkg, descriptors);

    patchLE16(pkg, recordLengthOffset, pkg.size() - recordLengthOffset);

    const auto clos = appendComponentImageInfoArea1(pkg);

    for (const size_t clo : clos)
    {
        patchLE32(pkg, clo, pkg.size() + 4);
    }

    patchLE16(pkg, hdrSizeOffset, pkg.size() + 4);

    appendCRC(pkg);

    // component image
    pkg.push_back(0x00);

    return pkg;
}

// FR02h package, one FD record, one downstream record with the given
// descriptors, one component
static std::vector<uint8_t>
    makePkgWithDDDescriptors(const std::vector<TestDescriptor>& descriptors,
                             uint8_t descriptorCount)
{
    std::vector<uint8_t> pkg;

    appendPackageHeaderIdentifier(pkg, PackagePin::v1_1_0);

    const size_t hdrSizeOffset = pkg.size();
    // pkg header size
    appendLE16(pkg, PLACEHOLDER);

    // pkg release date time (13 bytes, timestamp104)
    appendTimestamp104(pkg);

    // component bitmap bit length
    appendLE16(pkg, 0x08);

    // package version string
    appendTypeLengthString(pkg, "v");

    // firmware device record count
    pkg.push_back(0x01);
    appendFirmwareDeviceIdRecord2(pkg);

    // DownstreamDeviceIDRecordCount
    pkg.push_back(0x01);

    const size_t recordLengthOffset = pkg.size();

    // DownstreamDeviceRecordLength
    appendLE16(pkg, PLACEHOLDER);

    // DownstreamDeviceDescriptorCount
    pkg.push_back(descriptorCount);

    // DownstreamDeviceUpdateOptionFlags
    appendLE32(pkg, 0x01);

    // DownstreamDeviceSelfContainedActivationMinVersionStringType
    pkg.push_back(0x01);

    // DownstreamDeviceSelfContainedActivationMinVersionStringLength
    pkg.push_back(0x01);

    // DownstreamDevicePackageDataLength
    appendLE16(pkg, 0x00);

    // DownstreamDeviceApplicableComponents
    pkg.push_back(0x01);

    // DownstreamDeviceSelfContainedActivationMinVersionString
    appendString(pkg, "v");

    // DownstreamDeviceSelfContainedActivationMinVersionComparisonStamp
    appendLE32(pkg, 0x09080706);

    // DownstreamDeviceRecordDescriptors
    appendDescriptors(pkg, descriptors);

    patchLE16(pkg, recordLengthOffset, pkg.size() - recordLengthOffset);

    const auto clos = appendComponentImageInfoArea1(pkg);

    for (const size_t clo : clos)
    {
        patchLE32(pkg, clo, pkg.size() + 4);
    }

    patchLE16(pkg, hdrSizeOffset, pkg.size() + 4);

    appendCRC(pkg);

    // component image
    pkg.push_back(0x00);

    return pkg;
}

TEST(PackageParserTest, FDNoDescriptorsRejected)
{
    auto pkg = makePkgWithFDDescriptors({}, 0);
    auto res = PackageParser::parse(pkg, PackagePin::v1);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().msg, "firmware device id record has no descriptors");
}

TEST(PackageParserTest, DDNoDescriptorsRejected)
{
    auto pkg = makePkgWithDDDescriptors({}, 0);
    auto res = PackageParser::parse(pkg, PackagePin::v1_1_0);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().msg,
              "downstream device id record has no descriptors");
}

TEST(PackageParserTest, FDDescriptorWrongLengthRejected)
{
    std::vector<uint8_t> uuid15(uuid16.begin(), uuid16.end() - 1);

    auto pkg = makePkgWithFDDescriptors({{PLDM_FWUP_UUID, uuid15}}, 1);
    auto res = PackageParser::parse(pkg, PackagePin::v1);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().msg, "descriptor type 2 has length 15, expected 16");
}

TEST(PackageParserTest, FDAdditionalDescriptorWrongLengthRejected)
{
    auto pkg = makePkgWithFDDescriptors(
        {{PLDM_FWUP_IANA_ENTERPRISE_ID, iana4},
         {PLDM_FWUP_PCI_REVISION_ID_RANGE, {0x01, 0x02, 0x03}}},
        2);
    auto res = PackageParser::parse(pkg, PackagePin::v1);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().msg, "descriptor type 267 has length 3, expected 2");
}

TEST(PackageParserTest, FDStandardDescriptorsAccepted)
{
    auto pkg = makePkgWithFDDescriptors(
        {{PLDM_FWUP_IANA_ENTERPRISE_ID, iana4},
         {PLDM_FWUP_PCI_REVISION_ID_RANGE, {0x01, 0x02}},
         {PLDM_FWUP_IEEE_EUI_64_ID,
          {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}},
         {PLDM_FWUP_VENDOR_DEFINED, vendorDefined}},
        4);
    auto res = PackageParser::parse(pkg, PackagePin::v1);

    ASSERT_TRUE(res.has_value()) << res.error().msg;

    const auto& records = res.value()->firmwareDeviceIdRecords;
    ASSERT_EQ(records.size(), 1);

    const auto& descriptors = records[0].recordDescriptors;
    EXPECT_EQ(descriptors.size(), 4);
    EXPECT_EQ(descriptors.at(PLDM_FWUP_IANA_ENTERPRISE_ID)->data, iana4);
    EXPECT_EQ(descriptors.at(PLDM_FWUP_PCI_REVISION_ID_RANGE)->data,
              std::vector<uint8_t>({0x01, 0x02}));
    EXPECT_EQ(descriptors.at(PLDM_FWUP_IEEE_EUI_64_ID)->data.size(), 8);
    EXPECT_EQ(
        descriptors.at(PLDM_FWUP_VENDOR_DEFINED)->vendorDefinedDescriptorTitle,
        "OpenBMC");
}

TEST(PackageParserTest, FDUnknownDescriptorTypePassedThrough)
{
    const uint16_t unknownType = 0x0200;
    const std::vector<uint8_t> unknownData{0xA1, 0xB2, 0xC3, 0xD4, 0xE5};

    auto pkg = makePkgWithFDDescriptors(
        {{PLDM_FWUP_IANA_ENTERPRISE_ID, iana4}, {unknownType, unknownData}}, 2);
    auto res = PackageParser::parse(pkg, PackagePin::v1);

    ASSERT_TRUE(res.has_value()) << res.error().msg;

    const auto& descriptors =
        res.value()->firmwareDeviceIdRecords[0].recordDescriptors;
    EXPECT_EQ(descriptors.size(), 2);
    EXPECT_EQ(descriptors.at(unknownType)->data, unknownData);
}

TEST(PackageParserTest, DDDescriptorWrongLengthRejected)
{
    std::vector<uint8_t> uuid15(uuid16.begin(), uuid16.end() - 1);

    auto pkg = makePkgWithDDDescriptors({{PLDM_FWUP_UUID, uuid15}}, 1);
    auto res = PackageParser::parse(pkg, PackagePin::v1_1_0);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().msg, "descriptor type 2 has length 15, expected 16");
}

} // namespace fw_update
} // namespace pldm
