#include "packagebuilder.hpp"

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

void patchLE32(std::vector<uint8_t>& pkg, size_t offset, uint32_t value)
{

    auto v = le32Vector(value);
    for (size_t i = offset; i < pkg.size() && (i - offset) < v.size(); i++)
    {
        pkg[i] = v[i - offset];
    }
}

void patchLE16(std::vector<uint8_t>& pkg, size_t offset, uint32_t value)
{

    auto v = le16Vector(value);
    for (size_t i = offset; i < pkg.size() && (i - offset) < v.size(); i++)
    {
        pkg[i] = v[i - offset];
    }
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

void appendDescriptorTLV(std::vector<uint8_t>& pkg, uint16_t descriptorType,
                         const std::vector<uint8_t>& descriptorData)
{

    // DescriptorType
    appendLE16(pkg, descriptorType);

    // InitialDescriptorLength
    appendLE16(pkg, descriptorData.size());

    // DescriptorData
    pkg.insert(pkg.end(), descriptorData.begin(), descriptorData.end());
}

void appendTimestamp104(std::vector<uint8_t>& pkg)
{

    std::vector<uint8_t> timestamp104Bytes{0x00, 0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x00, 0x00, 0x19, 0x0C,
                                           0xE5, 0x07, 0x00};

    pkg.insert(pkg.end(), timestamp104Bytes.begin(), timestamp104Bytes.end());
}
