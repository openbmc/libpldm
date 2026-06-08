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

// The focus of this test is to check for things newly added between
// package format revisions v1.0.0 and v1.1.0

#if HAVE_LIBPLDM_API_TESTING

static std::vector<uint8_t> makePkgV1_1_0()
{

    const std::vector<uint8_t> headerUUID{
        // clang-format off
    0x12, 0x44, 0xd2, 0x64, 0x8d, 0x7d, 0x47, 0x18,
    0xa0, 0x30, 0xfc, 0x8a, 0x56, 0x58, 0x7d, 0x5a, // UUID
        // clang-format on
    };

    const std::vector<uint8_t> header{
        // clang-format off
    0x00, 0x00, // pkg header size, this is updated later

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x19, 0x0C, 0xE5, 0x07,
    0x00,       // pkg release date time (13 bytes, timestamp104)

    0x08, 0x00, // component bitmap bit length

    0x01,       // package version string type

    0x01,       // package version string length

    'v', // package version string
        // clang-format on
    };

    std::vector<uint8_t> fwdevidarea{
        // Firmware Device Identification Area

        // clang-format off
    0x00, 0x00, // record 0: record length

    0x01,                   // record 0: descriptor count
    0x00, 0x00, 0x00, 0x00, // record 0: device update options flags

    0x01,       // record 0: component image set version string type
    0x01,       // record 0: component image set version string length
    0x00, 0x00, // record 0: firmware device package data length

    0x01,       // applicable components

    'v',       // component image set version string

    // record descriptors below
    0x02, 0x00, // record 0: descriptor type: UUID

    0x10, 0x00, // record 0: InitialDescriptorLength (16 bytes)

    0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41,
    0x15, 0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49,
    0xD6, 0x75, // record 0: InitialDescriptorData (UUID)

    // firmware device package data (empty here)
        // clang-format on
    };

    {
        // set Recordlength
        const uint16_t recordLength = fwdevidarea.size();

        std::cout << "firmware device area length: " << recordLength
                  << std::endl;

        fwdevidarea[0] = recordLength & 0xff;
        fwdevidarea[1] = (recordLength >> 8) & 0xff;
    }

    std::vector<uint8_t> downstreamdevidarea{
        // clang-format off
    // Downstream Device Identification Area
    0x00, 0x00, // DownstreamDeviceRecordLength, this is updated later
    0x01, // DownstreamDeviceDescriptorCount
    0x01, 0x00, 0x00, 0x00, // DownstreamDeviceUpdateOptionFlags

    0x01,       // DownstreamDeviceSelfContainedActivationMinVersionStringType
    0x01,       // DownstreamDeviceSelfContainedActivationMinVersionStringLength

    0x03, 0x00, // DownstreamDevicePackageDataLength

    0x01, // DownstreamDeviceApplicableComponents

    'v', // DownstreamDeviceSelfContainedActivationMinVersionString

    // DownstreamDeviceSelfContainedActivationMinVersionComparisonStamp
    0x06, 0x07, 0x08, 0x09,

    // DownstreamDeviceRecordDescriptors
    // record descriptors below
    0x02, 0x00, // record 0: descriptor type: UUID

    0x10, 0x00, // record 0: InitialDescriptorLength (16 bytes)

    0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41,
    0x15, 0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49,
    0xD6, 0x75, // record 0: InitialDescriptorData (UUID)

    // DownstreamDevicePackageData
    0x83, 0x27, 0x72,
        // clang-format on
    };

    {
        // set DownstreamDeviceRecordLength
        const uint16_t downstreamdevidareaLength = downstreamdevidarea.size();

        std::cout << "downstream device area length: "
                  << downstreamdevidareaLength << std::endl;

        downstreamdevidarea[0] = downstreamdevidareaLength & 0xff;
        downstreamdevidarea[1] = (downstreamdevidareaLength >> 8) & 0xff;
    }

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
        // clang-format on
    };

    std::vector<uint8_t> pkg{};

    pkg.insert(pkg.end(), headerUUID.begin(), headerUUID.end());
    pkg.push_back(0x02); // pkg header format revision
    const size_t packageHeaderSizeOffset = pkg.size();
    pkg.insert(pkg.end(), header.begin(), header.end());

    // firmware device record count
    pkg.push_back(0x01);
    pkg.insert(pkg.end(), fwdevidarea.begin(), fwdevidarea.end());

    // DownstreamDeviceIDRecordCount
    pkg.push_back(0x01);
    pkg.insert(pkg.end(), downstreamdevidarea.begin(),
               downstreamdevidarea.end());

    pkg.insert(pkg.end(), componentimageinfoarea.begin(),
               componentimageinfoarea.end());

    // count in the checksum bytes still to be added
    const uint16_t finalSize = pkg.size() + 4;
    // set PackageHeaderSize
    pkg[packageHeaderSizeOffset] = finalSize & 0xff;
    pkg[packageHeaderSizeOffset + 1] = (finalSize >> 8) & 0xff;

    // actual checksum
    const uint32_t check = pldm_edac_crc32(pkg.data(), pkg.size());

    // PackageHeaderChecksum
    auto checksum =
        std::vector<uint8_t>{static_cast<uint8_t>(check & 0xff),
                             static_cast<uint8_t>((check >> 8) & 0xff),
                             static_cast<uint8_t>((check >> 16) & 0xff),
                             static_cast<uint8_t>((check >> 24) & 0xff)};
    pkg.insert(pkg.end(), checksum.begin(), checksum.end());

    // end of package header

    pkg.push_back(0x00); // Component Image

    return pkg;
}

