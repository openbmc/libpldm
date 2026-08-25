#include "testpkg-util.hpp"

#include <libpldm/api.h>

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
static std::vector<uint8_t> makePkg2DescriptorsSameTypeSingleComponent()
{

    // single firmware device id record
    // multiple firmware device id record record descriptors with the same type
    // single component
    std::vector<uint8_t> pkg;

    appendPackageHeaderIdentifier(pkg, pldm::fw_update::PackagePin::v1_3_0);

    const size_t packageHeaderSizeOffset = pkg.size();

    // pkg header size, updated later
    appendLE16(pkg, PLACEHOLDER);

    // pkg release date time (13 bytes, timestamp104)
    appendTimestamp104(pkg);

    // component bitmap bit length
    appendLE16(pkg, 0x08);

    // package version string
    appendTypeLengthString(pkg, "VersionString1");

    // device id record count
    pkg.push_back(0x01);

    appendFirmwareDeviceIdRecord4(pkg);

    // downstream device id record count
    pkg.push_back(0x01);

    appendDownstreamDeviceIDRecords2(pkg);

    const auto clos = appendComponentImageInfoArea1(pkg);

    appendComponentOpaqueData(pkg);

    // set PackageHeaderSize
    patchLE16(pkg, packageHeaderSizeOffset, pkg.size() + 8);

    // now we know component location offset
    for (const size_t clo : clos)
    {
        patchLE32(pkg, clo, pkg.size() + 8);
    }

    appendCRC(pkg);

    // PLDMFWPackagePayloadChecksum
    auto payloadChecksum = std::vector<uint8_t>{0x8d, 0xef, 0x02, 0xd2};
    pkg.insert(pkg.end(), payloadChecksum.begin(), payloadChecksum.end());

    // component image
    pkg.push_back(0x00);

    return pkg;
}
#endif

#if HAVE_LIBPLDM_API_TESTING
static void testFWDeviceRecordDescriptors2(
    const std::vector<FirmwareDeviceIDRecord>& outfwDeviceIDRecords)
{

    std::vector<uint8_t> dd1Data{0xD6, 0x75, 0x02, 0x38};
    std::vector<uint8_t> dd2Data{0xD6, 0x77, 0x03, 0x39};

    ASSERT_EQ(outfwDeviceIDRecords.size(), 1);

    const auto& fwDeviceIDRecord = outfwDeviceIDRecords[0];

    EXPECT_EQ(fwDeviceIDRecord.getDescriptorTypes(),
              std::vector<uint16_t>({PLDM_FWUP_IANA_ENTERPRISE_ID,
                                     PLDM_FWUP_IANA_ENTERPRISE_ID}));

    EXPECT_EQ(
        fwDeviceIDRecord.recordDescriptors.count(PLDM_FWUP_IANA_ENTERPRISE_ID),
        2);

    bool found1 = false;
    bool found2 = false;

    for (const auto& [k, v] : fwDeviceIDRecord.recordDescriptors)
    {
        EXPECT_EQ(k, PLDM_FWUP_IANA_ENTERPRISE_ID);
        ASSERT_TRUE(v);

        if (v->data == dd1Data)
        {
            found1 = true;
        }
        if (v->data == dd2Data)
        {
            found2 = true;
        }

        EXPECT_EQ(v->vendorDefinedDescriptorTitle, std::nullopt);
    }

    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
static void testDownstreamDeviceRecordDescriptors2(
    const std::vector<DownstreamDeviceIDRecord>& ddir)
{

    const std::vector<uint8_t> iana1 = {
        0x16,
        0x20,
        0x23,
        0xC9,
    };
    const std::vector<uint8_t> iana2 = {
        0x16,
        0x20,
        0x23,
        0xCA,
    };

    ASSERT_EQ(ddir.size(), 1);
    const DownstreamDeviceIDRecord& ddir0 = ddir[0];

    const std::multimap<uint16_t, std::unique_ptr<DescriptorData>>& map =
        ddir0.downstreamDeviceRecordDescriptors;

    EXPECT_EQ(map.size(), 2);

    EXPECT_EQ(map.count(PLDM_FWUP_IANA_ENTERPRISE_ID), 2);

    bool found1 = false;
    bool found2 = false;

    for (const auto& [k, v] : map)
    {
        EXPECT_EQ(k, PLDM_FWUP_IANA_ENTERPRISE_ID);
        ASSERT_TRUE(v);

        if (v->data == iana1)
        {
            found1 = true;
        }
        if (v->data == iana2)
        {
            found2 = true;
        }

        EXPECT_EQ(v->vendorDefinedDescriptorTitle, std::nullopt);
    }

    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}
#endif

#if HAVE_LIBPLDM_API_TESTING
TEST(PackageParserTest, ValidPkg2DescriptorsSameTypeSingleComponent)
{
    const auto pkg = makePkg2DescriptorsSameTypeSingleComponent();

    auto res = PackageParser::parse(pkg, PackagePin::v1_3_0_amend);

    if (!res.has_value())
    {
        std::cout << res.error().msg << std::endl;
        if (res.error().rc.has_value())
        {
            std::cout << res.error().rc.value() << std::endl;
        }
    }

    ASSERT_TRUE(res.has_value());

    const auto& outfwDeviceIDRecords = res.value()->firmwareDeviceIdRecords;
    const auto& ddir = res.value()->downstreamDeviceIdRecords;

    testFWDeviceRecordDescriptors2(outfwDeviceIDRecords);

    testDownstreamDeviceRecordDescriptors2(ddir);
}
#endif

} // namespace fw_update
} // namespace pldm
