#include "testpkg-util.hpp"

#include <libpldm/edac.h>
#include <libpldm/firmware_update.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

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
    auto checksum =
        std::vector<uint8_t>{static_cast<uint8_t>(check & 0xff),
                             static_cast<uint8_t>((check >> 8) & 0xff),
                             static_cast<uint8_t>((check >> 16) & 0xff),
                             static_cast<uint8_t>((check >> 24) & 0xff)};
    pkg.insert(pkg.end(), checksum.begin(), checksum.end());
}

void appendTypeLengthString(std::vector<uint8_t>& pkg,
                            const std::vector<uint8_t>& str)
{

    // DSP0267, Table 33 String Type Values
    // ASCII
    pkg.push_back(0x01);

    // string length
    pkg.push_back(str.size());

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

    std::vector<uint8_t> info = {
        // clang-format off
    0x0A, 0x00,             // component classification
    0x64, 0x00,             // component identifier
    0xFF, 0xFF, 0xFF, 0xFF, // component comparison stamp
    0x00, 0x00,             // component options

    0x00, 0x00,             // requested component activation method
    0x8B, 0x00, 0x00, 0x00, // component location offset
    0x01, 0x00, 0x00, 0x00, // component size

        // clang-format on
    };

    pkg.insert(pkg.end(), info.begin(), info.end());

    // component version string
    appendTypeLengthString(
        pkg, std::vector<uint8_t>{0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E,
                                  0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x33});
}

void appendComponentImageInfo2(std::vector<uint8_t>& pkg)
{

    std::vector<uint8_t> info = {
        // clang-format off
        0x0A, 0x00,             // component classification
        0x64, 0x00,             // component identifier
        0xFF, 0xFF, 0xFF, 0xFF, // component comparison stamp
        0x00, 0x00,             // component options

        0x00, 0x00,             // requested component activation method
        0x46, 0x01, 0x00, 0x00, // component location offset
        0x01, 0x00, 0x00, 0x00, // component size

        // clang-format on
    };

    pkg.insert(pkg.end(), info.begin(), info.end());

    // component version string
    appendTypeLengthString(
        pkg, std::vector<uint8_t>{0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E,
                                  0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x35});
}

void appendComponentImageInfo3(std::vector<uint8_t>& pkg)
{

    std::vector<uint8_t> info = {
        // clang-format off
        0x0A, 0x00,             // component classification
        0xC8, 0x00,             // component identifier
        0xFF, 0xFF, 0xFF, 0xFF, // component comparison stamp
        0x00, 0x00,             // component options

        0x01, 0x00,             // requested component activation method
        0x46, 0x01, 0x00, 0x00, // component location offset
        0x01, 0x00, 0x00, 0x00, // component size

        // clang-format on
    };

    pkg.insert(pkg.end(), info.begin(), info.end());

    // component version string
    appendTypeLengthString(
        pkg, std::vector<uint8_t>{0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E,
                                  0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x36});
}

void appendComponentImageInfo4(std::vector<uint8_t>& pkg)
{

    std::vector<uint8_t> info = {
        // clang-format off
        0xB,  0x00,             // component classification
        0x2C, 0x01,             // component identifier
        0xFF, 0xFF, 0xFF, 0xFF, // component comparison stamp
        0x01, 0x00,             // component options

        0x0C, 0x00,             // requested component activation method
        0x46, 0x01, 0x00, 0x00, // component location offset
        0x01, 0x00, 0x00, 0x00, // component size

        // clang-format on
    };

    pkg.insert(pkg.end(), info.begin(), info.end());

    // component version string
    appendTypeLengthString(
        pkg, std::vector<uint8_t>{0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E,
                                  0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x37});
}

void appendComponentImageInfoArea1(std::vector<uint8_t>& pkg)
{

    // component image info area
    // component image count
    std::vector<uint8_t> buffer{0x01, 0x00};

    pkg.insert(pkg.end(), buffer.begin(), buffer.end());
    appendComponentImageInfo1(pkg);
}

void appendComponentImageInfoArea2(std::vector<uint8_t>& pkg)
{

    // component image info area
    // component image count
    std::vector<uint8_t> buffer{0x03, 0x00};

    pkg.insert(pkg.end(), buffer.begin(), buffer.end());
    appendComponentImageInfo2(pkg);
    appendComponentImageInfo3(pkg);
    appendComponentImageInfo4(pkg);
}

void appendComponentOpaqueData(std::vector<uint8_t>& pkg)
{
    const std::vector<uint8_t> componentOpaqueData{
        // clang-format off
      0x5, 0x00, 0x00, 0x00, // ComponentOpaqueDataLength
      0x05, 0x04, 0x03, 0x02, 0x01, // ComponentOpaqueData
        // clang-format on
    };
    pkg.insert(pkg.end(), componentOpaqueData.begin(),
               componentOpaqueData.end());
}