#endif

#if HAVE_LIBPLDM_API_TESTING

TEST(PackageParserTest, ValidPkgV1_1_0_ParseFailWithLowerPin)
{
    // An older libpldm++ user code COULD still successfully use a package
    // with a higher format revision when linked with a newer libpldm++.
    //
    // The user code would simply not access the newer members in that case as
    // it does not know about them.
    //
    // In case of libpldm++, the user code is simply iterating over STL
    // containers and growable structs which are already fully parsed and do not
    // require any state tracking on part of the user code.
    //
    // However we disable this possibility to be in line with what libpldm does.

    const auto res = PackageParser::parse(makePkgV1_1_0(), PackagePin::v1);

    ASSERT_FALSE(res.has_value());
}

#endif

#if HAVE_LIBPLDM_API_TESTING

TEST(PackageParserTest, ValidPkgV1_1_0_ParseSuccess)
{
    const auto res = PackageParser::parse(makePkgV1_1_0(), PackagePin::v1_1_0);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;
        if (res.error().rc.has_value())
        {
            std::cout << res.error().rc.value() << std::endl;
        }
    }

    ASSERT_TRUE(res.has_value());
}

#endif

#if HAVE_LIBPLDM_API_TESTING

TEST(PackageParserTest, ValidPkgV1_1_0_SelfContainedActivation)
{
    const auto res = PackageParser::parse(makePkgV1_1_0(), PackagePin::v1_1_0);

    ASSERT_TRUE(res.has_value());

    const auto& dsDeviceIdRecords = res.value()->downstreamDeviceIdRecords;

    ASSERT_EQ(dsDeviceIdRecords.size(), 1);

    const auto& dsDevice = dsDeviceIdRecords[0];

    EXPECT_EQ(dsDevice.downstreamDeviceUpdateOptionFlags, 0x01);

    EXPECT_EQ(dsDevice.downstreamDeviceSelfContainedActivationMinVersionString,
              "v");

    EXPECT_EQ(
        dsDevice
            .downstreamDeviceSelfContainedActivationMinVersionComparisonStamp,
        0x09080706);
}

#endif

#if HAVE_LIBPLDM_API_TESTING

TEST(PackageParserTest, ValidPkgV1_1_0_Applicable_Components)
{
    const auto res = PackageParser::parse(makePkgV1_1_0(), PackagePin::v1_1_0);

    ASSERT_TRUE(res.has_value());

    const auto& dsDeviceIdRecords = res.value()->downstreamDeviceIdRecords;

    ASSERT_EQ(dsDeviceIdRecords.size(), 1);

    const auto& dsDevice = dsDeviceIdRecords[0];

    EXPECT_EQ(dsDevice.downstreamDeviceApplicableComponents.size(), 1);
    EXPECT_EQ(dsDevice.downstreamDeviceApplicableComponents,
              std::vector<size_t>{0});
}

#endif

#if HAVE_LIBPLDM_API_TESTING

TEST(PackageParserTest, ValidPkgV1_1_0_DownstreamDeviceRecordDescriptors)
{
    const auto res = PackageParser::parse(makePkgV1_1_0(), PackagePin::v1_1_0);

    ASSERT_TRUE(res.has_value());

    const auto& dsDeviceIdRecords = res.value()->downstreamDeviceIdRecords;

    ASSERT_EQ(dsDeviceIdRecords.size(), 1);

    const auto& dsDevice = dsDeviceIdRecords[0];

    EXPECT_EQ(dsDevice.downstreamDeviceRecordDescriptors.size(), 1);
    for (auto& [k, v] : dsDevice.downstreamDeviceRecordDescriptors)
    {
        EXPECT_EQ(k, 0x02);
        EXPECT_EQ(v->data.size(), 16);
        EXPECT_EQ(v->data[0], 0x16);
        EXPECT_EQ(v->data[1], 0x20);
    }
}

#endif

#if HAVE_LIBPLDM_API_TESTING

TEST(PackageParserTest, ValidPkgV1_1_0_DownstreamDevicePackageData)
{
    const auto res = PackageParser::parse(makePkgV1_1_0(), PackagePin::v1_1_0);

    ASSERT_TRUE(res.has_value());

    const auto& dsDeviceIdRecords = res.value()->downstreamDeviceIdRecords;

    ASSERT_EQ(dsDeviceIdRecords.size(), 1);

    const auto& dsDevice = dsDeviceIdRecords[0];

    EXPECT_EQ(dsDevice.downstreamDevicePackageData.size(), 3);

    std::vector<uint8_t> expectedDsDevicePackageData = {0x83, 0x27, 0x72};

    EXPECT_EQ(dsDevice.downstreamDevicePackageData,
              expectedDsDevicePackageData);
}

#endif

} // namespace fw_update
} // namespace pldm
