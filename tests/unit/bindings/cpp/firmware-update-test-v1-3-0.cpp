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
static std::vector<uint8_t> makePkgV1_3_0()
{
    std::vector<uint8_t> header;

    // pkg header size, this is updated later
    appendLE16(header, PLACEHOLDER);

    // pkg release date time (13 bytes, timestamp104)
    appendTimestamp104(header);

    // component bitmap bit length
    appendLE16(header, 0x08);

    // package version string
    appendTypeLengthString(header, "VersionString1");

    std::vector<uint8_t> ddevidarea{};

    // Downstream Device Identification Area

    // DownstreamDeviceRecordLength
    appendLE16(ddevidarea, PLACEHOLDER);

    // DownstreamDeviceDescriptorCount
    ddevidarea.push_back(0x01);

    // DownstreamDeviceUpdateOptionFlags
    appendLE32(ddevidarea, 0x01);

    // DownstreamDeviceSelfContainedActivationMinVersionStringType
    ddevidarea.push_back(0x01);

    // DownstreamDeviceSelfContainedActivationMinVersionStringLength
    ddevidarea.push_back(0x0E);

    // DownstreamDevicePackageDataLength
    appendLE16(ddevidarea, 0x03);

    // DownstreamDeviceReferenceManifestLength
    appendLE32(ddevidarea, 0x08);

    // DownstreamDeviceApplicableComponents
    ddevidarea.push_back(0x01);

    // DownstreamDeviceSelfContainedActivationMinVersionString
    appendString(ddevidarea, "VersionString1");

    // DownstreamDeviceSelfContainedActivationMinVersionComparisonStamp
    appendLE32(ddevidarea, 0x0a090a09);

    // DownstreamDeviceRecordDescriptors
    // record descriptors below

    appendDescriptorTLV(ddevidarea, PLDM_FWUP_UUID,
                        std::vector<uint8_t>{
                            // clang-format off
        0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
        0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75,
                            // clang-format on
                        });

    // DownstreamDevicePackageData
    const std::vector<uint8_t> ddpd = {0xde, 0xde, 0xfe};
    ddevidarea.insert(ddevidarea.end(), ddpd.begin(), ddpd.end());

    // DownstreamDeviceReferenceManifestData (as per Table 10)
    // SVHID,  (e.g. PCI SIG)
    ddevidarea.push_back(0x03);

    // VendorIDLen
    ddevidarea.push_back(0x02);

    // (e.g. 3M Company)
    ddevidarea.push_back(0x1d);
    ddevidarea.push_back(0xa0);

    // actual manifest data (completely made up and without meaning)
    const std::vector<uint8_t> rmd = {0x31, 0x32, 0x33, 0x34};
    ddevidarea.insert(ddevidarea.end(), rmd.begin(), rmd.end());

    // set DownstreamDeviceRecordLength
    patchLE16(ddevidarea, 0, ddevidarea.size());

    std::vector<uint8_t> pkg{};

    appendPackageHeaderIdentifier(pkg, pldm::fw_update::PackagePin::v1_3_0);

    const size_t packageHeaderSizeOffset = pkg.size();
    pkg.insert(pkg.end(), header.begin(), header.end());

    // Firmware Device Identification Area
    pkg.push_back(0x01); // device id record count
    appendFirmwareDeviceIdRecord3(pkg);

    // DownstreamDeviceIDRecordCount
    pkg.push_back(0x01);
    pkg.insert(pkg.end(), ddevidarea.begin(), ddevidarea.end());

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
