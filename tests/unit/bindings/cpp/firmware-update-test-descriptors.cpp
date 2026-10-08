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

} // namespace fw_update
} // namespace pldm
