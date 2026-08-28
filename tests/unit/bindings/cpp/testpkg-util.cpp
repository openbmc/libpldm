#include "testpkg-util.hpp"

#include <libpldm/edac.h>
#include <libpldm/firmware_update.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

std::vector<uint8_t> le32Vector(uint32_t x)
{
    return std::vector<uint8_t>{static_cast<uint8_t>(x & 0xff),
                                static_cast<uint8_t>((x >> 8) & 0xff),
                                static_cast<uint8_t>((x >> 16) & 0xff),
                                static_cast<uint8_t>((x >> 24) & 0xff)};
}

std::vector<uint8_t> le16Vector(uint16_t x)
{
    return std::vector<uint8_t>{static_cast<uint8_t>(x & 0xff),
                                static_cast<uint8_t>((x >> 8) & 0xff)};
}

void appendLE32(std::vector<uint8_t>& pkg, uint32_t x)
{

    auto v = le32Vector(x);
    pkg.insert(pkg.end(), v.begin(), v.end());
}

void appendLE16(std::vector<uint8_t>& pkg, uint16_t x)
{

    auto v = le16Vector(x);
    pkg.insert(pkg.end(), v.begin(), v.end());
}

void appendPackageHeaderIdentifier(std::vector<uint8_t>& pkg,
                                   pldm::fw_update::PackagePin pin)
{

    std::vector<uint8_t> uuid;
    if (pin == pldm::fw_update::PackagePin::v1)
    {
        uuid = PLDM_PACKAGE_HEADER_IDENTIFIER_V1_0;
    }
    else if (pin == pldm::fw_update::PackagePin::v1_1_0)
    {
        uuid = PLDM_PACKAGE_HEADER_IDENTIFIER_V1_1;
    }
    else if (pin == pldm::fw_update::PackagePin::v1_2_0)
    {
        uuid = PLDM_PACKAGE_HEADER_IDENTIFIER_V1_2;
    }
    else if (pin == pldm::fw_update::PackagePin::v1_3_0)
    {
        uuid = PLDM_PACKAGE_HEADER_IDENTIFIER_V1_3;
    }
    else
    {
        throw std::invalid_argument("unsupported package pin");
    }

    // PackageHeaderIdentifier
    pkg.insert(pkg.end(), uuid.begin(), uuid.end());

    // PackageHeaderFormatRevision
    if (pin == pldm::fw_update::PackagePin::v1)
    {
        pkg.push_back(PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR01H);
    }
    else if (pin == pldm::fw_update::PackagePin::v1_1_0)
    {
        pkg.push_back(PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR02H);
    }
    else if (pin == pldm::fw_update::PackagePin::v1_2_0)
    {
        pkg.push_back(PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR03H);
    }
    else if (pin == pldm::fw_update::PackagePin::v1_3_0)
    {
        pkg.push_back(PLDM_PACKAGE_HEADER_FORMAT_REVISION_FR04H);
    }
}

void appendCRC(std::vector<uint8_t>& pkg)
{

    // actual checksum
    const uint32_t check = pldm_edac_crc32(pkg.data(), pkg.size());

    // PackageHeaderChecksum
    const auto checksum = le32Vector(check);

    pkg.insert(pkg.end(), checksum.begin(), checksum.end());
}

void appendTypeLengthString(std::vector<uint8_t>& pkg,
                            const std::string_view& str)
{

    // DSP0267, Table 33 String Type Values
    // ASCII
    pkg.push_back(0x01);

    // string length
    pkg.push_back(str.size());

    pkg.insert(pkg.end(), str.begin(), str.end());
}

void appendString(std::vector<uint8_t>& pkg, const std::string_view& str)
{
    pkg.insert(pkg.end(), str.begin(), str.end());
}

void appendTimestamp104(std::vector<uint8_t>& pkg)
{

    std::vector<uint8_t> timestamp104Bytes{0x00, 0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x00, 0x00, 0x19, 0x0C,
                                           0xE5, 0x07, 0x00};

    pkg.insert(pkg.end(), timestamp104Bytes.begin(), timestamp104Bytes.end());
}

