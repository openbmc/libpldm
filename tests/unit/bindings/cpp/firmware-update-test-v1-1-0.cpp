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

// The focus of this test is to check for things newly added between
// package format revisions v1.0.0 and v1.1.0

#if HAVE_LIBPLDM_API_TESTING
static std::vector<uint8_t> makePkgV1_1_0()
{
    std::vector<uint8_t> header;

    // pkg header size, updated later
    appendLE16(header, PLACEHOLDER);

    // pkg release date time (13 bytes, timestamp104)
    appendTimestamp104(header);

    // component bitmap bit length
    appendLE16(header, 0x08);

    // package version string
    appendTypeLengthString(header, "v");

    // Downstream Device Identification Area
    std::vector<uint8_t> ddevidarea{};

    // DownstreamDeviceRecordLength, this is updated later
    appendLE16(ddevidarea, PLACEHOLDER);

    // DownstreamDeviceDescriptorCount
    ddevidarea.push_back(0x01);

    // DownstreamDeviceUpdateOptionFlags
    appendLE32(ddevidarea, 0x01);

    // DownstreamDeviceSelfContainedActivationMinVersionStringType
    ddevidarea.push_back(0x01);

    // DownstreamDeviceSelfContainedActivationMinVersionStringLength
    ddevidarea.push_back(0x01);

    // DownstreamDevicePackageDataLength
    appendLE16(ddevidarea, 0x03);

    // DownstreamDeviceApplicableComponents
    ddevidarea.push_back(0x01);

    // DownstreamDeviceSelfContainedActivationMinVersionString
    appendString(ddevidarea, "v");

    // DownstreamDeviceSelfContainedActivationMinVersionComparisonStamp
    appendLE32(ddevidarea, 0x09080706);

    // DownstreamDeviceRecordDescriptors
    // record descriptors below

    appendDescriptorTLV(ddevidarea, PLDM_FWUP_UUID,
                        std::vector<uint8_t>{
                            // clang-format off
        0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
        0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75
                            // clang-format on
                        });

    // DownstreamDevicePackageData
    std::vector<uint8_t> ddpd = {0x83, 0x27, 0x72};
    ddevidarea.insert(ddevidarea.end(), ddpd.begin(), ddpd.end());

    // set DownstreamDeviceRecordLength
    patchLE16(ddevidarea, 0, ddevidarea.size());

    std::vector<uint8_t> pkg{};

    appendPackageHeaderIdentifier(pkg, pldm::fw_update::PackagePin::v1_1_0);

    const size_t packageHeaderSizeOffset = pkg.size();
    pkg.insert(pkg.end(), header.begin(), header.end());

    // firmware device record count
    pkg.push_back(0x01);
    appendFirmwareDeviceIdRecord2(pkg);

    // DownstreamDeviceIDRecordCount
    pkg.push_back(0x01);
    pkg.insert(pkg.end(), ddevidarea.begin(), ddevidarea.end());

    const auto clos = appendComponentImageInfoArea1(pkg);

    // count in the checksum bytes still to be added
    const uint16_t finalSize = pkg.size() + 4;
    // set PackageHeaderSize
    patchLE16(pkg, packageHeaderSizeOffset, finalSize);

    for (const size_t clo : clos)
    {
        patchLE32(pkg, clo, finalSize);
    }

    appendCRC(pkg);

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