void appendFirmwareDeviceIdRecord1(std::vector<uint8_t>& pkg)
{

    const std::vector<uint8_t> fwdevidrecord{
        // clang-format off

    0x2E, 0x00, // record 0: record length

    0x01,                   // record 0: descriptor count
    0x01, 0x00, 0x00, 0x00, // record 0: device update options flags

    0x01,       // record 0: component image set version string type
    0x0E,       // record 0: component image set version string length
    0x00, 0x00, // record 0: firmware device package data length

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
        // clang-format on
    };
    pkg.insert(pkg.end(), fwdevidrecord.begin(), fwdevidrecord.end());
}

// same as above, but applicable component is OOB
void appendFirmwareDeviceIdRecord1InvalidApplicableComponentOOB(
    std::vector<uint8_t>& pkg)
{

    const std::vector<uint8_t> fwdevidrecord{
        // clang-format off

    0x2E, 0x00, // record 0: record length

    0x01,                   // record 0: descriptor count
    0x01, 0x00, 0x00, 0x00, // record 0: device update options flags

    0x01,       // record 0: component image set version string type
    0x0E,       // record 0: component image set version string length
    0x00, 0x00, // record 0: firmware device package data length

    0x09,       // applicable components

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
        // clang-format on
    };
    pkg.insert(pkg.end(), fwdevidrecord.begin(), fwdevidrecord.end());
}

void appendFirmwareDeviceIdRecord2(std::vector<uint8_t>& pkg)
{

    std::vector<uint8_t> fwdevidrecord{
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
        const uint16_t recordLength = fwdevidrecord.size();

        fwdevidrecord[0] = recordLength & 0xff;
        fwdevidrecord[1] = (recordLength >> 8) & 0xff;
    }

    pkg.insert(pkg.end(), fwdevidrecord.begin(), fwdevidrecord.end());
}

void appendFirmwareDeviceIdRecord3(std::vector<uint8_t>& pkg)
{

    std::vector<uint8_t> fwdevidrecord{
        // clang-format off
    0x3a, 0x00, // record 0: record length

    0x01,                   // record 0: descriptor count
    0x01, 0x00, 0x00, 0x00, // record 0: device update options flags

    0x01,       // record 0: component image set version string type
    0x0E,       // record 0: component image set version string length
    0x00, 0x00, // record 0: firmware device package data length

    0x08, 0x00, 0x00, 0x00, // record 0: ReferenceManifestLength

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
    // ReferenceManifestData
    0x03, // SVHID,  (e.g. PCI SIG)
    0x02, // VendorIDLen
    0x20, 0xaf, // (e.g. Accelink)
    // actual manifest data (completely made up and without meaning)
    0x21, 0x22, 0x23, 0x24,
        // clang-format on
    };

    pkg.insert(pkg.end(), fwdevidrecord.begin(), fwdevidrecord.end());
}

void appendFirmwareDeviceIdArea1(std::vector<uint8_t>& pkg)
{
    std::vector<uint8_t> area{
        // clang-format off
        0x03, // device id record count

        0x45, 0x00, // record 0: record length

        0x03,                   // record 0: descriptor count
        0x01, 0x00, 0x00, 0x00, // record 0: device update options flags
        0x01,       // record 0: component image set version string type
        0x0E,       // record 0: component image set version string length
        0x00, 0x00, // record 0: firmware device package data length
        0x03,       // applicable components:

        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x32, // component image set version string (14 bytes)

        0x02, 0x00, // record 0: record descriptor type: uuid
        0x10, 0x00, // record 0: initial descriptor length
        0x12, 0x44, 0xD2, 0x64,
        0x8D, 0x7D, 0x47, 0x18,
        0xA0, 0x30, 0xFC, 0x8A,
        0x56, 0x58, 0x7D, 0x5B, // record 0: initial descriptor data
        0x01, 0x00, // additional descriptor type
        0x04, 0x00, // additional descriptor length
        0x47, 0x16, 0x00, 0x00, // additional descriptor identifier data

        0xFF, 0xFF, // additional descriptor type
        0x0B, 0x00, // additional descriptor length
        0x01, 0x07, 0x4F, 0x70,
        0x65, 0x6E, 0x42, 0x4D,
        0x43, 0x12, 0x34, // additional descriptor identifier data

        0x2E, 0x00, // record 1: record length
        0x01, // record 1: descriptor count
        0x00, 0x00, 0x00, 0x00, // record 1: device update options flags
        0x01, // record 1: component image set version string type
        0x0E, // record 1: component image set version string length
        0x00, 0x00, // record 1: firmware device package data length
        0x07, // applicable components
        0x56, 0x65, 0x72, 0x73,
        0x69, 0x6F, 0x6E, 0x53,
        0x74, 0x72, 0x69, 0x6E,
        0x67, 0x33, 0x02, // component image set version string (15 bytes)

        0x00, 0x10,
        0x00, 0x12,
        0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18, 0xA0, 0x30, 0xFC, 0x8A, 0x56,
        0x58, 0x7D, 0x5C, 0x2E, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0E,
        0x00, 0x00, 0x01, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74,
        0x72, 0x69, 0x6E, 0x67, 0x34, 0x02, 0x00, 0x10, 0x00, 0x12, 0x44, 0xD2,
        0x64, 0x8D, 0x7D, 0x47, 0x18, 0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D,
        0x5D,
        // clang-format on
    };

    pkg.insert(pkg.end(), area.begin(), area.end());
}