void appendComponentImageInfo1(std::vector<uint8_t>& pkg)
{
    // component classification
    appendLE16(pkg, 0x000A);
    // component identifier
    appendLE16(pkg, 0x64);
    // component comparison stamp
    appendLE32(pkg, 0xFFFFFFFF);
    // component options
    appendLE16(pkg, 0x0000);
    // requested component activation method
    appendLE16(pkg, 0x0000);
    // component location offset
    appendLE32(pkg, 0x8B);
    // component size
    appendLE32(pkg, 0x01);

    // component version string
    appendTypeLengthString(pkg, "VersionString3");
}

void appendComponentImageInfo2(std::vector<uint8_t>& pkg)
{
    // component classification
    appendLE16(pkg, 0x000A);
    // component identifier
    appendLE16(pkg, 0x64);
    // component comparison stamp
    appendLE32(pkg, 0xFFFFFFFF);
    // component options
    appendLE16(pkg, 0x0000);
    // requested component activation method
    appendLE16(pkg, 0x0000);
    // component location offset
    appendLE32(pkg, 0x146);
    // component size
    appendLE32(pkg, 0x01);

    // component version string
    appendTypeLengthString(pkg, "VersionString5");
}

void appendComponentImageInfo3(std::vector<uint8_t>& pkg)
{
    // component classification
    appendLE16(pkg, 0x000A);
    // component identifier
    appendLE16(pkg, 0xC8);
    // component comparison stamp
    appendLE32(pkg, 0xFFFFFFFF);
    // component options
    appendLE16(pkg, 0x0000);
    // requested component activation method
    appendLE16(pkg, 0x0001);
    // component location offset
    appendLE32(pkg, 0x146);
    // component size
    appendLE32(pkg, 0x01);

    // component version string
    appendTypeLengthString(pkg, "VersionString6");
}

void appendComponentImageInfo4(std::vector<uint8_t>& pkg)
{
    // component classification
    appendLE16(pkg, 0x000B);
    // component identifier
    appendLE16(pkg, 0x012C);
    // component comparison stamp
    appendLE32(pkg, 0xFFFFFFFF);
    // component options
    appendLE16(pkg, 0x0001);
    // requested component activation method
    appendLE16(pkg, 0x000C);
    // component location offset
    appendLE32(pkg, 0x146);
    // component size
    appendLE32(pkg, 0x01);

    // component version string
    appendTypeLengthString(pkg, "VersionString7");
}

void appendComponentImageInfoArea1(std::vector<uint8_t>& pkg)
{

    // component image info area
    // component image count
    appendLE16(pkg, 0x01);

    appendComponentImageInfo1(pkg);
}

void appendComponentImageInfoArea2(std::vector<uint8_t>& pkg)
{

    // component image info area
    // component image count
    appendLE16(pkg, 0x03);

    appendComponentImageInfo2(pkg);
    appendComponentImageInfo3(pkg);
    appendComponentImageInfo4(pkg);
}

void appendComponentOpaqueData(std::vector<uint8_t>& pkg)
{
    // ComponentOpaqueDataLength
    appendLE32(pkg, 0x05);

    // ComponentOpaqueData
    const std::vector<uint8_t> componentOpaqueData{0x05, 0x04, 0x03, 0x02,
                                                   0x01};

    pkg.insert(pkg.end(), componentOpaqueData.begin(),
               componentOpaqueData.end());
}

void appendFirmwareDeviceIdRecord1(std::vector<uint8_t>& pkg)
{
    // record 0: record length
    appendLE16(pkg, 0x2E);

    // record 0: descriptor count
    pkg.push_back(0x01);

    appendLE32(pkg, 0x01);

    // record 0: component image set version string type
    pkg.push_back(0x01);

    // record 0: component image set version string length
    pkg.push_back(0x0E);

    // record 0: firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x01);

    // component image set version string (14 bytes)
    appendString(pkg, "VersionString2");

    // record descriptors below
    // record 0: descriptor type: UUID
    appendLE16(pkg, 0x02);

    // InitialDescriptorLength
    appendLE16(pkg, 0x10);

    // record 0: InitialDescriptorData (UUID)
    const std::vector<uint8_t> initialDescriptorData{
        0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
        0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75,
    };
    pkg.insert(pkg.end(), initialDescriptorData.begin(),
               initialDescriptorData.end());

    // firmware device package data (empty here)
}

