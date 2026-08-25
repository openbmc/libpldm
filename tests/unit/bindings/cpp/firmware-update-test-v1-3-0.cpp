#include "packagebuilder.hpp"
#include "testpkg-util.hpp"

#include <libpldm/api.h>
#include <libpldm/edac.h>

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

#if HAVE_LIBPLDM_API_TESTING
static void
    appendDownstreamDeviceIDRecord(std::vector<uint8_t>& pkg,
                                   uint8_t downstreamApplicableComponents)
{
    const size_t recordLengthOffset = pkg.size();
    appendLE16(pkg, PLACEHOLDER); // DownstreamDeviceRecordLength
    pkg.push_back(0x01);          // DownstreamDeviceDescriptorCount
    appendLE32(pkg, 0x01);        // DownstreamDeviceUpdateOptionFlags
    pkg.push_back(0x01);          // Minimum version string type: ASCII
    pkg.push_back(0x0E);          // Minimum version string length
    appendLE16(pkg, 0x03);        // DownstreamDevicePackageDataLength
    appendLE32(pkg, 0x08);        // DownstreamDeviceReferenceManifestLength
    pkg.push_back(downstreamApplicableComponents);
    appendString(pkg, "VersionString1");
    appendLE32(pkg, 0x0a090a09); // Minimum version comparison stamp

    appendDescriptorTLV(pkg, PLDM_FWUP_UUID,
                        {0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15, 0x95,
                         0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75});

    // DownstreamDevicePackageData
    const std::vector<uint8_t> packageData{0xde, 0xde, 0xfe};
    pkg.insert(pkg.end(), packageData.begin(), packageData.end());

    // DownstreamDeviceReferenceManifestData: PCI-SIG vendor ID and manifest
    const std::vector<uint8_t> manifestData{0x03, 0x02, 0x1d, 0xa0,
                                            0x31, 0x32, 0x33, 0x34};
    pkg.insert(pkg.end(), manifestData.begin(), manifestData.end());

    patchLE16(pkg, recordLengthOffset, pkg.size() - recordLengthOffset);
}

