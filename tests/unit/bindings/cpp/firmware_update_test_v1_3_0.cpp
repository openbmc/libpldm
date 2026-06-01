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

TEST(PackageParserTest, ValidPkgSingleDescriptorWithDownstreamDevices)
{

    const std::vector<uint8_t> headerUUID{
        // clang-format off
    0x7B, 0x29, 0x1C, 0x99, 0x6D, 0xB6, 0x42, 0x08,
    0x80, 0x1B, 0x02, 0x02, 0x6E, 0x46, 0x3C, 0x78, // UUID
        // clang-format on
    };

    const std::vector<uint8_t> header{
        // clang-format off
    0x8b, 0x00, // pkg header size, this is updated later

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x19, 0x0C, 0xE5, 0x07,
    0x00,       // pkg release date time (13 bytes, timestamp104)

    0x08, 0x00, // component bitmap bit length

    0x01,       // package version string type

    0x0E,       // package version string length

    0x56, 0x65, 0x72, 0x73, 0x69, 0x6F,
    0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
    0x67, 0x31, // package version string
        // clang-format on
    };

    const std::vector<uint8_t> fwdevidarea{
        // Firmware Device Identification Area

        // clang-format off
    0x01,       // device id record count

    0x32, 0x00, // record 0: record length

    0x01,                   // record 0: descriptor count
    0x01, 0x00, 0x00, 0x00, // record 0: device update options flags

    0x01,       // record 0: component image set version string type
    0x0E,       // record 0: component image set version string length
    0x00, 0x00, // record 0: firmware device package data length

    0x00, 0x00, 0x00, 0x00, // record 0: ReferenceManifestLength

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
    // ReferenceManifestData (empty here)
        // clang-format on
    };

    std::vector<uint8_t> downstreamdevidarea{
        // clang-format off
    // Downstream Device Identification Area
    0x00, 0x10, // DownstreamDeviceRecordLength
    0x01, // DownstreamDeviceDescriptorCount
    0x01, 0x00, 0x00, 0x00, // DownstreamDeviceUpdateOptionFlags

    0x01,       // DownstreamDeviceSelfContainedActivationMinVersionStringType
    0x0E,       // DownstreamDeviceSelfContainedActivationMinVersionStringLength

    0x03, 0x00, // DownstreamDevicePackageDataLength
    0x08, 0x00, 0x00, 0x00, // DownstreamDeviceReferenceManifestLength

    0x01, // DownstreamDeviceApplicableComponents

    0x56, 0x65, 0x72, 0x73, 0x69, 0x6F,
    0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
    0x67, 0x31, // DownstreamDeviceSelfContainedActivationMinVersionString

    0x09, 0xa, 0x9, 0xa, // DownstreamDeviceSelfContainedActivationMinVersionComparisonStamp

    // DownstreamDeviceRecordDescriptors
    // record descriptors below
    0x02, 0x00, // record 0: descriptor type: UUID

    0x10, 0x00, // record 0: InitialDescriptorLength (16 bytes)

    0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41,
    0x15, 0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49,
    0xD6, 0x75, // record 0: InitialDescriptorData (UUID)

    // DownstreamDevicePackageData
    0xde, 0xde, 0xfe,

    // DownstreamDeviceReferenceManifestData (as per Table 10)
    0x03, // SVHID,  (e.g. PCI SIG)
    0x02, // VendorIDLen
    0x1d, 0xa0, // (e.g. 3M Company)
    // actual manifest data (completely made up and without meaning)
    0x31, 0x32, 0x33, 0x34,

        // clang-format on
    };

    // set DownstreamDeviceRecordLength
    const uint16_t downstreamdevidareaLength = downstreamdevidarea.size();
    downstreamdevidarea[0] = downstreamdevidareaLength & 0xff;
    downstreamdevidarea[1] = (downstreamdevidareaLength >> 8) & 0xff;

    const std::vector<uint8_t> componentimageinfoarea{
        // clang-format off
    // Component Image Information Area
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
    0x5, 0x00, 0x00, 0x00, // ComponentOpaqueDataLength
    0x05, 0x04, 0x03, 0x02, 0x01, // ComponentOpaqueData
        // clang-format on
    };

    std::vector<uint8_t> pkg{};

    pkg.append_range(headerUUID);
    pkg.push_back(0x04); // pkg header format revision
    const size_t packageHeaderSizeOffset = pkg.size();
    pkg.append_range(header);
    pkg.append_range(fwdevidarea);

    // DownstreamDeviceIDRecordCount
    pkg.push_back(0x01);
    pkg.append_range(downstreamdevidarea);

    pkg.append_range(componentimageinfoarea);

    // count in the checksum bytes still to be added
    const uint16_t finalSize = pkg.size() + 8;
    // set PackageHeaderSize
    pkg[packageHeaderSizeOffset] = finalSize & 0xff;
    pkg[packageHeaderSizeOffset + 1] = (finalSize >> 8) & 0xff;

    // actual checksum
    const uint32_t check = pldm_edac_crc32(pkg.data(), pkg.size());

    // PackageHeaderChecksum
    pkg.append_range(
        std::vector<uint8_t>{static_cast<uint8_t>(check & 0xff),
                             static_cast<uint8_t>((check >> 8) & 0xff),
                             static_cast<uint8_t>((check >> 16) & 0xff),
                             static_cast<uint8_t>((check >> 24) & 0xff)});

    // PLDMFWPackagePayloadChecksum
    pkg.append_range(std::vector<uint8_t>{0x8d, 0xef, 0x02, 0xd2});

    // end of package header

    pkg.push_back(0x00); // Component Image

    uintmax_t pkgSize = pkg.size();

    std::cout << "package size: " << pkgSize << std::endl;

    auto res = PackageParser::parse(pkg, PackagePin::v1_3_0);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;
    }

    ASSERT_TRUE(res.has_value());

    auto outfwDeviceIDRecords = res.value()->firmwareDeviceIdRecords;

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

    EXPECT_EQ(dsDevice.applicableComponents.size(), 1);
    EXPECT_EQ(dsDevice.applicableComponents, std::vector<size_t>{0});

    EXPECT_EQ(dsDevice.recordDescriptors.size(), 1);
    for (auto& [k, v] : dsDevice.recordDescriptors)
    {
        EXPECT_EQ(k, 0x02);
        EXPECT_EQ(v->data.size(), 16);
        EXPECT_EQ(v->data[0], 0x16);
        EXPECT_EQ(v->data[1], 0x20);
    }

    EXPECT_EQ(dsDevice.downstreamDevicePackageData.size(), 3);

    const std::vector<uint8_t> expectedDSDevicePkgData{0xde, 0xde, 0xfe};
    EXPECT_EQ(dsDevice.downstreamDevicePackageData, expectedDSDevicePkgData);

    EXPECT_TRUE(dsDevice.downstreamDeviceReferenceManifestData.has_value());

    EXPECT_EQ(dsDevice.downstreamDeviceReferenceManifestData.value().SVHID,
              0x03);

    const std::vector<uint8_t> expectedDSVendorID{0x1d, 0xa0};
    EXPECT_EQ(dsDevice.downstreamDeviceReferenceManifestData.value().vendorID,
              expectedDSVendorID);

    const std::vector<uint8_t> expectedReferenceManifestData{0x31, 0x32, 0x33,
                                                             0x34};
    EXPECT_EQ(dsDevice.downstreamDeviceReferenceManifestData->data,
              expectedReferenceManifestData);

    const auto& compInfos = res.value()->componentImageInformation;

    EXPECT_EQ(compInfos.size(), 1);

    const auto& compInfo = compInfos[0];

    EXPECT_EQ(compInfo.componentOpaqueData.size(), 5);
}

#endif

} // namespace fw_update
} // namespace pldm