// same as above, but applicable component is OOB
void appendFirmwareDeviceIdRecord1InvalidApplicableComponentOOB(
    std::vector<uint8_t>& pkg)
{
    // record 0: record length
    appendLE16(pkg, 0x2E);

    // record 0: descriptor count
    pkg.push_back(0x01);

    // record 0: device update option flags
    appendLE32(pkg, 0x01);

    // record 0: component image set version string type
    pkg.push_back(0x01);

    // record 0: component image set version string length
    pkg.push_back(0x0E);

    // record 0: firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x09);

    // component image set version string (14 bytes)
    appendString(pkg, "VersionString2");

    // record descriptors below
    // record 0: descriptor type: UUID
    appendLE16(pkg, 0x02);

    // InitialDescriptorLength
    appendLE16(pkg, 0x10);

    // record 0: InitialDescriptorData (UUID)
    const std::vector<uint8_t> initialDescriptorData{
        0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
        0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75,
    };
    pkg.insert(pkg.end(), initialDescriptorData.begin(),
               initialDescriptorData.end());

    // firmware device package data (empty here)
}

void appendFirmwareDeviceIdRecord2(std::vector<uint8_t>& pkg)
{
    // Firmware Device Identification Area
    const size_t recordLengthOffset = pkg.size();

    // record 0: record length, patched later
    appendLE16(pkg, 0x0000);

    // record 0: descriptor count
    pkg.push_back(0x01);

    appendLE32(pkg, 0x00);

    // record 0: component image set version string type
    pkg.push_back(0x01);

    // record 0: component image set version string length
    pkg.push_back(0x01);

    // record 0: firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x01);

    // component image set version string
    appendString(pkg, "v");

    // record descriptors below

    // record 0: descriptor type: UUID
    appendLE16(pkg, 0x02);

    // record 0: InitialDescriptorLength
    appendLE16(pkg, 0x10);

    // record 0: InitialDescriptorData (UUID)
    std::vector<uint8_t> initialDescriptorData{
        0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
        0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75,
    };

    pkg.insert(pkg.end(), initialDescriptorData.begin(),
               initialDescriptorData.end());

    {
        // set Recordlength
        const uint16_t recordLength = pkg.size() - recordLengthOffset;

        pkg[recordLengthOffset] = recordLength & 0xff;
        pkg[recordLengthOffset + 1] = (recordLength >> 8) & 0xff;
    }
    // firmware device package data (empty here)
}

void appendFirmwareDeviceIdRecord3(std::vector<uint8_t>& pkg)
{
    // record 0: record length
    appendLE16(pkg, 0x003a);

    // record 0: descriptor count
    pkg.push_back(0x01);

    // record 0: device update option flags
    appendLE32(pkg, 0x01);

    // record 0: component image set version string type
    pkg.push_back(0x01);

    // record 0: component image set version string length
    pkg.push_back(0x0E);

    // record 0: firmware device package data length
    appendLE16(pkg, 0x00);

    // record 0: ReferenceManifestLength
    appendLE32(pkg, 0x08);

    // applicable components
    pkg.push_back(0x01);

    // component image set version string (14 bytes)
    appendString(pkg, "VersionString2");

    // record descriptors below
    // record 0: descriptor type: UUID
    appendLE16(pkg, 0x02);

    // record 0: InitialDescriptorLength (16 bytes)
    appendLE16(pkg, 0x10);

    // record 0: InitialDescriptorData (UUID)
    std::vector<uint8_t> initialDescriptorData{
        0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
        0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75,
    };
    pkg.insert(pkg.end(), initialDescriptorData.begin(),
               initialDescriptorData.end());

    // firmware device package data (empty here)

    // ReferenceManifestData
    // SVHID,  (e.g. PCI SIG)
    pkg.push_back(0x03);

    // VendorIDLen
    pkg.push_back(0x02);

    // VendorID (e.g. Accelink)
    pkg.push_back(0x20);
    pkg.push_back(0xaf);

    const std::vector<uint8_t> referenceManifestData = {0x21, 0x22, 0x23, 0x24};
    pkg.insert(pkg.end(), referenceManifestData.begin(),
               referenceManifestData.end());
}