static std::vector<uint8_t>
    makePkgV1_3_0(uint8_t downstreamApplicableComponents = 0x01)
{
    std::vector<uint8_t> pkg;
    appendPackageHeaderIdentifier(pkg, PackagePin::v1_3_0);
    const size_t packageHeaderSizeOffset = pkg.size();

    // pkg header size, this is updated later
    appendLE16(pkg, PLACEHOLDER);

    // pkg release date time (13 bytes, timestamp104)
    appendTimestamp104(pkg);

    // component bitmap bit length
    appendLE16(pkg, 0x08);

    // package version string
    appendTypeLengthString(pkg, "VersionString1");

    // Firmware Device Identification Area
    pkg.push_back(0x01); // device id record count
    appendFirmwareDeviceIdRecord3(pkg);

    // Downstream Device Identification Area
    // DownstreamDeviceIDRecordCount
    pkg.push_back(0x01);
    appendDownstreamDeviceIDRecord(pkg, downstreamApplicableComponents);

    const auto clos = appendComponentImageInfoArea1(pkg);

    appendComponentOpaqueData(pkg);

    // count in the checksum bytes still to be added
    const uint16_t finalSize = pkg.size() + 8;
    // set PackageHeaderSize
    patchLE16(pkg, packageHeaderSizeOffset, finalSize);

    for (const size_t clo : clos)
    {
        patchLE32(pkg, clo, finalSize);
    }

    appendCRC(pkg);

    // PLDMFWPackagePayloadChecksum
    auto payloadChecksum = std::vector<uint8_t>{0x8d, 0xef, 0x02, 0xd2};
    pkg.insert(pkg.end(), payloadChecksum.begin(), payloadChecksum.end());

    // end of package header

    pkg.push_back(0x00); // Component Image

    uintmax_t pkgSize = pkg.size();

    std::cout << "package size: " << pkgSize << std::endl;

    return pkg;
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageParserTest, ValidPkgV1_3_0_SingleDescriptorWithDownstreamDevices)
{
    auto pkg = makePkgV1_3_0();
    auto res = PackageParser::parse(pkg, PackagePin::v1_3_0);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;
    }

    ASSERT_TRUE(res.has_value());

    const auto dsDeviceIdRecords = res.value()->downstreamDeviceIdRecords;

    ASSERT_EQ(dsDeviceIdRecords.size(), 1);

    const auto& dsDevice = dsDeviceIdRecords[0];

    EXPECT_EQ(dsDevice.downstreamDeviceUpdateOptionFlags, 0x01);

    EXPECT_EQ(dsDevice.downstreamDeviceSelfContainedActivationMinVersionString,
              "VersionString1");

    EXPECT_EQ(
        dsDevice
            .downstreamDeviceSelfContainedActivationMinVersionComparisonStamp,
        0x0a090a09);

    EXPECT_EQ(dsDevice.downstreamDeviceApplicableComponents.size(), 1);
    EXPECT_EQ(dsDevice.downstreamDeviceApplicableComponents,
              std::vector<size_t>{0});

    EXPECT_EQ(dsDevice.downstreamDeviceRecordDescriptors.size(), 1);
    for (auto& [k, v] : dsDevice.downstreamDeviceRecordDescriptors)
    {
        EXPECT_EQ(k, 0x02);
        EXPECT_EQ(v->data.size(), 16);
        EXPECT_EQ(v->data[0], 0x16);
        EXPECT_EQ(v->data[1], 0x20);
    }

    EXPECT_EQ(dsDevice.downstreamDevicePackageData.size(), 3);

    const std::vector<uint8_t> expectedDSDevicePkgData{0xde, 0xde, 0xfe};
    EXPECT_EQ(dsDevice.downstreamDevicePackageData, expectedDSDevicePkgData);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageParserTest, InvalidPkgV1_3_0_DownstreamApplicableComponentOOB)
{
    auto pkg = makePkgV1_3_0(0x02);
    auto res = PackageParser::parse(pkg, PackagePin::v1_3_0);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().msg, "applicable component index 1 is out of bounds");
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageParserTest, ValidPkgV1_3_0_FirmwareDeviceIdReferenceManifestData)
{
    auto pkg = makePkgV1_3_0();
    auto res = PackageParser::parse(pkg, PackagePin::v1_3_0);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;
    }

    ASSERT_TRUE(res.has_value());

    const auto fwdevices = res.value()->firmwareDeviceIdRecords;

    ASSERT_EQ(fwdevices.size(), 1);

    const auto& fwdevice = fwdevices[0];

    EXPECT_TRUE(fwdevice.referenceManifestData.has_value());

    EXPECT_EQ(fwdevice.referenceManifestData.value().SVHID, 0x03);

    const std::vector<uint8_t> expectedVendorID{0x20, 0xaf};
    EXPECT_EQ(fwdevice.referenceManifestData.value().vendorID,
              expectedVendorID);

    const std::vector<uint8_t> expectedReferenceManifestData{0x21, 0x22, 0x23,
                                                             0x24};
    EXPECT_EQ(fwdevice.referenceManifestData->data,
              expectedReferenceManifestData);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageParserTest, ValidPkgV1_3_0_DownstreamDeviceReferenceManifestData)
{
    auto pkg = makePkgV1_3_0();
    auto res = PackageParser::parse(pkg, PackagePin::v1_3_0);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;
    }

    ASSERT_TRUE(res.has_value());

    const auto dsDeviceIdRecords = res.value()->downstreamDeviceIdRecords;

    ASSERT_EQ(dsDeviceIdRecords.size(), 1);

    const auto& dsDevice = dsDeviceIdRecords[0];

    EXPECT_TRUE(dsDevice.downstreamDeviceReferenceManifestData.has_value());

    EXPECT_EQ(dsDevice.downstreamDeviceReferenceManifestData.value().SVHID,
              0x03);

    const std::vector<uint8_t> expectedVendorID{0x1d, 0xa0};
    EXPECT_EQ(dsDevice.downstreamDeviceReferenceManifestData.value().vendorID,
              expectedVendorID);

    const std::vector<uint8_t> expectedReferenceManifestData{0x31, 0x32, 0x33,
                                                             0x34};
    EXPECT_EQ(dsDevice.downstreamDeviceReferenceManifestData->data,
              expectedReferenceManifestData);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageParserTest, ValidPkgV1_3_0_ComponentOpaqueData)
{
    auto pkg = makePkgV1_3_0();
    auto res = PackageParser::parse(pkg, PackagePin::v1_3_0);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;
    }

    ASSERT_TRUE(res.has_value());

    const auto& compInfos = res.value()->componentImageInformation;

    EXPECT_EQ(compInfos.size(), 1);

    const auto& compInfo = compInfos[0];

    EXPECT_EQ(compInfo.componentOpaqueData.size(), 5);
}
#endif

} // namespace fw_update
} // namespace pldm