void appendFirmwareDeviceIdArea1(std::vector<uint8_t>& pkg)
{
    // DeviceIDRecordCount
    pkg.push_back(0x03);

    // record 0: record length
    appendLE16(pkg, 0x45);

    // record 0: descriptor count
    pkg.push_back(0x03);

    // record 0: device update option flags
    appendLE32(pkg, 0x01);

    // record 0: component image set version string type
    pkg.push_back(0x01);

    // record 0: component image set version string length
    pkg.push_back(0x0E);

    // record 0: firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x03);

    // component image set version string (14 bytes)
    appendString(pkg, "VersionString2");

    // record 0: descriptor 0: record descriptor type: uuid
    appendLE16(pkg, 0x02);

    // record 0: descriptor 0: initial descriptor length
    appendLE16(pkg, 0x10);

    // record 0: descriptor 0: initial descriptor data
    std::vector<uint8_t> dd1{
        0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18,
        0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5B,
    };
    pkg.insert(pkg.end(), dd1.begin(), dd1.end());

    // record 0: descriptor 1: additional descriptor type
    appendLE16(pkg, 0x01);

    // record 0: descriptor 1: additional descriptor length
    appendLE16(pkg, 0x04);

    // record 0: descriptor 1: additional descriptor identifier data
    std::vector<uint8_t> dd2{0x47, 0x16, 0x00, 0x00};
    pkg.insert(pkg.end(), dd2.begin(), dd2.end());

    // record 0: descriptor 2: additional descriptor type
    appendLE16(pkg, 0xFFFF);

    // record 0: descriptor 2: additional descriptor length
    appendLE16(pkg, 0x0B);

    // record 0: descriptor 2: additional descriptor identifier data
    std::vector<uint8_t> dd3{
        0x01, 0x07, 0x4F, 0x70, 0x65, 0x6E, 0x42, 0x4D, 0x43, 0x12, 0x34,
    };
    pkg.insert(pkg.end(), dd3.begin(), dd3.end());

    // record 1: record length
    appendLE16(pkg, 0x2E);

    // record 1: descriptor count
    pkg.push_back(0x01);

    appendLE32(pkg, 0x00);

    // record 1: component image set version string type
    pkg.push_back(0x01);

    // record 1: component image set version string length
    pkg.push_back(0x0E);

    // record 1: firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x07);

    // component image set version string (15 bytes)
    appendString(pkg, "VersionString3\x02");

    // record 1: descriptor 0:

    // descriptor type
    appendLE16(pkg, 0x1000);

    // descriptor length
    appendLE16(pkg, 0x1200);

    // descriptorData
    std::vector<uint8_t> r1dd0{
        0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18, 0xA0,
        0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5C,
    };
    pkg.insert(pkg.end(), r1dd0.begin(), r1dd0.end());

    // record 2:
    // record length
    appendLE16(pkg, 0x2E);

    // descriptor count
    pkg.push_back(0x01);

    appendLE32(pkg, 0x00);

    // component image set version string type
    pkg.push_back(0x01);

    // component image set version string length
    pkg.push_back(0x0E);

    // firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x01);

    // component image set version string (15 bytes)
    appendString(pkg, "VersionString4\x02");

    // descriptor type
    appendLE16(pkg, 0x1000);

    // descriptor length
    appendLE16(pkg, 0x1200);

    // descriptorData
    std::vector<uint8_t> r2dd0{
        0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18, 0xA0,
        0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5D,
    };

    pkg.insert(pkg.end(), r2dd0.begin(), r2dd0.end());
}
